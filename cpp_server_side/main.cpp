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

#include <bitset>

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
};

struct ConnectionThread
{
	std::thread recvThread;
};

SOCKET newClient;
unsigned int newClientId = 0u;
std::mutex gameBoard;
std::vector<Connection> connections;
std::vector<ConnectionThread> connectionThreads;
unsigned int seqNum;

bool isLegalMove(const unsigned int& clientId, const Coordinate& newPos)
{
	if (connections.size() == 0)
	{
		// should never happen
		std::cerr << "number of connected client was 0" << std::endl;
		exit(1);
	}
	for (Connection& c : connections)
	{
		if (c.id == clientId)
		{
			if (std::sqrt((newPos.x - c.coord.x) * (newPos.x - c.coord.x) + (newPos.y - c.coord.y) * (newPos.y - c.coord.y)) > 1.)
			{
				return false;
			}
			else
			{
				for (Connection& d : connections)
				{
					if (newPos == d.coord)
					{
						return false;
					}
				}
				c.coord = newPos;
				return true;
			}
		}
		if (newPos == c.coord)
			return false; // no new player position to broadcast
	}
	return false;
}

char* serialize(NewPlayerMsg* newPlayerMsg)
{
	char* q = new char[7];
	q[0] = (char)newPlayerMsg->msg.head.length;
	q[1] = (char)newPlayerMsg->msg.head.seqNo;
	q[2] = (char)newPlayerMsg->msg.head.id;
	q[3] = (char)newPlayerMsg->msg.head.type;
	q[4] = (char)newPlayerMsg->msg.type;
	q[5] = (char)newPlayerMsg->desc;
	q[6] = (char)newPlayerMsg->form;
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

char* serialize(NewPlayerPositionMsg* newPlayerPositionMsg)
{
	char* q = new char[7];
	q[0] = (char)newPlayerPositionMsg->msg.head.length;
	q[1] = (char)newPlayerPositionMsg->msg.head.seqNo;
	q[2] = (char)newPlayerPositionMsg->msg.head.id;
	q[3] = (char)newPlayerPositionMsg->msg.head.type;
	q[4] = (char)newPlayerPositionMsg->msg.type;
	q[5] = (char)newPlayerPositionMsg->pos.x;
	q[6] = (char)newPlayerPositionMsg->pos.y;
	return q;
}

void deserialize(char* buf, JoinMsg* joinMsg) // make a new JoinMsg before this call and populate it
{
	joinMsg->head.length = buf[0];
	joinMsg->head.seqNo = buf[1];
	joinMsg->head.id = buf[2];
	joinMsg->head.type = (MsgType)buf[3];
	
	joinMsg->desc = (ObjectDesc)buf[4];
	joinMsg->form = (ObjectForm)buf[5];
}

void deserialize(char* buf, LeaveMsg* leaveMsg) // make a new LeaveMsg before this call and populate it
{
	leaveMsg->head.length = buf[0];
	leaveMsg->head.seqNo = buf[1];
	leaveMsg->head.id = buf[2];
	leaveMsg->head.type = (MsgType)buf[3];
}


void deserialize(char* buf, MoveEvent* moveEvent) // make a new MoveEvent before this call and populate it
{
	moveEvent->event.head.length = buf[0];
	moveEvent->event.head.seqNo = buf[1];
	moveEvent->event.head.id = buf[2];
	moveEvent->event.head.type = (MsgType)buf[3];
	moveEvent->event.type = (EventType)buf[4];

	moveEvent->pos.x = buf[5];
	moveEvent->pos.y = buf[6];
}

void sendAll(const char* buf, const int& len);


void moved(const int& clientid, const Coordinate& newPos)
{
	if (newPos.x < -100 || newPos.x > 100 || newPos.y < -100 || newPos.y > 100) return;
	for (Connection& c : connections)
	{
		if (newPos == c.coord)
			return; // no new player position to broadcast
	}
	NewPlayerPositionMsg newPlayerPositionMsg =
	{
		/*ChangeMsg*/{
			/*MsgHead*/{
				7,
				++seqNum,
				clientid,
				Change
				},
		/*ChangeType*/{NewPlayerPosition}
		},
		/*Coordinate*/{newPos},
		/*Coordinate*/{0, 0}
	};
	char* buf = serialize(&newPlayerPositionMsg);
	sendAll(buf, newPlayerPositionMsg.msg.head.length);
}

void joinedAndMoved(const int& clientid)
{
	NewPlayerMsg newPlayerMsg =
	{
		/*ChangeMsg*/{
			/*MsgHead*/{
				7,
				++seqNum,
				clientid,
				Change
				},
			/*ChangeType*/{NewPlayer}
		},
		Human,
		Pyramid,
	};
	char* buf = serialize(&newPlayerMsg);
	sendAll(buf, newPlayerMsg.msg.head.length);

	NewPlayerPositionMsg newPlayerPositionMsg =
	{
		/*ChangeMsg*/{
			/*MsgHead*/{
				7,
				++seqNum,
				clientid,
				Change
				},
		/*ChangeType*/{NewPlayerPosition}
		},
		connections[connections.size() - 1].coord,
		{0, 0}
	};
	char* buf2 = serialize(&newPlayerPositionMsg);
	sendAll(buf2, newPlayerPositionMsg.msg.head.length);
}

void kicked(const int& clientId)
{
	PlayerLeaveMsg playerLeaveMsg =
	{
		/*ChangeMsg*/{
			/*MsgHead*/ {
				5,
				++seqNum,
				clientId,
				Change
				},
		/*ChangeType*/{PlayerLeave}
		}
	};
	char* buf = serialize(&playerLeaveMsg);
	sendAll(buf, playerLeaveMsg.msg.head.length);
}

void kickAll(const std::vector<unsigned int> disconnected)
{
	for (int d : disconnected)
	{
		kicked(d);
	}
}

void sendAll(const char* buf, const int& len)
{
	gameBoard.lock();
	std::vector<unsigned int> disconnected;
restart:
	for (size_t i = 0; i < connections.size(); i++)
	{
		int attempt = send(connections[i].client, buf, len, 0);
		if (attempt == SOCKET_ERROR)
		{
			if (connectionThreads[i].recvThread.joinable())
			{
				connectionThreads[i].recvThread.join();
			}
			disconnected.push_back(connections[i].id);
			connections.erase(connections.begin() + i);
			connectionThreads.erase(connectionThreads.begin() + i);
			goto restart;
		}
	}

	// recursivley kick newly disconnected players
	gameBoard.unlock();
	//kickAll(disconnected);
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


	sockaddr_in hint{};
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

		
		JoinMsg* joinMsg = new JoinMsg();
		deserialize(buf, joinMsg); // check the resulting deserialization
		LeaveMsg* leaveMsg = new LeaveMsg();
		deserialize(buf, leaveMsg);
		MoveEvent* moveEvent = new MoveEvent();
		deserialize(buf, moveEvent);

		printf("MsgType: ");
		switch (joinMsg->head.type)
		{
		case Join:
			printf("join");
			break;

		case Leave:
			printf("Leave");

		case Event:
			printf("Event");
		default:
			break;
		}
		
		printf("\r\nconnections: %i\r\n", connections.size());
		switch (joinMsg->head.type)
		{
		case Join:
			//connections.push_back(Connection
			//	{
			//		newClientId,
			//		joinMsg->desc,
			//		joinMsg->form,
			//		first_avalible_Coordinate(),


			//	});
			for (size_t i = 0; i < connections.size(); i++)
			{
				if (joinMsg->head.id == connections[i].id)
				{
					connections[i].id = newClientId;
					connections[i].description = joinMsg->desc;
					connections[i].form = joinMsg->form;
					break;
				}
			}

			joinedAndMoved(newClientId);
			break;

		case Leave:
			for (size_t i = 0; i < connections.size(); i++)
			{
				if (leaveMsg->head.id == connections[i].id)
				{
					connections.erase(connections.begin() + i);
					connectionThreads.erase(connectionThreads.begin() + i);
				}
			}
			kicked(leaveMsg->head.id);
			break;

		case Event:
			printf("\treceiving: (%i, %i)\n", moveEvent->pos.x, moveEvent->pos.y);
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
			exit(1);
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
		
		//printf("%s\r\n", buf);
	}

// closes the socket after receiving a afk ping
	closesocket(s);
	WSACleanup();
}

const Coordinate first_avalible_Coordinate()
{
	bool found = false;
	Coordinate start = { -100, -100 };
	if (connections.size() == 0)
		return start;
	for (; start.y <= 100;)
	{
		found = true;
		for (Connection c : connections)
		{
			if (start.x > 100) // row is full so move to next one
			{
				start = { -100, start.y++ };
			}
			if (start == c.coord)
			{
				start.x++;
				found = false;
				break;
			}
		}
		if (found)
		{
			printf("sending: (%i, %i)", start.x, start.y);
			return start;
		}
	}
	// never happens (unless you have 10201 clients connected already connected)
	printf("Error in first_avalible_Coordinate(): no space left");
	return Coordinate{ 101, 101 };
}

int main()
{
	std::string ipAddress = "127.0.0.1";
	int starting_port = 54000; // linux_port

	SOCKET listening = setup_listening(ipAddress, starting_port);

	for (;;)
	{
		newClient = accept(listening, nullptr, nullptr);
		gameBoard.lock();

		
		// setting ObjectDesc and ObjectForm to 0
		connections.push_back({
			0,
			Human,
			Cube,
			first_avalible_Coordinate(),
			newClient
			});

		size_t last = connections.size() - 1;

		connectionThreads.push_back({
			});
		connectionThreads[last].recvThread = std::thread(receiving, connections[last].client);
		
		gameBoard.unlock();

		newClientId++;
		joinedAndMoved(newClientId);
	}

	// blocking until all threads have joined (which will never happen naturally due to the infinite loop)
	for (size_t i = 0; i < connections.size(); i++)
	{
		if (connectionThreads[i].recvThread.joinable())
		{
			connectionThreads[i].recvThread.join();
		}
	}
}
