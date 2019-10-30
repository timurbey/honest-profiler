#include "buffer_reader.h"
#include "math.h"
#include <ctime>
#include <thread>
#include <jni.h>

int BufferReader::size() { return data_size.load(std::memory_order_relaxed); }

bool BufferReader::empty() { return data_size.load(std::memory_order_relaxed) == 0; }

void BufferReader::record(const timespec &ts, const JVMPI_CallTrace &trace, ThreadBucketPtr info) {
  if (info.defined()) {
      long ms = ts.tv_sec * 1000;
      ms += ts.tv_nsec;

      timestamps.push(ms);

      ids.push(info->jid);
      names.push(info->name.c_str());

      vector<jmethodID> method_ids;
      for (int i = 0; i < trace.num_frames; i++)
          method_ids.push_back(trace.frames[i].method_id);

      traces.push(method_ids);

      data_size.store(size() + 1);
  }
}

ASGCTFrame BufferReader::pop() {
  ASGCTFrame frame;

  if (!empty()) {
    vector<jmethodID> trace = traces.front();
    if (trace.size() > 0) {
        frame.timestamp = timestamps.front();
        frame.id = ids.front();
        frame.name = names.front();

        for (int i = 0; i < trace.size(); i++)
          frame.trace.append(lookUpMethod(trace[i])).append("@");
        frame.trace.append("#");
    }

    timestamps.pop();
    ids.pop();
    names.pop();
    traces.pop();

    data_size.store(size() - 1);
  }

  return frame;
}

string BufferReader::lookUpMethod(jmethodID method_id) {
    // chopped up version of the frame look up in log writer
    JvmtiScopedPtr<char> methodName(jvmti_), methodSignature(jvmti_), methodGenericSignature(jvmti_);

    JVMTI_ERROR_CLEANUP_RET(
      jvmti_->GetMethodName(method_id, methodName.GetRef(), methodSignature.GetRef(), methodGenericSignature.GetRef()),
      "", { methodName.AbandonBecauseOfError(); methodSignature.AbandonBecauseOfError(); methodGenericSignature.AbandonBecauseOfError(); }
    );

    jclass declaring_class;
    JVMTI_ERROR_RET(jvmti_->GetMethodDeclaringClass(method_id, &declaring_class), "");

    JvmtiScopedPtr<char> classSignature(jvmti_), classSignatureGeneric(jvmti_);
    JVMTI_ERROR_CLEANUP_RET(
        jvmti_->GetClassSignature(declaring_class, classSignature.GetRef(), classSignatureGeneric.GetRef()),
        "", { classSignature.AbandonBecauseOfError(); classSignatureGeneric.AbandonBecauseOfError(); }
    );

    string method;
    method.append(classSignature.Get()).append(methodName.Get());

    return method;
}

extern void setReader(JNIEnv *env, BufferReader *reader) {
  // set a reference to the reader so the jni can access it
  jclass cls = env->FindClass("asgct/ASGCTReader");
  jfieldID fld = env->GetStaticFieldID(cls, "reader_ptr", "J");
  env->SetStaticLongField(cls, fld, (jlong)reader);
}

BufferReader *fetchReader(JNIEnv *env) {
  // grab the reference to the reader
  jclass cls = env->FindClass("asgct/ASGCTReader");
  jfieldID fld = env->GetStaticFieldID(cls, "reader_ptr", "J");
  return (BufferReader *)env->GetStaticLongField(cls, fld);
}

extern "C" JNIEXPORT jstring JNICALL Java_asgct_ASGCTReader_pop(JNIEnv *env, jclass jcls) {
  // grab the reader
  BufferReader *reader = fetchReader(env);

  string frames;

  ASGCTFrame last;
  last.timestamp = 0;
  last.id = 0;
  last.trace = "";

  while (!reader->empty()) {
      ASGCTFrame frame = reader->pop();

      // check if the frame is garbage or a duplicate
      if (frame.timestamp == 0 || (frame.timestamp == last.timestamp && frame.id == last.id && frame.trace == last.trace) || frame.trace.size() == 0)
        continue;

      frames.append(std::to_string(frame.timestamp)).append(",");
      frames.append(std::to_string(frame.id)).append(",");
      frames.append(frame.name).append(",");
      frames.append(frame.trace.c_str());

      last.timestamp = frame.timestamp;
      last.id = frame.id;
      last.trace = frame.trace;
  }

  return env->NewStringUTF(frames.c_str());
}
