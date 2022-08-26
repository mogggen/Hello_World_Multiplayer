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
import java.util.ArrayList;

public class Main extends JComponent {

    // Coordinate system: -100 - 100
    // Resolution: 505x505

    static void Log(String msg, int clientId)
    {
        System.out.println("[" + seqNo + "][" + clientId + "]" + msg);
    }

    public static class Client
    {
        int clientId;
        Coordinate position;
        ObjectDesc objectDesc; // used for color
        ObjectForm objectForm;
    }

    static ArrayList<Byte> globX = new ArrayList<>();
    static ArrayList<Byte> globY = new ArrayList<>();
    static ArrayList<Byte> globColor = new ArrayList<>();
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
    static int id = 0;
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


    public static class GUI
    {
        JFrame frame = new JFrame();
        PixelCanvas canvas = new PixelCanvas();
        public GUI()
        {
            canvas.setBounds(0, 0, 303, 303);
            frame.setSize(303, 303);
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            frame.setTitle("GUI Client");
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

                        case 87, 38 -> direction = (byte)dir.left.ordinal();
                        case 83, 40 -> direction = (byte)dir.right.ordinal();
                        case 65, 37 -> direction = (byte)dir.up.ordinal();
                        case 68, 39 -> direction = (byte)dir.down.ordinal();
                        default -> {
                            System.out.println("not a valid keypress.");
                        }
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
        ArrayList<Byte> xCoordBuf = new ArrayList<>();
        ArrayList<Byte> yCoordBuf = new ArrayList<>();
        ArrayList<Byte> colorBuf = new ArrayList<>();
        void SetParamArr()
        {
            this.xCoordBuf.clear();
            this.yCoordBuf.clear();
            this.colorBuf.clear();
            for (Client c : info){
                this.xCoordBuf.add((byte)c.position.x);
                this.yCoordBuf.add((byte)c.position.y);
                this.colorBuf.add((byte)c.objectDesc.ordinal());
            }
        }

        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g); // happens when the buffer.size changes
            System.out.println("Connections: " + info.size());
            if (info != null && info.size() > 0) {
                for (int i = 0; i < xCoordBuf.size(); i++){
                    if (colorBuf.get(i) == 0){
                        return;
                    }
                    int up = Fit(colorBuf.get(i), 1, 16);
                    int down = Fit(colorBuf.get(i), 16, 1);
                    g.setColor(new Color(down, up, down));
                    g.fillRect(xCoordBuf.get(i), yCoordBuf.get(i), 10, 10);
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

        // send joinMsg and receive

        byte[] buf = new byte[6]; // bytes in joinMsg

        //joinMsg.head.length
        buf[0] = 6;

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

        out.write(buf);
    }


    //Reads Bytes from the input stream
    public static void listen(GUI window) throws IOException {

        byte[] buf = new byte[7];
        int count = in.read(buf);
        if (count == -1)
        {
            System.out.println("Error: unable to receive");
            System.exit(1);
        }

        //Log("[" + ChangeType.values()[buf[4]] + "]", buf[2]);
        if ((byte)ChangeType.NewPlayer.ordinal() == buf[4]){
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

            seqNo = newPlayerMsg.msg.head.seqNo;
            if (id == 0) {
                id = newPlayerMsg.msg.head.id;
            }
            boolean found = false;
            for (Client c : info){
                if (c.clientId == newPlayerMsg.msg.head.id){
                    found = true;
                    break;
                }
            }
            if (!found){
                Client c = new Client();
                c.clientId = newPlayerMsg.msg.head.id;
                c.objectDesc = newPlayerMsg.desc;
                c.objectForm = newPlayerMsg.form;
                c.position = new Coordinate();
                c.position.x = 101; // not set
                c.position.y = 101; // not set
                info.add(c);
            }
        }

        if ((byte)ChangeType.PlayerLeave.ordinal() == buf[4]){
            PlayerLeaveMsg playerLeaveMsg = new PlayerLeaveMsg();
            playerLeaveMsg.msg = new ChangeMsg();
            playerLeaveMsg.msg.head = new MsgHead();

            playerLeaveMsg.msg.head.length = buf[0];
            playerLeaveMsg.msg.head.seqNo = buf[1];
            playerLeaveMsg.msg.head.id = buf[2];
            playerLeaveMsg.msg.head.type = MsgType.values()[buf[3]]; // Should always be MsgType.Change
            playerLeaveMsg.msg.type = ChangeType.values()[buf[4]]; // should always be ChangeType.PlayerLeave

            info.removeIf(c -> c.clientId == playerLeaveMsg.msg.head.id);
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

            for (Client c : info) {
                if (newPlayerPositionMsg.msg.head.id == c.clientId) {
                    c.position = new Coordinate();
                    c.position.x = newPlayerPositionMsg.pos.x;
                    c.position.y = newPlayerPositionMsg.pos.y;
                    System.out.println("\treceiving: " + "(" + (buf[5] + 100) + ", " + (buf[6] + 100) + ")");
                }
            }
        }
        window.canvas.SetParamArr();
        window.frame.repaint();
    }

    public static void speak() throws IOException
    {
        if (id == 0){
            System.out.println("Caution: Connect to server before attempting to move.");
            return;
        }
        byte[] pl = new byte[7];
        pl[0] = 7;
        pl[1] = (byte) ++seqNo;
        pl[2] = (byte)id;
        pl[3] = (byte)MsgType.Event.ordinal();
        pl[4] = (byte)EventType.Move.ordinal();
        pl[5] = (byte)getClient(id).position.x;
        pl[6] = (byte)getClient(id).position.y;

        //new position
        switch (direction) {
            case -1:
                break;
            case 0:
                pl[5] -= 1;
                break;
            case 1:
                pl[5] += 1;
                break;
            case 2:
                pl[6] -= 1;
                break;
            case 3:
                pl[6] += 1;
                break;
            default:
                System.out.println("Error in speak() switch");
                return;
        }

        System.out.print("sending: (" + pl[5] + ", " + pl[6] + ")");
        out.write(pl);
    }


    public static void main(String[] args) throws IOException {
        GUI window = new GUI();
        runSetup(4999);
        while(true) {
            try {
                listen(window);
            }catch (SocketException s){
                s.printStackTrace();
                System.exit(1);
            }
        }
    }
}