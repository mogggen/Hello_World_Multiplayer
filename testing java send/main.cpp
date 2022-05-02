#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
#include <cstring>
#include <thread>
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

void running(const bool& sending, const std::string& ipAddress, const int& port)
{
#ifdef _WIN32
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		std::cerr << "Can't start Winsock, Err #" << wsResult << std::endl;
		return 0;
	}
#endif

#ifdef __linux
#define SOCKET int
#endif

#ifndef errno
#define errno WSAGetLastError()
#endif

	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in socketAddr_in;
	socketAddr_in.sin_family = AF_INET;
	socketAddr_in.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &socketAddr_in.sin_addr);

	int conRes = connect(listening, (sockaddr*)&socketAddr_in, sizeof(socketAddr_in));

#ifdef _WIN32
	if (conRes == SOCKET_ERROR)
	{
		std::string prompt = "Can't connect to server, Error: ";
		int error = errno;
		std::cerr << prompt << '#' << error << std::endl;
		closesocket(listening);
		WSACleanup();
		return 0;
	}
#endif

	char recvbuf[1024];
	recvbuf[0] = '\0';
	char sendbuf[] = "Hello Neighbour";
	while(true)
	{
		if (sending)
		{
			send(listening, recvbuf, sizeof(recvbuf), 0);
		}
		else
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
	std::string ipAddress = "127.0.0.1";
	int java_port = 4999;
	int linux_port = 54000;

	std::thread threads[4];
	
	for (size_t i = 0; i < 4; i++)
	{
		threads[i] = std::thread(running, i % 2, ipAddress, (i / 2 % 2 ? linux_port : java_port));
	}
}
