// [copy-obs]


#include "CSourceLabel.h"

void AFQSourceLabel::SourceRenamed(void *data, calldata_t *params)
{
	auto &label = *static_cast<AFQSourceLabel *>(data);

	const char *name = calldata_string(params, "new_name");
	label.setText(name);

	emit label.Renamed(name);
}

void AFQSourceLabel::SourceRemoved(void *data, calldata_t *)
{
	auto &label = *static_cast<AFQSourceLabel *>(data);
	emit label.Removed();
}

void AFQSourceLabel::SourceDestroyed(void *data, calldata_t *)
{
	auto &label = *static_cast<AFQSourceLabel *>(data);
	emit label.Destroyed();

	label.destroyedSignal.Disconnect();
	label.removedSignal.Disconnect();
	label.renamedSignal.Disconnect();
}
