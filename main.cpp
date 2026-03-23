#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Callback для записи данных от CURL в строку
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

class APIClient {
private:
    string baseUrl;
    string token;

public:
    APIClient(const string& api_token, const string& url = "https://trefle.io/api/v1") 
        : token(api_token), baseUrl(url) {}

    json fetchData(const string& endpoint) {
        CURL* curl = curl_easy_init();
        string responseString;

        if (curl) {
            string url = baseUrl + endpoint;
            url += (endpoint.find('?') == string::npos ? "?token=" : "&token=") + token;

            // Настройки для стабильности в WSL
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
            
            // КРИТИЧНО: Представляемся браузером и форсируем IPv4
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
            curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
            
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Отключаем проверку SSL если нет сертификатов
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Accept: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");

            CURLcode res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                cerr << "CURL error: " << curl_easy_strerror(res) << endl;
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                return json::object();
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        try {
            if (responseString.empty()) return json::object();
            return json::parse(responseString);
        } catch (const exception& e) {
            cerr << "JSON Parse error: " << e.what() << endl;
            return json::object();
        }
    }

    json getPlants(int page = 1) {
        return fetchData("/plants?page=" + to_string(page));
    }
};

json getAllPlants(APIClient& client, int maxPages = 5) {
    json allData = json::array();
    cout << "Starting data collection..." << endl;

    for (int page = 1; page <= maxPages; page++) {
        cout << "Loading page " << page << "... ";
        json pageData = client.getPlants(page);

        if (!pageData.empty() && pageData.contains("data") && pageData["data"].is_array()) {
            for (const auto& plant : pageData["data"]) {
                allData.push_back(plant);
            }
            cout << "Done. Total: " << allData.size() << " plants." << endl;
        } else {
            cout << "Failed or empty." << endl;
            break; 
        }
        // Небольшая пауза, чтобы API не ругалось на лимиты
        this_thread::sleep_for(chrono::milliseconds(200));
    }

    json result;
    result["data"] = allData;
    result["meta"]["total_downloaded"] = allData.size();
    return result;
}

int main() {
    // 1. Глобальная инициализация один раз
    curl_global_init(CURL_GLOBAL_DEFAULT);

    string token;
    ifstream file("token.txt");
    if (file.is_open()) {
        getline(file, token);
        if (!token.empty() && token.back() == '\r') token.pop_back();
        file.close();
    } else {
        cerr << "Error: token.txt not found!" << endl;
        curl_global_cleanup();
        return 1;
    }

    if (token.empty()) {
        cerr << "Error: Token is empty!" << endl;
        curl_global_cleanup();
        return 1;
    }

    APIClient client(token);
    
    // Загружаем, скажем, 3 страницы для теста
    json plants = getAllPlants(client, 3);

    if (!plants["data"].empty()) {
        // Очистка данных перед сохранением (убираем синонимы, если они тяжелые)
        for (auto& plant : plants["data"]) {
            if (plant.contains("synonyms")) plant.erase("synonyms");
        }

        // Сохранение в файл
        ofstream outFile("plants.json");
        if (outFile.is_open()) {
            outFile << plants.dump(4);
            outFile.close();
            cout << "\nSUCCESS: Saved " << plants["data"].size() << " plants to plants.json" << endl;
        }
    } else {
        cout << "\nNo data to save." << endl;
    }

    // 2. Глобальная очистка в самом конце
    curl_global_cleanup();
    return 0;
}