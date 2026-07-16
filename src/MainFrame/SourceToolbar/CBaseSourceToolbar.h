#pragma once

#include <QFrame>
#include <QTimer>
#include <QComboBox>

#include "obs.hpp"

class CBaseSourceToolbar : public QWidget
{
	Q_OBJECT

public:
	explicit CBaseSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr, bool not_load_props = false);
	~CBaseSourceToolbar();

	OBSSource GetSource() { return OBSGetStrongRef(m_weakSource); }

protected:
	using properties_delete_t = decltype(&obs_properties_destroy);
	using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;

	properties_t props;
	OBSDataAutoRelease oldData;

	void UpdateSourceComboToolbarProperties(QComboBox* combo, OBSSource source,
											obs_properties_t* props, const char* prop_name, bool is_int);

	void UpdateSourceComboToolbarValue(QComboBox* combo, OBSSource source,
									   int idx, const char* prop_name, bool is_int);

	int FillPropertyCombo(QComboBox* c, obs_property_t* p, const std::string& cur_id, bool is_int = false);

	void SaveOldProperties(obs_source_t* source);
	void SetUndoProperties(obs_source_t* source, bool repeatable = false);

public:
	//enum ContextBarSize {
	//	ContextBarSize_Minimized,
	//	ContextBarSize_Reduced,
	//	ContextBarSize_Normal
	//};
	virtual void SetContextBarSize(int /*size*/) {}

private:
	OBSWeakSource m_weakSource;
};