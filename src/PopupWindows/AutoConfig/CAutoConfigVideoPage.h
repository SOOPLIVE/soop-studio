#ifndef CAUTOCONFIGVIDEOPAGE_H
#define CAUTOCONFIGVIDEOPAGE_H

#include <QWidget>
#include <QWizardPage>

class AFQAutoConfigWizard;

namespace Ui {
class AFQAutoConfigVideoPage;
}

class AFQAutoConfigVideoPage : public QWizardPage
{
    Q_OBJECT

	friend class AFQAutoConfigWizard;

public:
    explicit AFQAutoConfigVideoPage(AFQAutoConfigWizard *parent = nullptr);
    ~AFQAutoConfigVideoPage();

    virtual int nextId() const override;
    virtual bool validatePage() override;

private:
    Ui::AFQAutoConfigVideoPage *ui;

    AFQAutoConfigWizard* m_qWiz;
};

#endif // CAUTOCONFIGVIDEOPAGE_H
