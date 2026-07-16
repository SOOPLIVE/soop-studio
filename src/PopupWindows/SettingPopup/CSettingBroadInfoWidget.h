#pragma once
#include <QWidget>
#include <QPointer>
#include "PopupWindows/BroadInfoPopup/CStickerItemsDialog.h"

class AFQStreamSettingAreaWidget;

namespace Ui {
	class AFQSettingBroadInfoWidget;
}

class AFQSettingBroadInfoWidget : public QWidget
{
	Q_OBJECT

public:
	enum class BroadInfoPage {
		BroadInfo_Page,
		BroadAttribute_Page,
		Permission_Page
	};
	Q_ENUM(BroadInfoPage);

	enum class Permission {
		FanClub,
		Subscription,
		BlackList,
		VODAuth,
		Editor,
		UserClip_BlackList,
		SubscribeSetting
	};

#pragma region QT Field, CTOR/DTOR
public:
	AFQSettingBroadInfoWidget(QWidget* parent);
	~AFQSettingBroadInfoWidget();

signals:
	void qsignalDataChanged();

public slots:

private slots:
	void _qslotSetCurrentPage(BroadInfoPage page);
	void _qslotClickedManagePermission(Permission permission);
	void _qslotClickedHideStreamButton();
	void _qslotClickedCategoryButton();
	void _qslotClickedAddTagButton();
	void _qslotCategoryChanged(const std::string& categoryNum, const std::string& categoryName);
	void _qslotStreamTagChanged(const std::vector<std::string>& tags);
	void _qslotClickedAdultOnlyButton();
	void _qslotClickedUsePasswordButton();
	void _qslotClickedWatermarkButton();
	void _qslotClickedStickerItemButton();
	void _qslotClickedSubscribeBroadButton(bool checked);
	void _qslotReceiveSubscribeBroadAvailableSetting(const QByteArray& responseData);

	void _qslotUsePasswordChanged(bool usePassword, const QString& password);
	void _qslotWatermarkPositionChanged(int watermarkPos);

	void _qslotDataChanged();
	void _qslotTitleFocus(bool focusin);

	void _qslotSendBroadInfoReceived(int result, QString msg);
	void _qslotRequestBroadInfoReceived(int result, QString msg);

	void _qslotReceiveActiveItemInfo();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	bool SaveSettings();
	void LoadBroadInfoDatas();

	bool IsDataChanged() { return m_dataChanged; }
	void SetDataChanged(bool changed) { m_dataChanged = changed; }

	bool GetSubscribeLive();

	void TabButtonClick(BroadInfoPage tabNum);
#pragma endregion public func

#pragma region private func
private:
	void _Init();
	void _InitToolTip();
	void _InitWatermarkLocale();

	void _ConnectEvents();
	void _ConnectDataChangeEvents();
	void _RefreshCategoryName();

	void _StartReceivingActiveItemInfoTimer();
	void _StopReceivingActiveItemInfoTimer();

	bool _RefreshTags();
	bool _AddTags(std::vector<std::string> tags);

	QString _GetWatermarkPosTextByIndex(int idx);
#pragma endregion private func

#pragma region private member var
private:
	Ui::AFQSettingBroadInfoWidget* ui;

	AFQStreamSettingAreaWidget* m_streamSetting;

	bool m_loading = true;
	bool m_dataChanged = false;

	// Temporary data for discarding changes
	bool					 m_tempUsePassword;
	std::string				 m_tempPassword;
	std::string				 m_tempCategoryName;
	std::string				 m_tempCategoryNum;
	std::vector<std::string> m_tempStreamTags;
	int						 m_tempWatermarkPos;
	//

	std::vector<QString>	m_watermarkPosTexts;
	QPointer<QTimer>		m_receiveActiveItemInfoTimer = nullptr;
	QPointer<AFQStickerItemsDialog> m_stickerItemsDialog = nullptr;

	QString m_previousTitle = "";
	QString m_titleOnLoad = "";

	bool m_previousSubscribe = false;

#pragma endregion private member var

};
