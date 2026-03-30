#include <iostream>
#include <vector>
#include <pqxx/pqxx>
#include "sort_search.h"

using namespace std;

int main() {
    try {
        // Подключение к БД в Docker
        pqxx::connection C("host=localhost port=5433 dbname=flowers_db user=myuser password=mypassword");
        pqxx::work W(C);

        // Тянем только русские названия (используем DISTINCT для чистоты данных)
        pqxx::result r = W.exec("SELECT DISTINCT name_ru FROM flowers_base");

        vector<PlantData*> plants;
        for (auto const& row : r) {
            if (!row["name_ru"].is_null()) {
                PlantData* p = new PlantData();
                p->name_ru = row["name_ru"].as<string>();
                plants.push_back(p);
            }
        }

        cout << "Загружено записей: " << plants.size() << endl;

        // 1. Выполняем сортировку (обязательно перед бинарным поиском)
        quickSort(plants, 0, (int)plants.size() - 1, cmpByNameRU);
        
        cout << "\nПосле сортировки:" << endl;
        for(int i = 0; i < min((int)plants.size(), 61); ++i) {
            cout << i+1 << ": " << plants[i]->name_ru << endl;
        }

        // 2. Тестируем бинарный поиск
        string key = "Common nettle"; // Укажи здесь название, которое точно есть в базе
        int result = binarySearch(plants, key);

        if (result != -1) {
            cout << "\n[УСПЕХ] Объект '" << key << "' найден на позиции " << result << endl;
        } else {
            cout << "\n[ОШИБКА] Объект '" << key << "' не найден." << endl;
        }

        // Очистка памяти
        for (auto p : plants) delete p;

    } catch (const exception& e) {
        cerr << "Ошибка: " << e.what() << endl;
        return 1;
    }
    return 0;
}