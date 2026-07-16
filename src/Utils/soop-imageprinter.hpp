
#include <obs-module.h>

static inline bool soop_imageprinter_make_source(const char* giftImageData) {
	static auto psModule = obs_get_module("soop-imageprinter");
	if (psModule == nullptr) {
		blog(LOG_ERROR, "%s (%d) : psModule : %d", __FILE__, __LINE__, psModule);
		return 0;
	}

	static auto pVoid = obs_get_module_lib(psModule);
	if (pVoid == nullptr) {
		blog(LOG_ERROR, "%s (%d) : pVoid : %d", __FILE__, __LINE__, pVoid);
		return 0;
	}

	int (*pfMakeImageSource)(const char* giftImageData) = nullptr;
	pfMakeImageSource = (decltype(pfMakeImageSource))os_dlsym(pVoid, "MakeGiftImageSource");
	if (pfMakeImageSource == nullptr) {
		blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
		return 0;
	}

	//

	return pfMakeImageSource(giftImageData);
}