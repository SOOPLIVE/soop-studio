#include "CVolumeControl.h"

#include <util/platform.h>

#include <QHBoxLayout>
#include <QPainter>

#include "UIComponent/CSliderIgnoreWheel.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "Application/CApplication.h"
#include "qt-wrapper.h"

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define FADER_PRECISION 4096.0
#define DB_TEXT_UPDATE_INTERVAL 300

// Size of the audio indicator in pixels
#define INDICATOR_THICKNESS 3

// Padding on top and bottom of vertical meters
#define METER_PADDING 1

std::weak_ptr<AFQVolumeMeterTimer> AFQVolumeMeter::updateTimer;

static inline Qt::CheckState GetCheckState(bool muted, bool unassigned)
{
	if (muted)
		return Qt::Checked;
	else if (unassigned)
		return Qt::PartiallyChecked;
	else
		return Qt::Unchecked;
}


static inline bool IsSourceUnassigned(obs_source_t* source)
{
	uint32_t mixes = (obs_source_get_audio_mixers(source) &
		((1 << MAX_AUDIO_MIXES) - 1));
	obs_monitoring_type mt = obs_source_get_monitoring_type(source);

	return mixes == 0 && mt != OBS_MONITORING_TYPE_MONITOR_ONLY;
}


AFQVolumeMeter::AFQVolumeMeter(QWidget* parent,
	obs_volmeter_t* obs_volmeter,
	bool vertical)
	: QWidget(parent),
	  obs_volmeter(obs_volmeter),
	  vertical(vertical)
{
#ifdef __APPLE__
	setAttribute(Qt::WA_OpaquePaintEvent, true);
#endif

	// Default meter settings, they only show if
	// there is no stylesheet, do not remove.
	backgroundNominalColor.setRgb(0x26, 0x7f, 0x26); // Dark green
	backgroundWarningColor.setRgb(0x7f, 0x7f, 0x26); // Dark yellow
	backgroundErrorColor.setRgb(0x7f, 0x26, 0x26);   // Dark red
	foregroundNominalColor.setRgb(0x4c, 0xff, 0x4c); // Bright green
	foregroundWarningColor.setRgb(0xff, 0xff, 0x4c); // Bright yellow
	foregroundErrorColor.setRgb(0xff, 0x4c, 0x4c);   // Bright red

	backgroundNominalColorDisabled.setRgb(90, 90, 90);
	backgroundWarningColorDisabled.setRgb(117, 117, 117);
	backgroundErrorColorDisabled.setRgb(65, 65, 65);
	foregroundNominalColorDisabled.setRgb(163, 163, 163);
	foregroundWarningColorDisabled.setRgb(217, 217, 217);
	foregroundErrorColorDisabled.setRgb(113, 113, 113);

	clipColor.setRgb(0xff, 0xff, 0xff);      // Bright white
	magnitudeColor.setRgb(0x00, 0x00, 0x00); // Black
	majorTickColor.setRgb(0xff, 0xff, 0xff); // Black
	minorTickColor.setRgb(0xcc, 0xcc, 0xcc); // Black
	minimumLevel = -60.0;                    // -60 dB
	warningLevel = -20.0;                    // -20 dB
	errorLevel = -9.0;                       //  -9 dB
	clipLevel = -0.5;                        //  -0.5 dB
	minimumInputLevel = -50.0;               // -50 dB
	peakDecayRate = 11.76;                   //  20 dB / 1.7 sec
	magnitudeIntegrationTime = 0.3;          //  99% in 300 ms
	peakHoldDuration = 20.0;                 //  20 seconds
	inputPeakHoldDuration = 1.0;             //  1 second
	meterThickness = 1;                      // Bar thickness in pixels
	meterFontScaling =
		0.7; // Font size for numbers is 70% of Widget's font size
	channels = (int)audio_output_get_channels(obs_get_audio());

	doLayout();
	updateTimerRef = updateTimer.lock();
	if (!updateTimerRef) {
		updateTimerRef = std::make_shared<AFQVolumeMeterTimer>();
		updateTimerRef->setTimerType(Qt::PreciseTimer);
		updateTimerRef->start(16);
		updateTimer = updateTimerRef;
	}

	updateTimerRef->AddVolControl(this);
}

AFQVolumeMeter::~AFQVolumeMeter()
{
	updateTimerRef->RemoveVolControl(this);
}

void AFQVolumeMeter::setLevels(const float magnitude[MAX_AUDIO_CHANNELS],
							   const float peak[MAX_AUDIO_CHANNELS],
							   const float inputPeak[MAX_AUDIO_CHANNELS])
{
	uint64_t ts = os_gettime_ns();
	QMutexLocker locker(&dataMutex);

	currentLastUpdateTime = ts;

	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
		currentMagnitude[channelNr] = magnitude[channelNr];
		currentPeak[channelNr] = peak[channelNr];
		currentInputPeak[channelNr] = inputPeak[channelNr];
	}
	
	locker.unlock();
	calculateBallistics(ts);
}

QRect AFQVolumeMeter::getBarRect() const
{
	QRect rec = rect();
	if (vertical)
		rec.setWidth(displayNrAudioChannels * (meterThickness + 1) - 1);
	else
		rec.setHeight(displayNrAudioChannels * (meterThickness + 1) - 1);

	return rec;
}

bool AFQVolumeMeter::needLayoutChange()
{
	int currentNrAudioChannels = obs_volmeter_get_nr_channels(obs_volmeter);

	if (!currentNrAudioChannels) {
		struct obs_audio_info oai;
		obs_get_audio_info(&oai);
		currentNrAudioChannels = (oai.speakers == SPEAKERS_MONO) ? 1
			: 2;
	}

	if (displayNrAudioChannels != currentNrAudioChannels) {
		displayNrAudioChannels = currentNrAudioChannels;
		recalculateLayout = true;
	}

	return recalculateLayout;
}

static inline QColor color_from_int(long long val)
{
	QColor color(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff,
		(val >> 24) & 0xff);
	color.setAlpha(255);

	return color;
}

QColor AFQVolumeMeter::getBackgroundNominalColor() const
{
	return p_backgroundNominalColor;
}

QColor AFQVolumeMeter::getBackgroundNominalColorDisabled() const
{
	return backgroundNominalColorDisabled;
}

void AFQVolumeMeter::setBackgroundNominalColor(QColor c)
{
	p_backgroundNominalColor = std::move(c);

	backgroundNominalColor = color_from_int(config_get_int(
		AFConfigManager::GetSingletonInstance().GetGlobal(), "Accessibility", "MixerGreen"));
}

void AFQVolumeMeter::setBackgroundNominalColorDisabled(QColor c)
{
	backgroundNominalColorDisabled = std::move(c);
}

QColor AFQVolumeMeter::getBackgroundWarningColor() const
{
	return p_backgroundWarningColor;
}

QColor AFQVolumeMeter::getBackgroundWarningColorDisabled() const
{
	return backgroundWarningColorDisabled;
}

void AFQVolumeMeter::setBackgroundWarningColor(QColor c)
{
	p_backgroundWarningColor = std::move(c);

	backgroundWarningColor = color_from_int(config_get_int(
		AFConfigManager::GetSingletonInstance().GetGlobal(), "Accessibility", "MixerYellow"));
}

void AFQVolumeMeter::setBackgroundWarningColorDisabled(QColor c)
{
	backgroundWarningColorDisabled = std::move(c);
}

QColor AFQVolumeMeter::getBackgroundErrorColor() const
{
	return p_backgroundErrorColor;
}

QColor AFQVolumeMeter::getBackgroundErrorColorDisabled() const
{
	return backgroundErrorColorDisabled;
}

void AFQVolumeMeter::setBackgroundErrorColor(QColor c)
{
	p_backgroundErrorColor = std::move(c);

	backgroundErrorColor = color_from_int(config_get_int(
		AFConfigManager::GetSingletonInstance().GetGlobal(), "Accessibility", "MixerRed"));
}

void AFQVolumeMeter::setBackgroundErrorColorDisabled(QColor c)
{
	backgroundErrorColorDisabled = std::move(c);
}

QColor AFQVolumeMeter::getForegroundNominalColor() const
{
	return p_foregroundNominalColor;
}

QColor AFQVolumeMeter::getForegroundNominalColorDisabled() const
{
	return foregroundNominalColorDisabled;
}

void AFQVolumeMeter::setForegroundNominalColor(QColor c)
{
	p_foregroundNominalColor = std::move(c);

	foregroundNominalColor = color_from_int(
		config_get_int(AFConfigManager::GetSingletonInstance().GetGlobal(), "Accessibility",
			"MixerGreenActive"));
}

void AFQVolumeMeter::setForegroundNominalColorDisabled(QColor c)
{
	foregroundNominalColorDisabled = std::move(c);
}

QColor AFQVolumeMeter::getForegroundWarningColor() const
{
	return p_foregroundWarningColor;
}

QColor AFQVolumeMeter::getForegroundWarningColorDisabled() const
{
	return foregroundWarningColorDisabled;
}

void AFQVolumeMeter::setForegroundWarningColor(QColor c)
{
	p_foregroundWarningColor = std::move(c);

	foregroundWarningColor = color_from_int(
		config_get_int(AFConfigManager::GetSingletonInstance().GetGlobal(), "Accessibility",
			"MixerYellowActive"));
}

void AFQVolumeMeter::setForegroundWarningColorDisabled(QColor c)
{
	foregroundWarningColorDisabled = std::move(c);
}

QColor AFQVolumeMeter::getForegroundErrorColor() const
{
	return p_foregroundErrorColor;
}

QColor AFQVolumeMeter::getForegroundErrorColorDisabled() const
{
	return foregroundErrorColorDisabled;
}

void AFQVolumeMeter::setForegroundErrorColor(QColor c)
{
	p_foregroundErrorColor = std::move(c);

	foregroundErrorColor = color_from_int(config_get_int(
		AFConfigManager::GetSingletonInstance().GetGlobal(), "Accessibility", "MixerRedActive"));
}

void AFQVolumeMeter::setForegroundErrorColorDisabled(QColor c)
{
	foregroundErrorColorDisabled = std::move(c);
}

QColor AFQVolumeMeter::getClipColor() const
{
	return clipColor;
}

void AFQVolumeMeter::setClipColor(QColor c)
{
	clipColor = std::move(c);
}

QColor AFQVolumeMeter::getMagnitudeColor() const
{
	return magnitudeColor;
}

void AFQVolumeMeter::setMagnitudeColor(QColor c)
{
	magnitudeColor = std::move(c);
}

QColor AFQVolumeMeter::getMajorTickColor() const
{
	return majorTickColor;
}

void AFQVolumeMeter::setMajorTickColor(QColor c)
{
	majorTickColor = std::move(c);
}

QColor AFQVolumeMeter::getMinorTickColor() const
{
	return minorTickColor;
}

void AFQVolumeMeter::setMinorTickColor(QColor c)
{
	minorTickColor = std::move(c);
}

int AFQVolumeMeter::getMeterThickness() const
{
	return meterThickness;
}

void AFQVolumeMeter::setMeterThickness(int v)
{
	meterThickness = v;
	recalculateLayout = true;
}

qreal AFQVolumeMeter::getMeterFontScaling() const
{
	return meterFontScaling;
}

void AFQVolumeMeter::setMeterFontScaling(qreal v)
{
	meterFontScaling = v;
	recalculateLayout = true;
}

qreal AFQVolumeMeter::getMinimumLevel() const
{
	return minimumLevel;
}

void AFQVolumeMeter::setMinimumLevel(qreal v)
{
	minimumLevel = v;
}

qreal AFQVolumeMeter::getWarningLevel() const
{
	return warningLevel;
}

void AFQVolumeMeter::setWarningLevel(qreal v)
{
	warningLevel = v;
}

qreal AFQVolumeMeter::getErrorLevel() const
{
	return errorLevel;
}

void AFQVolumeMeter::setErrorLevel(qreal v)
{
	errorLevel = v;
}

qreal AFQVolumeMeter::getClipLevel() const
{
	return clipLevel;
}

void AFQVolumeMeter::setClipLevel(qreal v)
{
	clipLevel = v;
}

qreal AFQVolumeMeter::getMinimumInputLevel() const
{
	return minimumInputLevel;
}

void AFQVolumeMeter::setMinimumInputLevel(qreal v)
{
	minimumInputLevel = v;
}

qreal AFQVolumeMeter::getPeakDecayRate() const
{
	return peakDecayRate;
}

void AFQVolumeMeter::setPeakDecayRate(qreal v)
{
	peakDecayRate = v;
}

qreal AFQVolumeMeter::getMagnitudeIntegrationTime() const
{
	return magnitudeIntegrationTime;
}

void AFQVolumeMeter::setMagnitudeIntegrationTime(qreal v)
{
	magnitudeIntegrationTime = v;
}

qreal AFQVolumeMeter::getPeakHoldDuration() const
{
	return peakHoldDuration;
}

void AFQVolumeMeter::setPeakHoldDuration(qreal v)
{
	peakHoldDuration = v;
}

qreal AFQVolumeMeter::getInputPeakHoldDuration() const
{
	return inputPeakHoldDuration;
}

void AFQVolumeMeter::setInputPeakHoldDuration(qreal v)
{
	inputPeakHoldDuration = v;
}

void AFQVolumeMeter::setPeakMeterType(enum obs_peak_meter_type peakMeterType)
{
	obs_volmeter_set_peak_meter_type(obs_volmeter, peakMeterType);
	switch (peakMeterType) {
	case TRUE_PEAK_METER:
		// For true-peak meters EBU has defined the Permitted Maximum,
		// taking into account the accuracy of the meter and further
		// processing required by lossy audio compression.
		//
		// The alignment level was not specified, but I've adjusted
		// it compared to a sample-peak meter. Incidentally Youtube
		// uses this new Alignment Level as the maximum integrated
		// loudness of a video.
		//
		//  * Permitted Maximum Level (PML) = -2.0 dBTP
		//  * Alignment Level (AL) = -13 dBTP
		setErrorLevel(-2.0);
		setWarningLevel(-13.0);
		break;

	case SAMPLE_PEAK_METER:
	default:
		// For a sample Peak Meter EBU has the following level
		// definitions, taking into account inaccuracies of this meter:
		//
		//  * Permitted Maximum Level (PML) = -9.0 dBFS
		//  * Alignment Level (AL) = -20.0 dBFS
		setErrorLevel(-9.0);
		setWarningLevel(-20.0);
		break;
	}
}

void AFQVolumeMeter::mousePressEvent(QMouseEvent* event)
{
	setFocus(Qt::MouseFocusReason);
	event->accept();
}

void AFQVolumeMeter::wheelEvent(QWheelEvent* event)
{
	//QApplication::sendEvent(focusProxy(), event);
}

inline void AFQVolumeMeter::resetLevels()
{
	currentLastUpdateTime = 0;
	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
		currentMagnitude[channelNr] = -M_INFINITE;
		currentPeak[channelNr] = -M_INFINITE;
		currentInputPeak[channelNr] = -M_INFINITE;

		displayMagnitude[channelNr] = -M_INFINITE;
		displayPeak[channelNr] = -M_INFINITE;
		displayPeakHold[channelNr] = -M_INFINITE;
		displayPeakHoldLastUpdateTime[channelNr] = 0;
		displayInputPeakHold[channelNr] = -M_INFINITE;
		displayInputPeakHoldLastUpdateTime[channelNr] = 0;
	}
}

inline void AFQVolumeMeter::doLayout()
{
	QMutexLocker locker(&dataMutex);

	recalculateLayout = false;

	tickFont = font();
	QFontInfo info(tickFont);
	tickFont.setPointSizeF(info.pointSizeF() * meterFontScaling);
	QFontMetrics metrics(tickFont);
	if (vertical) {
		// Each meter channel is meterThickness pixels wide, plus one pixel
		// between channels, but not after the last.
		// Add 4 pixels for ticks, space to hold our longest label in this font,
		// and a few pixels before the fader.
		QRect scaleBounds = metrics.boundingRect("-88");
		setMinimumSize(displayNrAudioChannels * (meterThickness + 1) -
			1 + 4 + scaleBounds.width() + 2,
			130);
	}
	else {
		// Each meter channel is meterThickness pixels high, plus one pixel
		// between channels, but not after the last.
		// Add 4 pixels for ticks, and space high enough to hold our label in
		// this font, presuming that digits don't have descenders.
		setMinimumSize(130,
			displayNrAudioChannels * (meterThickness + 1) -
			1 + 4 + metrics.capHeight());
	}

	resetLevels();
}

bool AFQVolumeMeter::detectIdle(uint64_t ts)
{
	double timeSinceLastUpdate = (ts - currentLastUpdateTime) * 0.000000001;
	if (timeSinceLastUpdate > 0.5) {
		resetLevels();
		return true;
	}
	else {
		return false;
	}
}

void AFQVolumeMeter::calculateBallistics(uint64_t ts,
										 qreal timeSinceLastRedraw)
{
	QMutexLocker locker(&dataMutex);

	float maxPeak = -M_INFINITE;
	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) 
	{
		calculateBallisticsForChannel(channelNr, ts,
			timeSinceLastRedraw);
		maxPeak = maxPeak > currentPeak[channelNr] ? maxPeak : currentPeak[channelNr];
	}
	m_currentMaxPeak = maxPeak;
}

void AFQVolumeMeter::calculateBallisticsForChannel(int channelNr, uint64_t ts,
												   qreal timeSinceLastRedraw)
{
	if (currentPeak[channelNr] >= displayPeak[channelNr] ||
		isnan(displayPeak[channelNr])) {
		// Attack of peak is immediate.
		displayPeak[channelNr] = currentPeak[channelNr];
	}
	else {
		// Decay of peak is 40 dB / 1.7 seconds for Fast Profile
		// 20 dB / 1.7 seconds for Medium Profile (Type I PPM)
		// 24 dB / 2.8 seconds for Slow Profile (Type II PPM)
		float decay = float(peakDecayRate * timeSinceLastRedraw);
		displayPeak[channelNr] = CLAMP(displayPeak[channelNr] - decay,
			currentPeak[channelNr], 0);
	}

	if (currentPeak[channelNr] >= displayPeakHold[channelNr] ||
		!isfinite(displayPeakHold[channelNr])) {
		// Attack of peak-hold is immediate, but keep track
		// when it was last updated.
		displayPeakHold[channelNr] = currentPeak[channelNr];
		displayPeakHoldLastUpdateTime[channelNr] = ts;
	}
	else {
		// The peak and hold falls back to peak
		// after 20 seconds.
		qreal timeSinceLastPeak =
			(uint64_t)(ts -
				displayPeakHoldLastUpdateTime[channelNr]) *
			0.000000001;
		if (timeSinceLastPeak > peakHoldDuration) {
			displayPeakHold[channelNr] = currentPeak[channelNr];
			displayPeakHoldLastUpdateTime[channelNr] = ts;
		}
	}

	if (currentInputPeak[channelNr] >= displayInputPeakHold[channelNr] ||
		!isfinite(displayInputPeakHold[channelNr])) {
		// Attack of peak-hold is immediate, but keep track
		// when it was last updated.
		displayInputPeakHold[channelNr] = currentInputPeak[channelNr];
		displayInputPeakHoldLastUpdateTime[channelNr] = ts;
	}
	else {
		// The peak and hold falls back to peak after 1 second.
		qreal timeSinceLastPeak =
			(uint64_t)(ts -
				displayInputPeakHoldLastUpdateTime[channelNr]) *
			0.000000001;
		if (timeSinceLastPeak > inputPeakHoldDuration) {
			displayInputPeakHold[channelNr] =
				currentInputPeak[channelNr];
			displayInputPeakHoldLastUpdateTime[channelNr] = ts;
		}
	}

	if (!isfinite(displayMagnitude[channelNr])) {
		// The statements in the else-leg do not work with
		// NaN and infinite displayMagnitude.
		displayMagnitude[channelNr] = currentMagnitude[channelNr];
	}
	else {
		// A VU meter will integrate to the new value to 99% in 300 ms.
		// The calculation here is very simplified and is more accurate
		// with higher frame-rate.
		float attack =
			float((currentMagnitude[channelNr] -
				displayMagnitude[channelNr]) *
				(timeSinceLastRedraw / magnitudeIntegrationTime) *
				0.99);
		displayMagnitude[channelNr] =
			CLAMP(displayMagnitude[channelNr] + attack,
				(float)minimumLevel, 0);
	}
}

void AFQVolumeMeter::paintInputMeter(QPainter& painter, int x, int y, int width,
									 int height, float peakHold)
{
	QMutexLocker locker(&dataMutex);
	QColor color;

	if (peakHold < minimumInputLevel)
		color = backgroundNominalColor;
	else if (peakHold < warningLevel)
		color = foregroundNominalColor;
	else if (peakHold < errorLevel)
		color = foregroundWarningColor;
	else if (peakHold <= clipLevel)
		color = foregroundErrorColor;
	else
		color = clipColor;

	painter.fillRect(x, y, width, height, color);
}

#define CLIP_FLASH_DURATION_MS 1000

inline int AFQVolumeMeter::convertToInt(float number)
{
	constexpr int min = std::numeric_limits<int>::min();
	constexpr int max = std::numeric_limits<int>::max();

	// NOTE: Conversion from 'const int' to 'float' changes max value from 2147483647 to 2147483648
	if (number >= (float)max)
		return max;
	else if (number < min)
		return min;
	else
		return int(number);
}

void AFQVolumeMeter::paintHMeter(QPainter& painter, int x, int y, int width,
	int height, float magnitude, float peak,
	float peakHold)
{
	qreal scale = width / minimumLevel;

	if (muted)
	{
		peak = minimumLevel;
		magnitude = minimumLevel;
	}

	QMutexLocker locker(&dataMutex);
	int minimumPosition = x + 0;
	int maximumPosition = x + width;
	int magnitudePosition = x + width - convertToInt(magnitude * scale);
	int peakPosition = x + width - convertToInt(peak * scale);
	int peakHoldPosition = x + width - convertToInt(peakHold * scale);
	int warningPosition = x + width - convertToInt(warningLevel * scale);
	int errorPosition = x + width - convertToInt(errorLevel * scale);

	int nominalLength = warningPosition - minimumPosition;
	int warningLength = errorPosition - warningPosition;
	int errorLength = maximumPosition - errorPosition;
	locker.unlock();

	if (clipping) {
		peakPosition = maximumPosition;
	}

	if (peakPosition < minimumPosition) {
		painter.fillRect(minimumPosition, y, nominalLength, height,
			muted ? backgroundNominalColorDisabled
			: backgroundNominalColor);
		painter.fillRect(warningPosition, y, warningLength, height,
			muted ? backgroundWarningColorDisabled
			: backgroundWarningColor);
		painter.fillRect(errorPosition, y, errorLength, height,
			muted ? backgroundErrorColorDisabled
			: backgroundErrorColor);
	}
	else if (peakPosition < warningPosition) {
		painter.fillRect(minimumPosition, y,
			peakPosition - minimumPosition, height,
			muted ? foregroundNominalColorDisabled
			: foregroundNominalColor);
		painter.fillRect(peakPosition, y,
			warningPosition - peakPosition, height,
			muted ? backgroundNominalColorDisabled
			: backgroundNominalColor);
		painter.fillRect(warningPosition, y, warningLength, height,
			muted ? backgroundWarningColorDisabled
			: backgroundWarningColor);
		painter.fillRect(errorPosition, y, errorLength, height,
			muted ? backgroundErrorColorDisabled
			: backgroundErrorColor);
	}
	else if (peakPosition < errorPosition) {
		painter.fillRect(minimumPosition, y, nominalLength, height,
			muted ? foregroundNominalColorDisabled
			: foregroundNominalColor);
		painter.fillRect(warningPosition, y,
			peakPosition - warningPosition, height,
			muted ? foregroundWarningColorDisabled
			: foregroundWarningColor);
		painter.fillRect(peakPosition, y, errorPosition - peakPosition,
			height,
			muted ? backgroundWarningColorDisabled
			: backgroundWarningColor);
		painter.fillRect(errorPosition, y, errorLength, height,
			muted ? backgroundErrorColorDisabled
			: backgroundErrorColor);
	}
	else if (peakPosition < maximumPosition) {
		painter.fillRect(minimumPosition, y, nominalLength, height,
			muted ? foregroundNominalColorDisabled
			: foregroundNominalColor);
		painter.fillRect(warningPosition, y, warningLength, height,
			muted ? foregroundWarningColorDisabled
			: foregroundWarningColor);
		painter.fillRect(errorPosition, y, peakPosition - errorPosition,
			height,
			muted ? foregroundErrorColorDisabled
			: foregroundErrorColor);
		painter.fillRect(peakPosition, y,
			maximumPosition - peakPosition, height,
			muted ? backgroundErrorColorDisabled
			: backgroundErrorColor);
	}
	else if (int(magnitude) != 0) {
		if (!clipping) {
			QTimer::singleShot(CLIP_FLASH_DURATION_MS, this,
				[&]() { clipping = false; });
			clipping = true;
		}

		int end = errorLength + warningLength + nominalLength;
		painter.fillRect(minimumPosition, y, end, height,
			QBrush(muted ? foregroundErrorColorDisabled
				: foregroundErrorColor));
	}

	if (peakHoldPosition - 3 < minimumPosition)
		; // Peak-hold below minimum, no drawing.
	else if (peakHoldPosition < warningPosition)
		painter.fillRect(peakHoldPosition - 3, y, 3, height,
			muted ? foregroundNominalColorDisabled
			: foregroundNominalColor);
	else if (peakHoldPosition < errorPosition)
		painter.fillRect(peakHoldPosition - 3, y, 3, height,
			muted ? foregroundWarningColorDisabled
			: foregroundWarningColor);
	else
		painter.fillRect(peakHoldPosition - 3, y, 3, height,
			muted ? foregroundErrorColorDisabled
			: foregroundErrorColor);

	if (magnitudePosition - 3 >= minimumPosition)
		painter.fillRect(magnitudePosition - 3, y, 3, height,
			magnitudeColor);
}

void AFQVolumeMeter::paintVMeter(QPainter& painter, int x, int y, int width,
	int height, float magnitude, float peak,
	float peakHold)
{
	qreal scale = height / minimumLevel;

	if (muted)
	{
		peak = minimumLevel;
		magnitude = minimumLevel;
	}

	QMutexLocker locker(&dataMutex);
	int minimumPosition = y + 0;
	int maximumPosition = y + height;
	int magnitudePosition = y + height - convertToInt(magnitude * scale);
	int peakPosition = y + height - convertToInt(peak * scale);
	int peakHoldPosition = y + height - convertToInt(peakHold * scale);
	int warningPosition = y + height - convertToInt(warningLevel * scale);
	int errorPosition = y + height - convertToInt(errorLevel * scale);

	int nominalLength = warningPosition - minimumPosition;
	int warningLength = errorPosition - warningPosition;
	int errorLength = maximumPosition - errorPosition;
	locker.unlock();

	if (clipping) {
		peakPosition = maximumPosition;
	}

	if (peakPosition < minimumPosition) {
		painter.fillRect(x, minimumPosition, width, nominalLength,
			muted ? backgroundNominalColorDisabled
			: backgroundNominalColor);
		painter.fillRect(x, warningPosition, width, warningLength,
			muted ? backgroundWarningColorDisabled
			: backgroundWarningColor);
		painter.fillRect(x, errorPosition, width, errorLength,
			muted ? backgroundErrorColorDisabled
			: backgroundErrorColor);
	}
	else if (peakPosition < warningPosition) {
		painter.fillRect(x, minimumPosition, width,
			peakPosition - minimumPosition,
			muted ? foregroundNominalColorDisabled
			: foregroundNominalColor);
		painter.fillRect(x, peakPosition, width,
			warningPosition - peakPosition,
			muted ? backgroundNominalColorDisabled
			: backgroundNominalColor);
		painter.fillRect(x, warningPosition, width, warningLength,
			muted ? backgroundWarningColorDisabled
			: backgroundWarningColor);
		painter.fillRect(x, errorPosition, width, errorLength,
			muted ? backgroundErrorColorDisabled
			: backgroundErrorColor);
	}
	else if (peakPosition < errorPosition) {
		painter.fillRect(x, minimumPosition, width, nominalLength,
			muted ? foregroundNominalColorDisabled
			: foregroundNominalColor);
		painter.fillRect(x, warningPosition, width,
			peakPosition - warningPosition,
			muted ? foregroundWarningColorDisabled
			: foregroundWarningColor);
		painter.fillRect(x, peakPosition, width,
			errorPosition - peakPosition,
			muted ? backgroundWarningColorDisabled
			: backgroundWarningColor);
		painter.fillRect(x, errorPosition, width, errorLength,
			muted ? backgroundErrorColorDisabled
			: backgroundErrorColor);
	}
	else if (peakPosition < maximumPosition) {
		painter.fillRect(x, minimumPosition, width, nominalLength,
			muted ? foregroundNominalColorDisabled
			: foregroundNominalColor);
		painter.fillRect(x, warningPosition, width, warningLength,
			muted ? foregroundWarningColorDisabled
			: foregroundWarningColor);
		painter.fillRect(x, errorPosition, width,
			peakPosition - errorPosition,
			muted ? foregroundErrorColorDisabled
			: foregroundErrorColor);
		painter.fillRect(x, peakPosition, width,
			maximumPosition - peakPosition,
			muted ? backgroundErrorColorDisabled
			: backgroundErrorColor);
	}
	else {
		if (!clipping) {
			QTimer::singleShot(CLIP_FLASH_DURATION_MS, this,
				[&]() { clipping = false; });
			clipping = true;
		}

		int end = errorLength + warningLength + nominalLength;
		painter.fillRect(x, minimumPosition, width, end,
			QBrush(muted ? foregroundErrorColorDisabled
				: foregroundErrorColor));
	}

	if (peakHoldPosition - 3 < minimumPosition)
		; // Peak-hold below minimum, no drawing.
	else if (peakHoldPosition < warningPosition)
		painter.fillRect(x, peakHoldPosition - 3, width, 3,
			muted ? foregroundNominalColorDisabled
			: foregroundNominalColor);
	else if (peakHoldPosition < errorPosition)
		painter.fillRect(x, peakHoldPosition - 3, width, 3,
			muted ? foregroundWarningColorDisabled
			: foregroundWarningColor);
	else
		painter.fillRect(x, peakHoldPosition - 3, width, 3,
			muted ? foregroundErrorColorDisabled
			: foregroundErrorColor);

	if (magnitudePosition - 3 >= minimumPosition)
		painter.fillRect(x, magnitudePosition - 3, width, 3,
			magnitudeColor);
}

void AFQVolumeMeter::paintHTicks(QPainter& painter, int x, int y, int width)
{
	qreal scale = width / minimumLevel;

	painter.setFont(tickFont);
	QFontMetrics metrics(tickFont);
	painter.setPen(majorTickColor);

	// Draw major tick lines and numeric indicators.
	for (int i = 0; i >= minimumLevel; i -= 5) {
		int position = int(x + width - (i * scale) - 1);
		QString str = QString::number(i);

		// Center the number on the tick, but don't overflow
		QRect textBounds = metrics.boundingRect(str);
		int pos;
		if (i == 0) {
			pos = position - textBounds.width();
		}
		else {
			pos = position - (textBounds.width() / 2);
			if (pos < 0)
				pos = 0;
		}
		painter.drawText(pos, y + 4 + metrics.capHeight(), str);

		painter.drawLine(position, y, position, y + 2);
	}

	// Draw minor tick lines.
	painter.setPen(minorTickColor);
	for (int i = 0; i >= minimumLevel; i--) {
		int position = int(x + width - (i * scale) - 1);
		if (i % 5 != 0)
			painter.drawLine(position, y, position, y + 1);
	}
}

void AFQVolumeMeter::paintVTicks(QPainter& painter, int x, int y, int height)
{
	qreal scale = height / minimumLevel;

	painter.setFont(tickFont);
	QFontMetrics metrics(tickFont);
	painter.setPen(majorTickColor);

	// Draw major tick lines and numeric indicators.
	for (int i = 0; i >= minimumLevel; i -= 5) {
		int position = y + int(i * scale) + METER_PADDING;
		QString str = QString::number(i);

		// Center the number on the tick, but don't overflow
		if (i == 0) {
			painter.drawText(x + 6, position + metrics.capHeight(),
				str);
		}
		else {
			painter.drawText(x + 4,
				position + (metrics.capHeight() / 2),
				str);
		}

		painter.drawLine(x, position, x + 2, position);
	}

	// Draw minor tick lines.
	painter.setPen(minorTickColor);
	for (int i = 0; i >= minimumLevel; i--) {
		int position = y + int(i * scale) + METER_PADDING;
		if (i % 5 != 0)
			painter.drawLine(x, position, x + 1, position);
	}
}

void AFQVolumeMeter::paintEvent(QPaintEvent* event)
{
	uint64_t ts = os_gettime_ns();
	qreal timeSinceLastRedraw = (ts - lastRedrawTime) * 0.000000001;
	calculateBallistics(ts, timeSinceLastRedraw);
	bool idle = detectIdle(ts);

	QRect widgetRect = rect();
	int width = widgetRect.width();
	int height = widgetRect.height();

	QPainter painter(this);

	if (vertical)
		height -= METER_PADDING * 2;

	// timerEvent requests update of the bar(s) only, so we can avoid the
	// overhead of repainting the scale and labels.
	if (event->region().boundingRect() != getBarRect()) {
		if (needLayoutChange())
			doLayout();

		// Paint window background color (as widget is opaque)
		//QColor background = 
		//	palette().color(QPalette::ColorRole::Window);
		QColor background;
		//background.setRgb(50, 52, 58); // #32343A
		background.setRgb(36, 39, 45); // #24272D
		painter.fillRect(widgetRect, background);

		if (m_bMeterTickEnabled)
		{
			if (vertical) {
				paintVTicks(painter,
					displayNrAudioChannels *
					(meterThickness + 1) -
					1,
					0, height - (INDICATOR_THICKNESS + 3));
			}
			else {
				paintHTicks(painter, INDICATOR_THICKNESS + 3,
					displayNrAudioChannels *
					(meterThickness + 1) -
					1,
					width - (INDICATOR_THICKNESS + 3));
			}
		}
	}

	if (vertical) {
		// Invert the Y axis to ease the math
		painter.translate(0, height + METER_PADDING);
		painter.scale(1, -1);
	}

	for (int channelNr = 0; channelNr < displayNrAudioChannels;
		channelNr++) {

		int channelNrFixed =
			(displayNrAudioChannels == 1 && channels > 2)
			? 2
			: channelNr;

		if (vertical)
			paintVMeter(painter,
				channelNr * (meterThickness + 1),
				0 /*INDICATOR_THICKNESS + 2*/,
				meterThickness,
				height - (INDICATOR_THICKNESS + 2),
				displayMagnitude[channelNrFixed],
				displayPeak[channelNrFixed],
				displayPeakHold[channelNrFixed]);
		else
			paintHMeter(painter,
				0 /*INDICATOR_THICKNESS + 2*/,
				channelNr * (meterThickness + 4),
				width - (INDICATOR_THICKNESS + 2),
				meterThickness,
				displayMagnitude[channelNrFixed],
				displayPeak[channelNrFixed],
				displayPeakHold[channelNrFixed]);

		//if (idle)
		//	continue;

		// By not drawing the input meter boxes the user can
		// see that the audio stream has been stopped, without
		// having too much visual impact.
		//if (vertical)
		//	paintInputMeter(painter,
		//		channelNr * (meterThickness + 4), 0,
		//		meterThickness, INDICATOR_THICKNESS,
		//		displayInputPeakHold[channelNrFixed]);
		//else
		//	paintInputMeter(painter, 0,
		//		channelNr * (meterThickness + 4),
		//		INDICATOR_THICKNESS, meterThickness,
		//		displayInputPeakHold[channelNrFixed]);
	}

	lastRedrawTime = ts;
}

void AFQVolumeMeter::changeEvent(QEvent* e)
{
	if (e->type() == QEvent::StyleChange)
		recalculateLayout = true;

	QWidget::changeEvent(e);
}

void AFQVolumeMeterTimer::AddVolControl(AFQVolumeMeter* meter)
{
	volumeMeters.push_back(meter);
}

void AFQVolumeMeterTimer::RemoveVolControl(AFQVolumeMeter* meter)
{
	volumeMeters.removeOne(meter);
}

void AFQVolumeMeterTimer::timerEvent(QTimerEvent* event)
{
	for (AFQVolumeMeter* meter : volumeMeters) {
		if (meter->needLayoutChange()) {
			// Tell paintEvent to update layout and paint everything
			meter->update();
		}
		else {
			// Tell paintEvent to paint only the bars
			meter->update();
			//meter->update(meter->getBarRect());
		}
	}
}

AFQVolControl::AFQVolControl(QWidget* parent, OBSSource source_, bool showConfig, bool vertical)
	: QWidget(parent),
	  source(std::move(source_)),
	  obs_fader(obs_fader_create(OBS_FADER_LOG)),
	  obs_volmeter(obs_volmeter_create(OBS_FADER_LOG)),
	  contextMenu(nullptr)
{
	nameLabel = new QLabel();
	volLabel = new QLabel();
	mute = new AFQMuteCheckBox();
	mute->setFixedSize(QSize(24, 24));

	lockIcon = new QCheckBox();
	lockIcon->setText("");
	lockIcon->setFixedSize(QSize(12, 12));
	lockIcon->setCheckable(false);
	lockIcon->setObjectName("checkBox_lockIcon");

	nameLabel->setObjectName("AudioMixerNameLabel");
	volLabel->setObjectName("AudioMixerVolLabel");

	nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	QString sourceName = obs_source_get_name(source);
	setObjectName(sourceName);

	if (showConfig) {
		config = new QPushButton(this);
		config->setCheckable(true);
		config->setProperty("themeID", "menuIconSmall");
		config->setSizePolicy(QSizePolicy::Maximum,
			QSizePolicy::Maximum);
		config->setFixedSize(24, 24);
		config->setAutoDefault(false);

		config->setAccessibleName(
			QString::fromUtf8(AFLocaleTextManager::GetSingletonInstance().Str("VolControl.Properties")).arg(sourceName));

		connect(config, &QAbstractButton::clicked, this,
			&AFQVolControl::EmitConfigClicked);
	}

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(4, 4, 4, 4);
	mainLayout->setSpacing(2);

	if (vertical) {
		QHBoxLayout* nameLayout = new QHBoxLayout;
		QHBoxLayout* controlLayout = new QHBoxLayout;
		QHBoxLayout* volLayout = new QHBoxLayout;
		QHBoxLayout* meterLayout = new QHBoxLayout;

		volMeter = new AFQVolumeMeter(nullptr, obs_volmeter, true);
		slider = new AFCVolumeSlider(obs_fader, Qt::Vertical);
		slider->setLayoutDirection(Qt::LeftToRight);

		nameLayout->setAlignment(Qt::AlignCenter);
		meterLayout->setAlignment(Qt::AlignCenter);
		controlLayout->setAlignment(Qt::AlignCenter);
		volLayout->setAlignment(Qt::AlignCenter);

		nameLayout->setContentsMargins(0, 0, 0, 0);
		nameLayout->setSpacing(10);
		nameLayout->addWidget(lockIcon);
		nameLayout->addWidget(nameLabel);

		controlLayout->setContentsMargins(0, 0, 0, 0);
		controlLayout->setSpacing(0);

		if (showConfig) {
			controlLayout->addWidget(config);
			controlLayout->setAlignment(config, Qt::AlignVCenter);
		}

		controlLayout->addItem(new QSpacerItem(3, 0));
		// Add Headphone (audio monitoring) widget here
		controlLayout->addWidget(mute);
		controlLayout->setAlignment(mute, Qt::AlignVCenter);

		meterLayout->setContentsMargins(0, 0, 0, 0);
		meterLayout->setSpacing(0);
		meterLayout->addWidget(volMeter);
		meterLayout->addWidget(slider);

		volLayout->setContentsMargins(0, 0, 0, 0);
		volLayout->setSpacing(0);
		volLayout->addWidget(volLabel);

		// Default size can cause clipping of long names in vertical layout.
		//QFont font = nameLabel->font();
		//QFontInfo info(font);
		//font.setPointSizeF(0.8 * info.pointSizeF());
		//nameLabel->setFont(font);

		mainLayout->addItem(nameLayout);
		mainLayout->addItem(volLayout);
		mainLayout->addItem(meterLayout);
		mainLayout->addItem(controlLayout);

		volMeter->setFocusProxy(slider);

		setMaximumWidth(110);

	}
	else {

		QHBoxLayout* textLayout = new QHBoxLayout;
		QHBoxLayout* botLayout = new QHBoxLayout;

		volMeter = new AFQVolumeMeter(nullptr, obs_volmeter, false);
		slider = new AFCVolumeSlider(obs_fader, Qt::Horizontal);
		slider->setLayoutDirection(Qt::LeftToRight);

		textLayout->setContentsMargins(0, 0, 0, 0);
		textLayout->setSpacing(10);
		textLayout->addWidget(lockIcon);
		textLayout->addWidget(nameLabel);
		textLayout->addWidget(volLabel);
		//textLayout->setAlignment(nameLabel, Qt::AlignLeft);
		//textLayout->setAlignment(volLabel, Qt::AlignRight);

		botLayout->setContentsMargins(0, 0, 0, 0);
		botLayout->setSpacing(10);
		botLayout->addWidget(slider);
		botLayout->addWidget(mute);
		botLayout->setAlignment(slider, Qt::AlignVCenter);
		botLayout->setAlignment(mute, Qt::AlignVCenter);

		if (showConfig) {
			botLayout->addWidget(config);
			botLayout->setAlignment(config, Qt::AlignVCenter);
		}

		mainLayout->addItem(textLayout);
		mainLayout->addSpacing(20);
		mainLayout->addWidget(volMeter);
		mainLayout->addItem(botLayout);
		//mainLayout->addStretch();

		volMeter->setFocusProxy(slider);

	}
	setLayout(mainLayout);

	m_strSourceName = sourceName;
	nameLabel->setText(sourceName);
	if (vertical)
		nameLabel->setMaximumWidth(85);
	
	TruncateTextToLabelWidth(nameLabel, nameLabel->text());
	
	OBSDataAutoRelease settings = obs_source_get_private_settings(source);
	bool lock = obs_data_get_bool(settings, "volume_locked");
	if (lock)
		lockIcon->setVisible(true);
	else
		lockIcon->setVisible(false);

	slider->setMinimum(0);
	slider->setMaximum(int(FADER_PRECISION));

	bool muted = obs_source_muted(source);
	bool unassigned = IsSourceUnassigned(source);
	mute->setCheckState(GetCheckState(muted, unassigned));
	volMeter->muted = muted || unassigned;

	mute->setAccessibleName(QString::fromUtf8(AFLocaleTextManager::GetSingletonInstance().Str("VolControl.Mute")).arg(sourceName));
	obs_fader_add_callback(obs_fader, OBSVolumeChanged, this);
	obs_volmeter_add_callback(obs_volmeter, OBSVolumeLevel, this);

	signal_handler_connect(obs_source_get_signal_handler(source), "mute",
		OBSVolumeMuted, this);
	signal_handler_connect(obs_source_get_signal_handler(source),
		"audio_mixers", OBSMixersOrMonitoringChanged,
		this);
	signal_handler_connect(obs_source_get_signal_handler(source),
		"audio_monitoring", OBSMixersOrMonitoringChanged,
		this);

	QWidget::connect(slider, &AFCVolumeSlider::valueChanged, this,
		&AFQVolControl::SliderChanged);
	QWidget::connect(mute, &AFQMuteCheckBox::clicked, this,
		&AFQVolControl::SetMuted);
	
	m_UpdatePeakTextTimer = new QTimer(this);
	QWidget::connect(m_UpdatePeakTextTimer, &QTimer::timeout, this,
		&AFQVolControl::qslotUpdateCurrentDbText);
	m_UpdatePeakTextTimer->start(DB_TEXT_UPDATE_INTERVAL);

	obs_fader_attach_source(obs_fader, source);
	obs_volmeter_attach_source(obs_volmeter, source);

	/* Call volume changed once to init the slider position and label */
	VolumeChanged();
}

AFQVolControl::~AFQVolControl()
{
	obs_fader_remove_callback(obs_fader, OBSVolumeChanged, this);
	obs_volmeter_remove_callback(obs_volmeter, OBSVolumeLevel, this);

	signal_handler_disconnect(obs_source_get_signal_handler(source), "mute",
		OBSVolumeMuted, this);
	signal_handler_disconnect(obs_source_get_signal_handler(source),
		"audio_mixers", OBSMixersOrMonitoringChanged,
		this);
	signal_handler_disconnect(obs_source_get_signal_handler(source),
		"audio_monitoring",
		OBSMixersOrMonitoringChanged, this);

	lockIcon->deleteLater();

	if (contextMenu)
		contextMenu->close();
}

void AFQVolControl::OBSVolumeChanged(void* data, float db)
{
	Q_UNUSED(db);
	AFQVolControl* volControl = static_cast<AFQVolControl*>(data);

	QMetaObject::invokeMethod(volControl, "VolumeChanged");
}

void AFQVolControl::OBSVolumeLevel(void* data,
								   const float magnitude[MAX_AUDIO_CHANNELS],
								   const float peak[MAX_AUDIO_CHANNELS],
								   const float inputPeak[MAX_AUDIO_CHANNELS])
{
	AFQVolControl* volControl = static_cast<AFQVolControl*>(data);

	volControl->volMeter->setLevels(magnitude, peak, inputPeak);
}

void AFQVolControl::OBSVolumeMuted(void* data, calldata_t* calldata)
{
	AFQVolControl* volControl = static_cast<AFQVolControl*>(data);
	bool muted = calldata_bool(calldata, "muted");

	QMetaObject::invokeMethod(volControl, "VolumeMuted",
		Q_ARG(bool, muted));
}

void AFQVolControl::OBSMixersOrMonitoringChanged(void* data, calldata_t*)
{

	AFQVolControl* volControl = static_cast<AFQVolControl*>(data);
	QMetaObject::invokeMethod(volControl, "MixersOrMonitoringChanged",
		Qt::QueuedConnection);
}

void AFQVolControl::ChangeVolume(int volume)
{
	slider->setValue(volume);
}

void AFQVolControl::ChangeMuteState()
{
	if(mute->checkState() == Qt::Checked)
		mute->setCheckState(Qt::Unchecked);
	else
		mute->setCheckState(Qt::Checked);
}

bool AFQVolControl::IsMuted()
{
	return mute->checkState() == Qt::Checked;
}

int AFQVolControl::GetVolume()
{
	return slider->value();
}

float AFQVolControl::GetCurrentPeak() 
{
	return volMeter->GetCurrentMaxPeak();
}

void AFQVolControl::EmitConfigClicked()
{
	emit ConfigClicked();
}


void AFQVolControl::VolumeChanged()
{
	slider->blockSignals(true);
	slider->setValue(
		(int)(obs_fader_get_deflection(obs_fader) * FADER_PRECISION));
	slider->blockSignals(false);

	//updateText();
}

void AFQVolControl::VolumeMuted(bool muted)
{
	bool unassigned = IsSourceUnassigned(source);

	auto newState = GetCheckState(muted, unassigned);
	if (mute->checkState() != newState)
		mute->setCheckState(newState);

	volMeter->muted = muted || unassigned;
}

void AFQVolControl::MixersOrMonitoringChanged()
{
	bool muted = obs_source_muted(source);
	bool unassigned = IsSourceUnassigned(source);

	auto newState = GetCheckState(muted, unassigned);
	if (mute->checkState() != newState)
		mute->setCheckState(newState);

	volMeter->muted = muted || unassigned;
}

void AFQVolControl::SetMuted()
{
	bool checked = mute->checkState() == Qt::Checked;
	bool prev = obs_source_muted(source);
	obs_source_set_muted(source, checked);
	bool unassigned = IsSourceUnassigned(source);

	if (!checked && unassigned) {
		mute->setCheckState(Qt::PartiallyChecked);
		/* Show notice about the source no being assigned to any tracks */
		//bool has_shown_warning =
		//	config_get_bool(App()->GlobalConfig(), "General",
		//		"WarnedAboutUnassignedSources");
		//if (!has_shown_warning)
		//	ShowUnassignedWarning(obs_source_get_name(source));
	}

	auto undo_redo = [](const std::string& uuid, bool val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_muted(source, val);
	};

	QString text = QTStr(checked ? "Undo.Volume.Mute" : "Undo.Volume.Unmute");

	const char* name = obs_source_get_name(source);
	const char* uuid = obs_source_get_uuid(source);
	AFMainFrame* main = App()->GetMainView();
	main->m_undo_s.AddAction(text.arg(name),
							 std::bind(undo_redo, std::placeholders::_1, prev),
							 std::bind(undo_redo, std::placeholders::_1, checked), uuid,
							 uuid);

	if (m_bValueChangedByUser) 
		App()->GetMainView()->GetMainWindow()->qslotSetMainAudioSource();
}

void AFQVolControl::SliderChanged(int vol)
{
	float prev = obs_source_get_volume(source);

	obs_fader_set_deflection(obs_fader, float(vol) / FADER_PRECISION);
	//updateText();

	auto undo_redo = [](const std::string& uuid, float val) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		obs_source_set_volume(source, val);
	};
	
	float val = obs_source_get_volume(source);
	const char* name = obs_source_get_name(source);
	const char* uuid = obs_source_get_uuid(source);
	AFMainFrame* main = App()->GetMainView();
	main->m_undo_s.AddAction(QTStr("Undo.Volume.Change").arg(name),
							 std::bind(undo_redo, std::placeholders::_1, prev),
							 std::bind(undo_redo, std::placeholders::_1, val), uuid, uuid,
							 true);
	
	if (m_bValueChangedByUser)
		App()->GetMainView()->GetMainWindow()->qslotSetMainAudioSource();
}


void AFQVolControl::updateText()
{
	QString text;
	float db = obs_fader_get_db(obs_fader);

	//if (db < -96.0f)
	//	text = "-inf dB";
	//else
	//	text = QString::number(db, 'f', 1).append(" dB");

	if (db < -96.0f)
		db = -96.0f;
	text = QString::number(db, 'f', 1).append(" dB");

	volLabel->setText(text);

	bool muted = obs_source_muted(source);
	const char* accTextLookup = muted ? "VolControl.SliderMuted"
		: "VolControl.SliderUnmuted";

	QString sourceName = obs_source_get_name(source);
	QString accText = QString::fromUtf8(AFLocaleTextManager::GetSingletonInstance().Str(accTextLookup)).arg(sourceName);

	slider->setAccessibleName(accText);
}

void AFQVolControl::qslotUpdateCurrentDbText()
{
	QString text;
	float currentDb = volMeter->GetCurrentMaxPeak();
	if (currentDb < -96.0f)
		currentDb = -96.0f;
	currentDb += 96.0f;

	bool muted = obs_source_muted(source);
	if (muted)
		currentDb = 0.0f;

	text = QString::number(currentDb, 'f', 1).append(" dB");
	volLabel->setText(text);
}

void AFQVolControl::SetConfigButtonReleased()
{
	config->setChecked(false);
}

void AFQVolControl::SetConfigButtonPressed()
{
	config->setChecked(true);
}

QString AFQVolControl::GetName() const
{
	return nameLabel->text();
}

void AFQVolControl::SetName(const QString& newName)
{
	//nameLabel->setText(newName);
	m_strSourceName = newName;
	TruncateTextToLabelWidth(nameLabel, newName);
}

void AFQVolControl::SetMeterDecayRate(qreal q)
{
	volMeter->setPeakDecayRate(q);
}

void AFQVolControl::setPeakMeterType(enum obs_peak_meter_type peakMeterType)
{
	volMeter->setPeakMeterType(peakMeterType);
}

void AFQVolControl::EnableSlider(bool enable)
{
	if (enable)
		lockIcon->setVisible(false);
	else
		lockIcon->setVisible(true);

	slider->setEnabled(enable);
}

void AFQVolControl::SetContextMenu(AFQCustomMenu* cm)
{ 
	contextMenu = cm; 

	if (contextMenu != nullptr)
	{
		connect(contextMenu, &QMenu::aboutToHide, this, &AFQVolControl::SetConfigButtonReleased);
		connect(contextMenu, &QMenu::aboutToShow, this, &AFQVolControl::SetConfigButtonPressed);
	}
}

void AFQVolControl::refreshColors()
{
	volMeter->setBackgroundNominalColor(volMeter->getBackgroundNominalColor());
	volMeter->setBackgroundWarningColor(volMeter->getBackgroundWarningColor());
	volMeter->setBackgroundErrorColor(volMeter->getBackgroundErrorColor());
	volMeter->setForegroundNominalColor(volMeter->getForegroundNominalColor());
	volMeter->setForegroundWarningColor(volMeter->getForegroundWarningColor());
	volMeter->setForegroundErrorColor(volMeter->getForegroundErrorColor());
}

void AFQVolControl::showEvent(QShowEvent* event) {
	m_UpdatePeakTextTimer->start(DB_TEXT_UPDATE_INTERVAL);
}
void AFQVolControl::hideEvent(QHideEvent* event) {
	m_UpdatePeakTextTimer->stop();
}

void AFQVolControl::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	//TruncateTextToLabelWidth(nameLabel, m_strSourceName);
}