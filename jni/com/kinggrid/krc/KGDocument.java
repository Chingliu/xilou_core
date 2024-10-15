package com.kinggrid.krc;


import java.io.*;
import java.util.concurrent.*;

class KGLibrary{
    public static native void Init(String fontpath);
    public static native void Destroy();
    static {
        System.load("E:\\pdfium_jni.dll");
    }    
}
public class KGDocument {
    public static void main(String[] args) {
        try{
        System.out.println("[main]press any key to continue, it is a chance to attach to debug it");
        DataInputStream in = new DataInputStream(System.in);
        char ch = in.readChar();
        }catch(Exception e){
            e.printStackTrace();
        }
        try{
        KGLibrary.Init("");
        ExecutorService service = Executors.newCachedThreadPool();
        service.execute(new Task("task1"));
        service.execute(new Task("task2"));
        service.execute(new Task("task3"));
        service.shutdown();
        }finally{
            KGLibrary.Destroy();
        }
    }

    public static native long openDocumentFromSteam(byte[] data);

    public static native void closeDocument(long doc);

    public static native int getPagesCount(long doc);

    public static native int getPageHeight(long doc, int number);

    public static native int getPageWidth(long doc, int number);

    /**
     * @param doc
     * @param number
     * @param area
     * @param zoom
     * @param type   2:jpg
     * @return
     */
    public static native byte[] getPageImage(long doc, int number, float[] area, float zoom, float rotate, int type);

}
class Task implements Runnable {


    private final String name;

    Task(String name) {
        this.name = name;
    }
    @Override
    public void run() {
        byte[] bytes = getBytes("E:\\test.pdf");
        //DataInputStream in = new DataInputStream(System.in);
        //System.out.println("[main]press any key to continue, it is a chance to attach to debug it");
        //char ch = in.readChar();
        pdf2imageTest(bytes);
        //System.out.println("111");

    }



    public static byte[] getBytes(String filePath) {
        byte[] buffer = null;
        try {
            File file = new File(filePath);
            FileInputStream fis = new FileInputStream(file);
            ByteArrayOutputStream bos = new ByteArrayOutputStream(1000);
            byte[] b = new byte[1000];
            int n;
            while ((n = fis.read(b)) != -1) {
                bos.write(b, 0, n);
            }
            fis.close();
            bos.close();
            buffer = bos.toByteArray();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return buffer;
    }

    public static void saveFile(String filename, byte[] data) {
        try {
            if (data != null) {
                InputStream in = new ByteArrayInputStream(data);
                String filepath = "D:\\" + filename;
                File file = new File(filepath);
                if (file.exists()) {
                    file.delete();
                }
                FileOutputStream fos = new FileOutputStream(file);
                int len = 0;
                byte[] buf = new byte[1024];
                while ((len = in.read(buf)) != -1) {
                    fos.write(buf, 0, len);
                }
                fos.flush();
                fos.close();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    private static final float[] AREA = {0f, 0f, 0f, 0f};
    private static final float ZOOM = 1f;
    private static final float ROTATE = 0f;
    private static final int TYPE = 2;

    public static void pdf2imageTest(byte[] data) {
        long doc = 0L;
        try {
            doc = KGDocument.openDocumentFromSteam(data);
            int pages = KGDocument.getPagesCount(doc);
            int width = 0;
            int height = 0;
            for (int i = 1; i <= pages; i++) {
                byte[] imageData = null;
                width = KGDocument.getPageWidth(doc, i);
                if (width < 0) {
                    System.out.println("[java][pdf2imageTest]getPageWidth failed");
                    continue;
                }
                height = KGDocument.getPageHeight(doc, i);
                if (height <= 0) {
                    System.out.println("[java][pdf2imageTest]getPageHeight failed");
                    continue;
                }
                String fileName = i + "_" + width + "_" + height + ".png";
                System.out.println("[java]filename:::" + fileName);
                imageData = KGDocument.getPageImage(doc, i, AREA, ZOOM, ROTATE, TYPE);
                if (null == imageData) {
                    System.out.println("[java][pdf2imageTest]getPageImage failed");
                    continue;
                }
                System.out.println("[java][pdf2imageTest] page[" + i + "]imageData len=" + imageData.length);
                //saveFile(fileName, imageData);
            }

        } catch (Exception e) {
            System.out.println(e.getMessage());
        } finally {
            if (doc > 0) {
                KGDocument.closeDocument(doc);
            }
        }

    }
}

