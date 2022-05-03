#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <thread>
#include "InstantProject.h"
#ifdef _WIN32
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#elif __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

#ifdef __linux
#define SOCKET int
#endif

void running(const bool& sending, SOCKET listening)
{
	// what to handle
	// client connecting with id=0, seqNum=0 (NewPlayer)
	// replies with type=Join, id = newly assign clientId

	// a client moving (NewPlayerPosition)
	// replies with information about the avaliblity of the position
	// distribute the new positon of the client to the other clients

	if (sending)
	{
		char sendbuf[] = "Hello Neighbour";
		while(true)
		{
				send(listening, sendbuf, sizeof(sendbuf), 0);
		}
	}
	else
	{
		char recvbuf[1024];
		recvbuf[0] = '\0';
		while(true)
		{
#ifdef __linux__
			ssize_t r = recv(listening, recvbuf, sizeof(recvbuf), 0);
			if (r == -1 && errno == EWOULDBLOCK)
			{
				printf("recv failed with error: EWOULDBLOCK\n");
			}
#elif _WIN32
			recv(listening, recvbuf, sizeof(recvbuf), 0);
#endif
			// handle the received data here
			/* code */
		}
	}

	#ifdef _WIN32
		closesocket(listening);
		WSACleanup();
	#elif __linux__
		close(listening);
	#endif
}

int main()
{
#ifdef _WIN32
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		std::cerr << "Can't start Winsock, Err #" << wsResult << std::endl;
		return 1;
	}
#endif

	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);

#ifdef _WIN32
	if (listening == INVALID_SOCKET)
	{
		std::cerr << "Can't create a socket! Quitting" << std::endl;
		return 1;
	}
#endif
	sockaddr_in socketAddr_in;
	std::vector<std::thread> threads;
	uint16_t port = 54000;

	while (true)
	{
		// main: listens for new connections
		socketAddr_in.sin_family = AF_INET;
		socketAddr_in.sin_port = htons(port++);
		inet_pton(AF_INET, "127.0.0.1", &socketAddr_in.sin_addr);

		int conRes = connect(listening, (sockaddr*)&socketAddr_in, sizeof(socketAddr_in));

		// when a new connection is found:
		threads.emplace_back(std::thread(running, true, listening));
		threads.emplace_back(std::thread(running, false, listening));

		// run the send and receive on two new threads
		// continue to listen on the main thread
	}

	for (size_t i = 0; i < threads.size(); i++)
	{
		threads[i].join();
	}
}