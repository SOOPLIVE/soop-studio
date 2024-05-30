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

class AFQHotkeyLabel : public QLabel {
	Q_OBJECT

#pragma region public func
public:
	void HighlightPair(bool highlight);
	void SetToolTip(const QString& toolTip);
	void SetRegisterType(obs_hotkey_registerer_t type);
	obs_hotkey_registerer_t GetRegisterType();

	void SetHotkeyAreaName(QString name);
	QString GetHotkeyAreaName();
#pragma endregion public func

#pragma region protected func
protected:
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
#pragma endregion protected func

#pragma region public member var
public:
	QPointer<AFQHotkeyLabel> m_PairPartner;
	QPointer<AFHotkeyWidget> m_Widget;
#pragma endregion public member var

#pragma region private member var
private:
	obs_hotkey_registerer_t	 m_registerType = OBS_HOTKEY_REGISTERER_FRONTEND;
	QString	m_hotkeyAreaName;
#pragma endregion private member var
};

//########################################################
class AFQHotkeyEdit : public QLineEdit {
	Q_OBJECT;

#pragma region class initializer, destructor
public:
	AFQHotkeyEdit(QWidget* parent, obs_key_combination_t original,
		AFQHotkeySettingAreaWidget* settings);
	AFQHotkeyEdit(QWidget* parent = nullptr);
#pragma endregion class initializer, destructor

#pragma region QT Field, CTOR/DTOR
signals:
	void qsignalKeyChanged(obs_key_combination_t);
	void qsignalSearchKey(obs_key_combination_t);

public slots:
	void qslotHandleNewKey(obs_key_combination_t new_key);
	void qslotReloadKeyLayout();
	void qslotResetKey();
	void qslotClearKey();

#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
	void UpdateDuplicationState();
#pragma endregion public func

#pragma region protected func
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
#pragma endregion protected func

#pragma region public member var
public:
	obs_key_combination_t original;
	obs_key_combination_t key;
	AFQHotkeySettingAreaWidget* settings;
	bool changed = false;

	bool hasDuplicate = false;
#pragma endregion public member var

#pragma region protected member var
	OBSSignal layoutChanged;
	QAction* dupeIcon = nullptr;
#pragma endregion protected member var
};

//########################################################

namespace Ui {
class AFHotkeyWidget;
}

class AFHotkeyWidget : public QWidget
{
    Q_OBJECT
#pragma region class initializer, destructor
public:
	AFHotkeyWidget(QWidget* parent, obs_hotkey_id id, std::string name,
		AFQHotkeySettingAreaWidget* settings,
		const std::vector<obs_key_combination_t>& combos = {});

    ~AFHotkeyWidget();
#pragma endregion class initializer, destructor

#pragma region QT Field, CTOR/DTOR
private slots:
	void HandleChangedBindings(obs_hotkey_id id_);

signals:
	void qsignalKeyChanged();
	void qsignalSearchKey(obs_key_combination_t);
	void qsignalAddRemoveEdit();
#pragma endregion QT Field, CTOR/DTOR


#pragma region public func
public:
	void SetKeyCombinations(const std::vector<obs_key_combination_t>&);
	void SetToolTip(const QString& toolTip_)
	{
		m_sToolTip = toolTip_;
		for (auto& edit : m_Edits)
			edit->setToolTip(toolTip_);
	}

	void	Apply();
	void	GetCombinations(std::vector<obs_key_combination_t>&) const;
	void	Save();
	void	Save(std::vector<obs_key_combination_t>& combinations);
    void	Clear();
	bool	Changed() const;
	size_t  GetEditCount();

#pragma endregion public func

#pragma region protected func
protected:
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
#pragma endregion protected func

#pragma region private func
private:
	void _AddEdit(obs_key_combination combo, int idx = -1);
	void _RemoveEdit(size_t idx, bool signal = true);

	static void _BindingsChanged(void* data, calldata_t* param);


	QVBoxLayout* _Layout() const
	{
		return dynamic_cast<QVBoxLayout*>(QWidget::layout());
	}
#pragma endregion private func


#pragma region public member var
public:
	obs_hotkey_id m_hotkeyId;
	std::string m_sName;

	bool m_bChanged = false;

	QPointer<AFQHotkeyLabel> m_Label;
	std::vector<QPointer<AFQHotkeyEdit>> m_Edits;

	QString m_sToolTip;
#pragma endregion public member var

#pragma region private member var
private:
    Ui::AFHotkeyWidget* ui;

	std::vector<QPointer<QPushButton>> m_RemoveButtons;
	std::vector<QPointer<QPushButton>> m_RevertButtons;
	OBSSignal m_OBSSignalBindingsChanged;
	bool m_bIgnoreChangedBindings = false;
	AFQHotkeySettingAreaWidget* m_HotkeySettingsDialog;
#pragma endregion private member var
};

#endif // CHOTKEYCOMPONENT_H
