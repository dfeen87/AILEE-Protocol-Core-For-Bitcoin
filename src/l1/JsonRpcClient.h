#pragma once

#include "Global_Seven.h"
#include <curl/curl.h>
#include "third_party/nlohmann/json.hpp"
#include <optional>
#include <string>

namespace ailee {
namespace global_seven {

class JsonRpcClient {
public:
    JsonRpcClient(std::string endpoint,
                  std::string user,
                  std::string pass,
                  std::string version = "2.0",
                  std::string id = "ailee-core")
        : endpoint_(std::move(endpoint)),
          user_(std::move(user)),
          pass_(std::move(pass)),
          version_(std::move(version)),
          id_(std::move(id)) {}

    std::optional<nlohmann::json> call(const std::string& method,
                                       const nlohmann::json& params,
                                       ErrorCallback onError) const {
        nlohmann::json payload;
        payload["jsonrpc"] = version_;
        payload["id"] = id_;
        payload["method"] = method;
        payload["params"] = params;

        std::string response;
        if (!perform(payload.dump(), response, onError)) {
            return std::nullopt;
        }

        try {
            auto json = nlohmann::json::parse(response);
            if (json.contains("error")) {
                if (onError) {
                    onError(AdapterError{
                        Severity::Error,
                        "RPC error: " + json["error"].dump(),
                        "RPC",
                        0});
                }
                return std::nullopt;
            }
            return json;
        } catch (const std::exception& e) {
            if (onError) {
                onError(AdapterError{Severity::Error,
                                     std::string("RPC parse failed: ") + e.what(),
                                     "RPC",
                                     0});
            }
            return std::nullopt;
        }
    }

private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        auto* buffer = static_cast<std::string*>(userp);
        buffer->append(static_cast<char*>(contents), size * nmemb);
        return size * nmemb;
    }

    bool perform(const std::string& body, std::string& response, ErrorCallback onError) const {
        CURL* curl = curl_easy_init();
        if (!curl) {
            if (onError) {
                onError(AdapterError{Severity::Error, "Failed to init CURL", "RPC", 0});
            }
            return false;
        }

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, endpoint_.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        if (!user_.empty()) {
            std::string auth = user_ + ":" + pass_;
            curl_easy_setopt(curl, CURLOPT_USERPWD, auth.c_str());
        }

        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            if (onError) {
                onError(AdapterError{
                    Severity::Error,
                    std::string("RPC request failed: ") + curl_easy_strerror(res),
                    "RPC",
                    0});
            }
            return false;
        }

        if (http_code < 200 || http_code >= 300) {
            if (onError) {
                onError(AdapterError{
                    Severity::Error,
                    "RPC HTTP error: " + std::to_string(http_code),
                    "RPC",
                    0});
            }
            return false;
        }

        return true;
    }

    std::string endpoint_;
    std::string user_;
    std::string pass_;
    std::string version_;
    std::string id_;
};

inline std::optional<uint64_t> parseHexU64(const std::string& hex) {
    if (hex.empty()) return std::nullopt;
    size_t idx = 0;
    try {
        auto value = std::stoull(hex, &idx, 16);
        if (idx == 0) return std::nullopt;
        return value;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

inline std::chrono::system_clock::time_point fromUnixSeconds(uint64_t seconds) {
    return std::chrono::system_clock::time_point{std::chrono::seconds(seconds)};
}

} // namespace global_seven
} // namespace ailee
