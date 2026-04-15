#!/bin/bash
set -e

echo "--- [1/7] Installing dependencies ---"
sudo apt install libglew-dev
sudo apt install libsdl2-dev
sudo apt install libglm-dev
sudo apt install libpqxx-dev
sudo apt install g++
sudo apt install cmake
sudo apt install postgresql-client
git submodule update --init --recursive || git submodule update --remote --recursive 

echo "--- [2/7] Starting Database (Docker) ---"
docker-compose up -d

echo "Waiting for PostgreSQL to be ready..."
sleep 5 

echo "--- [3/7] Setting Environment Variables ---"
export DATABASE_URL="dbname=flowers_db user=myuser password=mypassword host=localhost port=5433"
export PGPASSWORD='mypassword'
export PAGER=cat

echo "--- [4/7] Running DB Loader (C++) ---"
chmod +x ./db_loader
./db_loader

echo "--- [5/7] Running Post-Import SQL Updates ---"
psql -h localhost -p 5433 -U myuser -d flowers_db -c "
BEGIN;
    -- Добавляем колонку
    ALTER TABLE flowers_base ADD COLUMN IF NOT EXISTS nazvanie TEXT;

    -- Чистим лишнее
    DELETE FROM flowers_base WHERE id > 20;

    -- Массово обновляем названия
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

    -- Обновляем пути к картинкам
    UPDATE flowers_base SET image_url = '/pictures/' || id || '_';
COMMIT;"

unset PGPASSWORD
unset PAGER

echo "--- [6/7] Building Main Project ---"
cd build
rm -rf *
cmake ..
make

echo "--- [7/7] Launching main application ---"
if [ -f "./main" ]; then
    ./main
else
    echo "CRITICAL ERROR: ./main binary not found after build!"
    exit 1
fi
