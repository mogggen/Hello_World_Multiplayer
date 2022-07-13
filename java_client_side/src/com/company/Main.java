package com.company;

import javax.swing.*;
import java.awt.*;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;

public class Main extends JComponent {

    // Coordinate system: -100 - 100
    // Resolution: 303x303

    public static class Client
    {
        int clientId;
        Coordinate position;
        ObjectDesc objectDesc; // used for color
        ObjectForm objectForm;
    }

    static ArrayList<Byte> posBuf = new ArrayList<>();
    static ArrayList<Byte> colorBuf = new ArrayList<>();
    static ArrayList<Client> info = new ArrayList<>();


    static ServerSocket ss;
    static Socket s;
    static InputStream in;
    static OutputStream out;

    //Enums och constants
    public int MAXNAMELEN = 32;

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

    public static class Coordinate {
        public int x;
        public int y;
    };

    enum MsgType {
        Join,           //Client joining game at server
        Leave,          //Client leaving game
        Change,         //Information to clients
        Event,          //Information from clients to server
        TextMessage     //Send text messages to one or all
    };
    //MESSAGE HEAD, Included first in all messages
    public static class MsgHead {
        public int length;     //Total length for whole message
        public int seqNo;      //Sequence number
        public int id;         //Client ID or 0;
        public MsgType type;   //Type of message
    };
    //JOIN MESSAGE (CLIENT->SERVER)
    public static class JoinMsg {
        public MsgHead head;
        public ObjectDesc desc;
        public ObjectForm form;
        //public char name[];   //null terminated!,or empty
    };
    //LEAVE MESSAGE (CLIENT->SERVER)
    public static class LeaveMsg {
        public MsgHead head;
    };
    //CHANGE MESSAGE (SERVER->CLIENT)
    enum ChangeType {
        NewPlayer,
        PlayerLeave,
        NewPlayerPosition
    };
    //Included first in all Change messages
    public static class ChangeMsg {
        public MsgHead head;
        public ChangeType type;
    };

    public static class NewPlayerMsg {
        public ChangeMsg msg;          //Change message header with new client id
        public ObjectDesc desc;
        public ObjectForm form;
        //public char name[];  //nullterminated!,or empty
    };
    public static class PlayerLeaveMsg {
        public ChangeMsg msg;          //Change message header with new client id
    };
    public static class NewPlayerPositionMsg {
        public ChangeMsg msg;          //Change message header
        public Coordinate pos;         //New object position
        public Coordinate dir;         //New object direction
    };
    //EVENT MESSAGE (CLIENT->SERVER)
    enum EventType { Move };
    //Included first in all Event messages
    public static class EventMsg {
        public MsgHead head;
        public EventType type;
    };
    //Variations of EventMsg
    public static class MoveEvent {
        public EventMsg event;
        public Coordinate pos;         //New object position
        public Coordinate dir;         //New object direction
    };
    //TEXT MESSAGE
    public static class TextMessageMsg {
        public MsgHead head;
        //public char text[];   //NULL-terminated array of chars.
    };

    static int seqNo = 0;
    static int id = -1;
    //static Coordinate pos = new Coordinate();
    static byte direction = -1;

    enum dir
    {
        up,
        down,
        left,
        right
    }

    public static Client getClient(int clientId)
    {
        for (Client c: info) {
            if (c.clientId == clientId){
                return c;
            }
        }
        return null;
    }

    public static void drawFigure(int clientId)
    {
        // check for attendance of the client
        Client curr = getClient(clientId);

        if (curr == null) {
            System.out.println("client not found, no corresponding figure was drawn");
            return;
        }

        switch (curr.objectForm){

        }
    }

    public static class GUI
    {
        JFrame frame = new JFrame();
        PixelCanvas canvas = new PixelCanvas();
        dir movel = dir.left;
        dir mover = dir.right;
        dir moved = dir.down;
        dir moveu = dir.up;

        // class constructor
        public GUI()
        {
            canvas.setBounds(0, 0, 303, 303);
            frame.setSize(303, 303);
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            frame.setTitle("GUI Server");
            frame.add(canvas);
            frame.pack();
            frame.setVisible(true);
            frame.addKeyListener(new KeyListener() {
                @Override
                public void keyTyped(KeyEvent e) {

                }

                @Override
                public void keyPressed(KeyEvent event) {
                    switch (event.getKeyCode()) {
                        case 27 -> System.exit(0);

                        case 87, 38 -> direction = (byte)moveu.ordinal();
                        case 83, 40 -> direction = (byte)moved.ordinal();
                        case 65, 37 -> direction = (byte)movel.ordinal();
                        case 68, 39 -> direction = (byte)mover.ordinal();
                    }
                    //final Runnable runnable = (Runnable) Toolkit.getDefaultToolkit().getDesktopProperty("win.sound.exclamation"); if (runnable != null) runnable.run();
                    try {
                        speak();
                    } catch (IOException exception) {
                        exception.printStackTrace();
                    }
                }

                @Override
                public void keyReleased(KeyEvent e) {

                }
            });
        }
    }

    static int Fit(int input, int inputStart, int inputEnd)
    {
        return (int)(((float) 255) / (inputEnd - inputStart) * (input - inputStart));
    }
    //handles the drawing of the input value
    static class PixelCanvas extends JComponent
    {
        ArrayList<Byte> arr;
        ArrayList<Byte> color;
        void SetParamArr(ArrayList<Byte> arr, ArrayList<Byte> color)
        {
            this.arr = arr;
            this.color = color;
        }

        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);
            if (arr != null && color != null) {
                for (int i = 0; i < arr.size(); i++){
                    if (color.get(i) == 0){
                        return;
                    }
                    int up = Fit(color.get(i), 1, 16);
                    int down = Fit(color.get(i), 16, 1);
                    g.setColor(new Color(down, up, down));
                    g.fillRect(arr.get(i), arr.get(i+1), 1, 1);
                }
            }
        }

        @Override
        public Dimension getPreferredSize() {
            return new Dimension(505, 505);
        }
    }

    public static void runSetup(int port) throws IOException {
        ss = new ServerSocket(port);

        s = ss.accept();
        in = s.getInputStream();
        out = s.getOutputStream();

        // send joinMsg and recevive

        byte[] buf = new byte[7]; // bytes in joinMsg

        //joinMsg.head.length
        buf[0] = 7;

        //joinMsg.head.seqNo
        buf[1] = (byte)++seqNo;

        //joinMsg.head.id
        buf[2] = (byte)id;

        //joinMsg.head.MsgType
        buf[3] = (byte)MsgType.Join.ordinal();

        //joinMsg.ObjectDesc
        buf[4] = (byte)ObjectDesc.Human.ordinal();

        //joinMsg.ObjectForm
        buf[5] = (byte) ObjectForm.Cone.ordinal();

        //joinMsg.name
        buf[6] = (byte)'-'; // ascii: '-'

        out.write(buf);
        System.out.println("client connected");
        System.out.println("forwarded joinMsg...");
    }

    //Reads Bytes from the input stream
    public static void listen(GUI window) throws IOException {

        byte[] buf = new byte[10];
        int count = in.read(buf);
        if (count == -1)
        {
            System.out.println("Error: unable to receive");
            System.exit(1);
        }

        seqNo = buf[1];
        System.out.println("attempting to handle incoming ChangeMsg...");
        System.out.println("Seqence Number: " + seqNo);
        if ((byte)ChangeType.NewPlayer.ordinal() == buf[4]){
            System.out.println("");
            NewPlayerMsg newPlayerMsg = new NewPlayerMsg();
            newPlayerMsg.msg = new ChangeMsg();
            newPlayerMsg.msg.head = new MsgHead();

            newPlayerMsg.msg.head.length = buf[0];
            newPlayerMsg.msg.head.seqNo = buf[1];
            newPlayerMsg.msg.head.id = buf[2];
            newPlayerMsg.msg.head.type = MsgType.values()[buf[3]]; // should always be MsgType.Change
            newPlayerMsg.msg.type = ChangeType.values()[buf[4]]; // should always be ChangeType.NewPlayer

            newPlayerMsg.desc = ObjectDesc.values()[buf[5]];
            newPlayerMsg.form = ObjectForm.values()[buf[6]];

            System.out.println("resulting newPlayerMsg:");
            System.out.println(newPlayerMsg.msg.head.length);
            System.out.println(newPlayerMsg.msg.head.seqNo);
            System.out.println(newPlayerMsg.msg.head.id);
            System.out.println(newPlayerMsg.msg.head.type); // should always be MsgType.Change
            System.out.println(newPlayerMsg.msg.type); // should always be ChangeType.Change
            System.out.println(newPlayerMsg.desc);
            System.out.println(newPlayerMsg.form);
        }

        if ((byte)ChangeType.PlayerLeave.ordinal() == buf[4]){
            PlayerLeaveMsg playerLeaveMsg = new PlayerLeaveMsg();
            playerLeaveMsg.msg = new ChangeMsg();
            playerLeaveMsg.msg.head = new MsgHead();

            playerLeaveMsg.msg.head.length = buf[0];
            playerLeaveMsg.msg.head.seqNo = buf[1];
            playerLeaveMsg.msg.head.id = buf[2];
            playerLeaveMsg.msg.head.type = MsgType.values()[buf[3]];
            playerLeaveMsg.msg.type = ChangeType.values()[buf[4]]; // should always be ChangeType.PlayerLeave

            System.out.println("resulting playerLeaveMsg:");
            System.out.println(playerLeaveMsg.msg.head.length);
            System.out.println(playerLeaveMsg.msg.head.seqNo);
            System.out.println(playerLeaveMsg.msg.head.id);
            System.out.println(playerLeaveMsg.msg.head.type);
            System.out.println(playerLeaveMsg.msg.type);
        }

        if ((byte)ChangeType.NewPlayerPosition.ordinal() == buf[4]){
            NewPlayerPositionMsg newPlayerPositionMsg = new NewPlayerPositionMsg();
            newPlayerPositionMsg.msg = new ChangeMsg();
            newPlayerPositionMsg.msg.head = new MsgHead();
            newPlayerPositionMsg.pos = new Coordinate();

            newPlayerPositionMsg.msg.head.length = buf[0];
            newPlayerPositionMsg.msg.head.seqNo = buf[1];
            newPlayerPositionMsg.msg.head.id = buf[2];
            newPlayerPositionMsg.msg.head.type = MsgType.values()[buf[3]]; // should always be MsgType.Change
            newPlayerPositionMsg.msg.type = ChangeType.values()[buf[4]]; // should always be ChangeType.NewPlayerPosition

            newPlayerPositionMsg.pos.x = buf[5];
            newPlayerPositionMsg.pos.y = buf[6];

            System.out.println("resulting newPlayerPositionMsg:");
            System.out.println(newPlayerPositionMsg.msg.head.length);
            System.out.println(newPlayerPositionMsg.msg.head.seqNo);
            System.out.println(newPlayerPositionMsg.msg.head.id);
            System.out.println(newPlayerPositionMsg.msg.head.type);
            System.out.println(newPlayerPositionMsg.msg.type);
            System.out.println(newPlayerPositionMsg.pos.x);
            System.out.println(newPlayerPositionMsg.pos.y);
        }

        //find the client that needs to be redrawn otherwise draw the newly joined client

        posBuf.clear();
        colorBuf.clear();
        for (int i = 0; i < info.size(); i++) {
            Coordinate center = info.get(i).position;
            int descColor = Fit(info.get(i).objectDesc.ordinal(), 16, 1);
            switch (info.get(i).objectForm) {
                case Cone, Pyramid -> {
                    posBuf.add((byte) (center.x));
                    posBuf.add((byte) (center.y + 1));
                    colorBuf.add((byte)descColor);

                    posBuf.add((byte) (center.x - 1));
                    posBuf.add((byte) (center.y));
                    colorBuf.add((byte)descColor);

                    posBuf.add((byte) (center.x));
                    posBuf.add((byte) (center.y));
                    colorBuf.add((byte)descColor);

                    posBuf.add((byte) (center.x + 1));
                    posBuf.add((byte) (center.y));
                    colorBuf.add((byte)descColor);

                    posBuf.add((byte) (center.x));
                    posBuf.add((byte) (center.y - 1));
                    colorBuf.add((byte)descColor); // romb
                }
            }
        }

        //Reformat
//        for (int k = 0; k < byteBuf.size(); k += 3) {
//            info.add(
//                    Byte.toUnsignedInt(byteBuf.get(k)),
//                    Byte.toUnsignedInt(byteBuf.get(k + 1)),
//                    Byte.toUnsignedInt(byteBuf.get(k + 2)))); // always a multiple of three
//            System.out.print(
//                    (byteBuf.get(k)+(byte)48) +
//                    (byteBuf.get(k+1)+(byte)48) +
//                    (byteBuf.get(k+2)+(byte)48));
//            info.clear();
//            info.add(new Pixel(Byte.toUnsignedInt(byteBuf.get(k)), Byte.toUnsignedInt(byteBuf.get(k+1)), Byte.toUnsignedInt(byteBuf.get(k+2))));
//        }
        window.canvas.SetParamArr(posBuf, colorBuf);
        window.frame.repaint();
    }

    public static void speak() throws IOException
    {
        //new position
        switch (direction)
        {
            case -1:
                break;
            case 0:
                getClient(id).position.y -= 1;
                break;
            case 1:
                getClient(id).position.y += 1;
                break;
            case 2:
                getClient(id).position.x -= 1;
                break;
            case 3:
                getClient(id).position.x += 1;
                break;
            default:
                System.out.println("Error in speak() switch");
                return;
        }
        byte[] pl = new byte[7];
        // send more basic values
        pl[0] = 1;
        pl[1] = 2;
        pl[2] = 3;
        pl[3] = 4;
        pl[4] = 5;
        pl[5] = 6;
        pl[6] = 7;

        pl[0] = ((byte)7);
        pl[1] = ((byte)++seqNo);
        pl[2] = ((byte)id);
        pl[3] = ((byte)MsgType.Event.ordinal());
        pl[4] = ((byte)EventType.Move.ordinal());
        pl[5] = ((byte)getClient(id).position.x);
        pl[6] = ((byte)getClient(id).position.y);


        System.out.println(pl);
        out.write(pl);
    }


    public static void main(String[] args) throws IOException {
        GUI window = new GUI();
        runSetup(4999);
        while(true) {
            // no specific protocol needs to be used
            try {
                listen(window);
            }catch (SocketException s){
                s.printStackTrace();
                System.exit(1);
            }
            System.out.println("received data");
        }
    }
}