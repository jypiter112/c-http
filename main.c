#include "framework.h"
int sHandleRequest();
/*
    Known limitations:
    File size: 255 bytes

    Security:
    http 1.0
    path escape possible and no path validation
*/
int sHandleRequest(){
    if(strlen(pserver->request) > 4) {
        // Get GET
        char get[4];
        memcpy(get, pserver->request, 3);
        get[3] = '\0';
        printf("\n\n%s\n", get);
        if(!strcmp(get, "GET")){
            printf("GET Request detected!\n");
            sPathFromRequest();
            sSendFile();
        }
    }
    return 0;
}
int main(void){
    sMakeServer();
    sBindServer(8080);
    sListen();
    while(pserver->isOnline){
        sAccept();
        if(pserver->recvLoop){
            sRecvRequest();
            sHandleRequest();
        }
    }
    sDestroyServer();
    return 0;
}
