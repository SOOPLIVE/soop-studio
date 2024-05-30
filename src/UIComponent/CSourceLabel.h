// [copy-obs]


// [obs-source-label.hpp]
#pragma once

#include <QLabel>
#include <obs.hpp>

class AFQSourceLabel : public QLabel {
	Q_OBJECT;

public:
	OBSSignal renamedSignal;
	OBSSignal removedSignal;
	OBSSignal destroyedSignal;

	AFQSourceLabel(const obs_source_t *source, QWidget *parent = nullptr,
		       Qt::WindowFlags f = Qt::WindowFlags())
		: QLabel(obs_source_get_name(source), parent, f),
		  renamedSignal(obs_source_get_signal_handler(source), "rename",
				&AFQSourceLabel::SourceRenamed, this),
		  removedSignal(obs_source_get_signal_handler(source), "remove",
				&AFQSourceLabel::SourceRemoved, this),
		  destroyedSignal(obs_source_get_signal_handler(source),
				  "destroy", &AFQSourceLabel::SourceDestroyed,
				  this)
	{
	}

protected:
	static void SourceRenamed(void *data, calldata_t *params);
	static void SourceRemoved(void *data, calldata_t *params);
	static void SourceDestroyed(void *data, calldata_t *params);

signals:
	void Renamed(const char *name);
	void Removed();
	void Destroyed();
};
