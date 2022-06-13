#include <iostream>
#include <string>
#include <thread>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "main.h"

SOCKET java_sock;
SOCKET linux_sock;

void leave(SOCKET sock)
{
	// 1337 = clientId
	LeaveMsg leaveMsg =
	{
		{0, 0, 1337, Leave}
	};
	leaveMsg.head.length = sizeof(LeaveMsg);
	send(sock, (char*)&leaveMsg, leaveMsg.head.length, 0);
}

void recv_from_java()
{
	char buf[1024];
	while (true)
	{
		int count = recv(java_sock, buf, sizeof(buf), 0);
		if (count == SOCKET_ERROR)
		{
			leave(linux_sock);
		}
		send(linux_sock, buf, sizeof(buf), 0);
	}

	// closesocket(java_sock);
	// WSACleanup();
}

void recv_from_server()
{
	char buf[1024];
	while (true)
	{
		int count = recv(linux_sock, buf, sizeof(buf), 0);
		if (count == SOCKET_ERROR)
		{
			strcpy(buf, std::string("disconnected from server!").c_str());
			send(java_sock, buf, sizeof(buf), 0);
		}
		send(java_sock, buf, sizeof(buf), 0);
	}
	
	// closesocket(linux_sock);
	// WSACleanup();
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
		printf("Java socket connected, press to continue");
	}
	std::cin.get();

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
