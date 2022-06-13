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
unsigned int seqNum;

void sendAll(char buf[])
{
	for (size_t i = 0; i < connections.size(); i++)
		send(connections[i].client, buf, sizeof(buf), 0);
}

void sendAll(const char* buf, int len)
{
	for (size_t i = 0; i < connections.size(); i++)
		send(connections[i].client, buf, sizeof(len), 0);
}

void moved(const int& clientid, const Coordinate& pos)
{
	NewPlayerPositionMsg newPlayerPositionMsg =
	{
		/*ChangeMsg*/{
			/*MsgHead*/{
				sizeof(NewPlayerPositionMsg),
				seqNum++,
				clientid,
				Change
				},
		/*ChangeType*/{NewPlayerPosition}
		},
		/*Coordinate*/{pos},
		/*Coordinate*/{0, 0}
	};
	sendAll((char*)&newPlayerPositionMsg, newPlayerPositionMsg.msg.head.length);
}

void joined(const int& clientid)
{
	NewPlayerMsg newPlayerMsg =
	{
		/*ChangeMsg*/{
			/*MsgHead*/{
				sizeof(NewPlayerMsg),
				seqNum++,
				clientid,
				Change
				},
			/*ChangeType*/{NewPlayer}
		},
		Human,
		Pyramid,
		'-'
	};
}

void kicked(const int& clientid)
{
	PlayerLeaveMsg leaveMsg =
	{
		/*ChangeMsg*/{
			/*MsgHead*/ {
				sizeof(PlayerLeaveMsg),
				seqNum++,
				clientid,
				Change
				},
		/*ChangeType*/{PlayerLeave}
		}
	};
	sendAll((char*)&leaveMsg, leaveMsg.msg.head.length);
}

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



void receiving(const SOCKET& s)
{
	char buf[1024];
	while (true)
	{
		int count = recv(s, buf, sizeof(buf), 0);
		if (count == SOCKET_ERROR)
		{
			//std::cerr << "SERVER Error code: " << WSAGetLastError() << std::endl;
			// broadcast that the client has left the server
			//kicked(player);
			break;
		}

		MsgHead* msgHead = (MsgHead*)buf;

		JoinMsg* joinMsg = (JoinMsg*)buf;
		LeaveMsg* leaveMsg = (LeaveMsg*)buf;
		EventMsg* eventMsg = (EventMsg*)buf;

		while (msgHead <= (MsgHead*)(buf + sizeof(buf) / sizeof(*buf)))
		{
			switch (msgHead->type)
			{
			//case Join:
			//	connections.push_back({
			//		joinMsg->head.id,
			//		joinMsg->desc,
			//		joinMsg->form,
			//		joinMsg->
			//		});
			//	joined(joinMsg->msg.head.id);
			//	break;

			//case PlayerLeave:
			//	// id tells you which to remove and what squre the java clients should stop rendering
			//	kicked(playerLeaveMsg->msg.head.id);
			//	break;

			//case NewPlayerPosition:
			//	break;

			default:
				std::cout << "switch defaulted" << std::endl;
				std::cin.get();
				break;
			}

			msgHead = (MsgHead*)(msgHead + msgHead->length);

			joinMsg = (JoinMsg*)msgHead;
			leaveMsg = (LeaveMsg*)msgHead;
			eventMsg = (EventMsg*)msgHead;
		}

		//if (isItOkToMove())
		//{
		//	// parse data
		//	// forward message
		//}

		//// on server check
		//if (okToMove())
		//{
		//	sendAll(NewPlayerPostion);
		//}
		//else
		//{
		//	// do nothing
		//}
		// from java to server
		
		printf("%s\r\n", buf);
	}

// closes the socket after receiving a afk ping
	closesocket(s);
	WSACleanup();
}

const Coordinate first_avalible_Coordinate(std::vector<connection>& occupied)
{
	//sort(occupied.begin(), occupied.end());
	//for (size_t i = 0; i < occupied.size() - 1; i++)
	//{
	//	if (occupied[i] + 1 != occupied[i + 1])
	//	{
	//		occupied.push_back(occupied[i] + 1);
	//		return occupied[i] + 1;
	//	}
	//}
	//// no gaps
	//occupied.push_back(occupied[occupied.size() - 1] + 1);
	//return occupied[occupied.size() - 1];
	return { -200, -200 };
}

int main()
{
	std::string ipAddress = "127.0.0.1";
	int starting_port = 54000; // linux_port
	unsigned int newClientId = 0u;

	SOCKET listening = setup_listening(ipAddress, starting_port);

	for (SOCKET client;;)
	{
		client = accept(listening, nullptr, nullptr);

		connections.push_back({
			newClientId++,
			NonHuman,
			Pyramid,
			first_avalible_Coordinate(connections),
			client,
			std::thread(receiving, client)
			});
		joined(newClientId);
	}

	// blocking until all threads have joined (which will never happen naturally due to the infinite loop)
	for (size_t i = 0; i < connections.size(); i++)
	{
		if (connections[i].recvThread.joinable())
			connections[i].recvThread.join();
	}
}
