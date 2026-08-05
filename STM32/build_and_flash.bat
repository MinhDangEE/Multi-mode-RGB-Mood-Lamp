@echo off
setlocal enabledelayedexpansion

echo ============================================
echo      BAT DAU QUA TRINH BUILD AND FLASH
echo ============================================

:: 1. Clean
echo.
echo [1/4] Thuc hien Clean...
if exist build (
    rmdir /s /q build
    echo - Da xoa thu muc build cu thanh cong.
) else (
    echo - Thu muc build chua ton tai, tiep tuc...
)

:: 2. Configure
echo.
echo [2/4] Thuc hien Configure bang CMake...
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
if %errorlevel% neq 0 (
    echo [LOI] CMake Configure that bai!
    exit /b %errorlevel%
)

:: 3. Build
echo.
echo [3/4] Thuc hien Build bang Ninja...
ninja -C build
if %errorlevel% neq 0 (
    echo [LOI] Ninja Build that bai!
    exit /b %errorlevel%
)

:: 4. Flash
echo.
echo [4/4] Thuc hien Flash va Reset mach (Mode Under Reset)...
if exist build\Debug\STM32.bin (
    STM32_Programmer_CLI -c port=SWD mode=UR freq=4000 -w build\Debug\STM32.bin 0x08000000 -v -rst
) else if exist build\STM32.bin (
    STM32_Programmer_CLI -c port=SWD mode=UR freq=4000 -w build\STM32.bin 0x08000000 -v -rst
) else (
    STM32_Programmer_CLI -c port=SWD mode=UR freq=4000 -w build\Debug\STM32.elf -v -rst
)


echo.
echo === HOAN THANH XUAT SAC! ===
pause