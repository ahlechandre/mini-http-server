/**
 * @author Alexandre Thebaldi <ahlechandre@gmail.com>
 * @author Thiago Brandão <thiagothgb@gmail.com>
 * @description "=========== Mini HTTP Server ===========
    - Responder requisições GET
    - Enviar STATUS 200, 404 e 505
    - Como parametro, o mini HTTP server deve receber um PATH para uma pasta que será o "/" do domínio local
    - Para cada GET, o arquivo alvo deve ser buscado e enviado como resposta
    - Caso a requisição seja para o "/", procurar pelo "index.html"
    - Nao utilizar webservers prontos, implementar utilizando sockets"
 */

// DEFINE SECTION

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define DEFAULT_PORT "3000"
#define DEFAULT_BUFLEN 20000

// INCLUDE SECTION

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

// INTERFACE SECTION

int initWinsock(WSADATA *wsaDataAddr);

int main()
{
    WSADATA wsaData;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    // Inicializa o winsock.
    if (!initWinsock(&(wsaData))) {
        printf("\ninit socket failed\n");
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, DEFAULT_PORT, &hints, &result) != 0) {
        printf("\ngetaddrinfo failed...\n");
        WSACleanup();
        return 1;
    }

    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (ListenSocket == INVALID_SOCKET) {
        printf("\nsocket() function error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    if (bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        printf("bind socket failed, error: %d", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen socket failed, error: %d", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    SOCKET ClientSocket;
    ClientSocket = INVALID_SOCKET;
    ClientSocket = accept(ListenSocket, NULL, NULL);

    if (ClientSocket == INVALID_SOCKET) {
        printf("accept socket client failed, error: %d", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    closesocket(ListenSocket);

    char recvbuffer[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    char HTTPResponse[] = "HTTP/1.1 200 OK\n";

    if (recv(ClientSocket, recvbuffer, recvbuflen, 0) == SOCKET_ERROR) {
        printf("recv failed, error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    printf("\n ========= HTTP Request ======== \n");
    printf("\n%s\n", recvbuffer);
    printf("\n ========= ENDOF HTTP Request ======== \n");

    printf("\nprocessing your request...\n");

    if (send(ClientSocket, HTTPResponse, strlen(HTTPResponse), 0) == SOCKET_ERROR) {
        printf("send failed, error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    printf("Your response was sent! Thanks for connecting, bye.");

    if (shutdown(ClientSocket, SD_SEND) == SOCKET_ERROR) {
        printf("shutdown failed, error: %d", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    closesocket(ClientSocket);
    WSACleanup();
    return 0;
}

/**
 * @brief initWinsock Tenta inicializar o Winsock.
 * @param wsaDataAddr
 * @return 1 para sucesso e 0 para falha.
 */
int initWinsock(WSADATA *wsaDataAddr) {
    return (WSAStartup(MAKEWORD(2, 2), wsaDataAddr) == 0);
}
