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
  string message;
  vector<string> vec;

  vec.push_back(std::to_string(timestamps.front()));
  vec.push_back(ids.front());

  vector<jmethodID> trace = traces.front();
  for (int i = 0; i < trace.size(); i++) {
      jmethodID method_id = trace[i];
      string method = lookUpMethod(method_id);
      message.append(method);
      message.append(";");
      break;
  }
  vec.push_back(message);

  timestamps.erase(timestamps.begin());
  ids.erase(ids.begin());
  traces.erase(traces.begin());

  return vec;
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

// extern void setupBufferReader(JNIEnv *env, Reader *reader) {
//     printf("setting up the field . . .");
//     jclass c = env->FindClass("Lasgct/ASGCTReader");
//     jfieldID f = env->GetStaticFieldID(c, "reader_ptr", "J");
//     env->SetStaticLongField(c, f, (jlong)reader);
//     printf("set the field!\n");
// }
//
// extern "C" JNIEXPORT jstring JNICALL Java_asgct_ASGCTReader_tracePop(JNIEnv *env, jclass jcls) {
//   printf("grabbing a trace . . .");
//   jclass c = env->FindClass("Lasgct/ASGCTReader");
//   jfieldID f = env->GetStaticFieldID(c, "reader_ptr", "J");
//   jlong reader_ptr = env->GetStaticLongField(c, f);
//   Reader* reader = (Reader* )reader_ptr;
//
//   jstring ret_record;
//   if (reader != nullptr && reader->size() > 0) {
//     vector<string> record = reader->pop();
//
//     string message;
//     message.append(record[0].c_str()).append(",");
//     message.append(record[1].c_str()).append(",");
//     message.append(record[2].c_str()).append(",");
//
//     // printf("%s", message.c_str());
//     ret_record = env->NewStringUTF(message.c_str());
//
//     printf("grabbed a trace!");
//     return ret_record;
//   } else {
//     printf("grabbed nothing!");
//     ret_record = env->NewStringUTF("");
//     return ret_record;
//   }
// }
