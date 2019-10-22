#include "buffer_reader.h"
#include "math.h"
#include <ctime>
#include <thread>
#include <jni.h>

using std::vector;

void BufferReader::record(const timespec &ts, const JVMPI_CallTrace &trace, ThreadBucketPtr info) {
  if (info.defined()) {
    std::cout << "pushing: " << ts.tv_sec << std::endl;
    long ms = ts.tv_sec * 1000;
    ms += round(ts.tv_nsec / 1.0e6);
    timestamps.push(ms);

    ids.push(info->jid);
    names.push(info->name.c_str());

    vector<jmethodID> method_ids;
    for (int i = 0; i < trace.num_frames; i++)
      method_ids.push_back(trace.frames[i].method_id);

    traces.push(method_ids);
  }
}

int BufferReader::size() {
  return timestamps.size();
}

ASGCTFrame BufferReader::pop() {
  vector<jmethodID> trace = traces.front();
  if (!timestamps.empty() && trace.size() > 0) {
    std::cout << "popping: " << timestamps.front() << std::endl;
    ASGCTFrame frame;

    frame.timestamp = timestamps.front();
    frame.id = ids.front();
    frame.name = names.front();

    for (int i = 0; i < trace.size(); i++)
      frame.trace.push_back(lookUpMethod(trace[i]));

    timestamps.pop();
    ids.pop();
    names.pop();
    traces.pop();

    return frame;
  } else {
    return ASGCTFrame();
  }
}

bool BufferReader::empty() {
  return timestamps.empty();
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

// extern "C" JNIEXPORT jobjectArray JNICALL Java_asgct_ASGCTReader_pop(JNIEnv *env, jclass jcls, jlong jsize) {
//   // grab the reader
//   BufferReader *reader = fetchReader(env);
//
//   // setup data structures that we will use more than once
//   vector<jobject> frames;
//
//   jclass cls = env->FindClass("asgct/ASGCTReader$ASGCTFrame");
//   jmethodID cls_cons = env->GetMethodID(cls, "<init>", "(JJLjava/lang/String;[Ljava/lang/String;)V");
//
//   jclass str = env->FindClass("java/lang/String");
//
//   long size = (long)jsize;
//   while (!reader->empty() && frames.size() < size) {
//     try {
//       ASGCTFrame frame = reader->pop();
//
//       // check if the timestamp is 0 or we exceeded the timestamp
//       if (frame.timestamp == 0 || frame.timestamp / size < 1)
//         break;
//
//       jstring jname = env->NewStringUTF(frame.name.c_str());
//
//       // we need to place the undefined size vector into a jarray
//       jobjectArray jtrace = env->NewObjectArray(frame.trace.size(), str, nullptr);
//
//       for (int i = 0; i < frame.trace.size(); i++) {
//         jstring message = env->NewStringUTF(frame.trace[i].c_str());
//         env->SetObjectArrayElement(jtrace, i, message);
//       }
//
//       // put everything in the jframe's constructor
//       jobject jframe = env->NewObject(cls, cls_cons, (jlong)frame.timestamp, (jlong)frame.id, jname, jtrace);
//
//       frames.push_back(jframe);
//     } catch (std::exception) {
//       continue;
//     }
//   }
//
//   // we need to place the undefined size vector into a jarray
//   jobjectArray jframes = env->NewObjectArray(frames.size(), cls, nullptr);
//   for (int i = 0; i < frames.size(); i++) {
//     env->SetObjectArrayElement(jframes, i, frames[i]);
//   }
//
//   return jframes;
// }
//
// extern "C" JNIEXPORT jobjectArray JNICALL Java_asgct_ASGCTReader_pop2(JNIEnv *env, jclass jcls, jlong jsize) {
//   // grab the reader
//   BufferReader *reader = fetchReader(env);
//
//   // setup data structures that we will use more than once
//   vector<jobjectArray> frames;
//
//   jclass cls = env->FindClass("java/lang/Object");
//
//   jclass lng = env->FindClass("java/lang/Long");
//   jmethodID lng_cons = env->GetMethodID(lng, "<init>", "(J)V");
//
//   jclass str = env->FindClass("java/lang/String");
//
//   long size = (long)jsize;
//   while (!reader->empty() && frames.size() < size) {
//     try {
//       ASGCTFrame frame = reader->pop();
//
//       // check if the timestamp is 0 or we exceeded the timestamp
//       if (frame.timestamp == 0 || frame.timestamp / size < 1)
//         break;
//
//       jobjectArray jframe = env->NewObjectArray(4, cls, nullptr);
//
//       env->SetObjectArrayElement(jframe, 0, env->NewObject(lng, lng_cons, frame.timestamp));
//       env->SetObjectArrayElement(jframe, 1, env->NewObject(lng, lng_cons, frame.id));
//       env->SetObjectArrayElement(jframe, 2, env->NewStringUTF(frame.name.c_str()));
//
//       // we need to place the undefined size vector into a jarray
//       jobjectArray jtrace = env->NewObjectArray(frame.trace.size(), str, nullptr);
//
//       for (int i = 0; i < frame.trace.size(); i++) {
//         jstring message = env->NewStringUTF(frame.trace[i].c_str());
//         env->SetObjectArrayElement(jtrace, i, message);
//       }
//       env->SetObjectArrayElement(jframe, 3, jtrace);
//
//       frames.push_back(jframe);
//     } catch (std::exception) {
//       continue;
//     }
//   }
//
//   // we need to place the undefined size vector into a jarray
//   jobjectArray jframes = env->NewObjectArray(frames.size(), cls, nullptr);
//   for (int i = 0; i < frames.size(); i++) {
//     env->SetObjectArrayElement(jframes, i, frames[i]);
//   }
//
//   return jframes;
// }

extern "C" JNIEXPORT jstring JNICALL Java_asgct_ASGCTReader_pop(JNIEnv *env, jclass jcls) {
  time_t now = time(nullptr) * 1000;

  // grab the reader
  BufferReader *reader = fetchReader(env);

  string frames;

  ASGCTFrame last;
  for (int k = 0; k < reader->size(); k++) {
  // while (!reader->empty() && i++ < size) {
    // try {
      // std::cout << reader->size() << " -> ";
      ASGCTFrame frame = reader->pop();

      // check if the timestamp is 0 or we exceeded the timestamp
      // if (frame.timestamp > 0) {
        // std::cout << reader->size() << ", " << frame.id << "@" << now << "/" << frame.timestamp;
        // if (frame.trace.size() > 0)
          // std::cout << ": " << frame.trace[0];
        // std::cout << std::endl;
      // }

      if (frame.timestamp == 0)
        break;
      else if (frame.timestamp == 0 || (last.timestamp == frame.timestamp && last.id == frame.id)) {
        continue;
      }

      frames.append(std::to_string(frame.timestamp)).append(",");
      frames.append(std::to_string(frame.id)).append(",");
      frames.append(frame.name).append(",");

      for (int i = 0; i < frame.trace.size(); i++)
        frames.append(frame.trace[i].c_str()).append("@");
      frames.append("#");

      if (now < frame.timestamp)
        break;
      last = frame;
    // } catch (std::exception) {
    //   continue;
    // }
  }

  return env->NewStringUTF(frames.c_str());
}
