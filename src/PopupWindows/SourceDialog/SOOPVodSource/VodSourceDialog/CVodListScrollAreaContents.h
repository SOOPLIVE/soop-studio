#pragma once

#include <vector>
#include <QTimer>
#include <QWidget>
#include <QBoxLayout>
#include <QStaticText>
#include <QScrollArea>

class AFQVodSourceDialog;

class AFQVodListScrollAreaContents : public QWidget
{
	Q_OBJECT
public:
	AFQVodListScrollAreaContents(QWidget* parent = nullptr);
	~AFQVodListScrollAreaContents();

protected:
	virtual void paintEvent(QPaintEvent* event) override;

public:
	void SetVodDialogPtr(AFQVodSourceDialog* vodDialog);
	void SetVodEmtpyPlayListMessage(QString msg_1, QString msg_2);

private:
	QStaticText  m_textNoItem;
	QStaticText  m_textNoItem_2;
	AFQVodSourceDialog* m_pVodDialog = nullptr;

	int  m_vodListCount = 0;
};
