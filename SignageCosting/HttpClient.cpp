#include "HttpClient.h"
#include <iostream>
#include <string>
#include <curl/curl.h>

static constexpr bool HTTP_DEBUG = false;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string HttpClient::get(const std::string& url) {

    if (HTTP_DEBUG)
        std::cout << "\n[HTTP] GET " << url << std::endl;

    CURL* curl = curl_easy_init();
    std::string response;

    if (!curl) {
        std::cerr << "[HTTP ERROR] CURL init failed\n";
        return "";
    }

    // =========================
    // REQUEST SETUP
    // =========================
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // =========================
    // NETWORK RELIABILITY
    // =========================
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    // =========================
    // SSL FIX (DEV ONLY)
    // =========================
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_easy_cleanup(curl);

    // =========================
    // ERROR HANDLING
    // =========================
    if (res != CURLE_OK) {
        std::cerr << "[HTTP ERROR] Request failed: "
            << curl_easy_strerror(res) << "\n";
        std::cerr << "URL: " << url << "\n";
        return "";
    }

    if (httpCode != 200) {
        std::cerr << "[HTTP ERROR] HTTP " << httpCode
            << " | URL: " << url << "\n";
        return "";
    }

    if (response.empty()) {
        std::cerr << "[HTTP ERROR] Empty response | URL: " << url << "\n";
        return "";
    }

    // =========================
    // CLEAN SUCCESS OUTPUT
    // =========================
    if (HTTP_DEBUG)
    {
        std::cout << "[HTTP] OK "
            << httpCode
            << " | "
            << response.size()
            << " bytes\n";
    }

    return response;
}