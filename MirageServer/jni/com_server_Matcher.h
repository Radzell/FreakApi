/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_server_Matcher */

#ifndef _Included_com_server_Matcher
#define _Included_com_server_Matcher
#ifdef __cplusplus
extern "C" {
#endif
#undef com_server_Matcher_WRONG_THRESHOLD
#define com_server_Matcher_WRONG_THRESHOLD 0.01f
/*
 * Class:     com_server_Matcher
 * Method:    recognition
 * Signature: (Ljava/lang/String;)[I
 */
JNIEXPORT jintArray JNICALL Java_com_server_Matcher_recognition
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_server_Matcher
 * Method:    load
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_server_Matcher_load
  (JNIEnv *, jclass);

/*
 * Class:     com_server_Matcher
 * Method:    print
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_server_Matcher_print
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif
