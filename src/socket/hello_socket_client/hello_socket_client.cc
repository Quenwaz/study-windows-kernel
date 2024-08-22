#include <cstdio>
#include <cstdlib>
#include <WinSock2.h>

void ErrorHandling(char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}


int main(int argc, char const *argv[])
{
    if (argc != 3){
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) !=0 )
    {
        ErrorHandling("WSAStartup() error!");
    }

    SOCKET hSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (hSocket == INVALID_SOCKET){
        ErrorHandling("socket() error");
    }

    SOCKADDR_IN servAddr;
    ZeroMemory(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);
    servAddr.sin_port = htons(atoi(argv[2]));

    if (connect(hSocket, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR){
        ErrorHandling("connect() error!");
    }

    printf("message from server: ");
    char message[32] = {0};
    int len_recv = 0;
    while(len_recv = recv(hSocket, message, sizeof(message), 0)){
        if (len_recv == -1){
            ErrorHandling("read() error");
        }

        printf("%s", message);
    }
    printf("\n");

    closesocket(hSocket);
    WSACleanup();
    return 0;
}

