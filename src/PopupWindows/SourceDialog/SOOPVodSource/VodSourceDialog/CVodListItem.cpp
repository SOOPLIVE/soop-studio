#include "CVodListItem.h"
#include "ui_vod-item-widget.h"

#include <QPixmap>
#include <QStyle>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QSvgRenderer>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "MainFrame/CMainFrame.h"

#define THUMBNAIL_WIDTH		640
#define THUMBNAIL_HEIGHT	360

AFQVodListItem::AFQVodListItem(QWidget* parent) :
	QFrame(parent),
	ui(new Ui::AFQVodListItem)
{
	setAttribute(Qt::WA_Hover, true);

	ui->setupUi(this);
}

AFQVodListItem::~AFQVodListItem()
{

}

void AFQVodListItem::SetVodInfo(VodInfo_s& info)
{
	m_info = info;

	QMetaObject::invokeMethod(this, "qslotRefreshItemUI",
							  Qt::QueuedConnection);
}

void AFQVodListItem::SetVodListItemStatus(bool playing, obs_media_state mediaState)
{
#ifndef _SOOP_VLC
	if ((OBS_MEDIA_STATE_PLAYING == mediaState) || (OBS_MEDIA_STATE_OPENING == mediaState))
#else
	if (OBS_MEDIA_STATE_PLAYING == mediaState)
#endif // !_SOOP_VLC
	{
		if (m_info.recentlyPlay)
		{
			m_info.recentlyPlay = false;
			setProperty("recentlyPlay", false);
			PolishStyleSheet(this);
		}
	}

	ui->label_Title->setProperty("vod_playing", playing);
	PolishStyleSheet(ui->label_Title);

	m_playing = playing;

	STATE state = _GetVODState();
	_ApplyPixmap(state);

}

void AFQVodListItem::SetRecentyPlayVodStatus()
{
	m_info.recentlyPlay = true;

	_ApplyPixmap(STATE::NORMAL);
}

void AFQVodListItem::qslotImageDownloaded(QByteArray responseData)
{
	QPixmap originPixmap;
	if (!originPixmap.loadFromData(responseData)) {
		blog(LOG_WARNING, "Failed to vod Thumnail image from response data.");
		return;
	}

	if (originPixmap.size().width() != THUMBNAIL_WIDTH || originPixmap.size().height() != THUMBNAIL_WIDTH) {
		m_pixmapImage = originPixmap.scaled(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}
	else {
		m_pixmapImage = originPixmap;
	}
	m_pixmapDuration = _MakeDurationPixmap(m_pixmapImage, m_info.duration);

	STATE state = _GetVODState();
	_ApplyPixmap(state);

	if (m_info.recentlyPlay) {
		setProperty("recentlyPlay", true);
		PolishStyleSheet(this);
	}
}


void AFQVodListItem::qslotRecentlyHighlightTimer()
{

}

void AFQVodListItem::qslotRefreshItemUI()
{
	QFontMetrics metrics(ui->label_Title->font());
	QString elidedTitleText = metrics.elidedText(m_info.vodTitle, Qt::ElideRight, 440);
	if (0 != elidedTitleText.compare(m_info.vodTitle)) {
		QString tooltip = QString("<b>%1</b><br>%2").arg(m_info.contentTitle).arg(m_info.vodTitle);
		setToolTip(tooltip);
	}

	ui->label_Season->setText(m_info.seasonTitle);
	ui->label_Title->setText(elidedTitleText);

	SOOP_API_HANDLER->downloadImage(m_info.vodImage.toStdString().c_str(),
		this, "qslotImageDownloaded");
}

void AFQVodListItem::mousePressEvent(QMouseEvent* event)
{
	bool playing = ui->label_Title->property("vod_playing").toBool();

	if (!playing)
		emit qsignalPlayVOD(m_info);
}

void AFQVodListItem::enterEvent(QEnterEvent* event)
{
	m_hover = true;

	STATE state = _GetVODState();
	_ApplyPixmap(state);

}

void AFQVodListItem::leaveEvent(QEvent* event)
{
	m_hover = false;

	STATE state = _GetVODState();
	_ApplyPixmap(state);
}

AFQVodListItem::STATE AFQVodListItem::_GetVODState()
{
	bool playing = ui->label_Title->property("vod_playing").toBool();
	if (playing)
		return STATE::PLAYING;

	if (m_hover)
		return STATE::HOVER;

	return STATE::NORMAL;
}

QPixmap AFQVodListItem::_MakeRecentlyPlayPixmap(const QPixmap& src)
{
	// Thumbnail Size (640, 360)
	QPixmap dest(src.size());
	dest.fill(Qt::transparent);

	QPainter painter(&dest);
	if (!painter.isActive()) return dest;

	painter.drawPixmap(0, 0, src);

	painter.setBrush(QColor(0, 0, 0, 158));

	QPainterPath path;
	path.addRoundedRect(QRect(-40, -20, 360, 150), 20, 20);
	painter.drawPath(path);

	QFont font;
	font.setPointSize(45);
	font.setWeight(QFont::Normal);

	painter.setPen(QColor(0, 163, 255));
	painter.setFont(font);

	painter.drawText(38, 93, QTStr("Popup.VodSource.RecentlyPlay"));

	return dest;
}

QPixmap AFQVodListItem::_MakeDurationPixmap(const QPixmap& src, QString duration)
{
	// Thumbnail Size (640, 360)
	QPixmap dest(src.size());
	dest.fill(Qt::transparent);

	QPainter painter(&dest);
	if (!painter.isActive()) return dest;

	painter.drawPixmap(0, 0, src);

	painter.setBrush(QColor(0, 0, 0, 158));

	QRect durationRect(380, 240, 240, 100);

	QPainterPath path;
	path.addRoundedRect(durationRect, 50, 50);
	painter.drawPath(path);

	QFont font;
	font.setPointSize(45);
	font.setWeight(QFont::Medium);

	painter.setPen(QColor(252, 252, 253));
	painter.setFont(font);

	painter.drawText(durationRect, Qt::AlignCenter, duration);

	return dest;
}

QPixmap AFQVodListItem::_MakeDimAndTextPixmap(const QPixmap& src, const int fontSize, const QString& text)
{
	QPixmap dest(src.size());
	dest.fill(Qt::transparent);

	QPainter painter(&dest);
	if (!painter.isActive()) return dest;

	painter.drawPixmap(0, 0, src);

	painter.setBrush(QColor(0, 0, 0, 178));
	painter.drawRect(dest.rect());

	QFont font;
	font.setPointSize(10);
	font.setWeight(QFont::Normal);

	painter.setPen(QColor(0, 163, 255));
	painter.setFont(font);

	QFontMetrics fontMetrics(font);

	int textWidth = fontMetrics.horizontalAdvance(text);

	int startX = (dest.width() - textWidth) / 2;
	int startY = (dest.height() + fontMetrics.ascent()) / 2;

	painter.drawText(startX, startY, text);

	return dest;
}

QPixmap AFQVodListItem::_MakeSymbolPixmap(const QPixmap& src, const QString& symbolPath)
{
	QPixmap dest(src.size());
	dest.fill(Qt::transparent);

	QPainter painter(&dest);
	if (!painter.isActive()) return dest;

	painter.drawPixmap(0, 0, src);

	painter.setBrush(QColor(0, 0, 0, 178));
	painter.drawRect(dest.rect());

	QSvgRenderer svgRenderer(symbolPath);
	if (svgRenderer.isValid()) {
		QSize svgSize = svgRenderer.defaultSize();
		if (svgSize.isEmpty()) {
			svgSize = QSize(50, 50);
		}

		QRect targetRect(
			(dest.width() - svgSize.width()) / 2,
			(dest.height() - svgSize.height()) / 2,
			svgSize.width(),
			svgSize.height()
		);

		svgRenderer.render(&painter, targetRect);
	}

	return dest;
}

QPixmap AFQVodListItem::_MakeRoundedPixmap(const QPixmap& src, int radius)
{
	QPixmap dest(src.size());
	dest.fill(Qt::transparent);

	QPainter painter(&dest);
	if (!painter.isActive()) return dest;

	painter.setRenderHint(QPainter::Antialiasing);
	QPainterPath path;
	path.addRoundedRect(0, 0, src.width(), src.height(), radius, radius);
	painter.setClipPath(path);
	painter.drawPixmap(0, 0, src);

	return dest;
}

void AFQVodListItem::_ApplyPixmap(STATE state)
{
	QPixmap roundedPixmap, tempPixmap;

	if (m_pixmapDuration.isNull()) {
		//blog(LOG_WARNING, "Skipping _ApplyPixmap: m_pixmapDuration is null");
		return;
	}

	QPixmap scaledDimPixmap;
	if (m_info.recentlyPlay && !m_playing) {
		scaledDimPixmap = _MakeRecentlyPlayPixmap(m_pixmapDuration)
							.scaled(ui->label_Thumbnail->size(),
							Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	}
	else {
		scaledDimPixmap = m_pixmapDuration.scaled(ui->label_Thumbnail->size(),
			Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	}

	if (state == STATE::NORMAL) {
		tempPixmap = scaledDimPixmap;
	}
	else if (state == STATE::HOVER) {
		std::string absPath;
		GetDataFilePath("assets", absPath);
		QString symbolPath = QString("%1/source-props/soop-source/img_vod_thumb_play.svg").arg(absPath.data());
		tempPixmap = _MakeSymbolPixmap(scaledDimPixmap, symbolPath);
	}
	else if (state == STATE::PLAYING) {
		tempPixmap = _MakeDimAndTextPixmap(scaledDimPixmap, 11, QTStr("Popup.VodSource.Playing"));
	}

	roundedPixmap = _MakeRoundedPixmap(tempPixmap, 7);
	ui->label_Thumbnail->setPixmap(roundedPixmap);
	ui->label_Thumbnail->setText("");
}