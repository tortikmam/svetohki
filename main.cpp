#include <iostream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>

#include "sort_search.h"
#include "dop_tree.h" // Добавили заголовок дерева

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
    APIClient(const string& api_token, const string& url = "https://trefle.io/api/v1"){
        token = api_token;
        baseUrl = url;
    };

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

 // это нужно для того, чтобы не записывать и не подтягивать каждый раз 60 записей
void loadFromFile(vector<PlantData*>& plants_vector, json& all_plants_array) {
    ifstream inFile("plants.json");
    if (inFile.is_open()) {
        try {
            inFile >> all_plants_array;
            for (const auto& item : all_plants_array) {
                PlantData* p = new PlantData();
                p->name_ru = item.value("name_ru", "");
                p->weight = 1; 
                plants_vector.push_back(p);
            }
            cout << "[System] load from plants.json: " << plants_vector.size() << " plants. " << endl;
        } catch (...) {
            cout << "[System] error reading plants.json or file is empty." << endl;
        }
        inFile.close();
    }
}

int main() {
    string token;
    ifstream tokenFile("token.txt");
    if (tokenFile.is_open()) {
        getline(tokenFile, token);
        tokenFile.close();
    } else {
        cerr << "error: token.txt not found!" << endl;
        return 1;
    }

    bool isSorted = false;

    APIClient client(token);
    vector<PlantData*> plants_vector; 
    json all_plants_array = json::array();
    DOPNode* rootDOP = nullptr; // Корень для ДОП
    
    // проверка наличия файла при запуске
    loadFromFile(plants_vector, all_plants_array);

    int choice = -1;
    while (choice != 0) {
        bool hasData = !plants_vector.empty();

        cout << "\n- - - - - menu - - - - -" << endl;
        cout << " " << endl;
        cout << "1. get new data from trefle API" << endl;
        if (hasData) {
            cout << "2. execute sort" << endl;
            // cout << "3. binary search" << endl;
            cout << "3. withdraw all plants" << endl;
            cout << "4. DOP tree" << endl;
        } else {
            cout << "[data not load, need to load data (1st option)]" << endl;
        }
        cout << "0. exit" << endl;
        cout << " " << endl;
        cout << "- - - - - - - - - -" << endl;
        cout << " " << endl;
        cout << "choose an option: ";
        
        if (!(cin >> choice)) { cin.clear(); cin.ignore(10000, '\n'); continue; }

        switch (choice) {
            case 1: {
                for (auto p : plants_vector) delete p;
                plants_vector.clear();
                all_plants_array = json::array();

                isSorted = false;

                int count = 0;
                int targetCount = 60; 
                int currentPage = 1;

                while (count < targetCount) {
                    json listResponse = client.getPlants(currentPage);
                    if (!listResponse.contains("data") || listResponse["data"].empty()) break;

                    for (const auto& item : listResponse["data"]) {
                        if (count >= targetCount) break;
                        int trefle_id = item.value("id", 0);
                        json detail = client.getPlantById(trefle_id);
                        if (detail.empty() || !detail.contains("data")) continue;

                        const auto& d = detail["data"];
                        json growth = (d.contains("main_species") && d["main_species"].is_object() && d["main_species"].contains("growth")) 
                                      ? d["main_species"]["growth"] : json::object();

                        auto t_min = extract_deg_c(growth, "minimum_temperature");
                        auto ph_min = growth.value("ph_minimum", json(nullptr));
                        if (t_min.is_null() && ph_min.is_null()) continue;

                        // Формируем JSON
                        json p;
                        p["trefle_id"] = trefle_id;
                        p["name_ru"] = item.value("common_name", "");
                        p["name_latin"] = item.value("scientific_name", "Unknown");
                        p["family"] = item.value("family", "");
                        p["genus"] = item.value("genus", "");
                        p["temp_min_c"] = t_min;
                        p["temp_max_c"] = extract_deg_c(growth, "maximum_temperature");
                        p["ph_min"] = ph_min;
                        p["ph_max"] = growth.value("ph_maximum", json(nullptr));
                        
                        all_plants_array.push_back(p);

                        PlantData* p_data = new PlantData();
                        p_data->name_ru = p["name_ru"];
                        p_data->weight = (int)p_data->name_ru.length() + 1; // Устанавливаем вес
                        plants_vector.push_back(p_data);

                        count++;
                        cout << "  [" << count << "/" << targetCount << "] OK: " << p["name_latin"] << " (ID: " << trefle_id << ")" << endl;
                        this_thread::sleep_for(chrono::milliseconds(50));
                    }
                    currentPage++;
                }
                // тут сохранение как было в прошлой версии, сразу при получении записей
                ofstream outFile("plants.json");
                outFile << all_plants_array.dump(4);
                outFile.close();
                cout << "data saved to plants.json" << endl;
                break;
            }

            case 2: {
                if (!hasData) break;
                quickSort(plants_vector, 0, (int)plants_vector.size() - 1, cmpByNameRU);
                isSorted = true;
                cout << "sort by name_ru completed" << endl;
                break;
            }

            // case 3: {
            //     if (!hasData) break;
            //     string key;
            //     cout << "need name_ru for search: ";
            //     cin.ignore();
            //     getline(cin, key);

            //     int resultIdx = binarySearch(plants_vector, key);
            //     if (resultIdx != -1) {
            //         string foundName = plants_vector[resultIdx]->name_ru;
            //         cout << " " << endl;
            //         cout << "\n- - - - - search result - - - - -" << endl;
                    
            //         for (const auto& j_obj : all_plants_array) {
            //             if (j_obj.value("name_ru", "") == foundName) {
            //                 cout << "family:          " << j_obj["family"] << endl;
            //                 cout << "genus:           " << j_obj["genus"] << endl;
            //                 cout << "name_latin:      " << j_obj["name_latin"] << endl;
            //                 cout << "name_ru:         " << j_obj["name_ru"] << endl;
            //                 cout << "ph_max:          " << j_obj["ph_max"] << endl;
            //                 cout << "ph_min:          " << j_obj["ph_min"] << endl;
            //                 cout << "temp_max_c:      " << j_obj["temp_max_c"] << " C" << endl;
            //                 cout << "temp_min_c:      " << j_obj["temp_min_c"] << " C" << endl;
            //                 cout << "trefle_id:       " << j_obj["trefle_id"] << endl;
            //                 break; 
            //             }
            //         }

            //     } else {
            //         cout << "plant '" << key << "' not found." << endl;
            //     }
            //     break;
            // }

            case 3: {
                if (!hasData) break;
                for(size_t i=0; i<plants_vector.size(); ++i) 
                    cout << i+1 << ". " << plants_vector[i]->name_ru << endl;
                break;
            }
            case 4: {
                if (!hasData) break;
                if (!isSorted) {
                    cout << "you need to sort the data first (option 2)" << endl;
                    break;
                }
                
                // Перестраиваем дерево перед поиском, чтобы учесть новые веса
                if (rootDOP) clearDOP(rootDOP);
                rootDOP = buildDOP_A2(plants_vector, 0, (int)plants_vector.size() - 1);

                string key;
                cout << "DOP search: ";
                cin.ignore();
                getline(cin, key);

                // Используем функцию из dop_tree.cpp
                DOPNode* found = searchDOP(rootDOP, key);
                
                if (found) {
                    found->data->weight++;
                    string foundName = found->data->name_ru;
                    
                    // Выводим всю инфу из JSON, как в пункте 3
                    cout << "\n- - - DOP search result - - -" << endl;
                    for (const auto& j_obj : all_plants_array) {
                        if (j_obj.value("name_ru", "") == foundName) {
                            cout << "family:          " << j_obj["family"] << endl;
                            cout << "genus:           " << j_obj["genus"] << endl;
                            cout << "name_latin:      " << j_obj["name_latin"] << endl;
                            cout << "name_ru:         " << j_obj["name_ru"] << endl;
                            cout << "temp_max_c:      " << j_obj["temp_max_c"] << " C" << endl;
                            cout << "temp_min_c:      " << j_obj["temp_min_c"] << " C" << endl;
                            cout << "weight:          " << found->data->weight << endl;
                            break; 
                        }
                    }

                } else {
                    cout << "plant not found in DOP tree." << endl;
                }
                break;
            }
        }
    }

    if (rootDOP) clearDOP(rootDOP);
    for (auto p : plants_vector) delete p;
    return 0;
}