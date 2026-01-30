// ailee_rpc_client.cpp
#include "BitcoinRPCClient.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace ailee {

BitcoinRPCClient::BitcoinRPCClient(const std::string& rpcUser,
                                   const std::string& rpcPassword,
                                   const std::string& rpcUrl)
    : rpcUser_(rpcUser), rpcPassword_(rpcPassword), rpcUrl_(rpcUrl) {}

bool BitcoinRPCClient::broadcastCheckpoint(const std::string& hexTx) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[Error] Failed to init CURL" << std::endl;
        return false;
    }

    // Construct JSON-RPC payload
    json payload = {
        {"jsonrpc", "1.0"},
        {"id", "ailee_bridge"},
        {"method", "sendrawtransaction"},
        {"params", {hexTx}}
    };

    std::string payloadStr = payload.dump();

    // Response buffer
    std::string responseBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, rpcUrl_.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payloadStr.size()));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Basic auth
    std::string auth = rpcUser_ + ":" + rpcPassword_;
    curl_easy_setopt(curl, CURLOPT_USERPWD, auth.c_str());

    // Capture response
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            std::string* buffer = static_cast<std::string*>(userdata);
            buffer->append(ptr, size * nmemb);
            return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "[Error] CURL failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }
    if (httpCode < 200 || httpCode >= 300) {
        std::cerr << "[Error] RPC HTTP status: " << httpCode << std::endl;
        return false;
    }

    try {
        json response = json::parse(responseBuffer);
        if (response.contains("error") && !response["error"].is_null()) {
            std::cerr << "[Error] RPC returned: " << response["error"] << std::endl;
            return false;
        }
        std::cout << "[Success] Broadcasted TX: " << response["result"] << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Error] Failed to parse RPC response: " << e.what() << std::endl;
        return false;
    }
}

} // namespace ailee
