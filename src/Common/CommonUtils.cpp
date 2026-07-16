#include "CommonUtils.h"

#include <util/curl/curl-helper.h>


static size_t string_write(char* ptr, size_t size, size_t nmemb, std::string& str)
{
    size_t total = size * nmemb;
    if(total)
        str.append(ptr, total);

    return total;
}
static size_t header_write(char* ptr, size_t size, size_t nmemb,
                           std::vector<std::string>& list)
{
    std::string str;

    size_t total = size * nmemb;
    if(total)
        str.append(ptr, total);

    if(str.back() == '\n')
        str.resize(str.size() - 1);
    if(str.back() == '\r')
        str.resize(str.size() - 1);

    list.push_back(std::move(str));
    return total;
}

 bool GetRemoteFile(const char* url, std::string& str, std::string& error,
                    long* responseCode, const char* contentType,
                    std::string request_type, const char* postData,
                    std::vector<std::string> extraHeaders,
                    std::string* signature, int timeoutSec,
                    bool fail_on_error, int postDataSize)
{
    std::vector<std::string> header_in_list;
    char error_in[CURL_ERROR_SIZE];
    CURLcode code = CURLE_FAILED_INIT;

    error_in[0] = 0;

    std::string versionString("");
    //versionString += App()->GetVersionString();

    std::string contentTypeString;
    if(contentType) {
        contentTypeString += "Content-Type: ";
        contentTypeString += contentType;
    }

    Curl curl {curl_easy_init(), curl_deleter};
    if(curl) {
        struct curl_slist* header = nullptr;

        header = curl_slist_append(header, versionString.c_str());

        if(!contentTypeString.empty()) {
            header = curl_slist_append(header,
                           contentTypeString.c_str());
        }

        for(std::string& h : extraHeaders)
            header = curl_slist_append(header, h.c_str());

        curl_easy_setopt(curl.get(), CURLOPT_URL, url);
        curl_easy_setopt(curl.get(), CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, header);
        curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_in);
        if(fail_on_error)
            curl_easy_setopt(curl.get(), CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, string_write);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &str);
        curl_obs_set_revoke_setting(curl.get());

        if(signature) {
            curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION, header_write);
            curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA,
                     &header_in_list);
        }

        if(timeoutSec)
            curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT,
                     timeoutSec);

        if(!request_type.empty()) {
            if(request_type != "GET")
                curl_easy_setopt(curl.get(),
                         CURLOPT_CUSTOMREQUEST,
                         request_type.c_str());

            // Special case of "POST"
            if(request_type == "POST") {
                curl_easy_setopt(curl.get(), CURLOPT_POST, 1);
                if(!postData)
                    curl_easy_setopt(curl.get(),
                             CURLOPT_POSTFIELDS,
                             "{}");
            }
        }
        if(postData) {
            if(postDataSize > 0) {
                curl_easy_setopt(curl.get(),
                         CURLOPT_POSTFIELDSIZE,
                         (long)postDataSize);
            }
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS,
                     postData);
        }

        code = curl_easy_perform(curl.get());
        if(responseCode)
            curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE,
                      responseCode);

        if(code != CURLE_OK) {
            error = strlen(error_in) ? error_in
                : curl_easy_strerror(code);
        } else if(signature) {
            for(std::string& h : header_in_list) {
                std::string name = h.substr(0, 13);
                if(name == "X-Signature: " ||
                    name == "x-signature: ") {
                    *signature = h.substr(13);
                    break;
                }
            }
        }

        curl_slist_free_all(header);
    }

    return code == CURLE_OK;
}
