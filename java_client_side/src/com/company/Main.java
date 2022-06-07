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

    static ArrayList<Byte> byteBuf = new ArrayList<>();
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

    public class Coordinate {
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
    public class MsgHead {
        int length;     //Total length for whole message
        int seqNo;      //Sequence number
        int id;         //Client ID or 0;
        MsgType type;   //Type of message
    };
    //JOIN MESSAGE (CLIENT->SERVER)
    public class JoinMsg {
        MsgHead head;
        ObjectDesc desc;
        ObjectForm form;
        char name[];   //null terminated!,or empty
    };
    //LEAVE MESSAGE (CLIENT->SERVER)
    public class LeaveMsg {
        MsgHead head;
    };
    //CHANGE MESSAGE (SERVER->CLIENT)
    enum ChangeType {
        NewPlayer,
        PlayerLeave,
        NewPlayerPosition
    };
    //Included first in all Change messages
    public class ChangeMsg {
        MsgHead head;
        ChangeType type;
    };

    public class NewPlayerMsg {
        ChangeMsg msg;          //Change message header with new client id
        ObjectDesc desc;
        ObjectForm form;
        char name[];  //nullterminated!,or empty
    };
    public class PlayerLeaveMsg {
        ChangeMsg msg;          //Change message header with new client id
    };
    public class NewPlayerPositionMsg {
        ChangeMsg msg;          //Change message header
        Coordinate pos;         //New object position
        Coordinate dir;         //New object direction
    };
    //EVENT MESSAGE (CLIENT->SERVER)
    enum EventType { Move };
    //Included first in all Event messages
    public class EventMsg {
        MsgHead head;
        EventType type;
    };
    //Variantions of EventMsg
    public class MoveEvent {
        EventMsg event;
        Coordinate pos;         //New object position
        Coordinate dir;         //New object direction
    };
    //TEXT MESSAGE
    public class TextMessageMsg {
        MsgHead head;
        char text[];   //NULL-terminerad array of chars.
    };

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
        int x, y, c;
        public Pixel(int x, int y, int c)
        {
            this.x = x;
            this.y = y;
            this.c = c;
        }

        public int getX() {
            return x;
        }

        public int getY() {
            return y;
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
        System.out.println("client connected");
    }

    //Reads Bytes from the input stream
    public static void listen(GUI window) throws IOException {

        byte[] buf = new byte[1024];
        int count = in.read(buf);
//        //Reformat
//        for (int k = 0; k < byteBuf.size(); k += 3) {
//            //info.add(new Pixel(Byte.toUnsignedInt(byteBuf.get(k)), Byte.toUnsignedInt(byteBuf.get(k + 1)), Byte.toUnsignedInt(byteBuf.get(k + 2)))); // always a multiple of three
//            System.out.print((byteBuf.get(k)+(byte)48) + (byteBuf.get(k+1)+(byte)48) + (byteBuf.get(k+2)+(byte)48));
//            info.clear();
//            info.add(new Pixel(Byte.toUnsignedInt(byteBuf.get(k)), Byte.toUnsignedInt(byteBuf.get(k+1)), Byte.toUnsignedInt(byteBuf.get(k+2))));
//        }
//        window.canvas.SetParamArr(info);
//        window.frame.repaint();
//
//        byteBuf.clear();
    }

    public static void speak() throws IOException
    {
        //new position
        byte x = 0, y = 0; // send five bytes or use the 2 left-most bits in the color byte
        switch (direction)
        {
            case -1:
                break;
            case 0:
                y -= 1;
                break;
            case 1:
                y += 1;
                break;
            case 2:
                x -= 1;
                break;
            case 3:
                x += 1;
                break;
            default:
                System.out.println("Error in speak switch");
                return;
        }
        byte[] pl = { x, y, (byte)7};
        out.write(pl);
    }


    public static void main(String[] args) throws IOException, SocketException {
        Main sendingThread = new Main();
        GUI window = new GUI();
        runSetup(4999);
        while(true) {
            // no specific protocol needs to be used
            try {
                listen(window);
            }catch (SocketException s){
                s.printStackTrace();
            }
            System.out.println("received data");
        }
    }
}