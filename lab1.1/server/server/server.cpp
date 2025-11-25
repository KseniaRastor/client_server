#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

#define PASSWORD "UUUUUUUUUUUUUUUUUUUU"

// функция преобразования строки пароля в гамму шифра
unsigned short gamma(char* pwd)
{
    char buf[20];
    int i;
    unsigned long flag;
    static unsigned long g;

    if (pwd)
    {
        memset(buf, 0x55, 20);
        memcpy(buf, pwd, strlen(pwd));
        for (i = 0, g = 0; i < 20; i++)
            g += (unsigned long)(buf[i] << (i % 23));
    }

    for (i = 5; i > 0; i--)
    {
        flag = g & 1;
        g = g >> 1;
        if (flag) g |= 0x80000000;
    }
    return g;
}

// шифрует открытый текст source по паролю pwd
int crypt(char* source, char* dest, char* pwd)
{
    int i, len, numblk;
    unsigned int* px, * py;
    unsigned short g;

    if (!source || !dest) return 0;

    px = (unsigned int*)source;
    py = (unsigned int*)dest;
    g = gamma(pwd);
    len = strlen(source);
    numblk = (len + 3) / 4;

    for (i = 0; i < numblk; i++, py++, px++)
        *py = *px ^ g;

    return numblk * 4;
}

// дешифрует текст source по паролю pwd
int decrypt(char* source, int source_len, char* dest, char* pwd)
{
    int i, numblk;
    unsigned int* px, * py;
    unsigned short g;

    if (!source || !dest) return 0;

    px = (unsigned int*)source;
    py = (unsigned int*)dest;
    g = gamma(pwd);
    numblk = source_len / 4;

    for (i = 0; i < numblk; i++, py++, px++)
        *py = *px ^ g;

    dest[numblk * 4] = '\0';
    return numblk * 4;
}

// вывод данных в hex формате
void print_hex(const char* label, const char* data, int length) {
    printf("%s (hex): ", label);
    for (int i = 0; i < length; i++) {
        printf("%02X ", (unsigned char)data[i]);
    }
    printf("\n");
}

typedef struct {
    SOCKET clientSocket;
    struct sockaddr_in clientAddr;
} CLIENT_INFO;

DWORD WINAPI ClientHandler(LPVOID lpParam)
{
    CLIENT_INFO* clientInfo = (CLIENT_INFO*)lpParam;
    SOCKET clientSocket = clientInfo->clientSocket;
    char clientIP[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &(clientInfo->clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
    printf("Client connected from: %s:%d\n", clientIP, ntohs(clientInfo->clientAddr.sin_port));

    char recvbuf[1024];
    char decrypted_buf[1024];
    int bytesReceived;

    while (1) {
        // Получаем данные от клиента
        bytesReceived = recv(clientSocket, recvbuf, sizeof(recvbuf) - 1, 0);
        if (bytesReceived <= 0) {
            printf("Client %s disconnected\n", clientIP);
            break;
        }

        recvbuf[bytesReceived] = '\0';

        printf("\n--- Received from %s ---\n", clientIP);
        //printf("Total bytes received: %d\n", bytesReceived);
        //print_hex("Raw data", recvbuf, bytesReceived);

        // Проверяем первый символ для определения типа сообщения
        int encrypted = 0;
        char* actual_message = recvbuf;
        int actual_length = bytesReceived;

        if (bytesReceived >= 1) {
            if (recvbuf[0] == '1') {
                encrypted = 1;
                actual_message = recvbuf + 1;
                actual_length = bytesReceived - 1;
            }
            else if (recvbuf[0] == '0') {
                encrypted = 0;
                actual_message = recvbuf + 1;
                actual_length = bytesReceived - 1;
            }
        }

        //printf("Message type: %s\n", encrypted ? "Encrypted" : "Plain text");
        //printf("Actual data length: %d bytes\n", actual_length);

        if (encrypted) {
            // Дешифруем сообщение
            int decrypted_len = decrypt(actual_message, actual_length, decrypted_buf, (char*)PASSWORD);
            print_hex("Encrypted data", actual_message, actual_length);
            printf("Decrypted message: %s\n", decrypted_buf);
            //printf("Decrypted length: %d bytes\n", decrypted_len);           
        }
        else {
            // Копируем как есть
            memcpy(decrypted_buf, actual_message, actual_length);
            decrypted_buf[actual_length] = '\0';
            printf("Message: %s\n", decrypted_buf);
        }
        printf("---------------------------\n\n");

        // Подготавливаем ответ
        char reply[1024] = { 0 };
        char temp_buf[1024] = { 0 };
        int reply_len = 0;

        if (encrypted) {
            // Шифруем ответ
            char response_text[1024];
            snprintf(response_text, sizeof(response_text), "Server received encrypted message: '%s'", decrypted_buf);

            int encrypted_len = crypt(response_text, temp_buf, (char*)PASSWORD);

            // Формируем ответ: префикс '1' + зашифрованные данные
            reply[0] = '1';
            memcpy(reply + 1, temp_buf, encrypted_len);
            reply_len = encrypted_len + 1;

            printf("Original response: %s\n", response_text);
            //printf("Encrypted response length: %d bytes\n", encrypted_len);
            print_hex("Encrypted response", temp_buf, encrypted_len);
        }
        else {
            // Незашифрованный ответ
            snprintf(reply, sizeof(reply), "0Server received plain text: '%s'", decrypted_buf);
            reply_len = strlen(reply);
            printf("Plain text response: %s\n", reply + 1);
        }

        // Отправляем ответ
        if (send(clientSocket, reply, reply_len, 0) == SOCKET_ERROR) {
            printf("Send failed for client %s\n", clientIP);
            break;
        }

        printf("--- Sent to %s ---\n", clientIP);
        printf("Response: %s\n", reply);
        //printf("Response length: %d bytes\n", reply_len);
        printf("-------------------------\n\n");
    }

    closesocket(clientSocket);
    free(clientInfo);
    return 0;
}

int main()
{
    WSADATA wsaData;
    int err;

    err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        printf("WSAStartup error: %d\n", err);
        return 1;
    }

    SOCKET ServerSock = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSock == INVALID_SOCKET) {
        printf("ServerSock error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    SOCKADDR_IN InternetAddr;
    ZeroMemory(&InternetAddr, sizeof(InternetAddr));
    InternetAddr.sin_family = AF_INET;
    InternetAddr.sin_addr.s_addr = INADDR_ANY;
    InternetAddr.sin_port = htons(5150);

    //start server
    if (bind(ServerSock, (SOCKADDR*)&InternetAddr, sizeof(InternetAddr)) != 0) {
        printf("bind failed\n");
        closesocket(ServerSock);
        WSACleanup();
        return 1;
    }

    if (listen(ServerSock, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(ServerSock);
        WSACleanup();
        return 1;
    }

    printf("Server started on port 5150...\n");

    while (1) {
        struct sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET ClientSocket = accept(ServerSock, (SOCKADDR*)&clientAddr, &clientAddrSize);

        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            continue;
        }

        CLIENT_INFO* clientInfo = (CLIENT_INFO*)malloc(sizeof(CLIENT_INFO));
        if (clientInfo == NULL) {
            printf("Memory allocation failed\n");
            closesocket(ClientSocket);
            continue;
        }

        clientInfo->clientSocket = ClientSocket;
        clientInfo->clientAddr = clientAddr;

        HANDLE hThread = CreateThread(NULL, 0, ClientHandler, clientInfo, 0, NULL);
        if (hThread == NULL) {
            printf("Failed to create thread: %d\n", GetLastError());
            free(clientInfo);
            closesocket(ClientSocket);
            continue;
        }
        CloseHandle(hThread);
    }

    closesocket(ServerSock);
    WSACleanup();
    return 0;
}