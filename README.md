```
чтобы компилировать main.cpp
g++ -std=c++17 main.cpp -o api_client -lcurl```
файл компилируется в api_client,
чтобы его запустить, пишите 

```
./client_api
```
Он выдает вам все данные из апи и записывает их в plants.json.
Я поставил там 10 страниц, на каждой странице по 20 записей растений(по желанию поменяете)

```
sudo apt install libpqxx-dev
```

```
g++ db_loader.cpp -o db_loader -lpqxx -lpq
```
