#pragma once

#include <qfileinfo.h>
#include <qmutex.h>
#include <qpointer.h>
#include <qthread.h>
#include <qstyleditemdelegate.h>
#include <memory>

#include "ui_remux.h"

#include "media-io/media-remux.h"
#include "util/threading.h"
#include "UIComponent/CRoundedDialogBase.h"

class AFRemuxQueueModel;
class AFRemuxWorker;

enum RemuxEntryState {
	Empty,
	Ready,
	Pending,
	InProgress,
	Complete,
	InvalidPath,
	Error
};
Q_DECLARE_METATYPE(RemuxEntryState);

class AFQRemux : public AFQRoundedDialogBase
{
	Q_OBJECT

#pragma region class initializer, destructor
public:
	explicit AFQRemux(const char* recPath, QWidget* parent = nullptr, bool autoRemux = false);
	virtual ~AFQRemux() override;

#pragma endregion class initializer, destructor


#pragma region public func
public:
	void AutoRemux(QString inFile, QString outFile);

public slots:
	void qSlotUpdateProgress(float percent);
	void qSlotRemuxFinished(bool success);
	void qSlotBeginRemux();
	bool qSlotStopRemux();
	void qSlotClearFinished();
	void qSlotClearAll();

signals:
	void qSignalRemux(const QString& source, const QString& target);

#pragma endregion public func


#pragma region protected func
protected:
	virtual void dropEvent(QDropEvent* event) override;
	virtual void dragEnterEvent(QDragEnterEvent * event) override;

	void RemuxNextEntry();

#pragma endregion protected func

#pragma region private func
private:
	virtual void closeEvent(QCloseEvent* event) override;
	virtual void reject() override;

private slots:
	void qSlotRowCountChanged(const QModelIndex& parent, int first, int last);

#pragma endregion private func


#pragma region public var
public:

#pragma region public var

#pragma region private var
private:
	Ui::AFQRemux* ui = nullptr;

	QPointer<AFRemuxQueueModel>		m_queueModel;
	QThread							m_remuxer;
	QPointer<AFRemuxWorker>			m_worker;

	const char*						m_recPath = nullptr;

	bool							m_autoRemux = false;
	QString							m_autoRemuxFile;

	using job_t = std::shared_ptr<struct media_remux_job>;

#pragma endregion private var
};

class AFRemuxQueueModel : public QAbstractTableModel
{
	Q_OBJECT

	friend class AFQRemux;

#pragma region class initializer, destructor
public:
	AFRemuxQueueModel(QObject* parent = 0)
		:QAbstractTableModel(parent),
		m_isProcessing(false)
	{}

#pragma endregion class initializer, destructor

#pragma region public func
public:
	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	int columnCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation,
						int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex& index) const;
	bool setData(const QModelIndex& index, const QVariant& value, int role);

	QFileInfoList CheckForOverwrites() const;
	bool CheckForErrors() const;
	void BeginProcessing();
	void EndProcessing();
	bool BeginNextEntry(QString& inPath, QString& outPath);
	void FinishEntry(bool sucess);
	bool CanClearFinished() const;
	void ClearFinished();
	void ClearAll();

#pragma endregion public func

#pragma region public var
public:
	bool m_autoRemux = false;

#pragma region public func

#pragma region private func
private:
	struct RemuxQueueEntry {
		RemuxEntryState state = RemuxEntryState::Empty;

		QString sourcePath;
		QString targetPath;
	};

	static QVariant GetIcon(RemuxEntryState state);
	void CheckInputPath(int row);

#pragma endregion private func

#pragma region private var
private:
	QList<RemuxQueueEntry> m_queue;
	bool m_isProcessing = false;

#pragma endregion private var
};

class AFRemuxWorker : public QObject
{
	Q_OBJECT

	friend class AFQRemux;

#pragma region class initializer, destructor
public:
	explicit AFRemuxWorker()
		:m_isWorking(false)
	{}
	virtual ~AFRemuxWorker() {}

#pragma endregion class initializer, destructor

#pragma region public func
public:
	void UpdateProgress(float percent);

#pragma endregion public func

#pragma region public var
public:
	bool m_isWorking = false;
	QMutex m_updateMutex;
	float m_lastProgress = 0.0f;

#pragma endregion public var

#pragma region protected func
protected:

#pragma endregion protected func

#pragma region private func
private:

private slots:
	void qSlotRemux(const QString& source, const QString& target);

signals:
	void qSignalUpdateProgress(float percent);
	void qSignalRemuxFinished(bool success);

#pragma endregion private func

#pragma region private var
private:

#pragma endregion private var
};

class AFRemuxEntryPathItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

#pragma region class initializer, destructor
public:
	AFRemuxEntryPathItemDelegate(bool isOutput, const QString& defaultPath);

#pragma endregion class initializer, destructor

#pragma region public func
public:
	virtual QWidget* createEditor(QWidget* parent,
								 const QStyleOptionViewItem& option,
								 const QModelIndex& index) const override;
	virtual void setEditorData(QWidget* editor,
							   const QModelIndex& index) const override;
	virtual void setModelData(QWidget* editor, QAbstractItemModel* model,
							  const QModelIndex& index) const override;
	virtual void paint(QPainter* painter,
					   const QStyleOptionViewItem& option,
					   const QModelIndex& index) const override;

#pragma endregion public func

#pragma region private func
private:
	void HandleBrowse(QWidget* container);
	void HandleClear(QWidget* container);

private slots:
	void qSlotUpdateText();


#pragma endregion private func


#pragma region private var
private:
	bool m_isOutput = false;
	QString m_defaultPath;

#pragma endregion private var
};