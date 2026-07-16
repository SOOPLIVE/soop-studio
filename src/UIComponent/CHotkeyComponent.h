#ifndef CHOTKEYCOMPONENT_H
#define CHOTKEYCOMPONENT_H

#include <QLineEdit>
#include <QKeyEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QPointer>
#include <QLabel>

#include <obs.hpp>

class AFQHotkeySettingAreaWidget;
class AFHotkeyWidget;

static inline bool operator!=(const obs_key_combination_t& c1,
	const obs_key_combination_t& c2)
{
	return c1.modifiers != c2.modifiers || c1.key != c2.key;
}

static inline bool operator==(const obs_key_combination_t& c1,
	const obs_key_combination_t& c2)
{
	return !(c1 != c2);
}

class AFQHotkeyLabel : public QLabel
{
	Q_OBJECT

public:
	void HighlightPair(bool highlight);
	void SetToolTip(const QString& toolTip);
	void SetRegisterType(obs_hotkey_registerer_t type);
	obs_hotkey_registerer_t GetRegisterType();

	void SetHotkeyAreaName(QString name);
	QString GetHotkeyAreaName();

protected:
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

public:
	QPointer<AFQHotkeyLabel> m_PairPartner;
	QPointer<AFHotkeyWidget> m_Widget;

private:
	obs_hotkey_registerer_t	 m_registerType = OBS_HOTKEY_REGISTERER_FRONTEND;
	QString	m_hotkeyAreaName;
};

//########################################################
class AFQHotkeyEdit : public QLineEdit
{
	Q_OBJECT

public:
	AFQHotkeyEdit(QWidget* parent, obs_key_combination_t original,
				  AFQHotkeySettingAreaWidget* settings);
	AFQHotkeyEdit(QWidget* parent = nullptr);

signals:
	void qsignalKeyChanged(obs_key_combination_t);
	void qsignalSearchKey(obs_key_combination_t);

public slots:
	void qslotHandleNewKey(obs_key_combination_t new_key);
	void qslotReloadKeyLayout();
	void qslotResetKey();
	void qslotClearKey();

public:
	void UpdateDuplicationState();

protected:
	void InitSignalHandler();
	void CreateDupeIcon();

	QVariant inputMethodQuery(Qt::InputMethodQuery) const override;
	void keyPressEvent(QKeyEvent* event) override;
#ifdef __APPLE__
	void keyReleaseEvent(QKeyEvent* event) override;
#endif
	void mousePressEvent(QMouseEvent* event) override;

	void RenderKey();

public:
	obs_key_combination_t original;
	obs_key_combination_t key;
	AFQHotkeySettingAreaWidget* settings;
	bool changed = false;

	bool hasDuplicate = false;

	OBSSignal layoutChanged;
	QAction* dupeIcon = nullptr;
};

//########################################################

namespace Ui {
	class AFHotkeyWidget;
}

class AFHotkeyWidget : public QWidget
{
    Q_OBJECT

public:
	AFHotkeyWidget(QWidget* parent, obs_hotkey_id id, std::string name,
				   AFQHotkeySettingAreaWidget* settings,
				   const std::vector<obs_key_combination_t>& combos = {});

    ~AFHotkeyWidget();

private slots:
	void HandleChangedBindings(obs_hotkey_id id_);

signals:
	void qsignalKeyChanged();
	void qsignalSearchKey(obs_key_combination_t);
	void qsignalAddRemoveEdit();

public:
	void SetKeyCombinations(const std::vector<obs_key_combination_t>&);
	void SetToolTip(const QString& toolTip_)
	{
		m_toolTip = toolTip_;
		for (auto& edit : m_edits)
			edit->setToolTip(toolTip_);
	}

	void Apply();
	void GetCombinations(std::vector<obs_key_combination_t>&) const;
	void Save();
	void Save(std::vector<obs_key_combination_t>& combinations);
    void Clear();
	bool Changed() const;
	size_t GetEditCount();

protected:
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	void _AddEdit(obs_key_combination combo, int idx = -1);
	void _RemoveEdit(size_t idx, bool signal = true);

	static void _BindingsChanged(void* data, calldata_t* param);

	QVBoxLayout* _Layout() const
	{
		return dynamic_cast<QVBoxLayout*>(QWidget::layout());
	}

	void _Default();

public:
	obs_hotkey_id m_hotkeyId;
	std::string m_name;

	bool m_changed = false;

	QPointer<AFQHotkeyLabel> m_Label;
	std::vector<QPointer<AFQHotkeyEdit>> m_edits;

	QString m_toolTip;

private:
    Ui::AFHotkeyWidget* ui;

	std::vector<QPointer<QPushButton>> m_removeButtons;
	std::vector<QPointer<QPushButton>> m_revertButtons;
	OBSSignal m_OBSSignalBindingsChanged;
	bool m_ignoreChangedBindings = false;
	AFQHotkeySettingAreaWidget* m_pHotkeySettingsDialog;
};

#endif // CHOTKEYCOMPONENT_H