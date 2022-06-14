#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <WS2tcpip.h>

#include "main.h"
#pragma comment(lib, "ws2_32.lib")

bool operator==(const Coordinate& lhs, Coordinate& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

struct ConnectionThread;
struct Connection
{
	unsigned int id;

	ObjectDesc description;
	ObjectForm form;
	Coordinate coord;

	SOCKET client;
	ConnectionThread* holdingHands;
};

struct ConnectionThread
{
	Connection* holdingHands;
	std::thread recvThread;
};

std::mutex gameBoard;
std::vector<Connection> connections;
std::vector<ConnectionThread> connectionThreads;
unsigned int seqNum;

void sendAll(const char* buf, int len)
{
	gameBoard.lock();
	for (size_t i = 0; i < connections.size(); i++)
		send(connections[i].client, buf, sizeof(len), 0);
	gameBoard.unlock();
}

void moved(const int& clientid, const Coordinate& newPos)
{
	for (Connection& c : connections)
	{
		if (newPos == c.coord)
			return; // no new player position to broadcast
	}
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
		/*Coordinate*/{newPos},
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
	sendAll((char*)&newPlayerMsg, newPlayerMsg.msg.head.length);
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
		std::printf("message recevied\n");

		JoinMsg* joinMsg = (JoinMsg*)buf;
		LeaveMsg* leaveMsg = (LeaveMsg*)buf;
		MoveEvent* moveEvent = (MoveEvent*)buf;

		switch (((ChangeMsg*)&buf)->head.type)
		{
		case Join:
			for (size_t i = 0; i < connections.size(); i++)
			{
				if (joinMsg->head.id == connections[i].id)
				{
					connections[i].description = joinMsg->desc;
					connections[i].form = joinMsg->form;
				}
			}

			joined(joinMsg->head.id);
			break;

		case Leave:
			for (size_t i = 0; i < connections.size(); i++)
			{
				if (leaveMsg->head.id == connections[i].id)
				{
					delete connections[i].holdingHands;
					delete connectionThreads[i].holdingHands;

					connections.erase(connections.begin() + i);
					connectionThreads.erase(connectionThreads.begin() + i);
				}
			}
				
			kicked(leaveMsg->head.id);
			break;

		case Event:
			for (size_t i = 0; i < connections.size(); i++)
			{
				if (moveEvent->event.head.id == connections[i].id)
				{
					moved(connections[i].id, moveEvent->pos);
				}
			}
			break;

		default:
			std::cout << "switch defaulted" << std::endl;
			std::cin.get();
			break;
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

const Coordinate first_avalible_Coordinate()
{
	// stupid bubble sort
	for (size_t i = 0; i < connections.size(); i++)
	{
		for (size_t j = 1; j < connections.size() - i; j++)
		{
			Connection temp;
			if (connections[j - 1].coord.y > connections[j].coord.y)
			{
				temp = connections[j - 1];
				connections[j - 1] = connections[j];
				connections[j] = temp;
			}
			else if (connections[j - 1].coord.y == connections[j].coord.y)
			{
				if (connections[j - 1].coord.x > connections[j].coord.x)
				{
					temp = connections[j - 1];
					connections[j - 1] = connections[j];
					connections[j] = temp;
				}
			}
		}
	}
	
	for (Coordinate start = { -200, -200 };;)
	{
		if (connections.size() == 0) return start;
		for (Connection c : connections)
		{
			if (start == c.coord)
			{
				if (start.x > 200) // row is full so move to next one
				{
					if (start.y > 200) // server is full
					{
						// nope
					}
					start = { -200, start.y++ };
				}
				else
				{
					start.x++;
				}
			}
			else
			{
				return start;
			}
		}
	}
	// never happens (unless you have 160801 clients connected already connected)
	return Coordinate{ 201, 201 };
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
		gameBoard.lock();

		connections.push_back({
			newClientId++,
			NonHuman,
			Pyramid,
			first_avalible_Coordinate(),
			client,
			nullptr
			});

		size_t last = connections.size() - 1;

		connectionThreads.push_back({
			&connections[last]
			});
		connectionThreads[last].recvThread = std::thread(receiving, connections[last].client);
		
		connections[last].holdingHands = &connectionThreads[last];
		
		gameBoard.unlock();

		joined(newClientId);
	}

	// blocking until all threads have joined (which will never happen naturally due to the infinite loop)
	for (size_t i = 0; i < connections.size(); i++)
	{
		if (connectionThreads[i].recvThread.joinable())
			connectionThreads[i].recvThread.join();
	}
}
