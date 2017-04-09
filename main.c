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
#define DEFAULT_SERVERROOT "C:\\webserver"

// INCLUDE SECTION

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

// INTERFACE SECTION

int initWinsock(WSADATA *wsaDataAddr);
void setAddrInfo(struct addrinfo *hints);
int defineServerInfo(struct addrinfo *hints, struct addrinfo *serverInfo);
int exitServer();
void log(char *message);
int getServerSocket(struct addrinfo serverInfo);
void logSockError(char *message);
int bindSocket(SOCKET ServerSocket, struct addrinfo serverInfo);
int listenSocket(SOCKET ServerSocket);
int webServer(SOCKET *ServerSocket);
int processHTTPRequest(SOCKET ClientSocket, char *HTTPRequest);
char *getHTTPRequestMethod(char *HTTPRequest);
char *getHTTPResponse(char *HTTPRequest);
char *processGetRequest(char *HTTPRequest);
char *getFileContent(char *filename);
char *resolvePathname(char *filename);

int main()
{
    WSADATA wsaData;
    struct addrinfo *serverInfo = NULL;
    struct addrinfo hints;
    SOCKET ServerSocket = INVALID_SOCKET;
    int success = 0;

    // Inicializa o winsock.
    if (!initWinsock(&(wsaData))) {
        log("Falha ao iniciar o Winsocket.");
        return 1;
    }

    // Define as informações de endereço.
    setAddrInfo(&(hints));

    // Define as informações do servidor com base na estrutura "hints" + porta.
    success = defineServerInfo(&hints, &serverInfo);

    if (!success) {
        log("Falha ao definir as informações do socket no servidor.");
        return exitServer();
    }

    // Cria o socket para o servidor com base nas informações definidas anteriormente.
    ServerSocket = getServerSocket(*serverInfo);

    // Verifica se o socket foi criado com sucesso.
    if (ServerSocket == INVALID_SOCKET) {
        logSockError("Falha ao criar o Socket servidor.");
        freeaddrinfo(serverInfo);
        return exitServer();
    }

    // Associa o Socket servidor a porta local.
    success = bindSocket(ServerSocket, *serverInfo);

    if (!success) {
        logSockError("Falha ao associar o Socket servidor a porta local.");
        freeaddrinfo(serverInfo);
        closesocket(ServerSocket);
        return exitServer();
    }

    // Remove a estrutura com informações do servidor da memoria.
    freeaddrinfo(serverInfo);

    // Habilita o Socket servidor a receber conexões com o cliente.
    success = listenSocket(ServerSocket);

    if (!success) {
        logSockError("Falha ao habilitar o Socket servidor para receber conexões.");
        closesocket(ServerSocket);
        return exitServer();
    }

    return webServer(&(ServerSocket));
}

/**
 * @brief initWinsock Tenta inicializar o Winsock.
 * @param wsaDataAddr
 * @return 1 (sucesso), 0 (falha).
 */
int initWinsock(WSADATA *wsaDataAddr)
{
    return (WSAStartup(MAKEWORD(2, 2), wsaDataAddr) == 0);
}

/**
 * @brief setAddrInfo
 * @param hints
 */
void setAddrInfo(struct addrinfo *hints)
{
    ZeroMemory(hints, sizeof(*(hints)));
    hints->ai_family = AF_INET;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_protocol = IPPROTO_TCP;
    hints->ai_flags = AI_PASSIVE;
}

/**
 * @brief defineServerInfo
 * @param hints
 * @param serverInfo
 * @return 1 (sucesso), 0 (falha).
 */
int defineServerInfo(struct addrinfo *hints, struct addrinfo *serverInfo)
{
    return (getaddrinfo(NULL, DEFAULT_PORT, hints, serverInfo) == 0);
}

/**
 * @brief exitServer
 * @return 1
 */
int exitServer()
{
    WSACleanup();
    return 1;
}

/**
 * @brief log
 * @param message
 */
void log(char *message)
{
    printf("\n%s\n", message);
    fflush(stdout);
}

/**
 * @brief getServerSocket
 * @param serverInfo
 * @return Retorna o "file descriptor".
 */
int getServerSocket(struct addrinfo serverInfo)
{
    return socket(serverInfo.ai_family, serverInfo.ai_socktype, serverInfo.ai_protocol);
}

/**
 * @brief logSockError
 * @param message
 */
void logSockError(char *message)
{
    log(message);
    printf("\nErro de Socket: %d\n", WSAGetLastError());
    fflush(stdout);
}

/**
 * @brief bindSocket
 * @param ServerSocket
 * @param serverInfo
 * @return
 */
int bindSocket(SOCKET ServerSocket, struct addrinfo serverInfo)
{
    return (bind(ServerSocket, serverInfo.ai_addr, serverInfo.ai_addrlen) != SOCKET_ERROR);
}

/**
 * @brief listenSocket
 * @param ServerSocket
 * @return
 */
int listenSocket(SOCKET ServerSocket)
{
    return (listen(ServerSocket, SOMAXCONN) != SOCKET_ERROR);
}

/**
 * @brief webServer
 * @param ServerSocket
 * @return
 */
int webServer(SOCKET *ServerSocket)
{
    SOCKET ClientSocket = INVALID_SOCKET;
    int success = 0;
    char HTTPRequest[DEFAULT_BUFLEN];

    printf("\n=======================================\n");
    printf("Servidor Web em localhost:%s", DEFAULT_PORT);
    printf("\n=======================================\n");

    // Aceita a conexão efetuada pelo cliente com o Socket servidor.
    ClientSocket = accept(*ServerSocket, NULL, NULL);

    if (ClientSocket == INVALID_SOCKET) {
        logSockError("Falha ao aceitar a conexão com o cliente.");
        closesocket(*ServerSocket);
        return exitServer();
    }

    // Fecha o Socket servidor.
    closesocket(*ServerSocket);

    // Recebe os dados da requisição HTTP.
    if (recv(ClientSocket, HTTPRequest, DEFAULT_BUFLEN, 0) == SOCKET_ERROR) {
        logSockError("Falha ao receber dados do cliente.");
        closesocket(ClientSocket);
        return exitServer();
    }

    return processHTTPRequest(ClientSocket, HTTPRequest);
}

/**
 * @brief processHTTPRequest
 * @param ClientSocket
 * @param HTTPRequest
 * @return
 */
int processHTTPRequest(SOCKET ClientSocket, char *HTTPRequest)
{
    char *HTTPResponse = getHTTPResponse(HTTPRequest);

    if (send(ClientSocket, HTTPResponse, strlen(HTTPResponse), 0) == SOCKET_ERROR) {
        logSockError("Falha ao enviar a resposta HTTP.");
        closesocket(ClientSocket);
        return exitServer();
    }

    if (shutdown(ClientSocket, SD_SEND) == SOCKET_ERROR) {
        logSockError("Falha ao desligar o Socket cliente.");
        closesocket(ClientSocket);
        return exitServer();
    }

    log("OK! Requisicao atendida com sucesso. Fechando a conexao...");
    closesocket(ClientSocket);
    return exitServer();
}

/**
 * @brief getHTTPRequestMethod
 * @param HTTPRequest
 * @return
 */
char *getHTTPRequestMethod(char *HTTPRequest)
{
    if (strstr(HTTPRequest, "GET")) {
        return "GET";
    }

    return NULL;
}

/**
 * @brief getHTTPResponse
 * @param HTTPRequest
 * @return
 */
char *getHTTPResponse(char *HTTPRequest)
{
    char *requestMethod = getHTTPRequestMethod(HTTPRequest);
    char *HTTPResponse;

    if (requestMethod != NULL && strcmp(requestMethod, "GET") == 0) {
        // Atende requisição GET.
        HTTPResponse = processGetRequest(HTTPRequest);
    } else {
        // Metodo nao permitido.
        HTTPResponse = "HTTP/1.1 405 Method Not Allowed\n\n<h1>405 Method Not Allowed</h1>";
    }

    return HTTPResponse;
}

/**
 * @brief processGetRequest
 * @param HTTPRequest
 * @return
 */
char *processGetRequest(char *HTTPRequest)
{
    char *response;
    char *content;
    char *status;
    char HTTPVersion[] = "HTTP/1.1";
    char *startOfPath = strchr(HTTPRequest, '/');
    char *endOfPath = strchr(startOfPath, ' ');
    char path[endOfPath - startOfPath];
    strncpy(path, startOfPath, endOfPath - startOfPath);
    path[sizeof(path)] = 0;

    // Valor padrao do status da requisicao.
    status = "200 OK";

    // Tratando "/" e "/index.html" igualmente.
    if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        content = getFileContent("/index.html");
    } else if (strcmp(path, "/error.html") == 0)  {
        content = getFileContent(path);
        status = "500 Internal Server Error";
    } else {
        content = getFileContent(path);
    }

    // Verifica se o arquivo não foi encontrado no servidor.
    if (content == NULL) {
        // Busca a pagina "404.html".
        content = getFileContent("/404.html");

        // Verifica se a pagina "404.html" nao foi encontrada.
        if (content == NULL) {
            content = "<h1>404 Not Found</h1>";
        }
        status = "404 Not Found";
    }

    response = (char *) malloc(strlen(HTTPVersion) + strlen(status) + strlen(content) + 1);

    strcpy(response, HTTPVersion);
    strcat(response, " ");
    strcat(response, status);
    strcat(response, "\n\n");
    strcat(response, content);

    return response;
}

/**
 * @brief getFileContent
 * @param filename
 * @return
 */
char *getFileContent(char *filename)
{
    FILE *file;
    char *pathname = resolvePathname(filename);
    char *fileContent;
    long fileSize;

    file = fopen(pathname, "r");

    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        rewind(file);
        fileContent = malloc((fileSize + 1) * (sizeof(char)));
        fread(fileContent, sizeof(char), fileSize, file);
        fclose(file);
        fileContent[fileSize] = 0;
        return fileContent;
    }

    return NULL;
}

/**
 * @brief resolvePathname
 * @param filename
 * @return
 */
char *resolvePathname(char *filename)
{
    char serverRoot[] = DEFAULT_SERVERROOT;
    char *resolvedFilename = malloc(strlen(filename) + 1);
    char *pathname;
    int i = 0;

    for (i; filename[i] != '\0'; i++) {

        if (filename[i] == '/') {
            resolvedFilename[i] = '\\';
        } else {
            resolvedFilename[i] = filename[i];
        }
    }
    resolvedFilename[i] = '\0';
    pathname = (char *) malloc(strlen(serverRoot) + strlen(resolvedFilename) + 1);

    strcpy(pathname, serverRoot);
    strcat(pathname, resolvedFilename);

    return pathname;
}
