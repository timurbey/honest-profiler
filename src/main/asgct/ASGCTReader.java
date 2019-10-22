package asgct;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;

public class ASGCTReader {
  // used to store the pointer to jvmti reader
  public static long reader_ptr;

  // pops the top off the underlying buffer
  // private native static ASGCTFrame[] pop(long size);
  // private native static Object[] pop2(long size);
  // private native static String pop3(long size);
  private native static String pop();

  public static class ASGCTFrame {
    public long timestamp;
    public long id;
    public String name;
    public String[] trace;

    ASGCTFrame(long timestamp, long id, String name, String[] trace) {
      this.timestamp = timestamp;
      this.id = id;
      this.name = name;
      this.trace = trace;

      for (int i = 0; i < trace.length; i++)
        this.trace[i] = this.trace[i].substring(1, this.trace[i].length()).replace(";", ".").replace("/", ".");
    }

    ASGCTFrame(Object[] items) {
      this.timestamp = (Long)items[0];
      this.id = (Long)items[1];
      this.name = (String)items[2];
      this.trace = (String[])items[3];

      for (int i = 0; i < trace.length; i++)
        this.trace[i] = this.trace[i].substring(1, this.trace[i].length()).replace(";", ".").replace("/", ".");
    }

    ASGCTFrame(String frame) {
      String[] items = frame.split(",");
      this.timestamp = Long.parseLong(items[0]);
      this.id = Long.parseLong(items[1]);
      this.name = items[2];
      this.trace = items[3].split("@");

      for (int i = 0; i < trace.length; i++)
        this.trace[i] = this.trace[i].substring(1, this.trace[i].length()).replace(";", ".").replace("/", ".");
    }

    public long getTimestamp() { return timestamp; }
    public long getId() { return id; }
    public String getName() { return name; }
    public String[] getTrace() { return trace; }

    public String toString() {
      String message = name + "(" + id +")" + ": @" + timestamp + "\n";
      for (String trc: trace)
        message += "\t" + trc + "()\n";

      return message;
    }
  }

  // static Runnable profiler = new Runnable() {
  //   @Override
  //   public void run() {
  //     ArrayList<ASGCTFrame> frames = new ArrayList<ASGCTFrame>();
  //     long elapsed = 0;
  //
  //     while(!Thread.currentThread().isInterrupted()) {
  //       long start = System.nanoTime();
  //       // String[] string_frames = pop3(Integer.MAX_VALUE).split("#");
  //       String[] string_frames = pop3(10000).split("#");
  //
  //       for (String frame: string_frames)
  //         if (frame.length() > 0)
  //           frames.add(new ASGCTFrame(frame));
  //       elapsed += System.nanoTime() - start;
  //
  //       try {
  //         Thread.sleep(100);
  //       } catch (InterruptedException e) {
  //         Thread.currentThread().interrupt();
  //       }
  //     }
  //
  //     // elapsed /= 1000000;
  //
  //     // if (elapsed > 0) {
  //     System.out.println(frames.size() + " / " + elapsed / 1000000 + "ms = " + frames.size() * 1000000 / elapsed + " traces/ms collected!");
  //     // }
  //   }
  // };

	public static void main(String[] args) throws IOException, InterruptedException {
    NativeUtils.loadLibraryFromJar("/asgct/liblagent.so");

    while(true) {
      pop();
    // for(int i = 0; i < 10; i++) {
      // Thread thread = new Thread(profiler);
      // thread.start();
      //
      // Thread.sleep(1000);

      // long elapsed = System.nanoTime();
      // // String[] string_frames = pop3(Integer.MAX_VALUE).split("#");
      // String[] string_frames = pop().split("#");
      //
      // ArrayList<ASGCTFrame> frames = new ArrayList<ASGCTFrame>();
      // for (String frame: string_frames)
      //   if (frame.length() > 0) {
      //     // System.out.println(frame);
      //     frames.add(new ASGCTFrame(frame));
      //   }
      //
      // elapsed = System.nanoTime() - elapsed;
      //
      // if (frames.size() > 0)
      //   System.out.println(frames.size() + " / " + elapsed / 1000000.0 + "ms = " + frames.size() * 1000000.0 / elapsed + " traces/ms collected!");
      // for (ASGCTFrame frame: frames)
      //   System.out.println(frame);

      // System.out.println("interrupting...");
      // thread.interrupt();
      // thread.join();
    }
  }
}
