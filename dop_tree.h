#ifndef DOP_TREE_H
#define DOP_TREE_H

#include <string>
#include <vector>
#include "sort_search.h" // Подключаем структуру PlantData

// Узел ДОП дерева
struct DOPNode {
    PlantData* data; // Указатель на данные
    DOPNode* left;
    DOPNode* right;

    DOPNode(PlantData* val) : data(val), left(nullptr), right(nullptr) {}
};

// Построение дерева по алгоритму А2 (Трудоемкость O(n*log n)) 
DOPNode* buildDOP_A2(const std::vector<PlantData*>& arr, int L, int R);

// Поиск по составному ключу (Семейство + Название)
DOPNode* searchDOP(DOPNode* root, const std::string& target_family, const std::string& target_name);

// Очистка только узлов дерева
void clearDOP(DOPNode* root);

#endif