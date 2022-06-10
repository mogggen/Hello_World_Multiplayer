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
class MsgHead {
    int length;     //Total length for whole message
    int seqNo;      //Sequence number
    int id;         //Client ID or 0;
    MsgType type;   //Type of message
};
//JOIN MESSAGE (CLIENT->SERVER)
class JoinMsg {
    MsgHead head;
    ObjectDesc desc;
    ObjectForm form;
    char name[];   //null terminated!,or empty
};
//LEAVE MESSAGE (CLIENT->SERVER)
class LeaveMsg {
    MsgHead head;
};
//CHANGE MESSAGE (SERVER->CLIENT)
enum ChangeType {
    NewPlayer,
    PlayerLeave,
    NewPlayerPosition
};
//Included first in all Change messages
class ChangeMsg {
    MsgHead head;
    ChangeType type;
};

class NewPlayerMsg {
    ChangeMsg msg;          //Change message header with new client id
    ObjectDesc desc;
    ObjectForm form;
    char name[];  //nullterminated!,or empty
};
class PlayerLeaveMsg {
    ChangeMsg msg;          //Change message header with new client id
};
class NewPlayerPositionMsg {
    ChangeMsg msg;          //Change message header
    Coordinate pos;         //New object position
    Coordinate dir;         //New object direction
};
//EVENT MESSAGE (CLIENT->SERVER)
enum EventType { Move };
//Included first in all Event messages
class EventMsg {
    MsgHead head;
    EventType type;
};
//Variantions of EventMsg
class MoveEvent {
    EventMsg eventMsg;
    Coordinate pos;         //New object position
    Coordinate dir;         //New object direction
};
//TEXT MESSAGE
class TextMessageMsg {
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
