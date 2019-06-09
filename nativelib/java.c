#include <stdio.h>
#include "../main.h"

#define JNIEXPORT
#define JNICALL

typedef struct JNIEnv {

} JNIEnv;

// ref. jni.h (/usr/lib/jvm/default/include/jni.h)
struct _jobject {};
typedef struct _jobject *jobject;
typedef int32_t jint;
typedef jobject jclass;

JNIEXPORT void JNICALL Java_java_lang_System_halt0(JNIEnv *env, jclass class, jint status) {
    printf("called halt0 with %d\n", status);
    request_shutdown(status);
}
