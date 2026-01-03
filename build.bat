@echo off
echo ========================================
echo Building VR Driver
echo ========================================

:: Build Rust library
echo.
echo [1/3] Building Rust core...
cd rust_core
cargo build --release
if %errorlevel% neq 0 (
    echo ERROR: Rust build failed!
    exit /b %errorlevel%
)
cd ..

:: Build C++ driver
echo.
echo [2/3] Building C++ driver...
cd build
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo ERROR: C++ build failed!
    exit /b %errorlevel%
)
cd ..

:: Deploy to SteamVR folder
echo.
echo [3/3] Deploying to SteamVR folder...
copy /Y "build\bin\driver_custom_vr_driver.dll" "steamvr_driver\bin\win64\"
copy /Y "build\bin\openvr_api.dll" "steamvr_driver\bin\win64\"
copy /Y "build\bin\vr_driver.dll" "steamvr_driver\bin\win64\"

echo.
echo ========================================
echo Build complete! Driver deployed to:
echo %cd%\steamvr_driver\bin\win64\
echo ========================================
