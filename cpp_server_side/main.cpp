#include <iostream>
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

bool operator==(const Coordinate& lhs, const Coordinate& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

struct Connection
{
	unsigned int id;

	ObjectDesc description;
	ObjectForm form;
	Coordinate coord;

	SOCKET client;
};

SOCKET newClient;
unsigned int newClientId = 0u;
std::mutex gameBoard;
std::vector<Connection> connections;
std::vector<std::thread> threads;
std::vector<NewPlayerMsg> prevNewMsg;
std::vector<NewPlayerPositionMsg> prevNewPosMsg;
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
			if (sqrt((newPos.x - c.coord.x) * (newPos.x - c.coord.x) + (newPos.y - c.coord.y) * (newPos.y - c.coord.y)) > 1.)
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
	for (const Connection& c : connections)
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
	for (Connection& c : connections)
	{
		if (c.id == clientid)
		{
			c.coord.x = newPos.x;
			c.coord.y = newPos.y;
		}
		printf("\r\nClient socket %llu, id %i: (%i, %i)\r\n", c.client, c.id, c.coord.x, c.coord.y);
	}
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

	// broadcasts to the new player all the current players' positions
	for (size_t i = 0; i < connections.size() - 1; i++)
	{
		char* buf3 = serialize(&prevNewMsg[i]);
		sendAll(buf3, prevNewMsg[i].msg.head.length);
		
		bool foundClient = false;
		for (NewPlayerPositionMsg& n : prevNewPosMsg)
		{
			if (n.msg.head.id == connections[i].id)
			{
				n.pos.x = connections[i].coord.x;
				n.pos.y = connections[i].coord.y;
				foundClient = true;
				break;
			}
		}

		if (foundClient)
		{
			char* buf4 = serialize(&prevNewPosMsg[i]);
			sendAll(buf4, prevNewPosMsg[i].msg.head.length);
		}
		else
		{
			std::cout << "test array lengths" << std::endl
				<< "ConnectionsId: ";
			for (size_t i = 0; i < connections.size(); i++)
			{
				std::cout << " " << connections[i].id;
			}
			std::cout << "\r\nprevNewMsgId: ";
			for (size_t i = 0; i < prevNewMsg.size(); i++)
			{
				std::cout << " " << prevNewMsg[i].msg.head.id;
			}
			std::cout << "\r\nprevNewPosMsgId: ";
			for (size_t i = 0; i < prevNewPosMsg.size(); i++)
			{
				std::cout << " " << prevNewPosMsg[i].msg.head.id;
			}


			std::cout << "\r\n\r\nedge cases found: ";
			if (prevNewMsg.size() > connections.size())
			{
				// a player has left the game and their position shouldn't be broadcasted anymore
				for (size_t i = 0; i < prevNewMsg.size();)
				{
					bool foundDelta = false;
					for (const Connection& c : connections)
					{
						if (prevNewMsg[i].msg.head.id == c.id)
						{
							foundDelta = true;
							i++;
							break;
						}
					}
					if (!foundDelta)
					{
						std::cout << " " << prevNewMsg[i].msg.head.id;
						prevNewMsg.erase(prevNewMsg.begin() + i);
					}
				}
			}
			if (prevNewMsg.size() != connections.size() || prevNewMsg.size() != prevNewPosMsg.size())
			{
				std::cerr << "Huh?" << std::endl;
			}
		}
	}
	prevNewMsg.push_back(newPlayerMsg);
	prevNewPosMsg.push_back(newPlayerPositionMsg);
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

void sendAll(const char* buf, const int& len)
{
	gameBoard.lock();
	for (size_t i = 0; i < connections.size(); i++)
	{
		int attempt = send(connections[i].client, buf, len, 0);
		if (attempt < 1)
		{
			closesocket(connections[i].client);
		}
	}
	gameBoard.unlock();
}

SOCKET setup_listening(const int& listening_port)
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
			closesocket(s);
			return;
		}

		JoinMsg* joinMsg = new JoinMsg();
		deserialize(buf, joinMsg); // check the resulting deserialization
		LeaveMsg* leaveMsg = new LeaveMsg();
		deserialize(buf, leaveMsg);
		MoveEvent* moveEvent = new MoveEvent();
		deserialize(buf, moveEvent);

		printf("\r\nMsgType: ");
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
		
		switch (joinMsg->head.type)
		{
		case Join:
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
					closesocket(s);
					return;
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
	}
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
		for (const Connection& c : connections)
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

void joinThreads()
{
	for (;;)
	{
		for (size_t i = 0; i < threads.size(); i++)
		{
			if (threads[i].joinable())
			{
				threads[i].join();
				connections.erase(connections.begin() + i);
				try
				{
					threads.erase(threads.begin() + i);
					printf("Connections: %llu\r\n", connections.size());
				}
				catch(...)
				{
					std::cerr << "Error joining threads" << std::endl;
				}
			}
		}
	}
}

int main()
{
	std::thread yes_platinum = std::thread(joinThreads);
	SOCKET listening = setup_listening(54000);

	for (;;)
	{
		newClient = accept(listening, nullptr, nullptr);
		if (newClient == 0xFFFFFFFFFFFFFFFF) // occurs when a connections is interrupted unexpectedly
		{
			newClient = 0x0; // default value
			continue;
		}

		newClientId++;
		// setting ObjectDesc and ObjectForm to 0
		connections.push_back({
			newClientId,
			Human,
			Cube,
			first_avalible_Coordinate(),
			newClient
			});

		size_t last = connections.size() - 1;

		threads.push_back(std::thread(receiving, connections[last].client));

		joinedAndMoved(newClientId);
	}
	closesocket(newClient);
	WSACleanup();
}
