#include <windows.h>
#include <wlanapi.h>
#include <stdio.h>

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
#define WLAN_PROFILE_GET_PLAINTEXT_KEY 4

void salvarPerfil(FILE *f, LPCWSTR profileXml) {
    char xmlA[8192];
    WideCharToMultiByte(CP_UTF8, 0, profileXml, -1, xmlA, sizeof(xmlA), NULL, NULL);

    char nome[256] = {0};
    char senha[256] = {0};

    // Extrair nome
    char *start = strstr(xmlA, "<name>");
    if (start) {
        start += 6;
        char *end = strstr(start, "</name>");
        if (end) {
            strncpy(nome, start, end - start);
        }
    }

    // Extrair senha
    start = strstr(xmlA, "<keyMaterial>");
    if (start) {
        start += 13;
        char *end = strstr(start, "</keyMaterial>");
        if (end) {
            strncpy(senha, start, end - start);
        }
    }

    fprintf(f, "Rede: %s\nSenha: %s\n\n", nome, senha);
}

int main() {
    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    PWLAN_PROFILE_INFO_LIST pProfileList = NULL;

    FILE *arquivo = fopen("wifi_senhas.txt", "w");
    if (!arquivo) {
        printf("Erro ao criar arquivo\n");
        return 1;
    }

    if (WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient) != ERROR_SUCCESS) {
        printf("Erro ao abrir handle\n");
        return 1;
    }

    if (WlanEnumInterfaces(hClient, NULL, &pIfList) != ERROR_SUCCESS) {
        printf("Erro ao listar interfaces\n");
        return 1;
    }

    for (int i = 0; i < pIfList->dwNumberOfItems; i++) {
        PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[i];

        if (WlanGetProfileList(hClient, &pIfInfo->InterfaceGuid, NULL, &pProfileList) != ERROR_SUCCESS) {
            printf("Erro ao obter perfis\n");
            continue;
        }

        for (int j = 0; j < pProfileList->dwNumberOfItems; j++) {
            LPWSTR profileXml = NULL;
            DWORD flags = WLAN_PROFILE_GET_PLAINTEXT_KEY;
            DWORD access = 0;

            if (WlanGetProfile(
                hClient,
                &pIfInfo->InterfaceGuid,
                pProfileList->ProfileInfo[j].strProfileName,
                NULL,
                &profileXml,
                &flags,
                &access
            ) == ERROR_SUCCESS) {

                salvarPerfil(arquivo, profileXml);
                WlanFreeMemory(profileXml);
            }
        }

        WlanFreeMemory(pProfileList);
    }

    fclose(arquivo);
    WlanFreeMemory(pIfList);
    WlanCloseHandle(hClient, NULL);

    printf("Dados salvos em wifi_senhas.txt\n");
    return 0;
}