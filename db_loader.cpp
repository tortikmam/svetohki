#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <optional>

using namespace std;
using json = nlohmann::json;

int main() {
    try {
        ifstream inFile("plants.json");
        if (!inFile.is_open()) {
            cerr << "Error: Could not open plants.json!" << endl;
            return 1;
        }

        json all_plants;
        inFile >> all_plants;
        inFile.close();
        const char* env_conn = getenv("DATABASE_URL");
        pqxx::connection C(env_conn ? env_conn : "dbname=flowers_db user=myuser password=mypassword host=localhost port=5432");
        pqxx::work W(C);

        cout << "Connected to database. Processing " << all_plants.size() << " records..." << endl;

        C.prepare("ins_base", 
            "INSERT INTO flowers_base (trefle_id, name_ru, name_latin, family, genus, image_url) "
            "VALUES ($1, $2, $3, $4, $5, $6) "
            "ON CONFLICT (trefle_id) DO UPDATE SET "
            "name_ru = EXCLUDED.name_ru, image_url = EXCLUDED.image_url "
            "RETURNING id");

        C.prepare("ins_tech", 
            "INSERT INTO flowers_technical (flower_id, temp_min_c, temp_max_c, ph_min, ph_max, light_level, humidity, toxicity, user_notes) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) "
            "ON CONFLICT (flower_id) DO UPDATE SET "
            "temp_min_c = EXCLUDED.temp_min_c, temp_max_c = EXCLUDED.temp_max_c");

        int imported = 0;
        for (const auto& p : all_plants) {
            // Вставка в flowers_base
            pqxx::result res = W.exec_prepared("ins_base",
                p.value("trefle_id", 0),
                p.value("name_ru", ""),
                p.value("name_latin", "Unknown"),
                p.value("family", ""),
                p.value("genus", ""),
                p.value("image_url", "")
            );

            int internal_id = res[0][0].as<int>();

            auto to_opt_double = [](const json& j) -> std::optional<double> {
                return j.is_null() ? std::nullopt : std::optional<double>(j.get<double>());
            };
            auto to_opt_int = [](const json& j) -> std::optional<int> {
                return j.is_null() ? std::nullopt : std::optional<int>(j.get<int>());
            };
            auto to_opt_str = [](const json& j) -> std::optional<string> {
                return j.is_null() ? std::nullopt : std::optional<string>(j.get<string>());
            };

            W.exec_prepared("ins_tech",
                internal_id,
                to_opt_double(p["temp_min_c"]),
                to_opt_double(p["temp_max_c"]),
                to_opt_double(p["ph_min"]),
                to_opt_double(p["ph_max"]),
                to_opt_int(p["light_level"]),
                to_opt_int(p["humidity"]),
                to_opt_str(p["toxicity"]),
                p.value("user_notes", "")
            );

            imported++;
            if (imported % 10 == 0) cout << "Imported " << imported << " plants..." << endl;
        }

        W.commit();
        cout << "\nSUCCESS! All data from plants.json moved to PostgreSQL." << endl;

    } catch (const exception &e) {
        cerr << "CRITICAL ERROR: " << e.what() << endl;
        return 1;
    }

    return 0;
}