# WinProcAdmin

C++20 Win32 process administration tool with a Raylib + Raygui GUI.

Dependencies (Raylib 5.5, Raygui 5.0, GoogleTest v1.17.0) are downloaded and
built automatically by CMake using the active MinGW toolchain.

## Build

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

Each build also writes `compile_commands.json` to the project root with literal
include paths, so editors can find `raylib.h` and `raygui.h`. It is gitignored
because it contains machine-specific paths.

## Run

```powershell
.\build\WinProcAdmin.exe
```

## Test

```powershell
ctest --test-dir build --output-on-failure
```
