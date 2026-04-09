Компиляция db_loader:
```
g++ db_loader.cpp -o db_loader -lpqxx -lpq
```

Первым делом откройте бд в VsCode и сделайте следующие запросы:

```
ALTER TABLE flowers_base ADD COLUMN nazvanie TEXT;
UPDATE flowers_base
SET nazvanie = CASE 
    WHEN id = 1  THEN 'Крапива двудомная'
    WHEN id = 2  THEN 'Ежовник'
    WHEN id = 3  THEN 'Подорожник ланцетный'
    WHEN id = 4  THEN 'Тысячелистник обыкновенный'
    WHEN id = 5  THEN 'Клевер ползучий'
    WHEN id = 6  THEN 'Бухарник шерстистый'
    WHEN id = 7  THEN 'Лютик ползучий'
    WHEN id = 8  THEN 'Дуб черешчатый'
    WHEN id = 9  THEN 'Овсяница красная'
    WHEN id = 10 THEN 'Ясень обыкновенный'
    WHEN id = 11 THEN 'Клевер луговой'
    WHEN id = 12 THEN 'Бук лесной'
    WHEN id = 13 THEN 'Ситник развесистый'
    WHEN id = 14 THEN 'Лютик едкий'
    WHEN id = 15 THEN 'Боярышник однопестичный'
    WHEN id = 16 THEN 'Щавель кислый'
    WHEN id = 17 THEN 'Лабазник вязолистный'
    WHEN id = 18 THEN 'Лещина обыкновенная'
    WHEN id = 19 THEN 'Тростник обыкновенный'
    WHEN id = 20 THEN 'Будра плющевидная'
END
WHERE id IN (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);


UPDATE flowers_base 
SET image_url = '/pictures/' || id || '_';
```

Далее запустите wsl в папке и проинициализируйте гит:

```
git init
git branch -m main 

git remote add origin git@github.com:tortikmam/svetohki.git

git pull

git checkout ImGui

git submodule update --init --recursive
или
git submodule update --remote --recursive
```

Установите основные библиотеки:

```
sudo apt install libglew-dev
sudo apt install libsdl2-dev
```

После этого залетите в папку build и пишите:
```
rm -rf *
cmake ..
make -j16
./main
```
