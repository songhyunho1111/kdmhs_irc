#include "irc_header.h"

#define MAX_TEXT_LEN 1000
#define SOCKET_PORT 12345
#define SOCKET_IP "127.0.0.1"

void inputUserID();
void sendMsg(SOCKET sock);
DWORD WINAPI getMsg(LPVOID lpParam);
void tossMSG();
void removeClient(int index);
void broadcastMsg(const char* msg, SOCKET sender);
void trimNewline(char* str);
DWORD WINAPI clientThread(LPVOID lpParam);


#define MAX_CLIENT 10

typedef struct ClientInfo {
    SOCKET sock;
    char id[1001];
    int active;
} ClientInfo;

ClientInfo clients[MAX_CLIENT];
char userID[1001];

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    inputUserID();
    
    if (strcmp(userID, "server")==0)
    {
        tossMSG();
        return 0;
    }

    HANDLE hThread;
    DWORD threadId;
    int id = 1;

    WSADATA wsa;

    // Winsock 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup 실패\n");
        return 1;
    }

    // 클라이언트 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == INVALID_SOCKET) {
        printf("소켓 생성 실패\n");
        WSACleanup();
        return 1;
    }

    // 서버 주소 설정
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SOCKET_PORT);

    // 서버 IP
    inet_pton(AF_INET, SOCKET_IP, &serverAddr.sin_addr);

    // 서버 접속
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {

        printf("서버 접속 실패\n");

        closesocket(sock);
        WSACleanup();

        return 1;
        }

    // getMsg 스레드 생성
    hThread = CreateThread(NULL, 0, getMsg, (LPVOID)(uintptr_t)sock, 0, &threadId);

    if (hThread == NULL)
    {
        printf("스레드 생성 실패\n");
        return 0;
    }

    sendMsg(sock);
}

void inputUserID()
{
    fgets(userID, 100, stdin);
    userID[strcspn(userID, "\n")] = '\0';
}

void sendMsg(SOCKET sock)
{

    char text[MAX_TEXT_LEN];

    while (1)
    {
        if (fgets(text, MAX_TEXT_LEN, stdin) == NULL) break;

        send(sock, text, strlen(text), 0);
    }
}

DWORD WINAPI getMsg(LPVOID lpParam)
{
    SOCKET sock = (SOCKET)(uintptr_t)lpParam;

    char buffer[MAX_TEXT_LEN + 1];
    int recvLen;

    while (1)
    {
        recvLen = recv(sock, buffer, MAX_TEXT_LEN, 0);

        // 연결 종료
        if (recvLen == 0)
        {
            printf("서버와 연결이 종료되었습니다.\n");
            break;
        }

        // 에러
        if (recvLen == SOCKET_ERROR)
        {
            printf("수신 오류\n");
            break;
        }

        buffer[recvLen] = '\0';

        printf(buffer);
    }

    closesocket(sock);

    return 0;
}




void tossMSG()
{
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup 실패\n");
        return;
    }


    for (int i = 0; i < MAX_CLIENT; i++)
    {
        clients[i].sock = INVALID_SOCKET;
        clients[i].active = 0;
        clients[i].id[0] = '\0';
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server == INVALID_SOCKET)
    {
        printf("서버 소켓 생성 실패\n");
        WSACleanup();
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SOCKET_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        printf("bind 실패\n");
        closesocket(server);
        WSACleanup();
        return;
    }

    if (listen(server, 5) == SOCKET_ERROR)
    {
        printf("listen 실패\n");
        closesocket(server);
        WSACleanup();
        return;
    }

    printf("IRC 서버 실행 중. 포트: %d\n", SOCKET_PORT);

    while (1)
    {
        struct sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);

        SOCKET client = accept(server, (struct sockaddr*)&clientAddr, &clientLen);

        if (client == INVALID_SOCKET)
        {
            printf("accept 실패\n");
            continue;
        }

        int index = -1;

        for (int i = 0; i < MAX_CLIENT; i++)
        {
            if (clients[i].active == 0)
            {
                index = i;
                clients[i].sock = client;
                clients[i].active = 1;
                clients[i].id[0] = '\0';
                break;
            }
        }

        if (index == -1)
        {
            char* fullMsg = "서버 인원이 가득 찼습니다.\n";
            send(client, fullMsg, strlen(fullMsg), 0);
            closesocket(client);
            continue;
        }

        HANDLE hThread = CreateThread(
            NULL,
            0,
            clientThread,
            (LPVOID)(INT_PTR)index,
            0,
            NULL
        );

        if (hThread == NULL)
        {
            printf("클라이언트 스레드 생성 실패\n");
            removeClient(index);
            continue;
        }

        CloseHandle(hThread);
    }

    closesocket(server);
    WSACleanup();
}


DWORD WINAPI clientThread(LPVOID lpParam)
{
    int index = (int)(INT_PTR)lpParam;

    SOCKET clientSock = clients[index].sock;

    char buffer[MAX_TEXT_LEN + 1];
    char msg[MAX_TEXT_LEN + 1200];

    int recvLen;

    recvLen = recv(clientSock, buffer, MAX_TEXT_LEN, 0);

    if (recvLen <= 0)
    {
        removeClient(index);
        return 0;
    }

    buffer[recvLen] = '\0';
    trimNewline(buffer);

    strncpy(clients[index].id, buffer, sizeof(clients[index].id) - 1);
    clients[index].id[sizeof(clients[index].id) - 1] = '\0';

    snprintf(msg, sizeof(msg), "[server] %s 님이 입장했습니다.\n", clients[index].id);
    printf("%s", msg);
    broadcastMsg(msg, INVALID_SOCKET);

    while (1)
    {
        recvLen = recv(clientSock, buffer, MAX_TEXT_LEN, 0);

        if (recvLen == 0)
        {
            break;
        }

        if (recvLen == SOCKET_ERROR)
        {
            break;
        }

        buffer[recvLen] = '\0';

        snprintf(
            msg,
            sizeof(msg),
            "[%s] %s",
            clients[index].id,
            buffer
        );

        printf("%s", msg);

        broadcastMsg(msg, clientSock);
    }

    snprintf(msg, sizeof(msg), "[server] %s 님이 퇴장했습니다.\n", clients[index].id);
    printf("%s", msg);
    broadcastMsg(msg, clientSock);

    removeClient(index);

    return 0;
}

void trimNewline(char* str)
{
    int len = strlen(str);

    while (len > 0)
    {
        if (str[len - 1] == '\n' || str[len - 1] == '\r') // windows has \r 
        {
            str[len - 1] = '\0';
            len--;
        }
        else
        {
            break;
        }
    }
}

void broadcastMsg(const char* msg, SOCKET sender)
{

    for (int i = 0; i < MAX_CLIENT; i++)
    {
        if (clients[i].active && clients[i].sock != INVALID_SOCKET)
        {
            if (clients[i].sock != sender)
            {
                send(clients[i].sock, msg, strlen(msg), 0);
            }
        }
    }

}

void removeClient(int index)
{

    if (clients[index].active)
    {
        closesocket(clients[index].sock);
        clients[index].sock = INVALID_SOCKET;
        clients[index].active = 0;
        clients[index].id[0] = '\0';
    }

}

