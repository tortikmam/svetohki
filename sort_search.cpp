#include "sort_search.h"
#include <algorithm>

// Вспомогательная функция разделения для QuickSort
void partition(std::vector<PlantData*>& arr, int L, int R, int& i_out, int& j_out, CompareFunc cmp) {
    PlantData* x = arr[L + (R - L) / 2];
    int i = L, j = R;
    while (i <= j) {
        while (cmp(arr[i], x)) i++;
        while (cmp(x, arr[j])) j--;
        if (i <= j) {
            std::swap(arr[i], arr[j]);
            i++;
            j--;
        }
    }
    i_out = i; j_out = j;
}

// Быстрая сортировка с минимизацией стека (из методички)
void quickSort(std::vector<PlantData*>& arr, int L, int R, CompareFunc cmp) {
    while (L < R) {
        int i, j;
        partition(arr, L, R, i, j, cmp);
        if ((j - L) > (R - i)) {
            if (i < R) quickSort(arr, i, R, cmp);
            R = j;
        } else {
            if (L < j) quickSort(arr, L, j, cmp);
            L = i;
        }
    }
}

// Бинарный поиск СТРОГО ПО ПСЕВДОКОДУ из скрина
int binarySearch(const std::vector<PlantData*>& arr, const std::string& key) {
    int L = 0;             // L = 1 в псевдокоде
    int R = arr.size() - 1; // R = n в псевдокоде
    
    while (L <= R) {
        int m = (L + R) / 2;
        
        if (arr[m]->name_ru < key) {
            L = m + 1;
        } 
        else if (arr[m]->name_ru > key) {
            R = m - 1;
        } 
        else {
            return m; // Найдено!
        }
    }
    return -1; // Возвращаем -1 вместо 0 (так как в C++ индексы с 0)
}

bool cmpByNameRU(const PlantData* a, const PlantData* b) {
    return a->name_ru < b->name_ru;
}

bool cmpByFamilyAndName(const PlantData* a, const PlantData* b) {
    if (a->family == b->family) {
        return a->name_ru < b->name_ru;
    }
    return a->family < b->family;
}