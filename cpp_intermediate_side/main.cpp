#include <iostream>
#include <string>
#include <thread>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "main.h"

#define UNDEFINED -1

SOCKET java_sock;
SOCKET linux_sock;

unsigned int seqNo = 0u;
int javaId = UNDEFINED;

char* serialize(LeaveMsg* leaveMsg)
{
	char* q = new char[4];
	q[0] = (char)leaveMsg->head.length;
	q[1] = (char)leaveMsg->head.seqNo;
	q[2] = (char)leaveMsg->head.id;
	q[3] = (char)leaveMsg->head.type;
	return q;
}

char* serialize(PlayerLeaveMsg* playerLeaveMsg)
{
	char* q = new char[5];
	q[0] = (char)playerLeaveMsg->msg.head.length;
	q[1] = (char)playerLeaveMsg->msg.head.seqNo;
	q[2] = (char)playerLeaveMsg->msg.head.id;
	q[3] = (char)playerLeaveMsg->msg.head.type;
	q[4] = (char)playerLeaveMsg->msg.type;
	return q;
}

void sendLeaveMsgToServer(const SOCKET& sock, const int& clientid)
{
	LeaveMsg leaveMsg =
	{
		{
			4,
			seqNo++,
			clientid,
			Leave
		}
	};
	char* buf = serialize(&leaveMsg);
	printf("buf[2]: %i leaveMsg.head.id: %i\r\n", buf[2], leaveMsg.head.id);
	if (javaId == UNDEFINED)
		printf("Den får ju aldrig ens ett jävla fitt värde\r\n");
	send(sock, buf, leaveMsg.head.length, 0);
}

void sendLeaveMsgToJava(const SOCKET& sock)
{
	printf("PlayerLeaveMsg id: %i\r\n", javaId);
	if (javaId == UNDEFINED)
		printf("VAD FAN I HELASTE HELVETE\r\n");
	PlayerLeaveMsg playerLeaveMsg =
	{
		{
			{
				5,
				seqNo++,
				javaId,
				Change
			},
			PlayerLeave
		}
	};
	printf("printing the value of the id recevied from socket %llu: %i\r\n", java_sock, playerLeaveMsg.msg.head.id);
}

void recv_from_java()
{
	char buf[7];
	ZeroMemory(buf, 7);
	for(;;)
	{
		int count = recv(java_sock, buf, sizeof(buf), 0);
		// FITT HOoOoOoR UNGE
		printf("javaId: %i", javaId);
		if (count == SOCKET_ERROR)
		{
			sendLeaveMsgToServer(linux_sock, javaId);
			closesocket(java_sock);
			return;
		}

		if (count == 0)
		{
			sendLeaveMsgToServer(linux_sock, javaId);
			closesocket(java_sock);
			return;
		}

		if (buf[3] == Change)
		{
			printf("ChangeType Change så jävla äckligt snyggt\r\n");
			if (buf[4] == PlayerLeave)
			{
				printf("såja, bra där Einar\r\n");
				javaId = buf[2];
				printf("nu är javaId fanskapet: %i\r\n", javaId);
			}
		}

		send(linux_sock, buf, buf[0], 0);
		printf("traffic: java -> linux: %d bytes\n", count);


	}

}

void recv_from_server()
{
	char buf[7];
	ZeroMemory(buf, 7);
	for(;;)
	{
		int count = recv(linux_sock, buf, sizeof(buf), 0);
		printf("javaId: %i", javaId);
		if (count == SOCKET_ERROR)
		{
			sendLeaveMsgToJava(java_sock);
			closesocket(linux_sock);
			return;
		}
		
		if (count == 0)
		{
			sendLeaveMsgToJava(java_sock);
			closesocket(linux_sock);
			return;
		}
		if (buf[3] == Change)
		{
			printf("kom igen nu, ola conny\r\n");
			if (buf[4] == NewPlayer)
			{
				printf("nu tycker jag vi sätter spiken i kistan\r\n");
				javaId = buf[2];
				printf("Snyggt byggt, fräsig kärra\r\n");
			}
		}
		send(java_sock, buf, buf[0], 0);
		printf("traffic: java <- linux: %d bytes\n", count);
	}
	printf("printing the value of the id recevied from socket %llu: %i\r\n", linux_sock, buf[2]);
}

void main()
{
	std::thread threads[2];
	std::string ipAddress = "127.0.0.1";
	int java_port = 4999;
	int linux_port = 54000;

	// setup java socket
	{
		WSAData data;
		WORD ver = MAKEWORD(2, 2);
		int wsResult = WSAStartup(ver, &data);
		if (wsResult != 0)
		{
			std::cerr << "Can't start Winsock, Err #" << wsResult << std::endl;
			return;
		}

		// Create socket
		java_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (java_sock == INVALID_SOCKET)
		{
			std::cerr << "Can't create socket, Err #" << WSAGetLastError() << std::endl;
			WSACleanup();
			return;
		}

		// Fill in a hint structure
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(java_port);
		inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

		// Connect to server
		int connResult = connect(java_sock, (sockaddr*)&hint, sizeof(hint));
		if (connResult == SOCKET_ERROR)
		{
			std::cerr << "Can't connect to server, Err #" << WSAGetLastError() << std::endl;
			closesocket(java_sock);
			WSACleanup();
			return;
		}
		//printf("Java socket connected\n");
	}
	//system("pause");

	// setup linux socket
	{
		WSAData data;
		WORD ver = MAKEWORD(2, 2);
		int wsResult = WSAStartup(ver, &data);
		if (wsResult != 0)
		{
			std::cerr << "Can't start Winsock, Err #" << wsResult << std::endl;
			return;
		}

		// Create socket
		linux_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (linux_sock == INVALID_SOCKET)
		{
			std::cerr << "Can't create socket, Err #" << WSAGetLastError() << std::endl;
			WSACleanup();
			return;
		}

		// Fill in a hint structure
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(linux_port);
		inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

		// Connect to server
		int connResult = connect(linux_sock, (sockaddr*)&hint, sizeof(hint));
		if (connResult == SOCKET_ERROR)
		{
			std::cerr << "Can't connect to server, Err #" << WSAGetLastError() << std::endl;
			closesocket(linux_sock);
			WSACleanup();
			return;
		}
		//printf("Linux socket connected\n");
	}

	threads[0] = std::thread(recv_from_java);
	threads[1] = std::thread(recv_from_server);

	for (size_t i = 0; i < sizeof(threads) / sizeof(*threads); i++)
	{
		if (threads[i].joinable())
			threads[i].join();
	}

	// Gracefully close down everything
	closesocket(java_sock);
	closesocket(linux_sock);
	WSACleanup();
}
