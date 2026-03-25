#ifndef SORT_SEARCH_H
#define SORT_SEARCH_H

#include <vector>
#include <string>

struct PlantData {
    std::string name_ru;
};

// Тип функции-компаратора для универсальной сортировки
typedef bool (*CompareFunc)(const PlantData*, const PlantData*);

void quickSort(std::vector<PlantData*>& arr, int L, int R, CompareFunc cmp);
int binarySearch(const std::vector<PlantData*>& arr, const std::string& key);

// Компаратор для работы по имени
bool cmpByNameRU(const PlantData* a, const PlantData* b);

#endif