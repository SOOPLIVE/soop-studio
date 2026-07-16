#pragma once
#include "UIComponent/CTopBaseWindow.h"
#include <QAbstractButton>

#include "Blocks/CBlockManager.h"

namespace Ui {
	class AFQStickerItemsDialog;
}

class AFQStickerItemsDialog : public AFTTopBaseDialog
{
	Q_OBJECT

#pragma region class initializer, destructor
public:
	explicit AFQStickerItemsDialog(QWidget* parent);
	~AFQStickerItemsDialog();

private slots:
	void _qslotClickedExchangeStickerItemButton();
	void _qslotClickedSendMobileAlarmButton();
	void _qslotClickedBurnningTenButton();
	void _qslotResponseUseBurnningTenAPI(const QByteArray& responseData);
	void _qslotDataReceivedSticker(const QCefQuery& query);
#pragma endregion class initializer, destructor

#pragma region public func
public:
	void UpdateUI();
#pragma endregion public func

#pragma region protected func
protected:
	void closeEvent(QCloseEvent* event) override;
#pragma endregion protected func

#pragma region private func
private:
	void _Init();
	QString _GetFormattedTime(int addSec = 0);
#pragma endregion private func

#pragma region private member var
private:
	Ui::AFQStickerItemsDialog* ui;

	QCefWidget* m_pCefWidget = nullptr;



#pragma endregion private member var
};
