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
        public char name[];   //null terminated!,or empty
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
        public char name[];  //nullterminated!,or empty
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
        public char text[];   //NULL-terminated array of chars.
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
            return new Dimension(303, 303);
        }
    }

    public static byte[] intToBytes(int i)
    {
        byte[] result = new byte[4];

        result[0] = (byte)(i >> 24);
        result[1] = (byte)(i >> 16);
        result[2] = (byte)(i >> 8);
        result[3] = (byte)i;

        return result;
    }

    public static void runSetup(int port) throws IOException {
        ss = new ServerSocket(port);

        s = ss.accept();
        in = s.getInputStream();
        out = s.getOutputStream();

        ArrayList<Byte> joinBuffer = new ArrayList<>();

        //joinMsg.head.length
        byte[] buf = intToBytes(28); // bytes in joinMsg
        for (byte b : buf){
            joinBuffer.add(b);
        }

        //joinMsg.head.seqNo
        buf = intToBytes(++seqNo);
        for (byte b : buf){
            joinBuffer.add(b);
        }

        //joinMsg.head.id
        buf = intToBytes(id);
        for (byte b : buf){
            joinBuffer.add(b);
        }

        //joinMsg.head.MsgType
        buf = intToBytes(0); // MsgType.Join
        for (byte b : buf){
            joinBuffer.add(b);
        }

        //joinMsg.ObjectDesc
        buf = intToBytes(0); // ObjectDesc.Human
        for (byte b : buf){
            joinBuffer.add(b);
        }

        //joinMsg.ObjectForm
        buf = intToBytes(3); // ObjectForm.Cone
        for (byte b : buf){
            joinBuffer.add(b);
        }

        //joinMsg.name
        buf = intToBytes(45); // ascii: '-'
        for (byte b : buf){
            joinBuffer.add(b);
        }

        byte[] firstMessage = new byte[1024];
        for (int v = 0; v < joinBuffer.size(); v++){
            firstMessage[v] = joinBuffer.get(v);
        }
        firstMessage[joinBuffer.size()] = '\0';

        out.write(firstMessage);
        System.out.println("client connected");
        System.out.println("forwarded joinMsg...");
    }

    //Reads Bytes from the input stream
    public static void listen(GUI window) throws IOException {

        byte[] buf = new byte[1024];
        int count = in.read(buf);
        if (count == -1)
        {
            System.out.println("Error: unable to receive");
            System.exit(1);
        }

        ByteBuffer bb = ByteBuffer.wrap(buf);

        NewPlayerMsg newPlayerMsg = new NewPlayerMsg();
        newPlayerMsg.msg = new ChangeMsg();
        newPlayerMsg.msg.head = new MsgHead();

        newPlayerMsg.msg.head.length = bb.getInt();
        newPlayerMsg.msg.head.seqNo = bb.getInt();
        newPlayerMsg.msg.head.id = bb.getInt();
        newPlayerMsg.msg.head.type = MsgType.Change;
        newPlayerMsg.msg.type = ChangeType.values()[bb.getInt()];

        newPlayerMsg.desc = ObjectDesc.values()[bb.getInt()];
        newPlayerMsg.form = ObjectForm.values()[bb.getInt()];
        newPlayerMsg.name = new char[0];

        bb = ByteBuffer.wrap(buf);
        PlayerLeaveMsg playerLeaveMsg = new PlayerLeaveMsg();
        playerLeaveMsg.msg = new ChangeMsg();
        playerLeaveMsg.msg.head = new MsgHead();

        playerLeaveMsg.msg.head.length = bb.getInt();
        playerLeaveMsg.msg.head.seqNo = bb.getInt();
        playerLeaveMsg.msg.head.id = bb.getInt();
        playerLeaveMsg.msg.head.type = MsgType.Change;
        playerLeaveMsg.msg.type = ChangeType.values()[bb.getInt()];

        bb = ByteBuffer.wrap(buf);
        NewPlayerPositionMsg newPlayerPositionMsg = new NewPlayerPositionMsg();
        newPlayerPositionMsg.msg = new ChangeMsg();
        newPlayerPositionMsg.msg.head = new MsgHead();

        newPlayerPositionMsg.msg.head.length = bb.getInt();
        newPlayerPositionMsg.msg.head.seqNo = bb.getInt();
        newPlayerPositionMsg.msg.head.id = bb.getInt();
        newPlayerPositionMsg.msg.head.type = MsgType.Change;
        newPlayerPositionMsg.msg.type = ChangeType.values()[bb.getInt()];

        newPlayerPositionMsg.pos.x = bb.getInt();
        newPlayerPositionMsg.pos.y = bb.getInt();
        newPlayerPositionMsg.dir.x = bb.getInt();
        newPlayerPositionMsg.dir.y = bb.getInt();

        switch (newPlayerMsg.msg.type) {
            case NewPlayer -> {
                System.out.print('[' + newPlayerMsg.msg.head.seqNo + "]:\tplayer joined\tId=" + newPlayerMsg.msg.head.id);
                Client cli = new Client();
                //for each (client c in clientList)
                if (id == -1)
                {
                    //newPlayerMsg.msg.head.length;
                    id = newPlayerMsg.msg.head.id;
                    //pos = newPlayerPositionMsg.pos; // 'pos' not necessary as variable
                }
                cli.clientId = id;
                cli.objectForm = newPlayerMsg.form;
                info.add(cli);
            }
            case PlayerLeave -> {
                if (playerLeaveMsg.msg.head.id == id) {
                    System.out.println("you lost connection to the server");
                    System.exit(1);
                }
                else
                {
                    for (Client c : info){
                        if (c.clientId == playerLeaveMsg.msg.head.id)
                        {
                            System.out.println(playerLeaveMsg.msg.head.id + " lost connection to the server");
                            info.remove(c);
                        }
                    }
                }
            }
            case NewPlayerPosition -> {
                System.out.println('[' + newPlayerPositionMsg.msg.head.seqNo + "]:\tpos:(" +
                        newPlayerPositionMsg.pos.x + ";" +
                        newPlayerPositionMsg.pos.y + ")\tid=" + newPlayerPositionMsg.msg.head.id);
                for (Client c : info){

                    if (c.clientId == id) {
                        c.position = newPlayerPositionMsg.pos;
                    }
                }
            }
            default -> {
                System.out.println("pause debugger...\n");
                System.in.read();
            }
        }

        // CLient.coord, Fit(objectDesc) -> 3 ints
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
        ArrayList<Byte> pl = new ArrayList<>();
        pl.add(intToBytes(getClient(id).position.x)[0]);
        pl.add(intToBytes(getClient(id).position.x)[1]);
        pl.add(intToBytes(getClient(id).position.x)[2]);
        pl.add(intToBytes(getClient(id).position.x)[3]);

        pl.add(intToBytes(getClient(id).position.y)[0]);
        pl.add(intToBytes(getClient(id).position.y)[1]);
        pl.add(intToBytes(getClient(id).position.y)[2]);
        pl.add(intToBytes(getClient(id).position.y)[3]);

        pl.add((byte)7);
        byte[] first = new byte[1024];
        for (int v = 0; v < pl.size(); v++){
            first[v] = pl.get(v);
        }
        first[pl.size()] = '\0';
        out.write(first);
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