#pragma once

#include <QWidget>

namespace Ui {
    class AFQParticleThumbnailWidget;
}

class AFQParticleThumbnailWidget : public QWidget
{
    Q_OBJECT

public:
    AFQParticleThumbnailWidget(QWidget* parent, QString particleId);
    ~AFQParticleThumbnailWidget();

signals:
    void qsignalParticleThumbnailClicked(const QString& particleId);

private slots:
    void _qslotParticleThumbnailClicked();

public:
    void SetChecked(bool checked);

private:
    void _Init();
    void _SetStyle();

private:
    QString m_particleId = "";
    Ui::AFQParticleThumbnailWidget* ui = nullptr;
};