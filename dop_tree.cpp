#include "dop_tree.h"

DOPNode* buildDOP_A2(const std::vector<PlantData*>& arr, int L, int R) {
    if (L > R) return nullptr;

    // 1. Вычисляем суммарный вес поддерева 
    int wes = 0;
    for (int i = L; i <= R; ++i) {
        wes += arr[i]->weight;
    }

    // 2. Находим «центр тяжести» 
    int sum = 0;
    int k = L;
    for (int i = L; i <= R; ++i) {
        // Условие из алгоритма A2 в презентации [cite: 598]
        if (sum < (double)wes / 2.0 && (sum + arr[i]->weight) >= (double)wes / 2.0) {
            k = i;
            break;
        }
        sum += arr[i]->weight;
        k = i;
    }

    // 3. Создаем узел и рекурсивно строим поддеревья
    DOPNode* root = new DOPNode(arr[k]);
    root->left = buildDOP_A2(arr, L, k - 1);
    root->right = buildDOP_A2(arr, k + 1, R);

    return root;
}

DOPNode* searchDOP(DOPNode* root, const std::string& target_family, const std::string& target_name) {
    if (root == nullptr) {
        return nullptr;
    }

    // Совпало и семейство, и название — узел найден!
    if (root->data->family == target_family && root->data->name_ru == target_name) {
        return root;
    }

    // Логика бинарного поиска по составному ключу
    // Идем влево, если искомое семейство меньше, ИЛИ (семейства равны, но название меньше)
    if (target_family < root->data->family || 
       (target_family == root->data->family && target_name < root->data->name_ru)) {
        return searchDOP(root->left, target_family, target_name);
    }

    // Иначе идем вправо
    return searchDOP(root->right, target_family, target_name);
}

void clearDOP(DOPNode* root) {
    if (root) {
        clearDOP(root->left);
        clearDOP(root->right);
        delete root;
    }
}