#pragma once

#include <vector>
#include <QTimer>

#include <QAbstractListModel>
#include <QStandardItemModel>

#include "obs.hpp"

#include <util/util.hpp>

#include "UIComponent/CTopBaseWindow.h"

#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

namespace Ui {
	class AFQVodSourceDialog;
}

class AFQVodListItem;
class AFQVodContentWidget;
class AFQVodContentSeasonItem;
class AFQStandByImagePrinter;
class SOOPApiHandler;

class AFQVodSourceDialog : public AFTTopBaseDialog
{
	Q_OBJECT

public:
	AFQVodSourceDialog(QWidget* parent, obs_source_t* source);
	~AFQVodSourceDialog();

private slots:
	// button
	void qslotCloseButtonClicked();
	void qslotRefreshButtonClicked();
	void qslotShowContentList(bool checked);
	void qslotToggleRepeatSeries(bool checked);

	void qslotContentItemClicked(int contentIdx);
	void qslotAddContentSeasonVodItems(QString season);

	void qslotPlayVOD(VodInfo_s info);
	void qslotBackButtonClicked();
	void qslotPauseVodButtonClicked();
	void qslotPlayVodButtonClicked();
	void qslotPrevVodButtonClicked();
	void qslotNextVodButtonClicked();
	void qslotStopvodButtonClicked();
	void qslotRepeatButtonClicked(bool checked);
	void qslotRepeatSeriesInfoButtonClicked();

	// timer
	void qslotSliderPosition();
#ifdef _SOOP_VLC
	void qslotSeekTimerCallback();
#endif // _SOOP_VLC
	void qslotSeekForwardTimer();
	void qslotSeekBackwardTimer();
	void qslotPlayNextVodTimer();

	// slider
	void qslotMediaSliderClicked();
	void qslotMediaSliderReleased();
	void qslotMediaSliderMoved(int val);
	void qslotMediaSliderForward();
	void qslotMediaSliderBackward();

	void _qslotBuildVodListTick();

public slots:
	void qslotRefreshContents();
	void qslotRefreshSeasons(QString content, int contentNo, int recentlyListLoad);
	
	// common SOOP Source Props
	void qslotRecvOBSMediaStarted();
	void qslotRecvOBSMediaStopped();
	void qslotRecvOBSMediaEnded();
	void qslotRecvOBSMediaPlay();
	void qslotRecvOBSMediaPause();

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

	// common SOOP Source Props
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;

public:
	bool IsEmptyVodPlayList();
	void SetSource(obs_source_t* source);

private:
	bool _IsValidSource(OBSSource source);

	bool _SetVodListItemStatus(VodInfo_s vodInfo);
	void _MakeVodPlayList(QString season);

	void _PlayMedia();
	void _PauseMedia();
	void _StopMedia();

	void _StartMediaTimer();
	void _StopMediaTimer();

	void _RegisterUISignals();
	void _RefreshControls();

	QString _FormatSeconds(int totalSeconds);
	int64_t _GetSliderTime(int val);

	void _SetVODPlayingState();
	void _SetVODPauseState();
	void _SetVODStopState();

	bool _CheckVodEnableBroadSetting(VodInfo_s vodInfo);

	static void _SourceRemoved(void* data, calldata_t* params);

private:
	Ui::AFQVodSourceDialog* ui;

	QTimer m_timerMedia;
#ifdef _SOOP_VLC
	QTimer m_timerSeek;
#endif // _SOOP_VLC
	QTimer m_timerForward;
	QTimer m_timerBackward;
	QTimer m_timerNextVod;

	std::string		m_sourceId;
	SOOP_VOD_TYPE	m_vodType = SOOP_VOD_TYPE::NONE;
	int				m_curContentIdx = 0;
	QString			m_curSeason = "";

	int m_seek = 0;
	int m_lastSeek = 0;
#ifdef _SOOP_VLC
	bool m_prevPaused = false;
#endif // _SOOP_VLC

	bool m_ignoreShowEvent = false;


	std::vector<AFQVodListItem*> m_vodListItems;
	std::vector<AFQVodContentWidget*> m_vodContents;
	std::vector<AFQVodContentSeasonItem*> m_vodContentSeasons;

	std::vector<VodInfo_s> m_tmpMakeVodList;
	std::vector<VodInfo_s>::iterator m_iteratorMakeVodList;
	int m_scrollHeight = 0;

	QTimer* m_vodListBuildTimer = nullptr;
	static constexpr int kBuildIntervalMs = 50;
	static constexpr int kBuildItemPerTick = 1;

	OBSSignal removeSignal;
	OBSWeakSource weakSource = nullptr;
};
