#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
#include <cstring>
#include <WS2tcpip.h>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

int main()
{


	std::string ipAddress = "127.0.0.1";
	int port = 4999;

	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		std::cerr << "Can't start Winsock, Err #" << wsResult << std::endl;
		return 0;
	}

	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in socketAddr_in;
	socketAddr_in.sin_family = AF_INET;
	socketAddr_in.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &socketAddr_in.sin_addr);

	int conRes = connect(listening, (sockaddr*)&socketAddr_in, sizeof(socketAddr_in));
	if (conRes == SOCKET_ERROR)
	{
		std::string prompt = "Can't connect to server, Error: ";
		int error = WSAGetLastError();
		std::cerr << prompt << '#' << error << std::endl;
		closesocket(listening);
		WSACleanup();
		return 0;
	}
	char recvbuf[1024];
	char sendbuf[] = "Hello Neighbour";
	while(true)
	{
		recv(listening, recvbuf, sizeof(recvbuf), 0);
		std::cout << recvbuf << std::endl;
		send(listening, sendbuf, sizeof(sendbuf), 0);
	}
	closesocket(listening);
	WSACleanup();
	return 0;
}