#include <iostream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>

using json = nlohmann::json;

// функция записи данных полученных от curl в строку
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    ((std::string*)userp)->append((char*)contents, totalSize);
    return totalSize;
}
//класс для работы с API 
class APIClient {
private: // базовый URL и токен для доступа к API
    std::string baseUrl;
    std::string token;

public: // конструктор для инициализации токена и базового URL
    APIClient(const std::string& api_token, const std::string& url = "https://trefle.io/api/v1")     
        : token(api_token), baseUrl(url) {}

    json fetchData(const std::string& endpoint) { // функция для получения данных с API
        CURL* curl = curl_easy_init();
        CURLcode res;
        std::string responseString;

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if(curl) { // формируем полный URL для запроса
            std::string url = baseUrl + endpoint;
            if (endpoint.find('?') == std::string::npos) {
                url += "?token=" + token;
            } else {
                url += "&token=" + token;
            }
            std::cout << "Request URL: " << url << std::endl;
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
                std::cerr << "curl - error " << curl_easy_strerror(res) << std::endl;
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return json::object();
            }

            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            std::cout << "HTTP Status Code: " << http_code << std::endl;
            
            curl_slist_free_all(headers); 
            curl_easy_cleanup(curl); 
        }

        curl_global_cleanup(); 
        try {
            json data = json::parse(responseString);
            return data;
        } catch (const std::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            std::cerr << "Response: " << responseString << std::endl;
            return json::object();
        }
    }
    // функции для получения данных о растениях с API
    json getPlants(int page = 1) { // функция для получения списка растений с определенной страницы
        return fetchData("/plants?page=" + std::to_string(page));
    }

    json getPlantById(int id) { // функция для получения данных о конкретном растении по его ID
        return fetchData("/plants/" + std::to_string(id));
    }

    json searchPlants(const std::string& query) { // функция для поиска растений по запросу
        return fetchData("/plants/search?q=" + query);
    }
};

json getAllPlants(APIClient& client, int maxPages = 5) { // функция для получения всех растений с определенного количества страниц
    json allData = json::array();
    
    std::cout << "started receive data" << std::endl;
    
    for (int page = 1; page <= maxPages; page++) {
        std::cout << "load page " << page << "..." << std::endl;
        
        json pageData = client.getPlants(page);
        
        if (!pageData.empty() && pageData.contains("data")) {
            for (const auto& plant : pageData["data"]) {
                allData.push_back(plant);
            }
            std::cout << "  -> received " << pageData["data"].size() 
                      << " plants total: " << allData.size() << std::endl;
        }
    }
    // формируем итоговый JSON объект, который содержит массив всех растений и метаинформацию о количестве полученных данных и страницах
    json result;
    result["data"] = allData;
    result["meta"]["total"] = allData.size();
    result["meta"]["pages_downloaded"] = maxPages;
    
    std::cout << "total received: " << allData.size() << " plants" << std::endl;
    return result;
}

int main() { // берем из API данные о растениях по моему токену
    APIClient client("usr-jVbAaSE1s0aXz2EP729tqHDxNBJ0lm8gaPDNNoRww6Y");
    
    std::cout << "get data from API" << std::endl; 
    json plants = getAllPlants(client, 20);

    if(!plants.empty() && plants.contains("data")){ 
        std::cout << "data received:" << std::endl;
        std::cout << "found plants: " << plants["data"].size() << std::endl;

        for (auto& plant : plants["data"]) {
            if (plant.contains("synonyms")) {
                plant.erase("synonyms");
            }
        }

        int count = 0;
        for (const auto& plant : plants["data"]) {
            if (count >= 100) break;

            std::cout << "\nPlant " << count + 1 << ":" << std::endl;
            // эти штуки нужны чтобы вывелось только то, что есть, а не "null" или "[]"
            if (plant.contains("common_name") && !plant["common_name"].is_null())
                std::cout << " common_name: " << plant["common_name"] << std::endl;
            if (plant.contains("scientific_name") && !plant["scientific_name"].is_null())
                std::cout << " scientific_name: " << plant["scientific_name"] << std::endl;
            if (plant.contains("family") && !plant["family"].is_null()) 
                std::cout << " family: " << plant["family"] << std::endl;
            if (plant.contains("image_url") && !plant["image_url"].is_null())
                std::cout << " image_url: " << plant["image_url"] << std::endl;
            if (plant.contains("ph_minimum") && !plant["ph_minimum"].is_null())
                std::cout << " ph_minimum: " << plant["ph_minimum"] << std::endl;
            if (plant.contains("ph_maximum") && !plant["ph_maximum"].is_null())
                std::cout << " ph_maximum: " << plant["ph_maximum"] << std::endl;
            
            count++;
        }
        
        std::ofstream outFile("plants.json"); //запись полученных данных в файл plants.json
        if (outFile.is_open()) {
            outFile << plants.dump(4);
            outFile.close();
            std::cout << "\ndata saved to plants.json (" << plants["data"].size() << " plants)" << std::endl;
        } else {
            std::cerr << "error opening file for writing" << std::endl;
        }
    } else {
        std::cout << "no data received" << std::endl;
    }
    
    return 0;
}