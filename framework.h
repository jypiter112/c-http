#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#pragma comment(lib, "Ws2_32.lib")

#define FILETYPE_HTML 0
#define FILETYPE_ERROR 0
#define FILETYPE_JS 1
#define FILETYPE_CSS 2

#define S_ERROR -1
#define S_OK 0
/*
    Data types here
    Holds relevant pointers
*/
struct Server{
    int isOnline;
    int recvLoop;
    SOCKET server;
    SOCKET client;
    char request[256];
    char path[256];
} Server; 
struct Server* pserver = &Server;
/*
    s for server funcs
    _ for temp values
    p for pointers
*/

/*
    Holds function decls
    All return 0 on success
    On error returns non zero value
*/
void sDestroyServer();
int sMakeServer();
int sBindServer(int);
int sListen();
int sAccept();
int sClientConnected();
int sRecvRequest();
long sGetFileSize(FILE*);
int sSendFile();
int sGetFileType(char*);
int sPathFromRequest();

/*
    Main backend happens here
*/
/*
    File operations
*/
int sPathFromRequest(){
    char* r = (char*)malloc(strlen(pserver->request));
    strcpy(r, pserver->request+5);
    // Find space index
    int end = 0;
    for(int i = 0; i < strlen(r); i++){
        if(r[i] == ' '){
            end = i;
            r[i] = '\0';
            break;
        }
    }
    if(end > 0) {
        // Make path
        char* actual = (char*)malloc(end + 9);
        strcpy(actual, "./public/");
        char* T = (char*)malloc(end+1);
        memcpy(T, r, (size_t)end);
        strcat(actual, T);
        memcpy(pserver->path, actual, strlen(actual) + 1);
        printf(" - Parsed path: %s\n", pserver->path);
        free(T);
        free(actual);
        free(r);
        return 0;
    }
    // Create copy of request
    printf("%s\n", r);
    free(r);
    strcpy(pserver->path, "invalid");
    return 1;
}   
int sGetFileType(char* path){
    char* c_path = path;
    int dot_index = 1;
    *c_path++;
    while(*c_path != '\0'){
        if(*c_path == '.'){
            break;
        }
        dot_index++;
        *c_path++;
    }   
    char* T[5];
    memcpy(T, path+dot_index, 5);
    T[5] = '\0';

    if(!strcmp(T, ".html")){
        return FILETYPE_HTML;
    }
    if(!strcmp(T, ".css")){
        return FILETYPE_CSS;
    }
    if(!strcmp(T, ".js")){
        return FILETYPE_JS;
    }
    return FILETYPE_ERROR;
}

long sGetFileSize(FILE* fp){
    if(!fp) return -1L;
    if(fseek(fp, 0L, SEEK_END) != 0) return -1L;
    long sz = ftell(fp);
    rewind(fp);
    return (sz == -1L) ? S_ERROR : sz;
}
int sSendFile(){
    if(!pserver->path || !strcmp(pserver->path, "./public/favicon.ico")){
        printf("Error: Null path or Favicon\n");
        return 1;
    }
    FILE* fp = fopen(pserver->path, "rb");
    if(!fp){
        printf("Error: Fp = nullptr!\nFalling back\n");
        fp = fopen("./public/index.html", "rb");
        if(!fp){
            printf("Fall back failed...\n");
            return 1;
        }
    }

    // Get file size
    int file_size = (int)sGetFileSize(fp);
    if(file_size == -1){
        printf("File size invalid!\n");
        fclose(fp);
        return 1;
    }
    if(file_size <= 0){
        if(fp) fclose(fp);
        printf("Error: file size null.\n");
        return 1;
    }
    printf("File-size: %d\n", file_size);

    // Read file
    // + 1 for \0
    char* body = (char*)malloc(file_size);
    fread(body, file_size, 1, fp);
    if(fp) fclose(fp);
    body[file_size] = '\0';

    int filetype_int = sGetFileType(pserver->path);
    char* content_type[20];
    switch(filetype_int){
        case FILETYPE_CSS:
            printf("Type css\n");
            strcpy(content_type, "stylesheet");
            break;
        case FILETYPE_JS:
            printf("Type js\n");
            strcpy(content_type, "text/javascript");
            break;
        default:
            printf("Type html\n");
            strcpy(content_type, "text/html");
            break;
    }
    char head[1024];
    snprintf(head, sizeof(head), "HTTP/1.0 200 OK\n"
                             "Server: CServer\n"
                             "Content-Length: %d\n"
                             "Content-Type: %s; charset=UTF-8\n\n%s",
                             file_size, content_type, body);
    size_t req_size = strlen(head) + 1;
    printf("req-sz: %d", (int)req_size);
    printf(" --- Req:\n%s\n", head);
    char* req = (char*)malloc(req_size);
    strcpy_s(req, req_size, head);
    req[req_size] = '\0';

    // Finally send response
    free(body);
    int n = send(pserver->client, req, req_size, 0);
    free(req);
    printf("Server: sent %d bytes.\n", n);
    return 0;
}
/*
    ---- End of file operations
*/

/*
    This is called once every loop!
*/
int sRecvRequest(){
    /*
        Succesfull request
    */
    int recvResult = recv(pserver->client, pserver->request, sizeof(pserver->request) - 1, 0);
    if(recvResult > 0){
        pserver->request[recvResult] = '\0';
        // Handled in main.c
    }
    /*
        Error cases
    */
    else if (recvResult == 0){
        printf("Connection closed by client\n");
        pserver->recvLoop = 0;
        return 1;
    }
    else {
        printf("Recv failed: %d\n", WSAGetLastError());
        pserver->recvLoop = 0;
    }
    // Success signal
    return 0;
}
int sMakeServer(){
    WSADATA wsaData;
    // Wsa
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d", WSAGetLastError());
    }
    else {
        printf("WsaStartup done.\n");
    }
    // Create socket
    pserver->server = socket(AF_INET, SOCK_STREAM, 0);
    if (pserver->server == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        return 1;
    }
    pserver->isOnline = 1;
    return 0;
}
int sBindServer(int port){
    if(!pserver->isOnline){
        printf("Error: server offline.\n");
        return 1;
    }
    struct sockaddr_in addr;
    // Try bind
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // bind to 0.0.0.0
    addr.sin_port = htons(port);

    if (bind(pserver->server, (const struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        sDestroyServer();
        return 1;
    }
    printf("Bind successful.\n");
    return 0;
}
int sListen(){
    if(!pserver->isOnline){
        printf("Error: server offline.\n");
        return 1;
    }
    // Listen for incoming connections
    if (listen(pserver->server, 10) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        sDestroyServer();
        return 1;
    }
    printf("Listening for connections...");
    return 0;
}
int sAccept(){
    if(!pserver->isOnline){
        printf("Error: server offline.\n");
        return 1;
    }
    // Accept a client connection
    pserver->client = accept(pserver->server, NULL, NULL);
    if (pserver->client == INVALID_SOCKET) {
        printf("Accept failed: %d\n", WSAGetLastError());
        sDestroyServer();
        return 1;
    }
    printf("Client accepted.\n");
    pserver->recvLoop = 1;
    return 0;
}
/*
    1 = Connected
    0 = not connected
*/
int sClientConnected(){
    if(!pserver->isOnline){
        printf("Error: server offline.\n");
        return 1;
    }
    if(pserver->client != SOCKET_ERROR || pserver->client != SOCKET_ERROR){
        printf("Error: client error\n");
        return 1;
    }
    return 0;
}
void sDestroyServer(){
    if(pserver->client) {
        closesocket(pserver->client);
    }
    if(pserver->server) {
        closesocket(pserver->server);
    }
    WSACleanup();
    pserver->isOnline = 0;
}