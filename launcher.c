#include <windows.h>

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInst; (void)hPrev; (void)lpCmdLine; (void)nCmdShow;
    
    STARTUPINFO si1, si2;
    PROCESS_INFORMATION pi1, pi2;
    
    // Inicializa as estruturas
    ZeroMemory(&si1, sizeof(si1));
    si1.cb = sizeof(si1);
    ZeroMemory(&pi1, sizeof(pi1));
    
    ZeroMemory(&si2, sizeof(si2));
    si2.cb = sizeof(si2);
    ZeroMemory(&pi2, sizeof(pi2));
    
    // Tenta abrir o fkb.exe
    if (!CreateProcess(NULL, "fkb.exe", NULL, NULL, FALSE, 0, NULL, NULL, &si1, &pi1)) {
        return 1;
    }
    
    // Tenta abrir o Chrome
    if (!CreateProcess(NULL, "chrome.exe", NULL, NULL, FALSE, 0, NULL, NULL, &si2, &pi2)) {
        CloseHandle(pi1.hProcess);
        CloseHandle(pi1.hThread);
        return 1;
    }
    
    // Fecha os handles dos processos
    CloseHandle(pi1.hProcess);
    CloseHandle(pi1.hThread);
    CloseHandle(pi2.hProcess);
    CloseHandle(pi2.hThread);
    
    return 0;
}
