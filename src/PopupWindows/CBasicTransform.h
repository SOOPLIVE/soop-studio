#pragma once

#include <QDialog>

#include "obs.hpp"

#include "UIComponent/CRoundedDialogBase.h"

namespace Ui {
	class AFQBasicTransform;
}

class AFQBasicTransform : public AFQRoundedDialogBase {

#pragma region QT Field, CTOR/DTOR
	Q_OBJECT

public:
	explicit AFQBasicTransform(OBSSceneItem item, QWidget* parent = nullptr);
	~AFQBasicTransform();

private slots:
	void OnBoundsType(int index);
	void OnControlChanged();
	void OnCropChanged();

	static void AFChannelChanged(void* param, calldata_t* data);
	static void AFSceneItemTransform(void* param, calldata_t* data);
	static void AFSceneItemRemoved(void* param, calldata_t* data);
	static void AFSceneItemSelect(void* param, calldata_t* data);
	static void AFSceneItemDeselect(void* param, calldata_t* data);

	void RefreshControls();
	void SetItemQt(OBSSceneItem newItem);

	void qslotCloseButtonClicked();

#pragma endregion QT Field


#pragma region private func
private:
	template<typename Widget, typename WidgetParent, typename... SignalArgs,
		typename... SlotArgs>
	void HookWidget(Widget* widget,
		void (WidgetParent::* signal)(SignalArgs...),
		void (AFQBasicTransform::* slot)(SlotArgs...))
	{
		QObject::connect(widget, signal, this, slot);
	}

	void SetScene(OBSScene scene);
	void SetItem(OBSSceneItem newItem);

#pragma endregion private func

#pragma region private member var

private:
	std::unique_ptr<Ui::AFQBasicTransform> ui;

	OBSSceneItem m_sceneItem; 

	OBSSignal m_signalChannelChanged;
	OBSSignal m_signalTransform;
	OBSSignal m_signalRemove;
	OBSSignal m_signalSelect;
	OBSSignal m_signalDeselect;

	std::string undo_data;

	bool m_ignoreTransformSignal = false;
	bool m_ignoreItemChange = false;

#pragma endregion private member var

};