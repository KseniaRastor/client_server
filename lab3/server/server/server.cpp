#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string>
#include <algorithm>

// MySQL C API - более стабильный
#include <mysql.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libmysql.lib")

#define MAXLINE 4096
#define DEFAULT_PORT 6000
#define BUFFER_SIZE 4096

// Configuration
int SERV_TCP_PORT = DEFAULT_PORT;

// MySQL configuration
const char* DB_HOST = "127.0.0.1";
const char* DB_USER = "root";
const char* DB_PASS = "Root123!";
const char* DB_NAME = "test_db";
const int DB_PORT = 3306;

SOCKET sockfd;

// Function declarations
int readline(SOCKET fd, char* ptr, int maxlen);
int writen(SOCKET fd, const char* ptr, int nbytes);
void WriteToLog(const char* message);
MYSQL* create_mysql_session();
const char* execute_sql_query(MYSQL* mysql, const char* query);
DWORD WINAPI ClientHandler(LPVOID lpParam);

//--------------------------------------------------
void WriteToLog(const char* message) {
    printf("[LOG] %s\n", message);

    FILE* logfile = fopen("serv.log", "a");
    if (logfile) {
        time_t now = time(NULL);
        char time_str[64];
        ctime_s(time_str, sizeof(time_str), &now);
        time_str[strlen(time_str) - 1] = '\0';
        fprintf(logfile, "[%s] %s\n", time_str, message);
        fclose(logfile);
    }
}

//--------------------------------------------------
int readline(SOCKET fd, char* ptr, int maxlen) {
    static char buffer[BUFFER_SIZE];
    static int buf_pos = 0;
    static int buf_len = 0;
    int n = 0;

    while (n < maxlen - 1) {
        // Если буфер пуст - читаем новый блок
        if (buf_pos >= buf_len) {
            buf_len = recv(fd, buffer, BUFFER_SIZE, 0);
            if (buf_len <= 0) return buf_len;  // Ошибка
            buf_pos = 0;
        }

        // Копируем из буфера, пока не \0
        while (buf_pos < buf_len && n < maxlen - 1) {
            char c = buffer[buf_pos++];
            ptr[n++] = c;
            if (c == '\0') return n;  // Конец строки
        }
    }

    ptr[n] = '\0';
    return n;
}

//--------------------------------------------------
int writen(SOCKET fd, const char* ptr, int nbytes) {
    if (!ptr || nbytes <= 0) return -1;

    int nleft = nbytes;
    int nwritten;

    while (nleft > 0) {
        nwritten = send(fd, ptr, nleft, 0);
        if (nwritten <= 0) {
            return nwritten;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return nbytes - nleft;      // 0 при успешной записи всех байт
}

//--------------------------------------------------
MYSQL* create_mysql_session() {
    WriteToLog("Creating MySQL session with C API...");

    MYSQL* mysql = mysql_init(NULL);
    if (!mysql) {
        WriteToLog("MySQL initialization failed");
        return nullptr;
    }

    // Настраиваем кодировку
    mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    WriteToLog("Attempting MySQL connection...");

    MYSQL* conn = mysql_real_connect(mysql, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, NULL, 0);

    if (!conn) {
        char error_msg[512];
        sprintf_s(error_msg, sizeof(error_msg), "MySQL Connection Error: %s", mysql_error(mysql));
        WriteToLog(error_msg);
        mysql_close(mysql);
        return nullptr;
    }

    WriteToLog("MySQL connection established successfully");
    return conn;
}

//--------------------------------------------------
const char* execute_sql_query(MYSQL* mysql, const char* query) {
    if (!query || strlen(query) == 0) {
        return "Error: Empty query";
    }

    if (!mysql) {
        return "Error: No database connection";
    }

    static char response[MAXLINE];
    response[0] = '\0';

    // Выполняем запрос
    if (mysql_query(mysql, query)) {
        sprintf_s(response, sizeof(response), "MySQL Error: %s", mysql_error(mysql));
        return response;
    }

    // Получаем результат
    MYSQL_RES* result = mysql_store_result(mysql);

    if (result) {
        // SELECT запрос - есть результат
        std::string result_str;
        MYSQL_ROW row;
        MYSQL_FIELD* fields;
        int num_fields = mysql_num_fields(result);

        // Заголовки столбцов
        fields = mysql_fetch_fields(result);
        for (int i = 0; i < num_fields; i++) {
            if (i > 0) result_str += " | ";
            result_str += fields[i].name;
        }
        result_str += "\n";

        // Разделитель
        for (int i = 0; i < num_fields; i++) {
            if (i > 0) result_str += "-+-";
            result_str += "---";
        }
        result_str += "\n";

        // Данные
        int row_count = 0;
        while ((row = mysql_fetch_row(result))) {
            for (int i = 0; i < num_fields; i++) {
                if (i > 0) result_str += " | ";
                result_str += row[i] ? row[i] : "NULL";
            }
            result_str += "\n";
            row_count++;
        }

        result_str += "\nTotal: " + std::to_string(row_count) + " rows";
        strncpy_s(response, sizeof(response), result_str.c_str(), _TRUNCATE);

        mysql_free_result(result);
    }
    else {
        // Не-SELECT запрос (INSERT, UPDATE, DELETE)
        if (mysql_field_count(mysql) == 0) {
            my_ulonglong affected_rows = mysql_affected_rows(mysql);
            sprintf_s(response, sizeof(response), "Query executed successfully. Affected rows: %lld", affected_rows);
        }
        else {
            sprintf_s(response, sizeof(response), "Query executed successfully");
        }
    }

    return response;
}

//--------------------------------------------------
DWORD WINAPI ClientHandler(LPVOID lpParam) {
    WriteToLog("ClientHandler thread started");

    SOCKET clientSocket = (SOCKET)lpParam;
    char line[MAXLINE];
    int n;

    MYSQL* mysql = create_mysql_session();

    if (mysql) {
        WriteToLog("MySQL session created for client");
    }
    else {
        WriteToLog("Failed to create MySQL session");
        const char* error_msg = "Error: Cannot connect to database";
        writen(clientSocket, error_msg, (int)strlen(error_msg) + 1);
        closesocket(clientSocket);
        return 1;
    }

    while (1) {
        WriteToLog("Waiting for client data...");

        n = readline(clientSocket, line, MAXLINE - 1);
        if (n <= 0) {
            WriteToLog(n == 0 ? "Client disconnected" : "Read error");
            break;
        }

        WriteToLog("Received query");

        // Log query
        FILE* queryLog = fopen("queries.log", "a");
        if (queryLog) {
            fprintf(queryLog, "%s\n", line);
            fclose(queryLog);
        }

        const char* result = execute_sql_query(mysql, line);

        WriteToLog("Sending response...");
        if (writen(clientSocket, result, (int)strlen(result) + 1) != strlen(result) + 1) {
            WriteToLog("Write error to client");
            break;
        }

        WriteToLog("Response sent successfully");
    }

    // Cleanup
    if (mysql) {
        mysql_close(mysql);
        WriteToLog("MySQL connection closed");
    }

    WriteToLog("ClientHandler thread finished");
    closesocket(clientSocket);
    return 0;
}

//--------------------------------------------------
int main(int argc, char* argv[]) {
    WriteToLog("----- Server starting (MySQL C API) -----");

    // Инициализируем MySQL client library
    if (mysql_library_init(0, NULL, NULL)) {
        WriteToLog("MySQL library initialization failed");
        return 1;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        WriteToLog("WSAStartup failed");
        mysql_library_end();
        return 1;
    }
    WriteToLog("WSAStartup OK");

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        WriteToLog("Cannot create socket");
        WSACleanup();
        mysql_library_end();
        return 1;
    }
    WriteToLog("Socket created");

    // Setup server address
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(SERV_TCP_PORT);

    // Bind
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        WriteToLog("Cannot bind address");
        closesocket(sockfd);
        WSACleanup();
        mysql_library_end();
        return 1;
    }
    WriteToLog("Bind OK");

    // Listen
    if (listen(sockfd, 5) == SOCKET_ERROR) {
        WriteToLog("Listen failed");
        closesocket(sockfd);
        WSACleanup();
        mysql_library_end();
        return 1;
    }
    WriteToLog("Listen OK");

    printf("Server listening on port %d...\n", SERV_TCP_PORT);
    WriteToLog("Server ready for connections");

    // Main accept loop
    while (1) {
        WriteToLog("Waiting for connections...");

        struct sockaddr_in cli_addr;
        int clilen = sizeof(cli_addr);

        SOCKET newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);

        if (newsockfd == INVALID_SOCKET) {
            WriteToLog("Accept error");
            Sleep(1000);
            continue;
        }

        WriteToLog("New connection accepted");

        // Create thread for client
        HANDLE thread = CreateThread(NULL, 0, ClientHandler, (LPVOID)newsockfd, 0, NULL);
        if (thread == NULL) {
            WriteToLog("Failed to create client thread");
            closesocket(newsockfd);
        }
        else {
            CloseHandle(thread);
            WriteToLog("Client thread created successfully");
        }
    }

    closesocket(sockfd);
    WSACleanup();
    mysql_library_end();
    WriteToLog("Server shutdown complete");
    return 0;
}