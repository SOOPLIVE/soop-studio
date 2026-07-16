
#include <obs-module.h>

static inline int soop_crypt_encrypt(char* pszIn, char* pszOut, int iMaxOutSize) {
	auto psModule = obs_get_module("soop-crypt");
	if (psModule == nullptr) {
		blog(LOG_ERROR, "%s (%d) : psModule : %d", __FILE__, __LINE__, psModule);
		return 0;
	}

	auto pVoid = obs_get_module_lib(psModule);
	if (pVoid == nullptr) {
		blog(LOG_ERROR, "%s (%d) : pVoid : %d", __FILE__, __LINE__, pVoid);
		return 0;
	}

	int (*pfEncrypt)(char* szIn, char* szOut, int nMaxOutStrSize) = nullptr;
	pfEncrypt = (decltype(pfEncrypt))os_dlsym(pVoid, "Encrypt");
	if (pfEncrypt == nullptr) {
		blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
		return 0;
	}

	//

	return pfEncrypt(pszIn, pszOut, iMaxOutSize);
}

static inline void soop_crypt_get_hash(uint8_t pHash[16], uint8_t* pBuf, size_t nLen) {
	do {
		auto psModule = obs_get_module("soop-crypt");
		if (psModule == nullptr) {
			blog(LOG_ERROR, "%s (%d) : psModule : %d", __FILE__, __LINE__, psModule);
			break;
		}

		auto pVoid = obs_get_module_lib(psModule);
		if (pVoid == nullptr) {
			blog(LOG_ERROR, "%s (%d) : pVoid : %d", __FILE__, __LINE__, pVoid);
			break;
		}

		int (*pfGetHash)(uint8_t pHash[16], uint8_t * pBuf, size_t nLen) = nullptr;
		pfGetHash = (decltype(pfGetHash))os_dlsym(pVoid, "GetHash");
		if (pfGetHash == nullptr) {
			blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
			break;
		}

		pfGetHash(pHash, pBuf, nLen);
	} while (false);
}
