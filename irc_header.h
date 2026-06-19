/* socket 통신 헤더 */
#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

/* 스레드 헤더 */
#include <windows.h>

/* 기타 헤더 */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <conio.h>

/* 함수 원형 */
void sendMsg(SOCKET sock);
DWORD WINAPI getMsg(LPVOID lpParam);
void tossMSG();
void removeClient(int index);
void broadcastMsg(const char* msg, SOCKET sender);
void trimNewline(char* str);
DWORD WINAPI clientThread(LPVOID lpParam);
void redrawInput();
void inputUserID();