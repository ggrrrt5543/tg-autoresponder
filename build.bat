@echo off
set PATH=C:\msys64\mingw64\bin;%PATH%

echo [1/4] Compiling main.cpp...
g++ -std=c++17 -O2 -c -o build\main.o src\main.cpp -Isrc
if errorlevel 1 goto :error

echo [2/4] Compiling config.cpp...
g++ -std=c++17 -O2 -c -o build\config.o src\config.cpp -Isrc
if errorlevel 1 goto :error

echo [3/4] Compiling handler.cpp...
g++ -std=c++17 -O2 -c -o build\handler.o src\handler.cpp -Isrc
if errorlevel 1 goto :error

echo [4/4] Compiling utils.cpp...
g++ -std=c++17 -O2 -c -o build\utils.o src\utils.cpp -Isrc
if errorlevel 1 goto :error

echo Linking (static)...
g++ -static -o tg-autoresponder.exe build\main.o build\config.o build\handler.o build\utils.o
if errorlevel 1 goto :error

echo.
echo === BUILD SUCCESS ===
echo Run: tg-autoresponder.exe (make sure tdjson.dll is in the same folder)
goto :end

:error
echo.
echo === BUILD FAILED ===

:end
pause
