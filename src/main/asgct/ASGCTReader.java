package asgct;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
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

  public static ASGCTFrame[] fetch() {
    ArrayList<ASGCTFrame> records = new ArrayList<ASGCTFrame>();
    String raw_record = pop();
    if (raw_record.length() > 0) {
      String[] record_arr = raw_record.split("#");
      for (String record: record_arr)
        records.add(new ASGCTFrame(record));
    }

    return records.toArray(new ASGCTFrame[records.size()]);
  }

  static Runnable profiler = new Runnable() {
    @Override
    public void run() {
      int i = 0;
      int records_size = 0;
      long elapsed = 0;

      while(!Thread.currentThread().isInterrupted()) {
        i++;

        long start = System.nanoTime();

        List<ASGCTFrame> records = Arrays.asList(fetch());
        records_size += records.size();

        long total = System.nanoTime() - start;
        elapsed += total;

        try {
          Thread.sleep(1);
        } catch (Exception e) {
          Thread.currentThread().interrupt();
        }
      }

      System.out.println("calls: " + i + ", records: " + records_size + ", ms: " + elapsed / 1000000);
    }
  };

	public static void main(String[] args) throws IOException, InterruptedException {
    NativeUtils.loadLibraryFromJar("/asgct/liblagent.so");

    while(true) {
      Thread thread = new Thread(profiler);
      thread.start();

      Thread.sleep(1000);

      thread.interrupt();
      thread.join();
    }
  }
}
