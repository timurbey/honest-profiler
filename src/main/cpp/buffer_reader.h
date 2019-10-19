#ifndef BUFFER_READER_H
#define BUFFER_READER_H

#include <jni.h>
#include "circular_queue.h"

using std::string;
using std::vector;

class BufferReader : public QueueListener {

public:
    explicit BufferReader(jvmtiEnv* jvmti) : jvmti_(jvmti) { }

    virtual void record(const timespec &ts, const JVMPI_CallTrace &trace, ThreadBucketPtr info = ThreadBucketPtr(nullptr));

    vector<string> pop();

private:
  jvmtiEnv *const jvmti_;

  vector<int> timestamps;
  vector<string> ids;
  vector<std::vector<jmethodID>> traces;

  string lookUpMethod(jmethodID);
};

// extern void setupReader(JNIEnv *env, Reader *reader);
//
// extern "C" JNIEXPORT jstring JNICALL Java_asgct_ASGCTReader_tracePop(JNIEnv *env, jclass);

#endif // BUFFER_READER_H
