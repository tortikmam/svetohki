
```
чтобы компилировать main.cpp
-> g++ -std=c++17 main.cpp -o api_client -I/opt/homebrew/Cellar/nlohmann-json/3.12.0/include -lcurl
```
файл компилируется в api_client,
чтобы его запустить, пишите 

```
./client_api
```
Он выдает вам все данные из апи и записывает их в plants.json.
Я поставил там 10 страниц, на каждой странице по 20 записей растений(по желанию поменяете)
=======
Сортировку и бинарный поиск выполняем по столбцу name_ru(который по факту просто название растение, не научное)
Внутри search.cpp лежат реализации функции сортировки и бинарного поиска. all.cpp - будущий файлик для сортировки, бинарного поиска и ДОПа.

в wsl: g++ -std=c++17 all.cpp sort_search.cpp -o lab_app -I/opt/homebrew/include -L/opt/homebrew/lib -lpqxx -lpq

Запустить: ./lab_app
`~
