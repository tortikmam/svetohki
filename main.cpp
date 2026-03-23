#include <iostream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>

using namespace std;
using json = nlohmann::json;

// функция записи данных полученных от curl в строку
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    ((string*)userp)->append((char*)contents, totalSize);
    return totalSize;
}
//класс для работы с API 
class APIClient {
private: // базовый URL и токен для доступа к API
    string baseUrl;
    string token;

public: // конструктор для инициализации токена и базового URL
    APIClient(const string& api_token, const string& url = "https://trefle.io/api/v1")     
        : token(api_token), baseUrl(url) {}

    json fetchData(const string& endpoint) { // функция для получения данных с API
        CURL* curl = curl_easy_init();
        CURLcode res;
        string responseString;

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if(curl) { // формируем полный URL для запроса
            string url = baseUrl + endpoint;
            if (endpoint.find('?') == string::npos) {
                url += "?token=" + token;
            } else {
                url += "&token=" + token;
            }
            cout << "Request URL: " << url << endl;
            // Настраиваем curl для выполнения GET запроса
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            // Устанавливаем заголовки для запроса
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Accept: application/json");
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            res = curl_easy_perform(curl);

            if(res != CURLE_OK) { 
                cerr << "curl - error " << curl_easy_strerror(res) << endl;
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return json::object();
            }

            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            cout << "HTTP Status Code: " << http_code << endl;
            
            curl_slist_free_all(headers); 
            curl_easy_cleanup(curl); 
        }

        curl_global_cleanup(); 
        try {
            json data = json::parse(responseString);
            return data;
        } catch (const exception& e) {
            cerr << "JSON parse error: " << e.what() << endl;
            cerr << "Response: " << responseString << endl;
            return json::object();
        }
    }
    // функции для получения данных о растениях с API
    json getPlants(int page = 1) { // функция для получения списка растений с определенной страницы
        return fetchData("/plants?page=" + to_string(page));
    }

    json getPlantById(int id) { // функция для получения данных о конкретном растении по его ID
        return fetchData("/plants/" + to_string(id));
    }

    json searchPlants(const string& query) { // функция для поиска растений по запросу
        return fetchData("/plants/search?q=" + query);
    }
};

json getAllPlants(APIClient& client, int maxPages = 5) { // функция для получения всех растений с определенного количества страниц
    json allData = json::array();
    
    cout << "started receive data" << endl;
    
    for (int page = 1; page <= maxPages; page++) {
        cout << "load page " << page << "..." << endl;
        
        json pageData = client.getPlants(page);
        
        if (!pageData.empty() && pageData.contains("data")) {
            for (const auto& plant : pageData["data"]) {
                allData.push_back(plant);
            }
            cout << "  -> received " << pageData["data"].size() 
                      << " plants total: " << allData.size() << endl;
        }
    }
    // формируем итоговый JSON объект, который содержит массив всех растений и метаинформацию о количестве полученных данных и страницах
    json result;
    result["data"] = allData;
    result["meta"]["total"] = allData.size();
    result["meta"]["pages_downloaded"] = maxPages;
    
    cout << "total received: " << allData.size() << " plants" << endl;
    return result;
}

int main() { // берем из API данные о растениях по моему токену

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
    cout << "get data from API" << endl; 
    json plants = getAllPlants(client, 20);

    if(!plants.empty() && plants.contains("data")){ 
        cout << "data received:" << endl;
        cout << "found plants: " << plants["data"].size() << endl;

        for (auto& plant : plants["data"]) {
            if (plant.contains("synonyms")) {
                plant.erase("synonyms");
            }
        }

        int count = 0;
        for (const auto& plant : plants["data"]) {
            if (count >= 100) break;

            cout << "\nPlant " << count + 1 << ":" << endl;
            // эти штуки нужны чтобы вывелось только то, что есть, а не "null" или "[]"
            if (plant.contains("common_name") && !plant["common_name"].is_null())
                cout << " common_name: " << plant["common_name"] << endl;
            if (plant.contains("scientific_name") && !plant["scientific_name"].is_null())
                cout << " scientific_name: " << plant["scientific_name"] << endl;
            if (plant.contains("family") && !plant["family"].is_null()) 
                cout << " family: " << plant["family"] << endl;
            if (plant.contains("image_url") && !plant["image_url"].is_null())
                cout << " image_url: " << plant["image_url"] << endl;
            if (plant.contains("ph_minimum") && !plant["ph_minimum"].is_null())
                cout << " ph_minimum: " << plant["ph_minimum"] << endl;
            if (plant.contains("ph_maximum") && !plant["ph_maximum"].is_null())
                cout << " ph_maximum: " << plant["ph_maximum"] << endl;
            
            count++;
        }
        
        ofstream outFile("plants.json"); //запись полученных данных в файл plants.json
        if (outFile.is_open()) {
            outFile << plants.dump(4);
            outFile.close();
            cout << "\ndata saved to plants.json (" << plants["data"].size() << " plants)" << endl;
        } else {
            cerr << "error opening file for writing" << endl;
        }
    } else {
        cout << "no data received" << endl;
    }
    
    return 0;
}