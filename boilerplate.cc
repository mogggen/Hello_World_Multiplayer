void move(SOCKET sock, int clientid)
{
	MoveEvent moving =
	{
		{{0, 0, clientid, Event}, Move},
		{x, y},
		{0, 0}
	};
	moving.event.head.length = sizeof(MoveEvent);
	send(sock, (char*)&moving, moving.event.head.length, 0);
}

void leave(SOCKET sock, int clientid)
{
	LeaveMsg leaveMsg =
	{
		{0, 0, clientid, Leave}
	};
	leaveMsg.head.length = sizeof(LeaveMsg);
	send(sock, (char*)&leaveMsg, leaveMsg.head.length, 0);
}

void ConnectServer(SOCKET sock)
{
	char buf[192];
	MsgHead* msgHead = (MsgHead*)buf;

	ChangeMsg* changeMsg = (ChangeMsg*)buf;

	NewPlayerMsg* newPlayerMsg = (NewPlayerMsg*)buf;
	PlayerLeaveMsg* playerLeaveMsg = (PlayerLeaveMsg*)buf;
	NewPlayerPositionMsg* newPlayerPositionMsg = (NewPlayerPositionMsg*)buf;
	int id = -1;


	while (loop)
	{
		msgHead = (MsgHead*)buf;

		changeMsg = (ChangeMsg*)buf;

		newPlayerMsg = (NewPlayerMsg*)buf;
		playerLeaveMsg = (PlayerLeaveMsg*)buf;
		newPlayerPositionMsg = (NewPlayerPositionMsg*)buf;

		int count = recv(sock, buf, sizeof(buf), 0);
		using namespace std;
		while (msgHead <= (MsgHead*)(buf + sizeof(buf) / sizeof(*buf)))
		{
			switch (changeMsg->type)
			{
			case NewPlayer:
				std::cout << '[' << newPlayerMsg->msg.head.seqNo << "]:\t" <<
					newPlayerMsg->name << " joined\tId=" << newPlayerMsg->msg.head.id << " " << endl;
				//for each (client c in clientList)
				if (id == -1)
				{
					id = newPlayerMsg->msg.head.id;
					SetConsoleTitle((string(newPlayerMsg->name) + string(": id=") + to_string(id)).c_str());
					move(sock, id); // dummy command to recive the player coordinates
				}
				break;

			case PlayerLeave:
				std::cout << '[' << playerLeaveMsg->msg.head.seqNo << "]:\tleft server\tId=" << playerLeaveMsg->msg.head.id << endl;
				if (playerLeaveMsg->msg.head.id == id) loop = true;
				break;

			case NewPlayerPosition:
				std::cout << '[' << newPlayerPositionMsg->msg.head.seqNo << "]:\tpos:(";

				std::cout << newPlayerPositionMsg->pos.x << ";";
				std::cout << newPlayerPositionMsg->pos.y << ")\tid=" << newPlayerPositionMsg->msg.head.id << endl;
				if (newPlayerPositionMsg->msg.head.id == id)
				{
					x = newPlayerPositionMsg->pos.x;
					y = newPlayerPositionMsg->pos.y;
				}
				break;

			default:
				std::cout << "pause debugger..." << endl;
				cin.get();
				break;
			}

			msgHead = (MsgHead*)(msgHead + msgHead->length);

			newPlayerMsg = (NewPlayerMsg*)msgHead;
			playerLeaveMsg = (PlayerLeaveMsg*)msgHead;
			newPlayerPositionMsg = (NewPlayerPositionMsg*)msgHead;
		}
	}
}

char Linux_buf[MAXNAMELEN];
char java_buf[MAXNAMELEN];
char command[MAXNAMELEN];

JoinMsg joining
{
	{0, 0, 0, Join},
	Human,
	Pyramid,
	"Lisa"
};
joining.head.length = sizeof(joining);

/*send(Linux_listening, (char*)&joining, joining.head.length, 0);
recv(Linux_listening, Linux_buf, sizeof(Linux_buf), 0);*/
MsgHead* msgHead = (MsgHead*)Linux_buf;

int clientid = msgHead->id;
joining.head.id = clientid;

send(java_listening, (char*)&joining, joining.head.length, 0);
recv(java_listening, java_buf, sizeof(java_buf), 0);