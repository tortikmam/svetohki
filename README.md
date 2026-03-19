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
