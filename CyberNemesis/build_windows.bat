@echo off
setlocal enableextensions
cd /d "%~dp0"
title CYBER-NEMESIS - Crear el .exe para Windows
echo.
echo ===== CYBER-NEMESIS: crear el .exe para Windows =====
echo.

where cmake >nul 2>nul
if errorlevel 1 goto nocmake
where git >nul 2>nul
if errorlevel 1 goto nogit

set "VSPATH="
set "VSDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
if not exist "%VSDIR%\vswhere.exe" goto vsdone
pushd "%VSDIR%"
for /f "usebackq delims=" %%i in (`vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
popd
:vsdone
if defined VSPATH echo Visual Studio con herramientas de C++ detectado.
if not defined VSPATH echo No pude detectar Visual Studio con vswhere. Intento compilar igualmente.

echo.
echo [1/3] Configurando el proyecto...
cmake -S . -B build -A x64
if errorlevel 1 goto fail

echo.
echo [2/3] Compilando. La primera vez descarga Raylib y puede tardar unos minutos...
cmake --build build --config Release
if errorlevel 1 goto fail

echo.
echo [3/3] Creando el paquete...
cmake --build build --config Release --target package_game
if errorlevel 1 goto fail

if not exist "release\CyberNemesis.exe" goto fail

set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" goto noinno
echo.
echo Inno Setup encontrado: creando el instalador...
"%ISCC%" "installer\CyberNemesis.iss"
:noinno

echo.
echo ===== LISTO =====
echo Todo esta en la carpeta release:
echo   release\CyberNemesis.exe                 el juego, listo para abrir
echo   release\CyberNemesis-windows-x64.zip     para compartir
if exist "release\CyberNemesis-Setup.exe" echo   release\CyberNemesis-Setup.exe           instalador
echo.
start "" "release"
pause
exit /b 0

:nocmake
echo Falta CMake. Instalalo con este comando, cierra esta ventana y vuelve a ejecutar este archivo:
echo   winget install -e --id Kitware.CMake
goto end

:nogit
echo Falta Git. Instalalo con este comando, cierra esta ventana y vuelve a ejecutar este archivo:
echo   winget install -e --id Git.Git
goto end

:fail
echo.
echo Algo fallo. Copia el mensaje de error de arriba para poder ayudarte.
echo.
echo Pistas frecuentes:
echo   - "No CMAKE_CXX_COMPILER could be found": faltan las herramientas de C++.
echo     Abre "Visual Studio Installer", pulsa Modificar y marca "Desarrollo para el escritorio con C++".
echo   - "could not find any instance of Visual Studio": tu CMake es mas antiguo que tu Visual Studio.
echo     Actualizalo con: winget upgrade -e --id Kitware.CMake

:end
echo.
pause
exit /b 1
