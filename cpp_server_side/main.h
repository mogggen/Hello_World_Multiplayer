//Enums och constants
int MAXNAMELEN = 32;

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
    int length;     //Total length for whole message
    int seqNo;      //Sequence number
    int id;         //Client ID or 0;
    MsgType type;   //Type of message
};
//JOIN MESSAGE (CLIENT->SERVER)
struct JoinMsg {
    MsgHead head;
    ObjectDesc desc;
    ObjectForm form;
    char name[1];   //null terminated!,or empty
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
    char name[1];  //nullterminated!,or empty
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

enum dir
{
	up,
	down,
	left,
	right
};
