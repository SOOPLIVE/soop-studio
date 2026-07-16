#include "CSceneListItem.h"

#include <QStyle>
#include <QEvent>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QFontMetrics>
#include <QPixmap>
#include <QLayout>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "display-helpers.hpp"

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

	m_pTimerHoverPreview = new QTimer(this);
	connect(m_pTimerHoverPreview, &QTimer::timeout, 
			this, &AFQSceneListItem::qslotTimerHoverPreview);

}

AFQSceneListItem::~AFQSceneListItem()
{
	if (m_sceneListPreviewWidget) {
		m_sceneListPreviewWidget->close();
		m_sceneListPreviewWidget = nullptr;
	}
}

void AFQSceneListItem::qslotRenameSceneItem()
{
    m_editSceneName = true;

	m_pFavoriteSceneButton->hide();
	m_pLabelSceneName->hide();

	const char* name = GetSceneName();

	m_pTextEdit->setText(name);

	m_pTextEdit->selectAll();
	m_pTextEdit->setFocus();
	m_pTextEdit->show();
}

void AFQSceneListItem::qslotFavoriteSceneItem(bool checked)
{
	obs_source_t* source = obs_scene_get_source(m_obsScene);
	obs_data_t* scene_data = obs_source_get_settings(source);

	bool refreshSceneList = false;
	bool checked_ = false;
	int favorite_scene = obs_data_get_int(scene_data, "favorite_scene");
	if (1 == favorite_scene) {
		obs_data_set_int(scene_data, "favorite_scene", 0);
		refreshSceneList = true;
		checked_ = false;
	}
	else {
		const int nMaxFavoriteSceneSize = SCENE_CONTEXT.GetFavoriteSceneMaxCount();
		int nFavoriteSceneCount = SCENE_CONTEXT.GetFavoriteSceneCount();
		if (nFavoriteSceneCount == nMaxFavoriteSceneSize) {
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
				"", QTStr("Basic.Scene.FavoriteInfo"), false, true);
			checked_ = false;
		}
		else {
			obs_data_set_int(scene_data, "favorite_scene", 1);
			refreshSceneList = true;
			checked_ = true;
		}
	}
	obs_data_release(scene_data);

	// favorit scene button check -> void AFMainFrame::RefreshSceneUI()
	if(checked != checked_)
		m_pFavoriteSceneButton->setChecked(checked_);

	if (refreshSceneList)
		MAINFRAME->RefreshSceneUI();
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

void AFQSceneListItem::qslotSetHoverSceneItemUI(bool hoverd)
{
	if (!m_pFavoriteSceneButton /*|| !m_preview*/)
		return;

	if(!m_selected)
		_SetHoverStyleSheet(hoverd);

	if (hoverd) {
		if (!m_editSceneName)
			m_pFavoriteSceneButton->show();

		if (!m_selected)
			_ShowPreview(true);
	}
	else {
		m_pFavoriteSceneButton->hide();
		_ShowPreview(false);
	}

	//if (!m_selected)
	//	qSignalHoverSceneItem(hoverd ? m_obsScene : nullptr);

}

void AFQSceneListItem::qslotTimerHoverPreview()
{
	m_pTimerHoverPreview->stop();

	if (m_sceneListPreviewWidget)
		return;

	m_sceneListPreviewWidget = new AFQSceneListPreview(nullptr, obs_scene_get_source(m_obsScene));

	QPoint pos = QCursor::pos();

	QRect pointInScreenRect;
	QList<QScreen*> screens = QGuiApplication::screens();
	for (QScreen* screen : screens) {
		QRect rcScreen = screen->geometry();
		if (rcScreen.contains(pos)) {
			pointInScreenRect = rcScreen;
			break;
		}
	}

	int x = pos.x();
	int y = pos.y();

	if (pos.x() + m_sceneListPreviewWidget->width() > pointInScreenRect.x() + pointInScreenRect.width())
		x = pos.x() - m_sceneListPreviewWidget->width() - 10;
	else
		x = pos.x() + 10;

	if (pos.y() + m_sceneListPreviewWidget->height() > pointInScreenRect.y() + pointInScreenRect.height())
		y = pos.y() - m_sceneListPreviewWidget->height() - 10;
	else
		y = pos.y() + 10;

	pos.setX(x);
	pos.setY(y);

	m_sceneListPreviewWidget->move(pos);
	m_sceneListPreviewWidget->show();
}

void AFQSceneListItem::SelectScene(bool select)
{
	obs_source_t* source = obs_scene_get_source(m_obsScene);
	obs_data_t* scene_data = obs_source_get_settings(source);

	if (select) {
		setProperty("sceneBtnType", "selected");
		m_pLabelSceneIndex->setProperty("selected", true);
		m_pLabelSceneName->setProperty("selected", true);
	}
	else {
		setProperty("sceneBtnType", "idle");
		m_pLabelSceneIndex->setProperty("selected", false);
		m_pLabelSceneName->setProperty("selected", "false");
	}

    m_selected = select;

	PolishStyleSheet(m_pLabelSceneIndex);
	PolishStyleSheet(m_pLabelSceneName);
	PolishStyleSheet(this);
}


void AFQSceneListItem::SetSceneIndexLabelNum(int index)
{
	if (!m_pLabelSceneIndex)
		return;

	m_sceneIndex = index;

	QString sceneIndex = QString("%1").arg(index + 1);
	m_pLabelSceneIndex->setText(sceneIndex);
}

void AFQSceneListItem::SetFavoriteSceneButton(bool favorite)
{
	if (m_pFavoriteSceneButton)
		m_pFavoriteSceneButton->setChecked(favorite);
}

void AFQSceneListItem::ShowRenameSceneUI()
{
	emit qsignalShowRenameSceneUI();
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
        m_changingName = true;
		_ChangeSceneName(false);
		return true;
	}

	if (obj == m_pTextEdit && LineEditChanged(event) && !m_changingName) {
        m_changingName = true;
		_ChangeSceneName(true);
		return true;
	}

	return QFrame::eventFilter(obj, event);
}

void AFQSceneListItem::mousePressEvent(QMouseEvent* event)
{
	SCENE_CONTEXT.SetCurSelectedSceneItem(this);

	m_startPos = mapToParent(event->pos());

	_ShowPreview(false);

	emit qsignalClickedSceneItem();
}

void AFQSceneListItem::mouseDoubleClickEvent(QMouseEvent* event)
{
	qslotRenameSceneItem();

	emit qsignalDoubleClickedSceneItem();
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
    m_hovered = true;

	qslotSetHoverSceneItemUI(true);

	emit qsignalHoverButton(QString());

	QFrame::enterEvent(event);
}

void AFQSceneListItem::leaveEvent(QEvent* event)
{
    m_hovered = false;

	qslotSetHoverSceneItemUI(false);

	emit qsignalLeaveButton();

	QFrame::leaveEvent(event);
} 

void AFQSceneListItem::_CreateSceneItemUI(QString scene_name)
{
	QHBoxLayout* hSceneInfoLayout = new QHBoxLayout(this);
	hSceneInfoLayout->setSpacing(0);
	hSceneInfoLayout->setContentsMargins(0, 0, 0, 0);

	m_pLabelSceneIndex = new QLabel(this);
	m_pLabelSceneIndex->setObjectName("label_SceneIndex");
	m_pLabelSceneIndex->setFixedHeight(22);
	m_pLabelSceneIndex->setText("");

	m_pLabelSceneName = new AFQElidedSlideLabel(this);
	m_pLabelSceneName->setFixedHeight(20);
	m_pLabelSceneName->setText(scene_name);
	m_pLabelSceneName->setObjectName("label_SceneName");

	connect(this, &AFQSceneListItem::qsignalHoverButton, m_pLabelSceneName, &AFQElidedSlideLabel::qslotHoverButton);
	connect(this, &AFQSceneListItem::qsignalLeaveButton, m_pLabelSceneName, &AFQElidedSlideLabel::qslotLeaveButton);

	m_pTextEdit = new QLineEdit(this);
	m_pTextEdit->setTextMargins(4, 0, 0, 0);
	m_pTextEdit->setText(scene_name);
	m_pTextEdit->hide();

	m_pTextEdit->installEventFilter(this);
	m_pTextEdit->setObjectName("sceneNameEdit");

	m_pFavoriteSceneButton = new QPushButton(this);
	m_pFavoriteSceneButton->setObjectName("favoriteSceneButton");
	m_pFavoriteSceneButton->setCheckable(true);
	m_pFavoriteSceneButton->setFixedSize(20, 20);
	m_pFavoriteSceneButton->setIconSize(QSize(20, 20));
	m_pFavoriteSceneButton->setContentsMargins(0, 0, 0, 0);
	m_pFavoriteSceneButton->hide();

	obs_source_t* source = obs_scene_get_source(m_obsScene);
	obs_data_t* scene_data = obs_source_get_settings(source);

	int favorite_scene = obs_data_get_int(scene_data, "favorite_scene");
	m_pFavoriteSceneButton->setChecked(favorite_scene);

	connect(m_pFavoriteSceneButton, &QPushButton::clicked, this, &AFQSceneListItem::qslotFavoriteSceneItem);
	connect(this, &AFQSceneListItem::qsignalShowRenameSceneUI, this, &AFQSceneListItem::qslotRenameSceneItem);

	hSceneInfoLayout->addSpacerItem(new QSpacerItem(10, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
	hSceneInfoLayout->addWidget(m_pLabelSceneIndex);
	hSceneInfoLayout->addWidget(m_pTextEdit);

	hSceneInfoLayout->addSpacerItem(new QSpacerItem(6, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
	hSceneInfoLayout->addWidget(m_pLabelSceneName);

	hSceneInfoLayout->addSpacerItem(new QSpacerItem(6, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
	hSceneInfoLayout->addWidget(m_pFavoriteSceneButton);
	hSceneInfoLayout->addSpacerItem(new QSpacerItem(14, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

	this->setLayout(hSceneInfoLayout);
}

void AFQSceneListItem::_StartDrag(QPoint pos)
{
	if (pos.x() <= 0 || pos.y() <= 0)
		return;

	QMimeData* mimeData = new QMimeData;
	QString data = QString("%1|%2|%3").arg(m_sceneIndex).arg(pos.x()).arg(pos.y());
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
	m_pTextEdit->hide();
	if(m_hovered)
		m_pFavoriteSceneButton->show();
	m_pLabelSceneName->show();

    m_editSceneName = false;
    m_changingName = false;

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
										   Str("NameExists.Text"));

			}
			else if (sceneName.empty()) {
				AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
										   QT_UTF8(""),
										   Str("NameExists.Text"));
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
			UNDO_STACK.AddAction(QTStr("Undo.Rename").arg(sceneName.c_str()),
								 undo, redo,
								 source_uuid, source_uuid);

			m_pLabelSceneName->setText(QT_UTF8(sceneName.c_str()));
			obs_source_set_name(source, sceneName.c_str());
			emit qsignalRenameSceneItem();
		}
	}
}

void AFQSceneListItem::_SetHoverStyleSheet(bool hover)
{
	if (hover) {
		setProperty("sceneBtnType", "hover");
	}
	else {
		if(m_selected)
			setProperty("sceneBtnType", "selected");
		else
			setProperty("sceneBtnType", "idle");
	}

	PolishStyleSheet(this);
}


void AFQSceneListItem::_ShowPreview(bool on)
{
	if (on) {
		m_pTimerHoverPreview->start(300);
	}

	if (!on) {
		m_pTimerHoverPreview->stop();

		if (m_sceneListPreviewWidget) {
			m_sceneListPreviewWidget->close();
			m_sceneListPreviewWidget = nullptr;
		}
	}
}

OBSSource AFQSceneListItem::_GetSceneSource()
{
	return obs_scene_get_source(m_obsScene);
}

void AFQSceneListItem::_SceneListPreviewRender(void* data, uint32_t cx, uint32_t cy)
{
	AFQSceneListItem* window = static_cast<AFQSceneListItem*>(data);

	if (!window->_GetSceneSource())
		return;

	uint32_t sourceCX = std::max(obs_source_get_width(window->_GetSceneSource()), 1u);
	uint32_t sourceCY = std::max(obs_source_get_height(window->_GetSceneSource()), 1u);

	int x, y;
	int newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	gs_viewport_push();
	gs_projection_push();
	const bool previous = gs_set_linear_srgb(true);

	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);

	obs_source_video_render(window->_GetSceneSource());

	gs_set_linear_srgb(previous);
	gs_projection_pop();
	gs_viewport_pop();
}













AFQSceneListPreview::AFQSceneListPreview(QWidget* parent, OBSSource source) :
	QWidget(parent),
	m_sceneSource(source)
{
	m_weakSceneSource = OBSGetWeakRef(m_sceneSource);

	setAttribute(Qt::WA_DeleteOnClose, true);
	setFocusPolicy(Qt::NoFocus);
	setWindowFlags(Qt::ToolTip);
	hide();

	QVBoxLayout* vLayout = new QVBoxLayout();	
	setFixedSize(220, 129);
	setStyleSheet("QWidget { background-color : #3A3D42; }");
	vLayout->setContentsMargins(QMargins(7, 7, 7, 7));
	setLayout(vLayout);

	m_previewScene = new AFQTDisplay(this);
	m_previewScene->setFixedSize(206, 115);

	vLayout->addWidget(m_previewScene);

	auto addDrawCallback = [this]() {
		obs_display_add_draw_callback(
			m_previewScene->GetDisplay(), AFQSceneListPreview::SceneListPreviewRender, this);
		obs_display_set_background_color(m_previewScene->GetDisplay(), GREY_COLOR_BACKGROUND);
	};

	connect(m_previewScene, &AFQTDisplay::qsignalDisplayCreated, addDrawCallback);

	if (m_sceneSource)
		obs_source_inc_showing(m_sceneSource);

}
AFQSceneListPreview::~AFQSceneListPreview()
{
	obs_display_remove_draw_callback(m_previewScene->GetDisplay(),
		AFQSceneListPreview::SceneListPreviewRender, this);

	if (m_sceneSource)
		obs_source_dec_showing(m_sceneSource);

	m_previewScene = nullptr;

	m_sceneSource = nullptr;
}

OBSSource AFQSceneListPreview::GetSceneSource()
{
	return OBSGetStrongRef(m_weakSceneSource);
}
void AFQSceneListPreview::SceneListPreviewRender(void* data, uint32_t cx, uint32_t cy)
{
	AFQSceneListPreview* window = static_cast<AFQSceneListPreview*>(data);

	if (!window->GetSceneSource())
		return;

	uint32_t sourceCX = std::max(obs_source_get_width(window->GetSceneSource()), 1u);
	uint32_t sourceCY = std::max(obs_source_get_height(window->GetSceneSource()), 1u);

	int x, y;
	int newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	gs_viewport_push();
	gs_projection_push();
	const bool previous = gs_set_linear_srgb(true);

	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);

	obs_source_video_render(window->GetSceneSource());

	gs_set_linear_srgb(previous);
	gs_projection_pop();
	gs_viewport_pop();
}
