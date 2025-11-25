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


int decrypt(char* source, int source_len, char* dest, const char* pwd)
{
    int i, numblk;
    unsigned int* px, * py;
    unsigned short g;

    if (!source || !dest) return 0;

    px = (unsigned int*)source;
    py = (unsigned int*)dest;
    g = gamma((char*)pwd);

    // Обрабатываем полные блоки по 4 байта
    numblk = source_len / 4;
    for (i = 0; i < numblk; i++, py++, px++)
        *py = *px ^ g;

    // Обрабатываем оставшиеся байты (если есть)
    int remaining = source_len % 4;
    if (remaining > 0) {
        unsigned char* last_block = (unsigned char*)px;
        unsigned char* dest_ptr = (unsigned char*)py;
        unsigned int partial_gamma = g; // Используем ту же гамму

        for (i = 0; i < remaining; i++) {
            dest_ptr[i] = last_block[i] ^ ((partial_gamma >> (i * 8)) & 0xFF);
        }
    }

    dest[source_len] = '\0';
    return source_len;
}

void print_hex(const char* label, const char* data, int length) {
    printf("%s (hex): ", label);
    for (int i = 0; i < length; i++) {
        printf("%02X ", (unsigned char)data[i]);
    }
    printf("\n");
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

    SOCKET ClientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (ClientSock == INVALID_SOCKET) {
        printf("ClientSock error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    char ip_server[256];
    printf("Enter server IP: ");
    fgets(ip_server, sizeof(ip_server), stdin);
    ip_server[strcspn(ip_server, "\n")] = '\0';

    SOCKADDR_IN InternetAddr;
    ZeroMemory(&InternetAddr, sizeof(InternetAddr));
    InternetAddr.sin_family = AF_INET;
    InternetAddr.sin_addr.s_addr = inet_addr(ip_server);
    InternetAddr.sin_port = htons(5150);

    if (connect(ClientSock, (SOCKADDR*)&InternetAddr, sizeof(InternetAddr)) != 0) {
        printf("connect failed\n");
        closesocket(ClientSock);
        WSACleanup();
        return 1;
    }

    printf("Connected to server! Type 'exit' to quit\n");

    char message[1024];
    char encrypted_buf[1024];
    char final_message[1024];

    while (1) {
        printf("\nEnter message (prefix with 0 - without cript, 1 - with cript): ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';

        if (strcmp(message, "exit") == 0) {
            closesocket(ClientSock);
            WSACleanup();
            break;
        }

        int encrypted = 0;
        char* actual_message = message;
        int final_length = 0;

        // Проверяем префикс
        if (message[0] == '1' && strlen(message) > 1) {
            encrypted = 1;
            actual_message = message + 1;

            // Шифруем сообщение
            int encrypted_len = crypt(actual_message, encrypted_buf, (char*)PASSWORD);

            // Формируем финальное сообщение с префиксом 1
            final_message[0] = '1';
            memcpy(final_message + 1, encrypted_buf, encrypted_len);
            final_length = encrypted_len + 1;

            printf("Original message: %s\n", actual_message);
            //printf("Message length: %d bytes\n", (int)strlen(actual_message));
            //printf("Encrypted length: %d bytes\n", encrypted_len);
            print_hex("Encrypted data", encrypted_buf, encrypted_len);
        }
        else if (message[0] == '0' && strlen(message) > 1) {
            encrypted = 0;
            actual_message = message + 1;

            // Просто копируем сообщение с префиксом 0
            final_message[0] = '0';
            strcpy(final_message + 1, actual_message);
            final_length = strlen(actual_message) + 1;

            printf("Original message: %s\n", actual_message);
        }
        else {
            // Если нет префикса, считаем незашифрованным
            encrypted = 0;
            actual_message = message;

            final_message[0] = '0';
            strcpy(final_message + 1, message);
            final_length = strlen(message) + 1;

            printf("Original message: %s\n", message);
        }

        // Отправляем сообщение
        int sent_bytes = send(ClientSock, final_message, final_length, 0);
        if (sent_bytes != final_length) {
            printf("Send failed: sent %d of %d bytes\n", sent_bytes, final_length);
            break;
        }

        //printf("Sent %d bytes to server\n", sent_bytes);

        // Получаем ответ от сервера
        char reply[1024];
        int bytes_received = recv(ClientSock, reply, sizeof(reply) - 1, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected\n");
            break;
        }

        reply[bytes_received] = '\0';

        //printf("\nReceived %d bytes from server\n", bytes_received);

        printf("\n--- Received from server ---\n");
        printf("Response: %s\n", reply);

        /// Обрабатываем ответ сервера
        if (bytes_received >= 1) {
           
            if (reply[0] == '1') {
                printf("Server response (hex): ");
                for (int i = 0; i < bytes_received; i++) {
                    printf("%02X ", (unsigned char)reply[i]);
                }
                printf("\n");

                char decrypted_reply[1024];
                int decrypt_result = decrypt(reply + 1, bytes_received - 1, decrypted_reply, PASSWORD);

                if (decrypt_result > 0) {
                    printf("Server response (decrypted): %s\n", decrypted_reply);
                }
                else {
                    printf("Decryption failed\n");
                    // Выводим как есть
                    memcpy(decrypted_reply, reply + 1, bytes_received - 1);
                    decrypted_reply[bytes_received - 1] = '\0';
                    printf("Encrypted response: %s\n", decrypted_reply);
                }
            }
            else if (reply[0] == '0') {
                printf("Server response: %s\n", reply + 1);
            }
            else {
                printf("Server response: %s\n", reply);
            }
        }
    }

    closesocket(ClientSock);
    WSACleanup();
    return 0;
}
