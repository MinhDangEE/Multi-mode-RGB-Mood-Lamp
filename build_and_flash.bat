@echo off
echo Running build and flash script inside STM32 directory...
cd /d "%~dp0STM32"
call build_and_flash.bat %*
