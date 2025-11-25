//#define _WINSOCK_DEPRECATED_NO_WARNINGS
//#define _CRT_SECURE_NO_WARNINGS
//
//#include <winsock2.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//
//#pragma comment(lib, "ws2_32.lib")
//
//#define MAXLINE 512
//#define DEFAULT_PORT 6000
//#define DEFAULT_HOST "127.0.0.1"
//
////--------------------------------------------------
//void clear_input_buffer() {
//    int c;
//    while ((c = getchar()) != '\n' && c != EOF);
//}
//
////--------------------------------------------------
//int main()
//{
//    char server_host[64] = DEFAULT_HOST;
//    int server_port = DEFAULT_PORT;
//    char input[64];
//
//    // Интерактивная настройка подключения
//    printf("MySQL Client Configuration\n");
//    printf("==========================\n");
//
//    // Ввод хоста
//    printf("Server host: ", DEFAULT_HOST);
//    if (fgets(input, sizeof(input), stdin) != NULL) {
//        // Убираем символ новой строки
//        input[strcspn(input, "\n")] = 0;
//
//        // Если пользователь ввел что-то (не просто Enter)
//        if (strlen(input) > 0) {
//            strncpy_s(server_host, sizeof(server_host), input, _TRUNCATE);
//        }
//    }
//
//    // Ввод порта
//    printf("Server port: ", DEFAULT_PORT);
//    if (fgets(input, sizeof(input), stdin) != NULL) {
//        input[strcspn(input, "\n")] = 0;
//        if (strlen(input) > 0) {
//            server_port = atoi(input);
//            if (server_port <= 0 || server_port > 65535) {
//                printf("Invalid port! Using default %d\n", DEFAULT_PORT);
//                server_port = DEFAULT_PORT;
//            }
//        }
//    }
//
//    printf("\nConnecting to %s:%d...\n\n", server_host, server_port);
//
//    WSADATA wsaData;
//    SOCKET sockfd;
//    struct sockaddr_in serv_addr;
//    char buffer[MAXLINE];
//    int bytesReceived;
//
//    // Initialize Winsock
//    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        printf("WSAStartup failed.\n");
//        return 1;
//    }
//
//    // Create socket
//    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
//        printf("Cannot create socket\n");
//        WSACleanup();
//        return 1;
//    }
//
//    // Setup server address
//    serv_addr.sin_family = AF_INET;
//    serv_addr.sin_addr.s_addr = inet_addr(server_host);
//    serv_addr.sin_port = htons(server_port);
//
//    // Connect to server
//    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
//        printf("Cannot connect to server at %s:%d\n", server_host, server_port);
//        printf("Error code: %d\n", WSAGetLastError());
//        closesocket(sockfd);
//        WSACleanup();
//        return 1;
//    }
//
//    printf("Connected to server at %s:%d\n", server_host, server_port);
//    printf("Enter SQL queries (type 'quit' or 'exit' to quit):\n\n");
//
//    // Main interaction loop
//    while (1) {
//        printf("SQL> ");
//
//        // Read input
//        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
//            break;
//        }
//
//        // Remove newline
//        buffer[strcspn(buffer, "\n")] = 0;
//
//        // Check for exit commands
//        if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
//            break;
//        }
//
//        // Skip empty queries
//        if (strlen(buffer) == 0) {
//            continue;
//        }
//
//        // Send query to server
//        if (send(sockfd, buffer, strlen(buffer) + 1, 0) == SOCKET_ERROR) {
//            printf("Send error\n");
//            break;
//        }
//
//        // Receive response from server
//        bytesReceived = recv(sockfd, buffer, MAXLINE - 1, 0);
//        if (bytesReceived > 0) {
//            buffer[bytesReceived] = '\0';
//            printf("Server response:\n%s\n\n", buffer);
//        }
//        else if (bytesReceived == 0) {
//            printf("Server closed connection\n");
//            break;
//        }
//        else {
//            printf("Receive error\n");
//            break;
//        }
//    }
//
//    closesocket(sockfd);
//    WSACleanup();
//    printf("Client terminated.\n");
//    return 0;
//}


#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define MAXLINE 512
#define DEFAULT_PORT 6000
#define DEFAULT_HOST "127.0.0.1"

//--------------------------------------------------
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

//--------------------------------------------------
void print_menu() {
    printf("\n----- MySQL Database Client Menu -----\n");
    printf("1. Show all tables\n");
    printf("2. Show table structure\n");
    printf("3. Show table data\n");
    printf("4. Add new record\n");
    printf("5. Delete record\n");
    printf("6. Show database status\n");
    printf("7. Exit\n");
    printf("Choose an option (1-7): ");
}

//--------------------------------------------------
int send_query_and_receive(SOCKET sockfd, const char* query, char* buffer) {
    // Send query to server
    if (send(sockfd, query, strlen(query) + 1, 0) == SOCKET_ERROR) {
        printf("Send error\n");
        return -1;
    }

    // Receive response from server
    int bytesReceived = recv(sockfd, buffer, MAXLINE - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        return 0;
    }
    else if (bytesReceived == 0) {
        printf("Server closed connection\n");
        return -1;
    }
    else {
        printf("Receive error\n");
        return -1;
    }
}

//--------------------------------------------------
void show_all_tables(SOCKET sockfd) {
    char buffer[MAXLINE];
    const char* query = "SHOW TABLES";

    printf("\n--- Showing all tables ---\n");

    if (send_query_and_receive(sockfd, query, buffer) == 0) {
        printf("%s\n", buffer);
    }
}

//--------------------------------------------------
void show_table_structure(SOCKET sockfd) {
    char table_name[64];
    char query[128];
    char buffer[MAXLINE];

    printf("\nEnter table name: ");
    scanf("%63s", table_name);
    clear_input_buffer();

    sprintf_s(query, sizeof(query), "DESCRIBE %s", table_name);

    printf("\n--- Structure of table '%s' ---\n", table_name);

    if (send_query_and_receive(sockfd, query, buffer) == 0) {
        printf("%s\n", buffer);
    }
}

//--------------------------------------------------
void show_table_data(SOCKET sockfd) {
    char table_name[64];
    char query[128];
    char buffer[MAXLINE];

    printf("\nEnter table name: ");
    scanf("%63s", table_name);
    clear_input_buffer();

    sprintf_s(query, sizeof(query), "SELECT * FROM %s", table_name);

    printf("\n--- Data from table '%s' ---\n", table_name);

    if (send_query_and_receive(sockfd, query, buffer) == 0) {
        printf("%s\n", buffer);
    }
}

//--------------------------------------------------
void add_new_user(SOCKET sockfd) {
    char name[64];
    int age;
    char query[256];
    char buffer[MAXLINE];

    printf("\n--- Add new user ---\n");
    printf("Enter user name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    printf("Enter user age: ");
    scanf("%d", &age);
    clear_input_buffer();

    sprintf_s(query, sizeof(query),
        "INSERT INTO users (name, age) VALUES ('%s', %d)",
        name, age);

    if (send_query_and_receive(sockfd, query, buffer) == 0) {
        printf("Result: %s\n", buffer);
    }
}

//--------------------------------------------------
void delete_user(SOCKET sockfd) {
    int user_id;
    char query[128];
    char buffer[MAXLINE];

    printf("\n--- Delete user ---\n");
    printf("Enter user ID to delete: ");
    scanf("%d", &user_id);
    clear_input_buffer();

    sprintf_s(query, sizeof(query), "DELETE FROM users WHERE id = %d", user_id);

    if (send_query_and_receive(sockfd, query, buffer) == 0) {
        printf("Result: %s\n", buffer);
    }
}

//--------------------------------------------------
void show_database_status(SOCKET sockfd) {
    char buffer[MAXLINE];

    printf("\n--- Database Status ---\n");

    // Show database version
    if (send_query_and_receive(sockfd, "SELECT VERSION() as DatabaseVersion", buffer) == 0) {
        printf("Database Version:\n%s\n", buffer);
    }

    // Show current database
    if (send_query_and_receive(sockfd, "SELECT DATABASE() as CurrentDatabase", buffer) == 0) {
        printf("Current Database:\n%s\n", buffer);
    }
}

//--------------------------------------------------
int main() {
    char server_host[64] = DEFAULT_HOST;
    int server_port = DEFAULT_PORT;
    char input[64];

    // Интерактивная настройка подключения
    printf("MySQL Client Configuration\n");
    printf("-------------------------\n");

    // Ввод хоста
    printf("Server host: ", DEFAULT_HOST);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            strncpy_s(server_host, sizeof(server_host), input, _TRUNCATE);
        }
    }

    // Ввод порта
    printf("Server port: ", DEFAULT_PORT);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            server_port = atoi(input);
            if (server_port <= 0 || server_port > 65535) {
                printf("Invalid port! Using default %d\n", DEFAULT_PORT);
                server_port = DEFAULT_PORT;
            }
        }
    }

    printf("\nConnecting to %s:%d...\n", server_host, server_port);

    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in serv_addr;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Cannot create socket\n");
        WSACleanup();
        return 1;
    }

    // Setup server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(server_host);
    serv_addr.sin_port = htons(server_port);

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Cannot connect to server at %s:%d\n", server_host, server_port);
        printf("Error code: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    printf("Connected to server at %s:%d\n", server_host, server_port);

    // Create users table if not exists
    char init_query[] = "CREATE TABLE IF NOT EXISTS users ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "name VARCHAR(100) NOT NULL, "
        "email VARCHAR(100) UNIQUE NOT NULL, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)";
    char buffer[MAXLINE];
    send_query_and_receive(sockfd, init_query, buffer);

    // Main menu loop
    int choice;
    while (1) {
        print_menu();

        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            printf("Invalid input! Please enter a number.\n");
            continue;
        }
        clear_input_buffer();

        switch (choice) {
        case 1:
            show_all_tables(sockfd);
            break;
        case 2:
            show_table_structure(sockfd);
            break;
        case 3:
            show_table_data(sockfd);
            break;
        case 4:
            add_new_user(sockfd);
            break;
        case 5:
            delete_user(sockfd);
            break;
        case 6:
            show_database_status(sockfd);
            break;
        case 7:
            closesocket(sockfd);
            WSACleanup();
            return 0;
        default:
            printf("Invalid option! Please choose 1-7.\n");
            break;
        }
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}