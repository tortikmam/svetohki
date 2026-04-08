Первым делом откройте бд в VsCode и сделайте следующий запрос:

```
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
