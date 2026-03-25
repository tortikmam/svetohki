Сортировку и бинарный поиск выполняем по столбцу name_ru(который по факту просто название растение, не научное)
Внутри search.cpp лежат реализации функции сортировки и бинарного поиска. all.cpp - будущий файлик для сортировки, бинарного поиска и ДОПа.

в wsl: g++ -std=c++17 all.cpp sort_search.cpp -o lab_app -I/opt/homebrew/include -L/opt/homebrew/lib -lpqxx -lpq

Запустить: ./lab_app
