#ifndef CAUTOCONFIGSTARTPAGE_H
#define CAUTOCONFIGSTARTPAGE_H

#include <QWidget>
#include <QWizardPage>

class AFQAutoConfigWizard;

namespace Ui {
class AFQAutoConfigStartPage;
}

class AFQAutoConfigStartPage : public QWizardPage
{
    Q_OBJECT

	friend class AFQAutoConfigWizard;

public:
	explicit AFQAutoConfigStartPage(AFQAutoConfigWizard* parent = nullptr);
	~AFQAutoConfigStartPage();

	virtual int nextId() const override;

public slots:
	void on_prioritizeStreaming_clicked();
	void on_prioritizeRecording_clicked();
	void PrioritizeVCam();


private:
    Ui::AFQAutoConfigStartPage *ui;

	AFQAutoConfigWizard* m_qWiz;
};

#endif // CAUTOCONFIGSTARTPAGE_H
