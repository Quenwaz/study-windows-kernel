#include <winsock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include <stdio.h>


 unsigned __stdcall thread_proc(void* data)
{
	 SOCKET NewConnection = *(SOCKET*)(data);
	 printf("We are waiting to receive data...\n");
	 for (;;)
	 {
		 int                  Ret = 0;
		 char                 DataBuffer[1024] = { 0 };
		 if ((Ret = recv(NewConnection, DataBuffer, sizeof(DataBuffer), 0))
			 == SOCKET_ERROR)
		 {
			 printf("recv failed with error %d\n", WSAGetLastError());
			 break;
		 }
		 printf("recv:%s\n", DataBuffer);

		 if (SOCKET_ERROR == send(NewConnection, DataBuffer, Ret, 0)) {
			 break;
		 }
	 }

	 printf("We are now going to close the client connection.\n");
	 closesocket(NewConnection);
	 return 0;
}

void main(void)
{
	WSADATA              wsaData;
	SOCKET               ListeningSocket;
	SOCKADDR_IN          ServerAddr;
	SOCKADDR_IN          ClientAddr;
	int                  ClientAddrLen;
	int                  Port = 5150;
	int                  Ret = 0;
	// Initialize Winsock version 2.2

	if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		// NOTE: Since Winsock failed to load we cannot use WSAGetLastError 
		// to determine the error code as is normally done when a Winsock 
		// API fails. We have to report the return status of the function.

		printf("WSAStartup failed with error %d\n", Ret);
		return;
	}

	// Create a new socket to listening for client connections.

	if ((ListeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
		== INVALID_SOCKET)
	{
		printf("socket failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return;
	}

	// Setup a SOCKADDR_IN structure that will tell bind that we
	// want to listen for connections on all interfaces using port
	// 5150. Notice how we convert the Port variable from host byte
	// order to network byte order.

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(Port);
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Associate the address information with the socket using bind.

	if (bind(ListeningSocket, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr))
		== SOCKET_ERROR)
	{
		printf("bind failed with error %d\n", WSAGetLastError());
		closesocket(ListeningSocket);
		WSACleanup();
		return;
	}

	// Listen for client connections. We used a backlog of 5 which is
	// normal for many applications.

	if (listen(ListeningSocket, 5) == SOCKET_ERROR)
	{
		printf("listen failed with error %d\n", WSAGetLastError());
		closesocket(ListeningSocket);
		WSACleanup();
		return;
	}

	printf("We are awaiting a connection on port %d.\n", Port);

	SOCKET  NewConnection;
	while (true)
	{
		// Accept a new connection when one arrives.
		ClientAddrLen = sizeof(SOCKADDR_IN);
		
		if ((NewConnection = accept(ListeningSocket, (SOCKADDR *)&ClientAddr,
			&ClientAddrLen)) == INVALID_SOCKET)
		{
			printf("accept failed with error %d\n", WSAGetLastError());
			break;
		}

		_beginthreadex(NULL, 0, thread_proc, &NewConnection, 0, NULL);

		char ip_client[16] = { 0 };
		inet_ntop(AF_INET, &ClientAddr.sin_addr, ip_client, sizeof ip_client);
		printf("We successfully got a connection from %s:%d.\n",
			ip_client, ntohs(ClientAddr.sin_port));
	}

	closesocket(ListeningSocket);
	WSACleanup();
}
