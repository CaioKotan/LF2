/*
 * killer.c
 *
 * Programa para encerrar o processo keylogger.exe
 * Compilação: gcc killer.c -o killer.exe
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

int main()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        printf("Erro ao criar snapshot de processos.\n");
        return 1;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, "fkb.exe") == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    if (TerminateProcess(hProcess, 0)) {
                        printf("Processo fkb.exe (PID: %lu) encerrado com sucesso.\n", pe.th32ProcessID);
                    } else {
                        printf("Erro ao encerrar o processo.\n");
                    }
                    CloseHandle(hProcess);
                } else {
                    printf("Erro ao abrir o processo.\n");
                }
                break; // Assume apenas um processo
            }
        } while (Process32Next(hSnapshot, &pe));
    } else {
        printf("Erro ao enumerar processos.\n");
    }

    CloseHandle(hSnapshot);
    return 0;
}