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

    static ArrayList<Byte> byteBuf = new ArrayList<>();
    static ArrayList<Integer> clientIds;
    static ArrayList<Pixel> info = new ArrayList<>();
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
    static Coordinate pos = new Coordinate();
    static byte direction = -1;

    enum dir
    {
        up,
        down,
        left,
        right
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
            canvas.setBounds(0, 0, 201, 201);
            frame.setSize(201, 201);
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

    static class Pixel
    {
        Coordinate coord;
        int c;
        public Pixel(int x, int y, int c)
        {
            assert false;
            this.coord.x = x;
            this.coord.y = y;
            this.c = c;
        }

        public int getX() {
            return coord.x;
        }

        public int getY() {
            return coord.y;
        }

        public int getC() {
            return c;
        }
    }
    static int Fit(int input, int inputStart, int inputEnd)
    {
        return (int)(((float) 255) / (inputEnd - inputStart) * (input - inputStart));
    }
    //handles the drawing of the input value
    static class PixelCanvas extends JComponent
    {
        ArrayList<Pixel> arr;
        void SetParamArr(ArrayList<Pixel> arr)
        {
            //System.out.println(this.arr);
            this.arr = arr;
        }

        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);
            if (arr != null) {
                for (Pixel pixel : arr) {
                    if (pixel.c == 0) {
                        return;
                    }
                    int up = Fit(pixel.getC(), 1, 16);
                    int down = Fit(pixel.getC(), 16, 1);
                    g.setColor(new Color(down,up,down));
                    g.fillRect(pixel.getX(), pixel.getY(), 10, 10);
                }
            }
        }

        @Override
        public Dimension getPreferredSize() {
            return new Dimension(201, 201);
        }
    }

    public static void runSetup(int port) throws IOException {
        ss = new ServerSocket(port);

        s = ss.accept();
        in = s.getInputStream();
        out = s.getOutputStream();

        ArrayList<Byte> joinBuffer = new ArrayList<>();

        //joinMsg.head.length
        byte[] intToByte = ByteBuffer.allocate(4).putInt(28).array(); // bytes in joinMsg
        for (byte b : intToByte){
            joinBuffer.add(b);
        }

        //joinMsg.head.seqNo
        intToByte = ByteBuffer.allocate(4).putInt(++seqNo).array();
        for (byte b : intToByte){
            joinBuffer.add(b);
        }

        //joinMsg.head.id
        intToByte = ByteBuffer.allocate(4).putInt(id).array();
        for (byte b : intToByte){
            joinBuffer.add(b);
        }

        //joinMsg.head.MsgType
        intToByte = ByteBuffer.allocate(4).putInt(0).array(); // MsgType.Join
        for (byte b : intToByte){
            joinBuffer.add(b);
        }

        //joinMsg.ObjectDesc
        intToByte = ByteBuffer.allocate(4).putInt(0).array(); // ObjectDesc.Human
        for (byte b : intToByte){
            joinBuffer.add(b);
        }

        //joinMsg.ObjectForm
        intToByte = ByteBuffer.allocate(4).putInt(3).array(); // ObjectForm.Cone
        for (byte b : intToByte){
            joinBuffer.add(b);
        }

        //joinMsg.name
        intToByte = ByteBuffer.allocate(4).putInt(45).array(); // ascii: '-'
        for (byte b : intToByte){
            joinBuffer.add(b);
        }

        byte[] first = new byte[1024];
        for (int v = 0; v < joinBuffer.size(); v++){
            first[v] = joinBuffer.get(v);
        }
        first[joinBuffer.size()] = '\0';

        out.write(first);
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
        newPlayerMsg.msg.head.length = bb.getInt();
        newPlayerMsg.msg.head.seqNo = bb.getInt();
        newPlayerMsg.msg.head.id = bb.getInt();
        newPlayerMsg.msg.head.type = MsgType.Change;
        newPlayerMsg.desc = ObjectDesc.values()[bb.getInt()];
        newPlayerMsg.form = ObjectForm.values()[bb.getInt()];
        newPlayerMsg.name = new char[0];

        PlayerLeaveMsg playerLeaveMsg = new PlayerLeaveMsg();
        playerLeaveMsg.msg.head.length = bb.getInt();
        playerLeaveMsg.msg.head.seqNo = bb.getInt();
        playerLeaveMsg.msg.head.id = bb.getInt();
        playerLeaveMsg.msg.head.type = MsgType.Change;

        NewPlayerPositionMsg newPlayerPositionMsg = new NewPlayerPositionMsg();
        newPlayerPositionMsg.msg.head.length = bb.getInt();
        newPlayerPositionMsg.msg.head.seqNo = bb.getInt();
        newPlayerPositionMsg.msg.head.id = bb.getInt();
        newPlayerPositionMsg.msg.head.type = MsgType.Change;
        newPlayerPositionMsg.pos.x = bb.getInt();
        newPlayerPositionMsg.pos.y = bb.getInt();
        newPlayerPositionMsg.dir.x = 0;
        newPlayerPositionMsg.dir.y = 0;

        switch (ChangeType.values()[bb.getInt()]) {
            case NewPlayer -> {
                System.out.print('[' + newPlayerMsg.msg.head.seqNo + "]:\tplayer joined\tId=" + newPlayerMsg.msg.head.id);
                //for each (client c in clientList)
                if (id == -1) {
                    //newPlayerMsg.msg.head.length;
                    id = newPlayerMsg.msg.head.id;
                    pos = newPlayerPositionMsg.pos;
                }
            }
            case PlayerLeave -> {
                System.out.println('[' + playerLeaveMsg.msg.head.seqNo + "]:\tplayer left server\tId=" + playerLeaveMsg.msg.head.id);
                if (playerLeaveMsg.msg.head.id == id) {
                    System.out.println("kicked from server");
                    System.exit(1);
                }
            }
            case NewPlayerPosition -> {
                System.out.println('[' + newPlayerPositionMsg.msg.head.seqNo + "]:\tpos:(" +
                        newPlayerPositionMsg.pos.x + ";" +
                        newPlayerPositionMsg.pos.y + ")\tid=" + newPlayerPositionMsg.msg.head.id);
                if (newPlayerPositionMsg.msg.head.id == id) {
                    pos.x = newPlayerPositionMsg.pos.x;
                    pos.y = newPlayerPositionMsg.pos.y;
                }
            }
            default -> {
                System.out.println("pause debugger...\n");
                System.in.read();
            }
        }

        //Reformat
        for (int k = 0; k < byteBuf.size(); k += 3) {
            info.add(new Pixel(
                    Byte.toUnsignedInt(byteBuf.get(k)),
                    Byte.toUnsignedInt(byteBuf.get(k + 1)),
                    Byte.toUnsignedInt(byteBuf.get(k + 2)))); // always a multiple of three
            System.out.print(
                    (byteBuf.get(k)+(byte)48) +
                    (byteBuf.get(k+1)+(byte)48) +
                    (byteBuf.get(k+2)+(byte)48));
            info.clear();
            info.add(new Pixel(Byte.toUnsignedInt(byteBuf.get(k)), Byte.toUnsignedInt(byteBuf.get(k+1)), Byte.toUnsignedInt(byteBuf.get(k+2))));
        }
        window.canvas.SetParamArr(info);
        window.frame.repaint();

        byteBuf.clear();
    }

    public static void speak() throws IOException
    {
        //new position
        switch (direction)
        {
            case -1:
                break;
            case 0:
                pos.y -= 1;
                break;
            case 1:
                pos.y += 1;
                break;
            case 2:
                pos.x -= 1;
                break;
            case 3:
                pos.x += 1;
                break;
            default:
                System.out.println("Error in speak() switch");
                return;
        }
        ArrayList<Byte> pl = new ArrayList<>();
        Collections.addAll(Arrays.asList(ByteBuffer.allocate(4).putInt(pos.x).array()));
        Collections.addAll(Arrays.asList(ByteBuffer.allocate(4).putInt(pos.y).array()));

        pl.add((byte)7);
        byte[] first = new byte[1024];
        for (int v = 0; v < pl.size(); v++){
            first[v] = pl.get(v);
        }
        first[pl.size()] = '\0';
        out.write(first);
    }


    public static void main(String[] args) throws IOException {
        pos.x = -200;
        pos.y = -200;
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