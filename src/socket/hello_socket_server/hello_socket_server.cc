#include <cstdio>
#include <WinSock2.h>

void ErrorHandling(char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}


int main(int argc, char const *argv[])
{
    if (argc != 2){
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2),&wsadata) != 0){

    }

    SOCKET hServSock = socket(PF_INET, SOCK_STREAM,0);
    if (hServSock == INVALID_SOCKET){
        ErrorHandling("socket() error");
    }

    SOCKADDR_IN servAddr;
    ZeroMemory(&servAddr, sizeof servAddr);
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(atoi(argv[1]));

    if (bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR){
        ErrorHandling("bind() error");
    }

    if (listen(hServSock,5) == SOCKET_ERROR){
        ErrorHandling("listen() error");
    }

    fprintf(stderr, "listen to %s\n");

    SOCKADDR_IN clntAddr;
    int szClntAddr = sizeof(clntAddr);
    SOCKET hClntSock = accept(hServSock,(SOCKADDR*)&clntAddr, &szClntAddr);
    if (hClntSock == INVALID_SOCKET){
        ErrorHandling("accept() error");
    }

    const char* pcszIPAddr = NULL;
    
#ifndef V1
    char szIpAddr[32] ={0};
    unsigned long sizeOfIpAddr = sizeof szIpAddr;
    WSAAddressToStringA((SOCKADDR*)&clntAddr,szClntAddr, NULL, szIpAddr, &sizeOfIpAddr);
    pcszIPAddr = szIpAddr;
#else
    pcszIPAddr = inet_ntoa(clntAddr.sin_addr);
#endif
    fprintf(stderr, "accept %s connection.\n",szIpAddr);

    const char* message = "Hello World.";
    send(hClntSock, message, strlen(message), 0);
    closesocket(hClntSock);
    closesocket(hServSock);
    WSACleanup();
    return 0;
}
