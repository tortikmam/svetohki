Запустите wsl, залетите в папку build и пишите:
```
git submodule update --init --recursive

sudo apt install libglew-dev
sudo apt install libsdl2-dev

rm -rf *
cmake ..
make -j16
./main
```
