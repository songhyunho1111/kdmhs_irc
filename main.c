#include "irc_header.h"

#define MAX_TEXT_LEN 1000

void inputUserID();
void sendMsg(SOCKET sock);
DWORD WINAPI getMsg(LPVOID lpParam);
void tossMSG();

char userID[1001];

int main(int argc, char* argv[])
{
    // ID 입력
    inputUserID();

    if (strcmp(userID, "server")==0) tossMSG();

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
    serverAddr.sin_port = htons(12345);

    // 서버 IP
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // 서버 접속
    if (connect(
            sock,
            (struct sockaddr*)&serverAddr,
            sizeof(serverAddr)
        ) == SOCKET_ERROR) {

        printf("서버 접속 실패\n");

        closesocket(sock);
        WSACleanup();

        return 1;
        }

    // getMsg 스레드 생성
    hThread = CreateThread(NULL,0,getMsg,&id,0,&threadId);

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

    fgets(text, MAX_TEXT_LEN, stdin);

    text[strcspn(text, "\r\n")] = '\0';

    send(sock, text, strlen(text), 0);
}

DWORD WINAPI getMsg(LPVOID lpParam)
{

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