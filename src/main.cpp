#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <pqxx/pqxx>
#include <iostream>
#include <vector>
#include <string>
#include <map>

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"

#include "sort_search.h"
#include "dop_tree.h"

// Подключаем библиотеку для загрузки изображений
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

// Глобальный вектор для хранения текстур текущего выбранного растения
vector<GLuint> current_textures;

// Функция для загрузки текстуры в OpenGL
GLuint LoadTextureFromFile(const char* filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
    if (data == NULL) return 0;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return texture;
}

// Очистка старых текстур
void ClearCurrentTextures() {
    if (!current_textures.empty()) {
        glDeleteTextures(current_textures.size(), current_textures.data());
        current_textures.clear();
    }
}

struct FlowerBase {
    int id;
    string name_ru;
    string name_latin;
    string family;
    string genus;
    string image_url;
};

struct FlowerFull : FlowerBase {
    float temp_min, temp_max;
    float ph_min, ph_max;
    int light_level;
    int humidity;
    string toxicity;
    string user_notes;
    string created_at;
};

// Глобальные данные
vector<FlowerBase> base_list;
map<int, FlowerBase> base_map; 

vector<PlantData*> sortable_list_ru;  // Для quick
vector<PlantData*> sortable_list_lat; // Для ДОП 

DOPNode* global_dop_root = nullptr; // Корень дерева

void ClearAllData() {
    for (auto p : sortable_list_ru) delete p;
    sortable_list_ru.clear();
    for (auto p : sortable_list_lat) delete p;
    sortable_list_lat.clear();
    if (global_dop_root) {
        clearDOP(global_dop_root); //функция очистки
        global_dop_root = nullptr;
    }
    base_map.clear();
    ClearCurrentTextures(); // Очищаем и текстуры
}

void SyncData() {
    ClearAllData();
    for (const auto& fb : base_list) {
        base_map[fb.id] = fb;
        
        // Данные для quick поиска
        PlantData* p_ru = new PlantData();
        p_ru->name_ru = fb.name_ru;
        p_ru->weight = fb.id;
        sortable_list_ru.push_back(p_ru);

        // Данные для дерева кладем латынь в name_ru для корректной работы
        PlantData* p_lat = new PlantData();
        p_lat->name_ru = fb.name_latin;
        p_lat->weight = fb.id; 
        sortable_list_lat.push_back(p_lat);
    }
}

// Компаратор для латыни    
bool cmpByLatin(const PlantData* a, const PlantData* b) {
    return a->name_ru < b->name_ru;
}

void LoadBaseList(vector<FlowerBase>& out_list) {
    try {
        pqxx::connection c("host=127.0.0.1 port=5433 dbname=flowers_db user=myuser password=mypassword");
        pqxx::work txn(c);
        // Добавили image_url в запрос
        pqxx::result r = txn.exec("SELECT id, name_ru, name_latin, family, genus, image_url FROM flowers_base ORDER BY id");
        out_list.clear();
        for (auto row : r) {
            out_list.push_back({
                row["id"].as<int>(),
                row["name_ru"].is_null() ? "---" : row["name_ru"].c_str(),
                row["name_latin"].c_str(),
                row["family"].is_null() ? "---" : row["family"].c_str(),
                row["genus"].is_null() ? "---" : row["genus"].c_str(),
                row["image_url"].is_null() ? "" : row["image_url"].c_str()
            });
        }
        SyncData();
    } catch (const exception &e) { cerr << "DB Error: " << e.what() << endl; }
}

FlowerFull GetFullDetails(int id) {
    FlowerFull f;
    try {
        pqxx::connection c("host=127.0.0.1 port=5433 dbname=flowers_db user=myuser password=mypassword");
        pqxx::work txn(c);
        string sql = "SELECT b.*, t.temp_min_c, t.temp_max_c, t.ph_min, t.ph_max, t.light_level, t.humidity, t.toxicity, t.user_notes "
                          "FROM flowers_base b LEFT JOIN flowers_technical t ON b.id = t.flower_id WHERE b.id = " + to_string(id);
        pqxx::row row = txn.exec1(sql);
        f.id = row["id"].as<int>();
        f.name_ru = row["name_ru"].is_null() ? "---" : row["name_ru"].c_str();
        f.name_latin = row["name_latin"].c_str();
        f.family = row["family"].is_null() ? "---" : row["family"].c_str();
        f.genus = row["genus"].is_null() ? "---" : row["genus"].c_str();
        f.image_url = row["image_url"].is_null() ? "" : row["image_url"].c_str();
        f.created_at = row["created_at"].c_str();
        f.temp_min = row["temp_min_c"].as<float>(0.0f);
        f.temp_max = row["temp_max_c"].as<float>(0.0f);
        f.ph_min = row["ph_min"].as<float>(0.0f);
        f.ph_max = row["ph_max"].as<float>(0.0f);
        f.light_level = row["light_level"].as<int>(0);
        f.humidity = row["humidity"].as<int>(0);
        f.toxicity = row["toxicity"].is_null() ? "Нет" : row["toxicity"].c_str();
        f.user_notes = row["user_notes"].is_null() ? "" : row["user_notes"].c_str();

        // ЗАГРУЗКА КАРТИНКИ: Загружаем один раз при получении деталей
        ClearCurrentTextures();
        if (!f.image_url.empty()) {
            for (int i = 1; i <= 3; ++i) {
                string full_path = ".." + f.image_url + to_string(i) + ".jpg";
                GLuint tid = LoadTextureFromFile(full_path.c_str());
                if (tid != 0) current_textures.push_back(tid);
            }
        }
    } catch (...) {}
    return f;
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return -1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_Window* window = SDL_CreateWindow("Flower Database & Algorithms", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    glewInit();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.Fonts->AddFontFromFileTTF("./fonts/arial.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 150");

    FlowerFull selected_flower;
    bool show_details = false;
    bool running = true;
    char search_ru[128] = "";
    char search_lat[128] = "";

    LoadBaseList(base_list);

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
        }

        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplSDL2_NewFrame(); ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_None);

        // управление
        ImGui::Begin("Алгоритмы и Поиск");
        
        if (ImGui::Button("Обновить данные")) LoadBaseList(base_list);
        
        ImGui::Separator();
        ImGui::Text("1. МАССИВ (RU)");
        if (ImGui::Button("QuickSort по RU")) {
            if (!sortable_list_ru.empty())
                quickSort(sortable_list_ru, 0, sortable_list_ru.size() - 1, cmpByNameRU);
        }
        ImGui::InputText("Имя (RU)", search_ru, IM_ARRAYSIZE(search_ru));
        if (ImGui::Button("BinarySearch RU")) {
            quickSort(sortable_list_ru, 0, sortable_list_ru.size() - 1, cmpByNameRU);
            int idx = binarySearch(sortable_list_ru, search_ru);
            if (idx != -1) {
                selected_flower = GetFullDetails(sortable_list_ru[idx]->weight);
                show_details = true;
            }
        }

        ImGui::Separator();
        ImGui::Text("2. ДОП ДЕРЕВО (Latin)");
        if (ImGui::Button("Построить дерево А2")) {
            if (!sortable_list_lat.empty()) {
                quickSort(sortable_list_lat, 0, sortable_list_lat.size() - 1, cmpByLatin);
                if (global_dop_root) clearDOP(global_dop_root);
                global_dop_root = buildDOP_A2(sortable_list_lat, 0, sortable_list_lat.size() - 1);
            }
        }
        ImGui::InputText("Имя (Latin)", search_lat, IM_ARRAYSIZE(search_lat));
        if (ImGui::Button("Поиск по ДОП")) {
            if (global_dop_root) {
                DOPNode* res = searchDOP(global_dop_root, search_lat);
                if (res) {
                    selected_flower = GetFullDetails(res->data->weight);
                    show_details = true;
                }
            }
        }
        if (global_dop_root) ImGui::TextColored(ImVec4(0, 1, 0, 1), "Дерево активно");
        else ImGui::TextColored(ImVec4(1, 1, 0, 1), "Дерево не построено");

        ImGui::End();

        // база
        ImGui::Begin("Реестр растений");
        if (ImGui::BeginTable("BaseTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Название (RU)");
            ImGui::TableSetupColumn("Латынь");
            ImGui::TableSetupColumn("Семейство");
            ImGui::TableSetupColumn("Род");
            ImGui::TableHeadersRow();

            for (const auto& p : sortable_list_ru) {
                FlowerBase& fb = base_map[p->weight];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                char label[32]; sprintf(label, "%d", fb.id);
                if (ImGui::Selectable(label, selected_flower.id == fb.id, ImGuiSelectableFlags_SpanAllColumns)) {
                    selected_flower = GetFullDetails(fb.id);
                    show_details = true;
                }
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", fb.name_ru.c_str());
                ImGui::TableSetColumnIndex(2); ImGui::Text("%s", fb.name_latin.c_str());
                ImGui::TableSetColumnIndex(3); ImGui::Text("%s", fb.family.c_str());
                ImGui::TableSetColumnIndex(4); ImGui::Text("%s", fb.genus.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::End();

        if (show_details) {
            ImGui::Begin("Подробная информация", &show_details, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "ОСНОВНЫЕ ДАННЫЕ");
            ImGui::Separator();
            ImGui::Text("ID: %d", selected_flower.id);
            ImGui::Text("Название (RU): %s", selected_flower.name_ru.c_str());
            ImGui::Text("Название (Latin): %s", selected_flower.name_latin.c_str());
            ImGui::Text("Семейство: %s", selected_flower.family.c_str());
            ImGui::Text("Род: %s", selected_flower.genus.c_str());
            
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "ТЕХНИЧЕСКИЕ ХАРАКТЕРИСТИКИ");
            ImGui::Separator();
            
            if (ImGui::BeginTable("TechTable", 2, ImGuiTableFlags_NoBordersInBody)) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Температура:");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%.1f ... %.1f C", selected_flower.temp_min, selected_flower.temp_max);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Кислотность (PH):");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%.1f ... %.1f", selected_flower.ph_min, selected_flower.ph_max);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Влажность:");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%d %%", selected_flower.humidity);
                
                ImGui::EndTable();
            }

            if (!selected_flower.user_notes.empty()) {
                ImGui::Separator();
                ImGui::Text("Заметки:");
                ImGui::TextWrapped("%s", selected_flower.user_notes.c_str());
            }

            // картинки
            if (!current_textures.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "ФОТОГРАФИИ");
                ImGui::Separator();
                for (size_t i = 0; i < current_textures.size(); ++i) {
                    ImGui::Image((void*)(intptr_t)current_textures[i], ImVec2(250, 250));
                    if (i < current_textures.size() - 1) ImGui::SameLine();
                }
            }

            ImGui::Spacing();
            if (ImGui::Button("Закрыть", ImVec2(120, 0))) {
                show_details = false;
                ClearCurrentTextures();
            }
            ImGui::End();
        }

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ClearAllData();
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplSDL2_Shutdown(); ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context); SDL_DestroyWindow(window); SDL_Quit();
    return 0;
}