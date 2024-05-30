#pragma once

#include <QFormLayout>
#include <QScrollArea>
#include <QPointer>
#include <QTimer>
#include <QComboBox>
#include <QSpinBox>
#include <QStackedWidget>

#include <QPushButton>

#include <obs.hpp>

#include "UIComponent/CVScrollArea.h"

class QFormLayout;
class AFQPropertiesView;
class QLabel;

static bool operator!=(const media_frames_per_second& a,
	const media_frames_per_second& b)
{
	return a.numerator != b.numerator || a.denominator != b.denominator;
}

static bool operator==(const media_frames_per_second& a,
	const media_frames_per_second& b)
{
	return !(a != b);
}

using frame_rate_range_t =
std::pair<media_frames_per_second, media_frames_per_second>;
using frame_rate_ranges_t = std::vector<frame_rate_range_t>;

class AFQFrameRatePropertyWidget : public QWidget {
	Q_OBJECT

public:
	frame_rate_ranges_t fps_ranges;

	QComboBox* modeSelect = nullptr;
	QStackedWidget* modeDisplay = nullptr;

	QWidget* labels = nullptr;
	QLabel* currentFPS = nullptr;
	QLabel* timePerFrame = nullptr;
	QLabel* minLabel = nullptr;
	QLabel* maxLabel = nullptr;

	QComboBox* simpleFPS = nullptr;

	QComboBox* fpsRange = nullptr;
	QSpinBox* numEdit = nullptr;
	QSpinBox* denEdit = nullptr;

	bool updating = false;

	const char* name = nullptr;
	obs_data_t* settings = nullptr;

	QLabel* warningLabel = nullptr;

	AFQFrameRatePropertyWidget() = default;
};


typedef obs_properties_t* (*PropertiesReloadCallback)(void* obj);
typedef void (*PropertiesUpdateCallback)(void* obj, obs_data_t* old_settings,
	obs_data_t* new_settings);
typedef void (*PropertiesVisualUpdateCb)(void* obj, obs_data_t* settings);

class WidgetInfo : public QObject {
	Q_OBJECT

		friend class AFQPropertiesView;

private:
	AFQPropertiesView* view;
	obs_property_t* property;
	QWidget* widget;
	QPointer<QTimer> update_timer;
	bool recently_updated = false;
	OBSData old_settings_cache;

	void BoolChanged(const char* setting);
	void IntChanged(const char* setting);
	void FloatChanged(const char* setting);
	void TextChanged(const char* setting);
	bool PathChanged(const char* setting);
	void ListChanged(const char* setting);
	bool ColorChangedInternal(const char* setting, bool supportAlpha);
	bool ColorChanged(const char* setting);
	bool ColorAlphaChanged(const char* setting);
	bool FontChanged(const char* setting);
	void GroupChanged(const char* setting);
	void EditableListChanged();
	void ButtonClicked();

	void TogglePasswordText(bool checked);

public:
	inline WidgetInfo(AFQPropertiesView* view_, obs_property_t* prop,
		QWidget* widget_)
		: view(view_),
		property(prop),
		widget(widget_)
	{
	}

	~WidgetInfo()
	{
		if (update_timer) {
			update_timer->stop();
			QMetaObject::invokeMethod(update_timer, "timeout");
			update_timer->deleteLater();
		}
	}

public slots:
	void ControlChanged();

	///* editable list */
	void EditListAdd();
	void EditListAddText();
	void EditListAddFiles();
	void EditListAddDir();
	void EditListRemove();
	void EditListEdit();
	void EditListUp();
	void EditListDown();
};

/* ------------------------------------------------------------------------- */

class AFQPropertiesView : public VScrollArea
{
#pragma region QT Field
	Q_OBJECT

	friend class WidgetInfo;

	using properties_delete_t = decltype(&obs_properties_destroy);
	using properties_t =
				std::unique_ptr<obs_properties_t, properties_delete_t>;

private slots:
	void qslotResizeWidget(bool empty);

public:
	AFQPropertiesView(OBSData settings, obs_object_t* obj,
					  PropertiesReloadCallback reloadCallback,
					  PropertiesUpdateCallback callback,
					  PropertiesVisualUpdateCb cb = nullptr,
					  int minSize = 0);
	AFQPropertiesView(OBSData settings, void* obj,
					  PropertiesReloadCallback reloadCallback,
					  PropertiesUpdateCallback callback,
					  PropertiesVisualUpdateCb cb = nullptr,
					  int minSize = 0);
	AFQPropertiesView(OBSData settings, const char* type,
					  PropertiesReloadCallback reloadCallback,
					  int minSize = 0);

#define obj_constructor(type)                                              \
	inline AFQPropertiesView(OBSData settings, obs_##type##_t *type,   \
				 PropertiesReloadCallback reloadCallback,  \
				 PropertiesUpdateCallback callback,        \
				 PropertiesVisualUpdateCb cb = nullptr,    \
				 int minSize = 0)                          \
		: AFQPropertiesView(settings, (obs_object_t *)type,        \
				    reloadCallback, callback, cb, minSize) \
	{                                                                  \
	}

	obj_constructor(source);
	obj_constructor(output);
	obj_constructor(encoder);
	obj_constructor(service);
#undef obj_constructor

#define Def_IsObject(type)                                \
	inline bool IsObject(obs_##type##_t *type) const  \
	{                                                 \
		OBSObject obj = OBSGetStrongRef(weakObj); \
		return obj.Get() == (obs_object_t *)type; \
	}

	/* clang-format off */
	Def_IsObject(source)
		Def_IsObject(output)
		Def_IsObject(encoder)
		Def_IsObject(service)
		/* clang-format on */

#undef Def_IsObject

	inline obs_data_t* GetSettings() const { return settings; }

	~AFQPropertiesView();

#pragma endregion QT Field

private:
	template<typename Sender, typename SenderParent, typename... Args>
	QWidget* NewWidget(obs_property_t* prop, Sender* widget,
		void (SenderParent::* signal)(Args...));

	QWidget* AddCheckbox(obs_property_t* prop);
	QWidget* AddText(obs_property_t* prop, QFormLayout* layout,
		QLabel*& label);
	void AddPath(obs_property_t* prop, QFormLayout* layout, QLabel** label);
	void AddInt(obs_property_t* prop, QFormLayout* layout, QLabel** label);
	void AddFloat(obs_property_t* prop, QFormLayout* layout,
				  QLabel** label);
	QWidget* AddList(obs_property_t* prop, bool& warning, bool group);
	void AddEditableList(obs_property_t* prop, QFormLayout* layout,
						 QLabel*& label);
	QWidget* AddButton(obs_property_t* prop);
	void AddColorInternal(obs_property_t* prop, QFormLayout* layout,
						  QLabel*& label, bool supportAlpha);
	void AddColor(obs_property_t* prop, QFormLayout* layout,
				  QLabel*& label);
	void AddColorAlpha(obs_property_t* prop, QFormLayout* layout,
					   QLabel*& label);
	void AddFont(obs_property_t* prop, QFormLayout* layout, QLabel*& label);
	void AddFrameRate(obs_property_t* prop, bool& warning,
					  QFormLayout* layout, QLabel*& label);

	void AddGroup(obs_property_t* prop, QFormLayout* layout);

	void AddProperty(obs_property_t* property, QFormLayout* layout, bool group = false);

	void CreatePropsMainWidget();

	void AddControlPanelButton(QFormLayout* layout);

	void AddAdvancedPropButton(QFormLayout* layout);
	bool IsHideAdvancedProperties();

	void GetScrollPos(int& h, int& v, int& hend, int& vend);
	void SetScrollPos(int h, int v, int old_hend, int old_vend);


	inline OBSObject GetOBSObject() const { return OBSGetStrongRef(weakObj); }
	OBSSource GetSourceObject();

	bool IsExistAdvanceMode(QString id);
	void SetAdvancedPropertiesById(QString id, bool showAdvancedProps);
	void SetGameCaptureProps(bool showProp);
	void SetWindowCaptureProps(bool showProp);
	void SetDshowInputProps(bool showProp);
	void SetFFmpegSourceProps(bool showProp);
	void SetTextGdiPlusProps(bool showProp);
	void SetBrowserSourceProps(bool showProp);

protected:
	void resizeEvent(QResizeEvent* event) override;

signals:
	void PropertiesRefreshed();
	void Changed();

private slots:
	void RefreshProperties();
	void ShowAdvancedProperties();
	void qSlotShowControlPanel();

public slots:
	void SignalChanged();
	void ReloadProperties();


public:
		inline void UpdateSettings()
		{
			if (callback)
				callback(OBSGetStrongRef(weakObj), nullptr, settings);
			else if (visUpdateCb)
				visUpdateCb(OBSGetStrongRef(weakObj), settings);
		}
		inline bool DeferUpdate() const { return deferUpdate; }

		inline void SetScrollDisabled() { m_bScrollEnabled = false; }
		inline void SetPropertiesViewWidth(int nWidth) { m_nPropertiesViewWidth = nWidth; }

		inline void SetLayoutMargin(int left, int top, int right, int bottom) 
		{ 
			m_nLayoutMargin[0] = left; 
			m_nLayoutMargin[1] = top;
			m_nLayoutMargin[2] = right;
			m_nLayoutMargin[3] = bottom;
		}
		inline void SetFormSpacing(int hSpacing, int vSpacing) { m_nFormHSpacing = hSpacing; m_nFormVSpacing = vSpacing; }

		inline void SetLabelFixedSize(QSize size) { m_LabelFixedSize = size; }
		inline void SetComboBoxFixedSize(QSize size) { m_ComboBoxFixedSize = size; }
		inline void SetSpinBoxFixedSize(QSize size) { m_SpinBoxFixedSize = size; }
		inline void SetLineEditFixedSize(QSize size) { m_LineEditFixedSize = size; }
		inline void SetCheckBoxFixedHeight(int height) { m_CheckBoxFixedHeight = height; }

private:
	QWidget*		m_propWidget = nullptr;
	QFormLayout*	m_formLayout = nullptr;

	properties_t properties;
	OBSData settings;
	OBSWeakObjectAutoRelease weakObj;
	void* rawObj = nullptr;
	std::string type;
	PropertiesReloadCallback reloadCallback;
	PropertiesUpdateCallback callback = nullptr;
	PropertiesVisualUpdateCb visUpdateCb = nullptr;
	int minSize;
	std::vector<std::unique_ptr<WidgetInfo>> children;
	std::string lastFocused;
	QWidget* lastWidget = nullptr;
	bool deferUpdate;
	bool enableDefer = true;

	bool m_bAdvancedProps = false;

	std::vector<std::unique_ptr<QPushButton>> vPushButton;

	int m_nLayoutMargin[4] = { -1, -1, -1, -1 };
	bool m_bScrollEnabled = true;
	int m_nPropertiesViewWidth = -1;
	int m_nFormVSpacing = -1;
	int m_nFormHSpacing = -1;

	QSize m_LabelFixedSize = QSize(-1, -1);
	QSize m_ComboBoxFixedSize = QSize(-1, -1);
	QSize m_SpinBoxFixedSize = QSize(-1, -1);
	QSize m_LineEditFixedSize = QSize(-1, -1);

	int m_CheckBoxFixedHeight = -1;
};
