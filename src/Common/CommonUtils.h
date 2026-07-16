
#pragma once

#include <string>
#include <vector>
#include <memory>

#include <curl/curl.h>

static size_t string_write(char* ptr, size_t size, size_t nmemb, std::string& str);
static size_t header_write(char* ptr, size_t size, size_t nmemb,
                           std::vector<std::string>& list);

static auto curl_deleter = [](CURL* curl) {
    curl_easy_cleanup(curl);
};
using Curl = std::unique_ptr<CURL, decltype(curl_deleter)>;

extern size_t string_write(char* ptr, size_t size, size_t nmemb, std::string& str);
extern size_t header_write(char* ptr, size_t size, size_t nmemb,
                           std::vector<std::string>& list);
extern bool GetRemoteFile(const char* url, std::string& str, std::string& error,
                          long* responseCode = nullptr, const char* contentType = nullptr,
                          std::string request_type = "", const char* postData = nullptr,
                          std::vector<std::string> extraHeaders = std::vector<std::string>(),
                          std::string* signature = nullptr, int timeoutSec = 0,
                          bool fail_on_error = true, int postDataSize = 0);
