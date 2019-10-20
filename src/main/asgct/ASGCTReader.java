package asgct;

import java.io.IOException;
import java.util.ArrayList;

public class ASGCTReader {
  // pops the top off the underlying buffer
  private native static String pop();

  public static class ASGCTFrame {
    long timestamp;
    String id;
    String[] trace;

    ASGCTFrame(String frame) {
      String[] frame_arr = frame.split(",");

      timestamp = Long.parseLong(frame_arr[0]);
      id = frame_arr[1];

      String s_trace = frame_arr[2];
      trace = s_trace.replaceAll(";", "@").replaceAll("/", "@").replaceAll("@@", ";").replaceAll("@", ".").split(";");
    }

    public String toString() {
      String message = id + "@" + timestamp + "\n";
      for (String trc: trace)
        message += "\t" + trc + "()\n";

      return message;
    }
  }

  public static ASGCTFrame read() {
    String record = pop();
    if (record.length() == 0)
      return null;
    else
      return new ASGCTFrame(record);
  }

  public static long reader_ptr;

	public static void main(String[] args) throws IOException {
    NativeUtils.loadLibraryFromJar("/asgct/liblagent.so");

    Thread thr = new Thread(new Runnable() {
      @Override
      public void run() {
        try {
          for (int i = 0; i < 10; i++) {
            // System.out.println("I'm waking up");
            Thread.sleep(1000);
          }
        } catch (InterruptedException e) { }
      }
    });

    thr.start();

    ArrayList<ASGCTFrame> frames = new ArrayList<ASGCTFrame>();
    while(thr.isAlive()) {
    // for (int i = 0; i < 100; i ++) {
      ASGCTFrame record = read();
      if (record != null)
        frames.add(record);
    }

    System.out.println(frames.size() + " traces collected!");
    for (ASGCTFrame frame: frames) {
      System.out.println(frame);
    }
	}
}
