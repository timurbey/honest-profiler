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
  vector<string> trace;
};

class BufferReader : public QueueListener {

public:
    explicit BufferReader(jvmtiEnv *jvmti) : jvmti_(jvmti) { }

    virtual void record(const timespec &ts, const JVMPI_CallTrace &trace, ThreadBucketPtr info = ThreadBucketPtr(nullptr));

    int size();

    ASGCTFrame pop();

    bool empty();

private:
  jvmtiEnv *const jvmti_;

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
