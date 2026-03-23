#include <iostream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>

using namespace std;
using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    ((string*)userp)->append((char*)contents, totalSize);
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
        string responseString;
        CURL* curl = curl_easy_init();
        if(!curl) return json::object();

        string url = baseUrl + endpoint;
        url += (endpoint.find('?') == string::npos ? "?token=" : "&token=") + token;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res != CURLE_OK) return json::object();
        try {
            return json::parse(responseString);
        } catch (...) { return json::object(); }
    }

    json getPlants(int page) { 
        return fetchData("/plants?page=" + to_string(page)); 
    }
    
    json getPlantById(int id) { return fetchData("/plants/" + to_string(id)); }
};

json extract_deg_c(const json& growth_obj, const string& key) {
    if (growth_obj.is_object() && growth_obj.contains(key) && growth_obj[key].is_object()) {
        if (growth_obj[key].contains("deg_c") && !growth_obj[key]["deg_c"].is_null()) {
            return growth_obj[key]["deg_c"];
        }
    }
    return nullptr;
}

int main() {
    string token;
    ifstream tokenFile("token.txt");
    if (tokenFile.is_open()) {
        getline(tokenFile, token);
        tokenFile.close();
    } else {
        cerr << "Ошибка: token.txt не найден!" << endl;
        return 1;
    }

    APIClient client(token);
    json all_plants_array = json::array();
    
    int count = 0;
    int targetCount = 60; 
    int currentPage = 1;

    cout << "--- ЗАПУСК СТАБИЛЬНОГО СБОРА ---" << endl;

    while (count < targetCount) {
        cout << "\n[СТРАНИЦА " << currentPage << "]" << endl;
        json listResponse = client.getPlants(currentPage);

        if (!listResponse.contains("data") || !listResponse["data"].is_array() || listResponse["data"].empty()) {
            cout << "База Trefle закончилась." << endl;
            break; 
        }

        for (const auto& item : listResponse["data"]) {
            if (count >= targetCount) break;

            int trefle_id = item.value("id", 0);
            if (trefle_id == 0) continue;

            json detail = client.getPlantById(trefle_id);
            if (detail.empty() || !detail.contains("data")) continue;

            const auto& d = detail["data"];
            json growth = json::object();
            
            if (d.contains("main_species") && d["main_species"].is_object()) {
                if (d["main_species"].contains("growth") && d["main_species"]["growth"].is_object()) {
                    growth = d["main_species"]["growth"];
                }
            }

            auto t_min = extract_deg_c(growth, "minimum_temperature");
            auto ph_min = growth.value("ph_minimum", json(nullptr));

            if (t_min.is_null() && ph_min.is_null()) continue; 

            json p;
            p["trefle_id"] = trefle_id;
            
            p["name_ru"] = item.value("common_name", "");
            p["name_latin"] = item.value("scientific_name", "Unknown");
            p["family"] = item.value("family", "");
            p["genus"] = item.value("genus", "");
            p["image_url"] = item.value("image_url", "");

            p["temp_min_c"] = t_min;
            p["temp_max_c"] = extract_deg_c(growth, "maximum_temperature");
            p["ph_min"] = ph_min;
            p["ph_max"] = growth.value("ph_maximum", json(nullptr));
            p["light_level"] = growth.value("light", json(nullptr));
            p["humidity"] = growth.value("atmospheric_humidity", json(nullptr));
            
            if (d.contains("main_species") && d["main_species"].is_object()) {
                p["toxicity"] = d["main_species"].value("toxicity", json(nullptr));
            } else {
                p["toxicity"] = nullptr;
            }
            p["user_notes"] = "";

            all_plants_array.push_back(p);
            count++;
            
            cout << "  [" << count << "/" << targetCount << "] OK: " 
                 << p.value("name_latin", "Unknown") << " (ID: " << trefle_id << ")" << endl;

            this_thread::sleep_for(chrono::milliseconds(100));
        }
        currentPage++; 
    }

    ofstream outFile("plants.json");
    if (outFile.is_open()) {
        outFile << all_plants_array.dump(4);
        cout << "\n--- ГОТОВО: " << count << " растений в plants.json ---" << endl;
    }

    return 0;
}