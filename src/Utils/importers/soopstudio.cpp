
#include "importers.hpp"

using namespace std;
using namespace json11;

bool SoopStudioImporter::Check(const string &path)
{
	BPtr<char> file_data = os_quick_read_utf8_file(path.c_str());
	string err;
	Json collection = Json::parse(file_data, err);

    if (collection["importer_type"] == Prog().c_str()) {
        return true;
    }

	return false;
}
