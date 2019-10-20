#include "buffer_reader.h"
#include "math.h"
#include <thread>
#include <jni.h>

void BufferReader::record(const timespec &ts, const JVMPI_CallTrace &trace, ThreadBucketPtr info) {
  if (info.defined()) {
    long ms = ts.tv_sec * 1000;
    ms += round(ts.tv_nsec / 1.0e6);
    timestamps.push_back(ms);

    ids.push_back(info->name.c_str());

    vector<jmethodID> method_ids;
    for (int i = 0; i < trace.num_frames; i++)
      method_ids.push_back(trace.frames[i].method_id);

    traces.push_back(method_ids);
  }
}

vector<string> BufferReader::pop() {
  if (!timestamps.empty()) {
    vector<jmethodID> trace = traces.front();
    if (trace.size() == 0)
      return vector<string>();

    string message;
    for (int i = 0; i < trace.size(); i++)
      message.append(lookUpMethod(trace[i])).append(";;");

    vector<string> vec;

    vec.push_back(std::to_string(timestamps.front()));
    vec.push_back(ids.front());
    vec.push_back(message);

    timestamps.erase(timestamps.begin());
    ids.erase(ids.begin());
    traces.erase(traces.begin());

    return vec;
  } else {
    return vector<string>();
  }
}

string BufferReader::lookUpMethod(jmethodID method_id) {
  string method;

  JvmtiScopedPtr<char> methodName(jvmti_), methodSignature(jvmti_), methodGenericSignature(jvmti_);

  jvmti_->GetMethodName(method_id, methodName.GetRef(), methodSignature.GetRef(), methodGenericSignature.GetRef());

  jclass declaring_class;
  jvmti_->GetMethodDeclaringClass(method_id, &declaring_class);

  JvmtiScopedPtr<char> classSignature(jvmti_), classSignatureGeneric(jvmti_);
  jvmti_->GetClassSignature(declaring_class, classSignature.GetRef(), classSignatureGeneric.GetRef());

  method.append(classSignature.Get()).append(methodName.Get());
  return method;
}

extern void setupReader(JNIEnv *env, BufferReader *reader) {
    jclass c = env->FindClass("asgct/ASGCTReader");
    jfieldID f = env->GetStaticFieldID(c, "reader_ptr", "J");
    env->SetStaticLongField(c, f, (jlong)reader);
}

extern "C" JNIEXPORT jstring JNICALL Java_asgct_ASGCTReader_pop(JNIEnv *env, jclass jcls) {
  jclass c = env->FindClass("asgct/ASGCTReader");
  jfieldID f = env->GetStaticFieldID(c, "reader_ptr", "J");
  BufferReader* reader = (BufferReader*)env->GetStaticLongField(c, f);

  vector<string> record = reader->pop();
  string message;
  for (int i = 0; i < record.size(); i++) {
    message.append(record[i]);
    if (i + 1 < record.size())
      message.append(",");
  }

  return env->NewStringUTF(message.c_str());
}
