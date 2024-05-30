#pragma once

#include <QWidget>

#include "ui_color-select.h"

namespace Ui {
	class AFQColorSelect;
}

class AFQColorSelect : public QWidget {

public:
	explicit AFQColorSelect(QWidget* parent = 0) : 
		QWidget(parent),
		ui(new Ui::AFQColorSelect)
	{
		ui->setupUi(this);
	}

private:
	std::unique_ptr<Ui::AFQColorSelect> ui;
};