source: https://ltu.instructure.com/courses/10174/assignments/82195
## Short specification

## Game start 
A Client send Join message to server with id=0 and sequence number=0; 
Server replies with a Message head where type= Join and id=clients assigned value. 
Server  update clients with ```NewPlayer``` Change message(s) including info about the new client and it’s id.
## Event messages
Clients send ```move``` request event to Server. 
Server may send ```NewPlayerPosition``` Change message(s) to connected clients.
## Remarks
All messages (except ```Join```) shall contain client id and sequence number.
Sequence number must increase for every message sent.
The Server decides actions in response to incoming messages.
Clients are only allowed to move 2D objects in response to incoming ```NewPlayerPosition``` messages. 
The 2D objects shall represent a figure according to the ObjectForm.
The game world coordinate system is (-100, -100 to 100, 100). 
