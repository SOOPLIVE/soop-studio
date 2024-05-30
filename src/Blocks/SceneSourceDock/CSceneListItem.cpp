#include "CSceneListItem.h"

#include <QStyle>
#include <QEvent>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QFontMetrics>
#include <QPixmap>

#include "qt-wrapper.h"
#include "platform/platform.hpp"

#include "CSceneListView.h"

#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Icon/CIconContext.h"

#include "UIComponent/CMessageBox.h"

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"

AFQSceneListItem::AFQSceneListItem(QWidget* parent, 
								   QFrame* sceneListFrame, 
								   OBSScene scene, 
								   QString name, 
								   const SignalContainer<OBSScene>& signalConainter) :
	QFrame(parent),
	m_signalContainer(signalConainter),
	m_obsScene(scene)
{
	setAcceptDrops(true);
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover,true); 

	_CreateSceneItemUI(name);


	m_timerScreenShot = new QTimer(this);
	connect(m_timerScreenShot, &QTimer::timeout, 
			this, &AFQSceneListItem::qSlotTimerScreenShot);

	m_timerHoverPreview = new QTimer(this);
	connect(m_timerHoverPreview, &QTimer::timeout, 
			this, &AFQSceneListItem::qSlotTimerHoverPreview);

}

AFQSceneListItem::~AFQSceneListItem()
{
	if (m_pScreenshotScene)
		delete m_pScreenshotScene;

	if (m_pScreenshotObj)
		delete m_pScreenshotObj;
}

void AFQSceneListItem::qSlotRenameSceneItem()
{
	m_bEditSceneName = true;

	m_pSceneNameEditButton->hide();
	m_pLabelSceneName->hide();

	const char* name = GetSceneName();

	m_pTextEdit->setText(name);

	m_pTextEdit->selectAll();
	m_pTextEdit->setFocus();
	m_pTextEdit->show();
}

inline void clearLayout(QLayout* layout) {
	while (QLayoutItem* item = layout->takeAt(0)) {
		if (QWidget* widget = item->widget()) {
			delete widget;
		}
		else if (QLayout* childLayout = item->layout()) {
			clearLayout(childLayout);
		}
	}
}

void AFQSceneListItem::qSlotSetHoverSceneItemUI(bool hoverd)
{
	if (!m_pSceneNameEditButton /*|| !m_preview*/)
		return;

	if(!m_bSelected)
		_SetHoverStyleSheet(hoverd);

	if (hoverd) {
		if (!m_bEditSceneName)
			m_pSceneNameEditButton->show();

		if (!m_bSelected)
			_ShowPreview(true);
	}
	else {
		m_pSceneNameEditButton->hide();
		_ShowPreview(false);
	}

	//if (!m_bSelected)
	//	qSignalHoverSceneItem(hoverd ? m_obsScene : nullptr);

}

void AFQSceneListItem::qSlotTimerScreenShot()
{
	obs_source_t* source = obs_scene_get_source(m_obsScene);
	delete m_pScreenshotObj;
	m_pScreenshotObj = new AFQScreenShotObj(source, 
											AFQScreenShotObj::Type::Screenshot_SceneButton);

	connect(m_pScreenshotObj, &AFQScreenShotObj::qsignalSetPreview,
			this, &AFQSceneListItem::qslotSetScreenShotPreview);
}

void AFQSceneListItem::qSlotTimerHoverPreview()
{
	m_timerHoverPreview->stop();

	QRect screenRect = QGuiApplication::primaryScreen()->geometry();
	QPoint pos = QCursor::pos();

	int x = pos.x();
	int y = pos.y();

	if (pos.x() + m_pScreenshotScene->width() > screenRect.width())
		x = pos.x() - m_pScreenshotScene->width() - 10;
	else
		x = pos.x() + 10;

	if (pos.y() + m_pScreenshotScene->height() > screenRect.height())
		y = pos.y() - m_pScreenshotScene->height() - 10;
	else
		y = pos.y() + 10;

	pos.setX(x);
	pos.setY(y);

	m_pScreenshotScene->move(pos);
	m_pScreenshotScene->show();
}

void AFQSceneListItem::qslotSetScreenShotPreview()
{
	QPixmap pixmap = m_pScreenshotObj->GetPixmap();
	m_pScreenshotScene->setPixmap(pixmap.scaled(208, 117));
}

void AFQSceneListItem::SelectScene(bool select)
{
	QColor labelFontColor = QColor(255,255,255);
	if (m_nSceneIndex < 4)
		labelFontColor = QColor(0, 224, 255);

	if (select) {
		setProperty("sceneBtnType", "selected");
		m_pLabelSceneIndex->setStyleSheet(QString("QLabel {	"
												  "color : #FFF;"
												  "border-radius: 2px;"
												  "border: 1px solid #67CFDF;"
												  "font-size : 14px; font-style: normal; font-weight: 400; line-height: normal;"
												  "padding:2px 3px 4px 3px; }"));

		m_pLabelSceneName->setStyleSheet("QLabel {			\
										   color : #FFF; padding:2px 0px 4px 0px;}");
	}
	else {
		setProperty("sceneBtnType", "idle");
		m_pLabelSceneIndex->setStyleSheet(QString("QLabel {"
												  "color : %1;"
												  "border-radius: 2px;"
												  "border: 1px solid #3A3D42;"
												  "font-size : 14px; font-style: normal; font-weight: 400; line-height: normal;"
												  "padding:2px 3px 4px 3px; }").arg(labelFontColor.name()));

		m_pLabelSceneName->setStyleSheet("QLabel {			\
										   color : rgba(255, 255, 255, 70%); padding:2px 0px 4px 0px;}");
	}

	m_bSelected = select;

	style()->unpolish(this);
	style()->polish(this);
}


void AFQSceneListItem::SetSceneIndexLabelNum(int index)
{
	if (!m_pLabelSceneIndex)
		return;

	m_nSceneIndex = index;

	QString sceneIndex = QString("%1").arg(index + 1);
	m_pLabelSceneIndex->setText(sceneIndex);
}

void AFQSceneListItem::ShowRenameSceneUI()
{
	emit qSignalShowRenameSceneUI();
}

OBSScene AFQSceneListItem::GetScene()
{
	return m_obsScene;
}

const char* AFQSceneListItem::GetSceneName()
{
	obs_source_t* source = obs_scene_get_source(m_obsScene);
	const char* name = obs_source_get_name(source);

	return name;
}

bool AFQSceneListItem::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == m_pTextEdit && LineEditCanceled(event)) {
		m_bChangingName = true;
		_ChangeSceneName(false);
		return true;
	}

	if (obj == m_pTextEdit && LineEditChanged(event) && !m_bChangingName) {
		m_bChangingName = true;
		_ChangeSceneName(true);
		return true;
	}

	return QFrame::eventFilter(obj, event);
}

void AFQSceneListItem::mousePressEvent(QMouseEvent* event)
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

	sceneContext.SetCurSelectedSceneItem(this);

	m_startPos = mapToParent(event->pos());

	_ShowPreview(false);

	emit qSignalClickedSceneItem();
}

void AFQSceneListItem::mouseDoubleClickEvent(QMouseEvent* event)
{
	emit qSignalDoubleClickedSceneItem();
}

void AFQSceneListItem::mouseReleaseEvent(QMouseEvent* event)
{
    m_startPos.setX(0);
    m_startPos.setY(0);
}

void AFQSceneListItem::mouseMoveEvent(QMouseEvent* event)
{
	if (!m_startPos.isNull() && (event->buttons() & Qt::LeftButton)) {
		int distance = (mapToParent(event->pos()) - m_startPos).manhattanLength();
		int customDragDistance = QApplication::startDragDistance();
		if (distance < customDragDistance)
			return;
	}
	_StartDrag(m_startPos);

	QFrame::mouseMoveEvent(event);
}

void AFQSceneListItem::enterEvent(QEnterEvent* event)
{
	m_bHovered = true;

	qSlotSetHoverSceneItemUI(true);

	emit qSignalHoverButton(QString());

	QFrame::enterEvent(event);
}

void AFQSceneListItem::leaveEvent(QEvent* event)
{
	m_bHovered = false;

	qSlotSetHoverSceneItemUI(false);

	emit qSignalLeaveButton();

	QFrame::leaveEvent(event);
} 

void AFQSceneListItem::_CreateSceneItemUI(QString scene_name)
{
	AFIconContext& iconContext = AFIconContext::GetSingletonInstance();

	QHBoxLayout* hSceneInfoLayout = new QHBoxLayout(this);
	hSceneInfoLayout->setSpacing(0);
	hSceneInfoLayout->setContentsMargins(0, 0, 0, 0);

	m_pLabelSceneIndex = new QLabel(this);
	m_pLabelSceneIndex->setProperty("labelType", "sceneIndex");
	m_pLabelSceneIndex->setFixedHeight(20);
	m_pLabelSceneIndex->setText("");

	m_pLabelSceneName = new AFQElidedSlideLabel(this);
	m_pLabelSceneName->setFixedSize(110,20);
	m_pLabelSceneName->setText(scene_name);
	m_pLabelSceneName->setObjectName("label_SceneName");

	connect(this, &AFQSceneListItem::qSignalHoverButton,
		m_pLabelSceneName, &AFQElidedSlideLabel::qSlotHoverButton);

	connect(this, &AFQSceneListItem::qSignalLeaveButton,
		m_pLabelSceneName, &AFQElidedSlideLabel::qSlotLeaveButton);

	m_pTextEdit = new QLineEdit(this);
	m_pTextEdit->setTextMargins(4, 0, 0, 0);
	m_pTextEdit->setText(scene_name);
	m_pTextEdit->hide();

	m_pTextEdit->installEventFilter(this);
	m_pTextEdit->setObjectName("sceneNameEdit");

	m_pSceneNameEditButton = new QPushButton(this);
	m_pSceneNameEditButton->setObjectName("renameSceneEditBtn");
	m_pSceneNameEditButton->setFixedSize(12, 12);
	m_pSceneNameEditButton->setContentsMargins(0, 0, 0, 0);

	m_pSceneNameEditButton->hide();

	connect(m_pSceneNameEditButton, &QPushButton::clicked,
			this, &AFQSceneListItem::qSlotRenameSceneItem);

	connect(this, &AFQSceneListItem::qSignalShowRenameSceneUI,
			this, &AFQSceneListItem::qSlotRenameSceneItem);

	hSceneInfoLayout->addSpacerItem(new QSpacerItem(10, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
	hSceneInfoLayout->addWidget(m_pLabelSceneIndex);
	hSceneInfoLayout->addWidget(m_pTextEdit);

	hSceneInfoLayout->addSpacerItem(new QSpacerItem(6, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
	hSceneInfoLayout->addWidget(m_pLabelSceneName);

	hSceneInfoLayout->addSpacerItem(new QSpacerItem(6, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
	hSceneInfoLayout->addWidget(m_pSceneNameEditButton);
	hSceneInfoLayout->addSpacerItem(new QSpacerItem(14, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

	m_pScreenshotScene = new QLabel(nullptr);
	m_pScreenshotScene->setFocusPolicy(Qt::NoFocus);
	m_pScreenshotScene->setWindowFlags(Qt::ToolTip);
	m_pScreenshotScene->setStyleSheet("QLabel { border: 1px solid #00E0FF; }");
	m_pScreenshotScene->setFixedSize(208, 117);
	m_pScreenshotScene->hide();

	this->setLayout(hSceneInfoLayout);

}

void AFQSceneListItem::_StartDrag(QPoint pos)
{
	if (pos.x() <= 0 || pos.y() <= 0)
		return;

	QMimeData* mimeData = new QMimeData;
	QString data = QString("%1|%2|%3").arg(m_nSceneIndex).arg(pos.x()).arg(pos.y());
	mimeData->setData(SCENE_ITEM_DRAG_MIME, data.toStdString().c_str());

	QDrag* drag = new QDrag(this);
	drag->setMimeData(mimeData);
	drag->exec();
	drag->deleteLater();

	m_startPos.setX(0);
	m_startPos.setY(0);
}

void AFQSceneListItem::_ChangeSceneName(bool change)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	m_pTextEdit->hide();
	if(m_bHovered)
		m_pSceneNameEditButton->show();
	m_pLabelSceneName->show();

	m_bEditSceneName = false;
	m_bChangingName = false;

	if (change) {

		std::string sceneName = QT_TO_UTF8(m_pTextEdit->text().trimmed());
		
		obs_source_t* source = obs_scene_get_source(m_obsScene);
		const char* prevName = obs_source_get_name(source);
		if (prevName == sceneName)
			return;

		OBSSourceAutoRelease foundSource = obs_get_source_by_name(sceneName.c_str());
		if (foundSource || sceneName.empty()) {
			m_pLabelSceneName->setText(QT_UTF8(prevName));

			if (foundSource) {
				AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
										   QT_UTF8(""),
										   locale.Str("NameExists.Text"));

			}
			else if (sceneName.empty()) {
				AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
										   QT_UTF8(""),
										   locale.Str("NameExists.Text"));
			}

		} else {
			auto undo = [prev = std::string(prevName)](const std::string& data) {
				OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
				obs_source_set_name(source, prev.c_str());
			};

			auto redo = [sceneName](const std::string& data) {
				OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
				obs_source_set_name(source, sceneName.c_str());
			};

			std::string source_uuid(obs_source_get_uuid(source));
			AFMainFrame* main = App()->GetMainView();
			main->m_undo_s.AddAction(QTStr("Undo.Rename").arg(sceneName.c_str()),
									 undo, redo,
									 source_uuid, source_uuid);

			m_pLabelSceneName->setText(QT_UTF8(sceneName.c_str()));
			obs_source_set_name(source, sceneName.c_str());
			emit qSignalRenameSceneItem();
		}
	}
}

void AFQSceneListItem::_SetHoverStyleSheet(bool hover)
{
	if (hover) {
		setProperty("sceneBtnType", "hover");
	}
	else {
		if(m_bSelected)
			setProperty("sceneBtnType", "selected");
		else
			setProperty("sceneBtnType", "idle");
	}

	style()->unpolish(this);
	style()->polish(this);
}


void AFQSceneListItem::_ShowPreview(bool on)
{
	if (!m_pScreenshotScene || !m_timerScreenShot)
		return;

	if (on /*&& !m_pScreenshotScene->isVisible()*/) {
		if (!m_pScreenshotScene->isVisible()) {
			if (!m_timerScreenShot->isActive()) {
				qSlotTimerScreenShot();

				m_timerHoverPreview->start(300);
				m_timerScreenShot->start(1000);
			}
		}
	}

	if (!on /*&& m_pScreenshotScene->isVisible()*/) {
		m_timerScreenShot->stop();
		m_timerHoverPreview->stop();
		m_pScreenshotScene->hide();

		if (m_pScreenshotScene)
			m_pScreenshotScene->setPixmap(QPixmap());
	}
}