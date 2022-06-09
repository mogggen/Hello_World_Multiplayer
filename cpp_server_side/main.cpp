#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <thread>
#include <WS2tcpip.h>

#include "main.h"
#pragma comment(lib, "ws2_32.lib")

unsigned int seqNum;

struct connection
{
	unsigned int id;
	
	ObjectDesc description;
	ObjectForm form;
	Coordinate coord;
	
	SOCKET client;
	std::thread recvThread;
};

std::vector<connection> connections;

SOCKET setup_listening(const std::string& ipAddress, const int& listening_port)
{
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		std::cerr << "Can't start Winsock, Err #" << wsResult << std::endl;
		exit(1);
	}

	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		std::cerr << "Can't create a socket! Quitting" << std::endl;
		exit(1);
	}


	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(listening_port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	int lsResult = listen(listening, SOMAXCONN);
	if (lsResult != 0)
	{
		std::cerr << "Can't establish a listening, Err #" << lsResult << std::endl;
		exit(1);
	}

	return listening;
}

void sending(char buf[], const SOCKET& s)
{	
	send(s, buf, sizeof(buf), 0);
}

void sendAll(char buf[])
{
	for (size_t i = 0; i < connections.size(); i++)
	{
		send(connections[i].client, buf, sizeof(buf), 0);
	}
}

void receiving(char buf[], const SOCKET& s)
{
	while (true)
	{
		recv(s, buf, sizeof(buf), 0);

		if (isItOkToMove())
		{
			// parse data
			// forward message
		}

		// on server check
		if (okToMove())
		{
			sendAll(NewPlayerPostion);
		}
		else
		{
			// do nothing
		}
		// from java to server
		
		printf("%s\r\n", buf);
	}

// closes the socket after receiving a afk ping
	closesocket(s);
	WSACleanup();
}

const Coordinate first_avalible_Coordinate(std::vector<connection>& occupied)
{
	sort(occupied.begin(), occupied.end());
	for (size_t i = 0; i < occupied.size() - 1; i++)
	{
		if (occupied[i] + 1 != occupied[i + 1])
		{
			occupied.push_back(occupied[i] + 1);
			return occupied[i] + 1;
		}
	}
	// no gaps
	occupied.push_back(occupied[occupied.size() - 1] + 1);
	return occupied[occupied.size() - 1];
}

int main()
{
	std::string ipAddress = "127.0.0.1";
	int starting_port = 54000; // linux_port
	unsigned int newClientId;

	SOCKET listening = setup_listening(ipAddress, starting_port);

	while (true)
	{
		SOCKET client = accept(listening, nullptr, nullptr);

		connections.push_back({
			newClientId++,
			NonHuman,
			Pyramid,
			first_avalible_Coordinate(connections),
			client,
			std::thread(receiving, client)
			});
	}

	// when all threads have joined
	std::cin.get();
}
