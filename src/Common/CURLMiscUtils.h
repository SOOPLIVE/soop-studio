#pragma once

#include <string>
#include <util/curl/curl-helper.h>


static size_t WriteCallback(void *contents, size_t size, 
                            size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static bool CURLWriteDataOutStringBuffer(const char* reqUrl, std::string& outrefReadBuffer)
{
    CURL* objCUrl;
    CURLcode resCUrl;
    
    objCUrl = curl_easy_init();
    
    
    if (objCUrl)
    {
        curl_easy_setopt(objCUrl, CURLOPT_URL, reqUrl);
        curl_easy_setopt(objCUrl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(objCUrl, CURLOPT_WRITEDATA, &outrefReadBuffer);
        resCUrl = curl_easy_perform(objCUrl);
        curl_easy_cleanup(objCUrl);
        
        if(resCUrl == CURLE_OK)
            return true;
        else
            return false;
    }
    
    return false;
}
