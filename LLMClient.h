#pragma once
#include <string>
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>

using json = nlohmann::json;

class LLMClient {
private:
    std::string apiKey;
    std::string modelName;
    std::string baseUrl;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

public:
    LLMClient(const std::string& model, const std::string& key) 
        : modelName(model), apiKey(key) {
            if (model.find("gemini") != std::string::npos) {
                baseUrl = "https://generativelanguage.googleapis.com/v1beta/models/";
            } else {
                baseUrl = "https://api.openai.com/v1/chat/completions";
            }
    }

    std::string query(const std::string& system_prompt, const std::string& user_input) {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if(curl) {
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            
            std::string finalUrl;
            std::string json_str;

            // --- BRANCH 1: GOOGLE GEMINI LOGIC ---
            if (modelName.find("gemini") != std::string::npos) {
                finalUrl = baseUrl + modelName + ":generateContent?key=" + apiKey;

                std::string full_prompt = system_prompt + "\n\nTask:\n" + user_input;
                
                json payload;
                payload["contents"] = json::array({
                    {
                        {"parts", json::array({
                            {{"text", full_prompt}}
                        })}
                    }
                });
                json_str = payload.dump();
            } 
            // --- BRANCH 2: OPENAI LOGIC ---
            else {
                finalUrl = baseUrl;
                std::string auth = "Authorization: Bearer " + apiKey;
                headers = curl_slist_append(headers, auth.c_str());

                json payload;
                payload["model"] = modelName;
                payload["messages"] = json::array({
                    {{"role", "system"}, {"content", system_prompt}},
                    {{"role", "user"}, {"content", user_input}}
                });
                json_str = payload.dump();
            }

            curl_easy_setopt(curl, CURLOPT_URL, finalUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            // 4. Send Request
            res = curl_easy_perform(curl);
            if(res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << "\n";
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        try {
            auto responseJson = json::parse(readBuffer);
            
            if (responseJson.contains("error")) {
                std::cerr << "API Error: " << responseJson["error"] << "\n";
                return "[]";
            }

            // Gemini Response Path
            if (modelName.find("gemini") != std::string::npos) {
                if (responseJson.contains("candidates") && !responseJson["candidates"].empty()) {
                    return responseJson["candidates"][0]["content"]["parts"][0]["text"];
                }
            } 
            // OpenAI Response Path
            else {
                if (responseJson.contains("choices")) {
                    return responseJson["choices"][0]["message"]["content"];
                }
            }
            
            std::cerr << "Unexpected JSON structure: " << readBuffer << "\n";
            return "[]";

        } catch (...) {
            std::cerr << "Failed to parse AI response. Raw: " << readBuffer << "\n";
            return "[]";
        }
    }
};
