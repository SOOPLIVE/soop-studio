#include "CRemuxFrame.h"

#include "Application/CApplication.h"

#include <qevent.h>
#include <qdiriterator.h>
#include <qitemdelegate.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qmimedata.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include <qstandarditemmodel.h>
#include <qstyleditemdelegate.h>
#include <qtoolbutton.h>
#include <qtimer.h>

#include "qt-wrappers.hpp"
#include "MainFrame/CMainFrame.h"

#include "UIComponent/CMessageBox.h"

#include <memory>
#include <cmath>

enum RemuxEntryColumn {
	State,
	InputPath,
	OutputPath,

	Count
};

enum RemuxEntryRole { EntryStateRole = Qt::UserRole, NewPathsToProcessRole };


AFQRemux::AFQRemux(const char* recPath, QWidget* parent, bool autoRemux)
	:AFQRoundedDialogBase(parent),
	m_queueModel(new AFRemuxQueueModel),
	m_worker(new AFRemuxWorker()),
	ui(new Ui::AFQRemux),
	m_recPath(recPath),
	m_autoRemux(autoRemux)
{
	setAcceptDrops(true);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setAttribute(Qt::WA_DeleteOnClose);

	ui->setupUi(this);
	ui->progressBar->setVisible(false);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(false);

	if(autoRemux) {
		resize(480, 114);
		ui->widget_TableViewArea->hide();
		ui->widget_ButtonBoxArea->hide();
	}
	ui->progressBar->setMinimum(0);
	ui->progressBar->setMaximum(1000);
	ui->progressBar->setValue(0);

	ui->tableView->setModel(m_queueModel);
	ui->tableView->setItemDelegateForColumn(RemuxEntryColumn::InputPath, new AFRemuxEntryPathItemDelegate(false, recPath));
	ui->tableView->setItemDelegateForColumn(RemuxEntryColumn::OutputPath, new AFRemuxEntryPathItemDelegate(true, recPath));
	ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
	ui->tableView->horizontalHeader()->setSectionResizeMode(RemuxEntryColumn::State, QHeaderView::ResizeMode::Fixed);
	ui->tableView->setEditTriggers(QAbstractItemView::EditTrigger::CurrentChanged);
	ui->tableView->setTextElideMode(Qt::ElideMiddle);
	ui->tableView->setWordWrap(false);

	installEventFilter(CreateShortcutFilter());

	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QTStr("Remux.Remux"));
	ui->buttonBox->button(QDialogButtonBox::Reset)->setText(QTStr("Remux.ClearFinished"));
	ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText(QTStr("Remux.ClearAll"));
	ui->buttonBox->button(QDialogButtonBox::Reset)->setDisabled(true);

	connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &AFQRemux::qSlotBeginRemux);
	connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &AFQRemux::qSlotClearFinished);
	connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &AFQRemux::qSlotClearAll);
	connect(ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &AFQRemux::close);
	connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQRemux::close);

	m_worker->moveToThread(&m_remuxer);
	m_remuxer.start();

	connect(m_worker.data(), &AFRemuxWorker::qSignalUpdateProgress, this, &AFQRemux::qSlotUpdateProgress);
	connect(&m_remuxer, &QThread::finished, m_worker.data(), &QObject::deleteLater);
	connect(m_worker.data(), &AFRemuxWorker::qSignalRemuxFinished, this, &AFQRemux::qSlotRemuxFinished);
	connect(this, &AFQRemux::qSignalRemux, m_worker.data(), &AFRemuxWorker::qSlotRemux);

	connect(m_queueModel.data(), &AFRemuxQueueModel::rowsInserted, this, &AFQRemux::qSlotRowCountChanged);
	connect(m_queueModel.data(), &AFRemuxQueueModel::rowsRemoved, this, &AFQRemux::qSlotRowCountChanged);

	QModelIndex index = m_queueModel->createIndex(0, 1);
	QMetaObject::invokeMethod(ui->tableView, "setCurrentIndex",
							  Qt::QueuedConnection,
							  Q_ARG(const QModelIndex&, index));
}
AFQRemux::~AFQRemux()
{
	qSlotStopRemux();
	m_remuxer.quit();
	m_remuxer.wait();
}
//
void AFQRemux::AutoRemux(QString inFile, QString outFile)
{
	if("" != inFile &&
	   "" != outFile &&
	   m_autoRemux) {
		ui->progressBar->setVisible(true);
		emit qSignalRemux(inFile, outFile);
		m_autoRemuxFile = outFile;
	}
}
//
void AFQRemux::qSlotUpdateProgress(float percent)
{
	int nPercent = percent * 10;
	if (nPercent > 55)
		ui->progressBar->setStyleSheet("QProgressBar { color: #000000; }");

	ui->progressBar->setValue(nPercent);
}
void AFQRemux::qSlotRemuxFinished(bool success)
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	m_queueModel->FinishEntry(success);
	if(m_autoRemux && "" != m_autoRemuxFile) {
		QTimer::singleShot(3000, this, &AFQRemux::close);

		AFMainFrame* main = App()->GetMainView();
//		main->ShowSystemAlert(QTStr("Basic.StatusBar.AutoRemuxedTo").arg(m_autoRemuxFile));
	}
	RemuxNextEntry();
}
void AFQRemux::qSlotBeginRemux()
{
	if(m_worker->m_isWorking) {
		qSlotStopRemux();
		return;
	}

	bool proceedWithRemux = true;
	QFileInfoList fileList = m_queueModel->CheckForOverwrites();
	if(!fileList.empty()) {
		QString msg = QTStr("Remux.FileExists") + "\n\n";
		for(QFileInfo info : fileList)
			msg += info.canonicalFilePath() + "\n";

		int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
												QTStr("Remux.FileExistsTitle"),
												msg);
		if(QDialogButtonBox::Ok != result) {
			proceedWithRemux = false;
		}
	}

	if(!proceedWithRemux) {
		return;
	}

	// Set all jobs to "pending" first.
	m_queueModel->BeginProcessing();
	ui->progressBar->setVisible(true);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QTStr("Remux.Stop"));
	setAcceptDrops(false);

	RemuxNextEntry();
}
bool AFQRemux::qSlotStopRemux()
{
	if(!m_worker->m_isWorking)
		return true;

	// By locking the worker thread's mutex, we ensure that its
	// update poll will be blocked as long as we're in here with
	// the popup open.
	QMutexLocker lock(&m_worker->m_updateMutex);

	int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, nullptr,
											QTStr("Remux.ExitUnfinishedTitle"),
											QTStr("Remux.ExitUnfinished"));
	bool exit = QDialogButtonBox::Ok == result ? true : false;
	if(exit) {
		// Inform the worker it should no longer be
		// working. It will interrupt accordingly in
		// its next update callback.
		m_worker->m_isWorking = false;
	}
	return exit;
}
void AFQRemux::qSlotClearFinished()
{
	m_queueModel->ClearFinished();
}
void AFQRemux::qSlotClearAll()
{
	m_queueModel->ClearAll();
}
//
void AFQRemux::dropEvent(QDropEvent* event)
{
	QStringList urlList;

	for(QUrl url : event->mimeData()->urls()) {
		QFileInfo info(url.toLocalFile());
		if(info.isDir()) {
			QStringList extFilters;
			extFilters	<< "*.flv"
						<< "*.mp4"
						<< "*.mov"
						<< "*.mkv"
						<< "*.ts"
						<< "*.m3u8";

			QDirIterator dirItr(info.absoluteFilePath(),
								extFilters, QDir::Files,
								QDirIterator::Subdirectories);
			while(dirItr.hasNext()) {
				urlList.append(dirItr.next());
			}
		} else {
			urlList.append(info.canonicalFilePath());
		}

		if(urlList.empty()) {
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
									   QTStr("Remux.NoFilesAddedTitle"),
									   QTStr("Remux.NoFilesAdded"));
		} else if(!m_autoRemux) {
			QModelIndex insertIndex = m_queueModel->index(m_queueModel->rowCount() - 1,
														  RemuxEntryColumn::InputPath);
			m_queueModel->setData(insertIndex, urlList, RemuxEntryRole::NewPathsToProcessRole);
		}
	}
}
void AFQRemux::dragEnterEvent(QDragEnterEvent* event)
{
	if(event->mimeData()->hasUrls() &&
	   !m_worker->m_isWorking)
		event->accept();
}

void AFQRemux::RemuxNextEntry()
{
	m_worker->m_lastProgress = 0.f;

	QString inputPath, outputPath;
	if(m_queueModel->BeginNextEntry(inputPath, outputPath)) {
		emit qSignalRemux(inputPath, outputPath);
	} else {
		m_queueModel->m_autoRemux = m_autoRemux;
		m_queueModel->EndProcessing();

		if(!m_autoRemux) {
			QString msg = m_queueModel->CheckForErrors() ? QTStr("Remux.FinishedError") : QTStr("Remux.Finished");
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
									   QTStr("Remux.FinishedTitle"),
									   msg);
		}

		ui->progressBar->setVisible(m_autoRemux);
		ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QTStr("Remux.Remux"));
		ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(true);
		ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(m_queueModel->CanClearFinished());
		setAcceptDrops(true);
	}
}
//
void AFQRemux::closeEvent(QCloseEvent* event)
{
	if(!qSlotStopRemux())
		event->ignore();
	else
		QDialog::closeEvent(event);
}
void AFQRemux::reject()
{
	if(!qSlotStopRemux())
		return;

	QDialog::reject();
}
//
void AFQRemux::qSlotRowCountChanged(const QModelIndex& parent, int first, int last)
{
	// See if there are still any rows ready to remux. Change
	// the state of the "go" button accordingly.
	// There must be more than one row, since there will always be
	// at least one row for the empty insertion point.
	if(m_queueModel->rowCount() > 1) {
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
		ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(true);
		ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(m_queueModel->CanClearFinished());
	} else {
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
		ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(false);
		ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(false);
	}
}
//

int AFRemuxQueueModel::rowCount(const QModelIndex& parent) const
{
	return m_queue.length() + (m_isProcessing ? 0 : 1);
}
int AFRemuxQueueModel::columnCount(const QModelIndex& parent) const
{
	return RemuxEntryColumn::Count;
}
QVariant AFRemuxQueueModel::data(const QModelIndex& index, int role) const
{
	QVariant result = QVariant();
	if(index.row() >= m_queue.length()) {
		;
	} else if(Qt::DisplayRole == role) {

		switch(index.column())
		{
			case RemuxEntryColumn::InputPath:
				result = m_queue[index.row()].sourcePath;
				break;

			case RemuxEntryColumn::OutputPath:
				result = m_queue[index.row()].targetPath;
				break;
		}
	} else if(Qt::DecorationRole == role &&
			  RemuxEntryColumn::State == index.column()) {
		result = GetIcon(m_queue[index.row()].state);
	} else if(RemuxEntryRole::EntryStateRole == role) {
		result = m_queue[index.row()].state;
	}
	return result;
}
QVariant AFRemuxQueueModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant result = QVariant();
	if(Qt::DisplayRole == role &&
	   Qt::Orientation::Horizontal == orientation) {

		switch(section)
		{
			case RemuxEntryColumn::State:
				result = QString();
				break;

			case RemuxEntryColumn::InputPath:
				result = QTStr("Remux.SourceFile");
				break;

			case RemuxEntryColumn::OutputPath:
				result = QTStr("Remux.TargetFile");
				break;
		}
	}
	return result;
}
Qt::ItemFlags AFRemuxQueueModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags flags = QAbstractTableModel::flags(index);
	if(RemuxEntryColumn::InputPath == index.column()) {
		flags |= Qt::ItemIsEditable;
	} else if(RemuxEntryColumn::OutputPath == index.column() &&
			  index.row() != m_queue.length()) {
		flags |= Qt::ItemIsEditable;
	}
	return flags;
}
bool AFRemuxQueueModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	bool success = false;
	if(RemuxEntryRole::NewPathsToProcessRole == role)
	{
		QStringList pathList = value.toStringList();
		if(pathList.isEmpty()) {
			if(m_queue.size() > index.row()) {
				beginRemoveRows(QModelIndex(), index.row(), index.row());
				m_queue.removeAt(index.row());
				endRemoveRows();
			}
		} else {
			if(!pathList.isEmpty() &&
			   m_queue.length() > index.row()) {
				m_queue[index.row()].sourcePath = pathList[0];
				CheckInputPath(index.row());
				pathList.removeAt(0);
				success = true;
			}

			if(!pathList.isEmpty()) {
				int row = index.row();
				int lastRow = row + pathList.size() - 1;

				beginInsertRows(QModelIndex(), row, lastRow);
				for(QString path : pathList) {
					RemuxQueueEntry entry;
					entry.sourcePath = path;
					entry.state = RemuxEntryState::Empty;
					m_queue.insert(row, entry);
					row++;
				}
				endInsertRows();

				for(row = index.row(); row <= lastRow; row++) {
					CheckInputPath(row);
				}
				success = true;
			}
		}
	}
	else if(index.row() == m_queue.length())
	{
		QString path = value.toString();
		if(!path.isEmpty()) {
			RemuxQueueEntry entry;
			entry.sourcePath = path;
			entry.state = RemuxEntryState::Empty;

			beginInsertRows(QModelIndex(), m_queue.length() +1, m_queue.length() +1);
			m_queue.append(entry);
			endInsertRows();

			CheckInputPath(index.row());
			success = true;
		}
	}
	else
	{
		QString path = value.toString();
		if(path.isEmpty()) {
			if(RemuxEntryColumn::InputPath) {
				beginRemoveRows(QModelIndex(), index.row(), index.row());
				m_queue.removeAt(index.row());
				endRemoveRows();
			}
		} else {

			switch(index.column())
			{
				case RemuxEntryColumn::InputPath:
				{
					m_queue[index.row()].sourcePath = value.toString();
					CheckInputPath(index.row());
					success = true;
				}
				break;

				case RemuxEntryColumn::OutputPath:
				{
					m_queue[index.row()].targetPath = value.toString();
					emit dataChanged(index, index);
					success = true;
				}
				break;
			}
		}
	}
	return success;
	
}
//
QFileInfoList AFRemuxQueueModel::CheckForOverwrites() const
{
	QFileInfoList infoList;
	for(const RemuxQueueEntry& entry : m_queue)
	{
		if(RemuxEntryState::Ready == entry.state) {
			QFileInfo info(entry.targetPath);
			if(info.exists()) {
				infoList.append(info);
			}
		}
	}
	return infoList;
}
bool AFRemuxQueueModel::CheckForErrors() const
{
	bool hasError = false;
	for(const RemuxQueueEntry& entry : m_queue)
	{
		if(RemuxEntryState::Error == entry.state) {
			hasError = true;
			break;
		}
	}
	return hasError;
}
void AFRemuxQueueModel::BeginProcessing()
{
	for(RemuxQueueEntry& entry : m_queue)
	{
		if(RemuxEntryState::Ready == entry.state) {
			entry.state = RemuxEntryState::Pending;
		}
	}

	// Signal that the insertion point no longer exists.
	beginInsertRows(QModelIndex(), m_queue.length(), m_queue.length());
	endInsertRows();

	m_isProcessing = true;

	emit dataChanged(index(0, RemuxEntryColumn::State),
					 index(m_queue.length(), RemuxEntryColumn::State));
}
void AFRemuxQueueModel::EndProcessing()
{
	for(RemuxQueueEntry& entry : m_queue)
	{
		if(RemuxEntryState::Pending == entry.state) {
			entry.state = RemuxEntryState::Ready;
		}
	}

	// Signal that the insertion point exists again.
	m_isProcessing = false;
	if(!m_autoRemux) {
		beginInsertRows(QModelIndex(), m_queue.length(), m_queue.length());
		endInsertRows();
	}

	emit dataChanged(index(0, RemuxEntryColumn::State),
					 index(m_queue.length(), RemuxEntryColumn::State));
}
bool AFRemuxQueueModel::BeginNextEntry(QString& inPath, QString& outPath)
{
	bool anyStarted = false;
	for(int row = 0; row < m_queue.length(); row++)
	{
		RemuxQueueEntry& entry = m_queue[row];
		if(RemuxEntryState::Pending == entry.state) {
			entry.state = RemuxEntryState::InProgress;
			inPath = entry.sourcePath;
			outPath = entry.targetPath;

			QModelIndex index = this->index(row, RemuxEntryColumn::State);

			emit dataChanged(index, index);

			anyStarted = true;
			break;
		}
	}
	return anyStarted;
}
void AFRemuxQueueModel::FinishEntry(bool sucess)
{
	for(int row = 0; row < m_queue.length(); row++)
	{
		RemuxQueueEntry& entry = m_queue[row];
		if(RemuxEntryState::InProgress == entry.state) {
			if(sucess)
				entry.state = RemuxEntryState::Complete;
			else
				entry.state = RemuxEntryState::Error;

			QModelIndex index = this->index(row, RemuxEntryColumn::State);

			emit dataChanged(index, index);
			break;
		}
	}
}
bool AFRemuxQueueModel::CanClearFinished() const
{
	bool canClearFinished = false;
	for(const RemuxQueueEntry& entry : m_queue)
	{
		if(RemuxEntryState::Complete == entry.state) {
			canClearFinished = true;
			break;
		}
	}
	return canClearFinished;
}
void AFRemuxQueueModel::ClearFinished()
{
	int index = 0;
	for(index = 0; index < m_queue.size(); index++) {
		const RemuxQueueEntry& entry = m_queue[index];
		if(RemuxEntryState::Complete == entry.state) {
			beginRemoveRows(QModelIndex(), index, index);
			m_queue.removeAt(index);
			endRemoveRows();
			index--;
		}
	}
}
void AFRemuxQueueModel::ClearAll()
{
	beginRemoveRows(QModelIndex(), 0, m_queue.size() -1);
	m_queue.clear();
	endRemoveRows();
}
//
QVariant AFRemuxQueueModel::GetIcon(RemuxEntryState state)
{
	QVariant icon;
	QStyle* style = QApplication::style();

	switch(state)
	{
		case RemuxEntryState::Complete:
			icon = style->standardIcon(QStyle::SP_DialogApplyButton);
			break;
			
		case RemuxEntryState::InProgress:
			icon = style->standardIcon(QStyle::SP_ArrowRight);
			break;

		case RemuxEntryState::Error:
			icon = style->standardIcon(QStyle::SP_DialogCancelButton);
			break;

		case RemuxEntryState::InvalidPath:
			icon = style->standardIcon(QStyle::SP_MessageBoxWarning);
			break;

		default:
			break;
	}
	return icon;
}
void AFRemuxQueueModel::CheckInputPath(int row)
{
	RemuxQueueEntry& entry = m_queue[row];
	if(entry.sourcePath.isEmpty()) {
		entry.state = RemuxEntryState::Empty;
	} else {
		entry.sourcePath = QDir::toNativeSeparators(entry.sourcePath);
		QFileInfo fileInfo(entry.sourcePath);
		if(fileInfo.exists())
			entry.state = RemuxEntryState::Ready;
		else
			entry.state = RemuxEntryState::InvalidPath;

		QString newExt = ".mp4";
		QString suffix = fileInfo.suffix();
		if(suffix.contains("mov", Qt::CaseInsensitive) ||
		   suffix.contains("mpr", Qt::CaseInsensitive)) {
			newExt = ".remuxed." + suffix;
		}

		if(RemuxEntryState::Ready == entry.state) {
			entry.targetPath = QDir::toNativeSeparators(
				fileInfo.path() + QDir::separator() + fileInfo.completeBaseName() + newExt);
		}
	}

	if(RemuxEntryState::Ready == entry.state && m_isProcessing) {
		entry.state = RemuxEntryState::Pending;
	}

	emit dataChanged(index(row, 0), index(row, RemuxEntryColumn::Count));
}
//

void AFRemuxWorker::UpdateProgress(float percent)
{
	if(abs(m_lastProgress - percent) < 0.1f)
		return;

	emit qSignalUpdateProgress(percent);
	m_lastProgress = percent;
}
//
void AFRemuxWorker::qSlotRemux(const QString& source, const QString& target)
{
	m_isWorking = true;

	auto callback = [](void* data, float percent) {
		AFRemuxWorker* worker = static_cast<AFRemuxWorker*>(data);
		QMutexLocker lock(&worker->m_updateMutex);
		worker->qSignalUpdateProgress(percent);
		return worker->m_isWorking;
	};

	bool stopped = false;
	bool success = false;
	media_remux_job_t job_ = nullptr;
	if(media_remux_job_create(&job_, QT_TO_UTF8(source), QT_TO_UTF8(target))) {
		success = media_remux_job_process(job_, callback, this);
		media_remux_job_destroy(job_);
		stopped = !m_isWorking;
	}
	m_isWorking = false;

	emit qSignalRemuxFinished(!stopped && success);
}
//

const char* PATH_LIST_PROP = "pathList";
AFRemuxEntryPathItemDelegate::AFRemuxEntryPathItemDelegate(bool isOutput, const QString& defaultPath)
	:QStyledItemDelegate(),
	m_isOutput(isOutput),
	m_defaultPath(defaultPath)
{
}
//
QWidget* AFRemuxEntryPathItemDelegate::createEditor(QWidget* parent,
													const QStyleOptionViewItem& option,
													const QModelIndex& index) const
{
	RemuxEntryState state = index.model()
		->index(index.row(), RemuxEntryColumn::State)
		.data(RemuxEntryRole::EntryStateRole)
		.value<RemuxEntryState>();
	if(RemuxEntryState::Pending == state ||
	   RemuxEntryState::InProgress == state) {
		// Never allow modification of rows that are
		// in progress.
		return Q_NULLPTR;
	} else if(m_isOutput && RemuxEntryState::Ready != state) {
		// Do not allow modification of output rows
		// that aren't associated with a valid input.

		return Q_NULLPTR;
	} else if(!m_isOutput && RemuxEntryState::Complete == state) {
		// Don't allow modification of rows that are
		// already complete.
		return Q_NULLPTR;
	} else {
		QSizePolicy buttonSizePolicy(QSizePolicy::Policy::Minimum,
									 QSizePolicy::Policy::Expanding,
									 QSizePolicy::ControlType::PushButton);

		QWidget* container = new QWidget(parent);

		auto browseCallback = [this, container]() {
			const_cast<AFRemuxEntryPathItemDelegate*>(this)->HandleBrowse(container);
		};
		auto clearCallback = [this, container]() {
			const_cast<AFRemuxEntryPathItemDelegate*>(this)->HandleClear(container);
		};

		QHBoxLayout* layout = new QHBoxLayout();
		layout->setContentsMargins(0,0,0,0);
		layout->setSpacing(0);

		QLineEdit* lineEdit = new QLineEdit();
		lineEdit->setObjectName(QStringLiteral("text"));
		lineEdit->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding,
											QSizePolicy::Policy::Expanding,
											QSizePolicy::ControlType::LineEdit));
		layout->addWidget(lineEdit);

		QObject::connect(lineEdit, &QLineEdit::editingFinished, this,
						 &AFRemuxEntryPathItemDelegate::qSlotUpdateText);

		QToolButton* browseButton = new QToolButton();
		browseButton->setText("...");
		browseButton->setSizePolicy(buttonSizePolicy);
		layout->addWidget(browseButton);

		container->connect(browseButton, &QToolButton::clicked, browseCallback);

		// The "clear" button is not shown in output cells
		// or the insertion point's input cell.
		if(!m_isOutput && RemuxEntryState::Empty != state) {
			QToolButton* clearButton = new QToolButton();
			clearButton->setText("X");
			clearButton->setSizePolicy(buttonSizePolicy);
			layout->addWidget(clearButton);

			container->connect(clearButton, &QToolButton::clicked, clearCallback);
		}

		container->setLayout(layout);
		container->setFocusProxy(lineEdit);
		return container;
	}
}
void AFRemuxEntryPathItemDelegate::setEditorData(QWidget* editor,
												 const QModelIndex& index) const
{
	QLineEdit* edit = editor->findChild<QLineEdit*>();
	edit->setText(index.data().toString());
	editor->setProperty(PATH_LIST_PROP, QVariant());
}
void AFRemuxEntryPathItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
												const QModelIndex& index) const
{
	// We use the PATH_LIST_PROP property to pass a list of
	// path strings from the editor widget into the model's
	// NewPathsToProcessRole. This is only used when paths
	// are selected through the "browse" or "delete" buttons
	// in the editor. If the user enters new text in the
	// text box, we simply pass that text on to the model
	// as normal text data in the default role.
	QVariant pathListProp = editor->property(PATH_LIST_PROP);
	if(pathListProp.isValid())
	{
		QStringList list = editor->property(PATH_LIST_PROP).toStringList();
		if(m_isOutput) {
			if(!list.isEmpty())
				model->setData(index, list);
		} else {
			model->setData(index, list, RemuxEntryRole::NewPathsToProcessRole);
		}
	}
	else
	{
		QLineEdit* lineEdit = editor->findChild<QLineEdit*>();
		model->setData(index, lineEdit->text());
	}
}
void AFRemuxEntryPathItemDelegate::paint(QPainter* painter,
										 const QStyleOptionViewItem& option,
										 const QModelIndex& index) const
{
	RemuxEntryState state = index.model()
		->index(index.row(), RemuxEntryColumn::State)
		.data(RemuxEntryRole::EntryStateRole)
		.value<RemuxEntryState>();

	QStyleOptionViewItem localOption = option;
	initStyleOption(&localOption, index);
	if(m_isOutput) {
		if(RemuxEntryState::Ready != state) {
			QColor background= localOption.palette.color(
				QPalette::ColorGroup::Disabled,
				QPalette::ColorRole::Window);
			localOption.backgroundBrush = QBrush(background);
		}
	}

	QApplication::style()->drawControl(QStyle::CE_ItemViewItem,
									   &localOption, painter);
}
//
void AFRemuxEntryPathItemDelegate::HandleBrowse(QWidget* container)
{
	QString ExtensionPattern = "(*.mp4 *.flv *.mov *.mkv *.ts *.m3u8)";
	QLineEdit* edit = container->findChild<QLineEdit*>();

	QString currentPath = edit->text();
	if(currentPath.isEmpty())
		currentPath = m_defaultPath;

	bool isSet = false;
	if(m_isOutput)
	{
		QString newPath = SaveFile(container,
								   QTStr("Remux.SelectTarget"),
								   currentPath, ExtensionPattern);
		if(!newPath.isEmpty()) {
			container->setProperty(PATH_LIST_PROP,
								   QStringList() << newPath);
			isSet = true;
		}
	}
	else
	{
		QStringList paths = OpenFiles(container, QTStr("Remux.SelectRecording"), currentPath,
									  QTStr("Remux.OBSRecording") + QString(" ") + ExtensionPattern);
		if(!paths.isEmpty()) {
			container->setProperty(PATH_LIST_PROP, paths);
			isSet = true;
		}
#ifdef __APPLE__
		// TODO: Revisit when QTBUG-42661 is fixed
		container->window()->raise();
#endif
	}

	if(isSet)
		emit commitData(container);
}
void AFRemuxEntryPathItemDelegate::HandleClear(QWidget* container)
{
	// An empty string list will indicate that the entry is being
	// blanked and should be deleted.
	container->setProperty(PATH_LIST_PROP, QStringList());
	//
	emit commitData(container);
}
//
void AFRemuxEntryPathItemDelegate::qSlotUpdateText()
{
	QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(sender());
	QWidget* editor = lineEdit->parentWidget();
	//
	emit commitData(editor);
}