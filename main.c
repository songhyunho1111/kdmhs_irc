#include "irc_header.h"

#define MAX_TEXT_LEN 1000
#define SOCKET_PORT 12345
#define SOCKET_IP "irc.shh.name"
#define MAX_CLIENT 10

// 구조체
typedef struct ClientInfo
{
    SOCKET sock;
    char id[1001];
    int active;
} ClientInfo;

// 전역 변수
HANDLE g_consoleMutex;
char g_inputBuf[MAX_TEXT_LEN] = {0};
int  g_inputLen = 0;
ClientInfo clients[MAX_CLIENT];
char userID[1001];

int main(int argc, char* argv[])
{
    // 한글 인코딩
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 서버 체크
    if (argc >= 2 && strcmp(argv[1], "--server") == 0)
    {
        tossMSG();
        return 0;
    }

    // user id 입력부
    inputUserID();

    // 클라이언트 소켓 생성
    WSADATA wsa;

    // Winsock 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("\033[31mWSAStartup 실패\033[0m\n");
        return 1;
    }

    // 서버 주소 찾기용 구조체
    struct addrinfo hints;
    struct addrinfo* result = NULL;

    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_INET;          // IPv4 사용
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_protocol = IPPROTO_TCP;

    // 포트 번호를 문자열로 변환
    char portStr[16];
    sprintf_s(portStr, sizeof(portStr), "%d", SOCKET_PORT);

    // 도메인 또는 IP 주소를 실제 주소로 변환
    if (getaddrinfo(SOCKET_IP, portStr, &hints, &result) != 0) {
        printf("\033[31m서버 주소 변환 실패\033[0m\n");
        WSACleanup();
        return 1;
    }

    // 소켓 생성
    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (sock == INVALID_SOCKET) {
        printf("\033[31m소켓 생성 실패\033[0m\n");

        freeaddrinfo(result);
        WSACleanup();

        return 1;
    }

    // 서버 접속
    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        printf("\033[31m서버 접속 실패\033[0m\n");

        freeaddrinfo(result);
        closesocket(sock);
        WSACleanup();

        return 1;
    }

    // 주소 정보 해제
    freeaddrinfo(result);

    // getMsg 스레드 생성
    HANDLE hThread;
    DWORD threadId;
    int id = 1;

    hThread = CreateThread(NULL, 0, getMsg, (LPVOID)(uintptr_t)sock, 0, &threadId);

    if (hThread == NULL)
    {
        printf("\033[31m스레드 생성 실패\033[31m\n");
        return 0;
    }

    sendMsg(sock);
}

/* client */

void inputUserID()
{
    printf("\033[36m████  ███ █   █ ███    ███ ████   ███\033[36m\n");
    printf("\033[36m█   █  █  ██ ██  █      █  █   █ █   \033[36m\n");
    printf("\033[36m█   █  █  █ █ █  █      █  ████  █   \033[36m\n");
    printf("\033[36m█   █  █  █   █  █      █  █  █  █   \033[36m\n");
    printf("\033[36m████  ███ █   █ ███    ███ █   █  ███\033[36m\n");
    printf("\n\n");

    
    printf("\033[33m사용할 ID 입력 (최대 20글자): \033[33m");
 
    fgets(userID, sizeof(userID), stdin);

    userID[strcspn(userID, "\n")]='\0';

}

void sendMsg(SOCKET sock)
{
    // userID 전달
    send(sock, userID, strlen(userID), 0);

    printf("> ");
    fflush(stdout);

    while (true)
    {
        int ch = _getch();

        WaitForSingleObject(g_consoleMutex, INFINITE);

        if (ch == '\r' || ch == '\n') // 전송
        {
            if (g_inputLen == 0)
            {
                ReleaseMutex(g_consoleMutex);
                continue;
            }

            g_inputBuf[g_inputLen] = '\n';
            g_inputBuf[g_inputLen + 1] = '\0';

            char tmp[g_inputLen+1];
            strcpy(tmp,g_inputBuf);

            send(sock, g_inputBuf, g_inputLen + 1, 0);

            g_inputLen = 0;
            g_inputBuf[0] = '\0';

            redrawInput();
            printf("\b\b\033[37m[%s] %s\033[0m",userID,tmp);

            printf("> ");
            fflush(stdout);
        }
        else if (ch == '\b') // 문자 삭제
        {
            if (g_inputLen > 0)
            {
                g_inputLen--;
                g_inputBuf[g_inputLen] = '\0';
                redrawInput();
            }
        }
        else if (ch >= 32 && g_inputLen < MAX_TEXT_LEN - 2)
        {
            g_inputBuf[g_inputLen++] = (char)ch;
            g_inputBuf[g_inputLen] = '\0';
            redrawInput();
        }

        ReleaseMutex(g_consoleMutex);
    }
}

DWORD WINAPI getMsg(LPVOID lpParam)
{
    SOCKET sock = (SOCKET)(uintptr_t)lpParam;
    char buffer[MAX_TEXT_LEN + 1];
    int recvLen;

    // 본인 입장 메시지 버림
    recvLen = recv(sock, buffer, MAX_TEXT_LEN, 0);

    while (true)
    {
        recvLen = recv(sock, buffer, MAX_TEXT_LEN, 0);

        // 연결 종료
        if (recvLen == 0)
        {
            printf("\033[31m서버와 연결이 종료되었습니다.\033[31m\n");
            break;
        }

        // 에러
        if (recvLen == SOCKET_ERROR)
        {
            printf("\033[31m수신 오류\033[31m\n");
            break;
        }

        buffer[recvLen] = '\0';

        WaitForSingleObject(g_consoleMutex, INFINITE); // mutex

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE); // cursor position
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        COORD pos = { 0, csbi.dwCursorPosition.Y };
        DWORD written;
        FillConsoleOutputCharacter(hOut, ' ', csbi.dwSize.X, pos, &written);
        SetConsoleCursorPosition(hOut, pos);

        printf("\033[33m%s\033[0m", buffer);
        redrawInput();

        ReleaseMutex(g_consoleMutex);
    }

    closesocket(sock);
    return 0;
}

void redrawInput()
{

    // 커서를 현재 줄 맨 앞으로
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);

    COORD pos = { 0, csbi.dwCursorPosition.Y };
    SetConsoleCursorPosition(hOut, pos);

    DWORD written;
    FillConsoleOutputCharacter(hOut, ' ', csbi.dwSize.X, pos, &written);
    SetConsoleCursorPosition(hOut, pos);

    printf("> %.*s", g_inputLen, g_inputBuf);
    fflush(stdout);
}

/* server */

void tossMSG()
{
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("\033[31mWSAStartup 실패\033[31m\n");
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
        printf("\033[31m서버 소켓 생성 실패\033[31m\n");
        WSACleanup();
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SOCKET_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        printf("\033[31mbind 실패\033[31m\n");
        closesocket(server);
        WSACleanup();
        return;
    }

    if (listen(server, 5) == SOCKET_ERROR)
    {
        printf("\033[31mlisten 실패\033[31m\n");
        closesocket(server);
        WSACleanup();
        return;
    }

    printf("\033[33mIRC 서버 실행 중. 포트: \033[33m%d\n", SOCKET_PORT);

    while (true)
    {
        struct sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);

        SOCKET client = accept(server, (struct sockaddr*)&clientAddr, &clientLen);

        if (client == INVALID_SOCKET)
        {
            printf("\033[31maccept 실패\033[31m\n");
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
            printf("\033[31m클라이언트 스레드 생성 실패\033[31m\n");
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

    while (true)
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
        if (str[len - 1] == '\n' || str[len - 1] == '\r')
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

