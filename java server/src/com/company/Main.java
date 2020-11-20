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

import java.util.ArrayList;

public class Main extends JComponent {

    public static class GUI
    {
        JFrame frame = new JFrame();
        PixelCanvas canvas = new PixelCanvas();
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
                public void keyPressed(KeyEvent e) {
                    switch (e.getKeyCode()){
                        case 87:
                        case 38:
                            //up
                            System.out.println("up");
                            break;

                        case 83:
                        case 40:
                            //down
                            System.out.println("down");
                            break;

                        case 65:
                        case 37:
                            //left
                            System.out.println("left");
                            break;

                        case 68:
                        case 39:
                            //right
                            System.out.println("right");
                            break;
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
            System.out.println(this.arr);
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

    public static class Setup
    {
        ArrayList<Byte> byteBuf = new ArrayList<>();
        ArrayList<Pixel> info = new ArrayList<>();
        ServerSocket ss;
        Socket s;
        InputStream in;
        OutputStream out;

        public Setup(int port) throws IOException {
            ss = new ServerSocket(port);
            s = ss.accept();
            in = s.getInputStream();
            out = s.getOutputStream();
            System.out.println("client connected");
        }

        //Reads Bytes from the input stream
        public void listen(GUI window) throws IOException {
            byte temp;
            for (byte i = 0; i < 3; i++) {
                temp = (byte) in.read();
                byteBuf.add(temp);
            }

            //Reformat
            for (int k = 0; k < byteBuf.size(); k += 3) {
                info.add(new Pixel(Byte.toUnsignedInt(byteBuf.get(k)), Byte.toUnsignedInt(byteBuf.get(k + 1)), Byte.toUnsignedInt(byteBuf.get(k + 2)))); // always a multiple of three
            }
            window.canvas.SetParamArr(info);
            window.frame.repaint();

            byteBuf.clear();
        }

        public void send(byte direction) throws IOException
        {
            out.write(direction);
        }
    }

    public static void main(String[] args) throws IOException, InterruptedException {

        GUI window = new GUI();
        Setup setup = new Setup(4999);
        new Main();
        while(true) {

            Thread.sleep(1000);
            setup.listen(window);
        }
    }
}