#pragma once

#include <QPointer>
#include <QComboBox>

#include "vertical-scroll-area.hpp"
#include <obs-data.h>
#include <obs.hpp>
#include <qtimer.h>
#include <vector>
#include <memory>

//
struct ResolutionItem {
	int width = 0;
	int height = 0;
	bool is1440p = false;
};
const ResolutionItem resolutionList[] = {
    { 2560, 1440, true },
    { 1920, 1080, false },
    { 1600, 1200, false },
    { 1600, 900, false },
    { 1440, 900, false },
    { 1280, 1024, false },
    { 1280, 800, false },
    { 1280, 768, false },
    { 1280, 720, false },
    { 1200, 900, false },
    { 1024, 768, false },
    { 1024, 576, false },
    { 960, 540, false },
    { 854, 480, false },
    { 800, 600, false },
    { 720, 1280, false },
    { 720, 480, false },
    { 640, 480, false },
    { 640, 360, false },
    { 540, 960, false },
    { 540, 960, false },
    { 480, 854, false },
    { 480, 360, false },
};
constexpr ResolutionItem av1Resolution = {2560, 1440, true};
constexpr ResolutionItem defResolution = {1920, 1080, false};
//
struct BitrateItem {
	int bitrate = 0;
	bool is1440p = false;
};
const BitrateItem bitrateList[] = {
	{ 16000, true },
	{ 15000, true },
	{ 14000, true },
	{ 13000, true },
	{ 12000, true },
	{ 11000, true },
	{ 10000, true },
	{ 9000, true },
	//
	{ 8000, false },
	{ 7000, false },
	{ 6000, false },
	{ 5000, false },
	{ 4000, false },
	{ 3500, false },
	{ 3000, false },
	{ 2500, false },
	{ 2000, false },
	{ 1500, false },
	{ 1000, false },
	{ 800, false },
	{ 640, false },
	{ 500, false },
	{ 400, false },
	{ 300, false },
	{ 250, false },
	{ 200, false },
	{ 150, false }
};
//

class QFormLayout;
class OBSPropertiesView;
class QLabel;

typedef obs_properties_t* (*PropertiesReloadCallback)(void* obj);
typedef void (*PropertiesUpdateCallback)(void* obj, obs_data_t* old_settings, obs_data_t* new_settings);
typedef void (*PropertiesVisualUpdateCb)(void* obj, obs_data_t* settings);

/* ------------------------------------------------------------------------- */

class WidgetInfo : public QObject {
	Q_OBJECT

	friend class OBSPropertiesView;

private:
	OBSPropertiesView* view;
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
	void ButtonGroupItemClicked();
	void TogglePasswordText(bool checked);

public:
	inline WidgetInfo(OBSPropertiesView* view_, obs_property_t* prop, QWidget* widget_)
		: view(view_),
		property(prop),
		widget(widget_)
	{}

	~WidgetInfo()
	{
		if(update_timer) {
			update_timer->stop();
			QMetaObject::invokeMethod(update_timer, "timeout");
			update_timer->deleteLater();
		}
	}

public slots:
	void ControlChanged();
	void EncorderBitrateChanged();
	void NotifyUpdateProperty(const char* propName);

	/* editable list */
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

class OBSPropertiesView : public VScrollArea {
	Q_OBJECT

	friend class WidgetInfo;

private:

	using properties_delete_t = decltype(&obs_properties_destroy);
	using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;

	QWidget* widget = nullptr;
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
	bool deferUpdate = false;
	bool enableDefer = true;
	bool disableScrolling = false;

	template<typename Sender, typename SenderParent, typename... Args>
	QWidget* NewWidget(obs_property_t* prop, Sender* widget, void (SenderParent::* signal)(Args...));

	QWidget* AddCheckbox(obs_property_t* prop);
	QWidget* AddText(obs_property_t* prop, QFormLayout* layout, QLabel*& label);
	void AddPath(obs_property_t* prop, QFormLayout* layout, QLabel** label);
	void AddInt(obs_property_t* prop, QFormLayout* layout, QLabel** label);
	void AddFloat(obs_property_t* prop, QFormLayout* layout, QLabel** label);
	QWidget* AddList(obs_property_t* prop, bool& warning, bool group);
	void AddEditableList(obs_property_t* prop, QFormLayout* layout, QLabel*& label);
	QWidget* AddButton(obs_property_t* prop, QLabel*& label);
	void AddColorInternal(obs_property_t* prop, QFormLayout* layout, QLabel*& label, bool supportAlpha);
	void AddColor(obs_property_t* prop, QFormLayout* layout, QLabel*& label);
	void AddColorAlpha(obs_property_t* prop, QFormLayout* layout, QLabel*& label);
	void AddFont(obs_property_t* prop, QFormLayout* layout, QLabel*& label);
	void AddFrameRate(obs_property_t* prop, bool& warning, QFormLayout* layout, QLabel*& label);

	void AddGroup(obs_property_t* prop, QFormLayout* layout);

	void AddProperty(obs_property_t* property, QFormLayout* layout, bool group = false);

	void resizeEvent(QResizeEvent* event) override;

	void GetScrollPos(int& h, int& v, int& hend, int& vend);
	void SetScrollPos(int h, int v, int old_hend, int old_vend);

private slots:
	void RefreshProperties();
	//void resizeWidget(bool empty);
	void showAdvancedProperties();
	void showControlPanel();

public slots:
	void ReloadProperties();
	void SignalChanged();

signals:
	void Changed();
	void PropertiesRefreshed();

public:
	OBSPropertiesView(OBSData settings, obs_object_t* obj, PropertiesReloadCallback reloadCallback,
					  PropertiesUpdateCallback callback, PropertiesVisualUpdateCb cb = nullptr, int minSize = 0);
	OBSPropertiesView(OBSData settings, void* obj, PropertiesReloadCallback reloadCallback,
					  PropertiesUpdateCallback callback, PropertiesVisualUpdateCb cb = nullptr, int minSize = 0);
	OBSPropertiesView(OBSData settings, const char* type, PropertiesReloadCallback reloadCallback, int minSize = 0,
					  bool isEncorderProps = false);

	~OBSPropertiesView();

#define obj_constructor(type)																				  \
	inline OBSPropertiesView(OBSData settings, obs_##type##_t *type, PropertiesReloadCallback reloadCallback, \
							 PropertiesUpdateCallback callback, PropertiesVisualUpdateCb cb = nullptr,        \
						     int minSize = 0)                                                                 \
		    : OBSPropertiesView(settings, (obs_object_t *)type, reloadCallback, callback, cb, minSize)        \
	{                                                                                                         \
	}

	obj_constructor(source);
	obj_constructor(output);
	obj_constructor(encoder);
	obj_constructor(service);
#undef obj_constructor

	inline obs_data_t* GetSettings() const { return settings; }

	inline void UpdateSettings()
	{
		if(callback)
			callback(OBSGetStrongRef(weakObj), nullptr, settings);
		else if(visUpdateCb)
			visUpdateCb(OBSGetStrongRef(weakObj), settings);
	}
	inline bool DeferUpdate() const { return deferUpdate; }
	inline void SetDeferrable(bool deferrable) { enableDefer = deferrable; }

	inline OBSObject GetOBSObject() const { return OBSGetStrongRef(weakObj); }

	void setScrolling(bool enabled)
	{
		disableScrolling = !enabled;
		RefreshProperties();
	}

	void SetDisabled(bool disabled);

#define Def_IsObject(type)                            \
	inline bool IsObject(obs_##type##_t *type) const  \
	{                                                 \
		OBSObject obj = OBSGetStrongRef(weakObj);	  \
		return obj.Get() == (obs_object_t *)type;	  \
	}

	/* clang-format off */
	Def_IsObject(source)
	Def_IsObject(output)
	Def_IsObject(encoder)
	Def_IsObject(service)
	/* clang-format on */

#undef Def_IsObject

	void setLayoutMargin(int left, int top, int right, int bottom) {
		m_layoutMargin[0] = left;
		m_layoutMargin[1] = top;
		m_layoutMargin[2] = right;
		m_layoutMargin[3] = bottom;
	}
	void setFormSpacing(int hSpacing, int vSpacing) { m_formHSpacing = hSpacing; m_formVSpacing = vSpacing; }

	void setCheckBoxFixedHeight(int height) { m_checkBoxFixedHeight = height; }

	QWidget* getPropWidget() { return widget; }
	QComboBox* getAdvancedBitrateComboBox() { return m_advancedBitrateComboBox; }

	QFormLayout* getFormLayout() { return m_formLayout; }

	bool m_NotUpdateFFMPEGLIST = false;

private:
	OBSSource getSourceObject();
	void createPropsMainWidget();
	void getScrollPos(int& h, int& v, int& hend, int& vend);
	void setScrollPos(int h, int v, int old_hend, int old_vend);

	void createAreaCaptureProp(OBSSource source);

	// ===============================
	// Add Custom Props Widget 
	// ( Used SOOP Studio Source )
	// ===============================
	void    AddButtonGroupItem(obs_property_t* prop, QGridLayout* layout,
								QButtonGroup* buttonGroup, int r, int c,
								bool exclusive, bool checked);
	void    AddButtonGroup(obs_property_t* prop, QFormLayout* layout, QLabel*& label);


	QWidget* addEncorderBitrateList(obs_property_t* prop);

	// ===============================
	// PropertiesView UI
	// (advanced mode, fixed widget size )
	// ===============================
	void addControlPanelButton(QFormLayout* layout);

	void addAdvancedPropButton(QFormLayout* layout);
	bool isHideAdvancedProperties();

	bool isExistAdvanceMode(QString id);
	void setAdvancedPropertiesById(QString id, bool showAdvancedProps);
	void setGameCaptureProps(bool showProp);
	void setWindowCaptureProps(bool showProp);
	void setWindowAreaCaptureProps(bool showProp);
	void setDshowInputProps(bool showProp);
	void setFFmpegSourceProps(bool showProp);
	void setFFmpegListSourceProps(bool showProp);
	void setTextGdiPlusProps(bool showProp);
	void setBrowserSourceProps(bool showProp);

private:
	QFormLayout* m_formLayout = nullptr;
	bool m_advancedProps = false;

	int m_layoutMargin[4] = {-1, -1, -1, -1};
	int m_formVSpacing = -1;
	int m_formHSpacing = -1;

	int m_checkBoxFixedHeight = -1;
	bool m_isEncorderProps = false;
	//
	QComboBox* m_advancedBitrateComboBox = nullptr;
};
