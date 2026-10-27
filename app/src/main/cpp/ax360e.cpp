// SPDX-License-Identifier: WTFPL

#include <jni.h>
#include <android/log.h>

#define LOGI(...) {}
#define LOGW(...) {}
#define LOGE(...) {}
#define PR {}
#define PRE(f) {}

int register_Emulator(JNIEnv* env);
int register_Emulator$Config(JNIEnv* env);

int register_ax360e_Emulator(JNIEnv* env);
int register_ax360e_profile(JNIEnv* env);
int cache_ax360e_activity_jni(JNIEnv* env);

JavaVM* g_jvm;

extern "C" __attribute__((visibility("default")))
jint JNI_OnLoad(JavaVM* vm, void* reserved){

    g_jvm = vm;

    JNIEnv* env = NULL;
    int result=-1;

    LOGW("JNI_OnLoad ");
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv failed");
        goto bail;
    }

    if(register_Emulator(env) != JNI_OK){
        LOGE("register_Emulator failed");
        goto bail;
    }

    if(register_Emulator$Config(env) != JNI_OK){
        LOGE("register_Emulator$Config failed");
        goto bail;
    }

    if(register_ax360e_Emulator(env) != JNI_OK){
        LOGE("register_ax360e_Emulator failed");
        goto bail;
    }

    if(register_ax360e_profile(env) != JNI_OK){
        LOGE("register_ax360e_profile failed");
        goto bail;
    }

    // Optional soft keyboard bridge; must not break library loading on failure.
    cache_ax360e_activity_jni(env);
    result = JNI_VERSION_1_6;

    LOGW("JNI_OnLoad OK");

    bail:
    return result;
}