#include <iostream>
#include <ctime>
#include <string>
#include <cstring>
#include <WS2tcpip.h>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

//Enums och constants
#define MAXNAMELEN  32 

using namespace std;

int main()
{
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





    string ipAddress = "130.240.40.7";// IP Address of the server
    int port = 49152;// Listening port on the server

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
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    // Error Handeling
    int conRes = connect(sock, (sockaddr*)&hint, sizeof(hint));
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

    JoinMsg joining
    {
        {0, 0, 0, Join},
        Human,
        Pyramid,
        "Connor"
    };
    joining.head.length = sizeof(joining);

    // while loop to send data
    char buf[9123];

    //send to linux server
    send(sock, (char*)&joining, joining.head.length, 0);
    
    //recive from linux server
    recv(sock, buf, sizeof(buf), 0);
    MsgHead* msg = (MsgHead*)buf;
    cout << msg << endl;

    //send to java client
    string input;
    getline(cin, input);


    // Close everything
    closesocket(sock);
    WSACleanup();
    return 0;
}
