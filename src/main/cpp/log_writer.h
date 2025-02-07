#include <jvmti.h>

#include <unordered_set>
#include <fstream>

#include "thread_map.h"
#include "circular_queue.h"
#include "stacktraces.h"

#ifndef LOG_WRITER_H
#define LOG_WRITER_H

using std::ostream;
using std::ofstream;
using std::unordered_set;

typedef unsigned char byte;
typedef int64_t method_id;

class MethodListener {
public:
    virtual void recordNewMethod(method_id methodId, const char *file_name,
            const char *class_name,
            const char *method_name) = 0;

    virtual void recordNewMethod(const map::HashType methodId, const char *fileName,
            const char *className, const char *genericClassName,
            const char *methodName, const char *methodSignature, const char *genericMethodSignature) = 0;

    virtual ~MethodListener() {
    }
};

typedef bool (*GetFrameInformation)(const JVMPI_CallFrame &frame, MethodListener &logWriter);

const size_t FIFO_SIZE = 10;
const byte TRACE_START = 1; // maintain backward compatibility
const byte TRACE_WITH_TIME = 11;
const byte FRAME_BCI_ONLY = 2; // maintain backward compatibility
const byte FRAME_FULL = 21;
const byte NEW_METHOD = 3; // maintain backward compatibility
const byte NEW_METHOD_SIGNATURE = 31;
const byte THREAD_META = 4;
// Error values for line number. If BCI is an error value we report the BCI error value.
const jint ERR_NO_LINE_INFO = -100;
const jint ERR_NO_LINE_FOUND= -101;
// For the record, known BCI error values


// LogWriter should be independently testable without spinning up a JVM
class LogWriter : public QueueListener, public MethodListener {

public:
    explicit LogWriter(std::string &fileName, jvmtiEnv *jvmti);

    explicit LogWriter(ostream &output, GetFrameInformation frameLookup, jvmtiEnv *jvmti);

    virtual void record(const timespec &ts, const JVMPI_CallTrace &trace, ThreadBucketPtr info = ThreadBucketPtr(nullptr));

    void record(const JVMPI_CallTrace &trace, ThreadBucketPtr info = ThreadBucketPtr(nullptr));

    void recordTraceStart(const jint numFrames, map::HashType envHash, ThreadBucketPtr& info);

    void recordTraceStart(const jint numFrames, map::HashType envHash, const timespec &ts, ThreadBucketPtr& info);

    // method are unique pointers, use a long to standardise
    // between 32 and 64 bits
    void recordFrame(const jint bci, const jint lineNumber, method_id methodId);

    void recordFrame(const jint bci, method_id methodId);

    bool lookupFrameInformation(const JVMPI_CallFrame &frame);
    bool lookupFrameInformation2(const JVMPI_CallFrame &frame, char *fqn);

    virtual void recordNewMethod(method_id methodId, const char *file_name,
            const char *class_name, const char *method_name);

    virtual void recordNewMethod(const map::HashType methodId, const char *fileName,
            const char *className, const char *genericClassName,
            const char *methodName, const char *methodSignature, const char *genericMethodSignature);

private:
    ofstream file;
    ostream& output_;
    GetFrameInformation frameInfoFoo;

    jvmtiEnv *const jvmti_;

    unordered_set<method_id> knownMethods;

    unordered_set<map::HashType> knownThreads;

    template<typename T>
    void writeValue(const T &value);

    void writeWithSize(const char *value);

    void inspectMethod(const method_id methodId, const JVMPI_CallFrame &frame);

    void inspectThread(map::HashType &threadId, ThreadBucketPtr& info);

    jint getLineNo(jint bci, jmethodID methodId);

    DISALLOW_COPY_AND_ASSIGN(LogWriter);
};

#endif // LOG_WRITER_H
