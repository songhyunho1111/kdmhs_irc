#include "irc_header.h"

#define MAX_TEXT_LEN 1000
#define SOCKET_PORT 12345
#define SOCKET_IP "127.0.0.1"

void inputUserID();
void sendMsg(SOCKET sock);
DWORD WINAPI getMsg(LPVOID lpParam);
void tossMSG();

char userID[1001];

int main(int argc, char* argv[])
{
    // ID 입력
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
    hThread = CreateThread(NULL, 0, getMsg, (LPVOID)&sock, 0,&threadId);

    if (hThread == NULL)
    {
        printf("스레드 생성 실패\n");
        return 0;
    }
}

void inputUserID()
{

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
    SOCKET sock = *((SOCKET*)lpParam);

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

        puts(buffer);
    }

    closesocket(sock);

    return 0;
}

void tossMSG()
{
    // 소켓 생성
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr*)&addr, sizeof(addr));

    listen(server, 5);

    // 접속 대기
    struct sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);

    SOCKET client = accept( server,(struct sockaddr*)&clientAddr,&clientLen);
}