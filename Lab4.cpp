#include <iostream>
#include <ctime>
#include <string>
#include <cstring>
#include <WS2tcpip.h>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

//Enums och constants
#define MAXNAMELEN  32 

enum ObjectDesc {
    Human,
    NonHuman,
    Vehicle,
    StaticObject
};

enum ObjectForm {
    Cube,
    Sphere,
    Pyramid,
    Cone
};

struct Coordinate {
    int x;
    int y;
};

enum MsgType {
    Join,           //Client joining game at server
    Leave,          //Client leaving game
    Change,         //Information to clients
    Event,          //Information from clients to server
    TextMessage     //Send text messages to one or all
};
//MESSAGE HEAD, Included first in all messages   
struct MsgHead {
    unsigned int length;     //Total length for whole message   
    unsigned int seqNo;      //Sequence number
    unsigned int id;         //Client ID or 0;
    MsgType type;            //Type of message
};
//JOIN MESSAGE (CLIENT->SERVER)
struct JoinMsg {
    MsgHead head;
    ObjectDesc desc;
    ObjectForm form;
    char name[MAXNAMELEN];   //null terminated!,or empty
};
//LEAVE MESSAGE (CLIENT->SERVER)
struct LeaveMsg {
    MsgHead head;
};
//CHANGE MESSAGE (SERVER->CLIENT)
enum ChangeType {
    NewPlayer,
    PlayerLeave,
    NewPlayerPosition
};
//Included first in all Change messages
struct ChangeMsg {
    MsgHead head;
    ChangeType type;
};

struct NewPlayerMsg {
    ChangeMsg msg;          //Change message header with new client id
    ObjectDesc desc;
    ObjectForm form;
    char name[MAXNAMELEN];  //nullterminated!,or empty
};
struct PlayerLeaveMsg {
    ChangeMsg msg;          //Change message header with new client id
};
struct NewPlayerPositionMsg {
    ChangeMsg msg;          //Change message header
    Coordinate pos;         //New object position
    Coordinate dir;         //New object direction
};
//EVENT MESSAGE (CLIENT->SERVER)
enum EventType { Move };
//Included first in all Event messages
struct EventMsg {
    MsgHead head;
    EventType type;
};
//Variantions of EventMsg
struct MoveEvent {
    EventMsg event;
    Coordinate pos;         //New object position
    Coordinate dir;         //New object direction
};
//TEXT MESSAGE
struct TextMessageMsg {
    MsgHead head;
    char text[1];   //NULL-terminerad array of chars.
};

using namespace std;

void ReadServer(SOCKET sock)
{
    NewPlayerPositionMsg* msg;
    char buf[MAXNAMELEN];
    recv(sock, buf, sizeof(buf), 0);
    msg = (NewPlayerPositionMsg*)buf;

    cout << "x = " << msg->pos.x << " y = " << msg->pos.y << endl;
}

void move(SOCKET sock, Coordinate cord, int clientid)
{
    MoveEvent moving =
    {
        {{0, 0, clientid, Event}, Move},
        {cord},
        {0, 0}
    };
    moving.event.head.length = sizeof(MoveEvent);
    send(sock, (char*)&moving, moving.event.head.length, 0);
}


int main()
{
    string ipAddress = "130.240.40.7";  // IP Address of the server
    int LinuxPort = 49153;                   // Linux Server port

    // Initialize WinSock
    WSAData data;
    WORD ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);
    if (wsResult != 0)
    {
        cerr << "Can't start Winsock, Err #" << wsResult << endl;
        return 0;
    }

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    // Fill in a hint structure
    sockaddr_in sockaddr_in;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(LinuxPort);
    inet_pton(AF_INET, ipAddress.c_str(), &sockaddr_in.sin_addr);

    // Error Handeling
    int conRes = connect(sock, (sockaddr*)&sockaddr_in, sizeof(sockaddr_in));
    if (conRes == SOCKET_ERROR)
    {
        string prompt = "Can't connect to server, Error: ";
        int error = WSAGetLastError();
        if (error == 10061)
            cerr << prompt << "timeout" << endl;
        else
            cerr << prompt << '#' << error << endl;
        closesocket(sock);
        WSACleanup();
        return 0;
    }

    int x = -100, y = -100; //set from recv() later
    Coordinate cord{ x, y };
    char buf[MAXNAMELEN];
    char command[MAXNAMELEN];

    thread listen(ReadServer, sock);

    JoinMsg joining
    {
        {0, 0, 0, Join},
        Human,
        Pyramid,
        "Connor"
    };
    joining.head.length = sizeof(joining);

    send(sock, (char*)&joining, joining.head.length, 0);
    recv(sock, buf, sizeof(buf), 0);
    MsgHead* msgHead = (MsgHead*)buf;

    int clientid = msgHead->id;
    joining.head.id = clientid;

    cout << "id: " << msgHead->id << endl;
    cout << "length: " << msgHead->length << endl;
    cout << "seqNo: " << msgHead->seqNo << endl;
    cout << "type: " << msgHead->type << endl << endl;

        
        
    // while loop to send data
    while (true)
    {
        //send to linux server
            

        //recive from linux server
        //recv(sock, buf, sizeof(buf), 0);

        cout << "> ";
        scanf_s("%s", command, 16);
        for (char i = 0; i < sizeof(command) / sizeof(char); i++)
        {
            command[i] = tolower(command[i]);
        }

        if (strcmp(command, "moveu") == 0)
        {
            cord.y += 1;
            move(sock, cord, clientid);
        }
        if (strcmp(command, "moved") == 0)
        {
            cord.y -= 1;
            move(sock, cord, clientid);
        }
        if (strcmp(command, "movel") == 0)
        {
            cord.x -= 1;
            move(sock, cord, clientid);
        }
        if (strcmp(command, "mover") == 0)
        {
            cord.x += 1;
            move(sock, cord, clientid);
        }

        if (strcmp(command, "leave") == 0)
        {

        }
            
        //send to java client
    }

    // Close everything
    listen.join();
    Sleep(500);
    closesocket(sock);
    WSACleanup();
    return 0;
}
