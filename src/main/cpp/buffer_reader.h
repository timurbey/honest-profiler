#ifndef BUFFER_READER_H
#define BUFFER_READER_H

#include <jni.h>
#include <jvmti.h>
#include "circular_queue.h"
#include <queue>

using std::string;
using std::queue;
using std::vector;

struct ASGCTFrame {
  long timestamp;
  long id;
  string name;
  string trace;

  ASGCTFrame() {
      this->timestamp = 0;
      this->id = 0;
      this->trace = "";
  }
};

class BufferReader : public QueueListener {

public:
    explicit BufferReader(jvmtiEnv *jvmti) : jvmti_(jvmti), data_size(0) { }

    int size();

    bool empty();

    virtual void record(const timespec &ts, const JVMPI_CallTrace &trace, ThreadBucketPtr info = ThreadBucketPtr(nullptr));

    ASGCTFrame pop();


private:
    jvmtiEnv *const jvmti_;

    std::atomic<int> data_size;

    queue<long> timestamps;
    queue<long> ids;
    queue<string> names;
    queue<vector<jmethodID>> traces;

    string lookUpMethod(jmethodID);
};

extern void setReader(JNIEnv *, BufferReader *);

extern BufferReader *fetchReader(JNIEnv *);

extern "C" JNIEXPORT jstring JNICALL Java_asgct_ASGCTReader_pop(JNIEnv *, jclass);

#endif // BUFFER_READER_H
