#pragma once

#include "../AstralPrtctn/json.hpp"
#include "../System/Core/LicenceToken.h"   // signed licence tokens (RSA-SHA256)
#include <jni.h>
#include <curl/curl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/md5.h>
#include <iomanip>
#include <sstream>
#include <ctime>

#include "../AstralPrtctn/json.hpp"
#include "oxorany/source/oxorany.h"

using json = nlohmann::json;
extern JavaVM* jvm;
std::string g_Token, g_Auth;
bool bValid = false;
std::string EXP = "LIFETIME";
std::string usedKey = " ";
std::string mod_status = " ";
std::string max_dev = " ";
std::string userType = "ETHNIR PREM";
int keyUserCount = 0;
time_t expiryTimestamp = 0;

std::string getExpiryCountdown() {
    if (expiryTimestamp == 0) {
        return "Unknown";
    }

    // A permanent key is signed as licence::kNeverExpires (2100-01-01) rather
    // than as 0, so the verifier keeps one "expired yet?" comparison; the UI
    // turns that sentinel back into a word instead of a 27000-day countdown.
    if ((long long)expiryTimestamp >= licence::kNeverExpires) {
        return "Lifetime";
    }

    time_t now = time(nullptr);
    if (now > expiryTimestamp) {
        return "EXPIRED";
    }

    time_t diff = expiryTimestamp - now;
    int days = diff / (24 * 3600);
    diff = diff % (24 * 3600);
    int hours = diff / 3600;
    diff = diff % 3600;
    int minutes = diff / 60;
    int seconds = diff % 60;

    std::stringstream ss;
    ss << days << "d " << hours << "h " << minutes << "m " << seconds << "s";
    return ss.str();
}
JNIEnv* AttachCurrentThread3(JavaVM* vm) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        vm->AttachCurrentThread(&env, nullptr);
    }
    return env;
}

const char *GetAndroidID(JNIEnv *env, jobject context) {
    jclass contextClass = env->FindClass("android/content/Context");
    jmethodID getContentResolverMethod = env->GetMethodID(contextClass, "getContentResolver", "()Landroid/content/ContentResolver;");
    jclass settingSecureClass = env->FindClass("android/provider/Settings$Secure");
    jmethodID getStringMethod = env->GetStaticMethodID(settingSecureClass, "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;");

    auto obj = env->CallObjectMethod(context, getContentResolverMethod);
    auto str = (jstring) env->CallStaticObjectMethod(settingSecureClass, getStringMethod, obj, env->NewStringUTF("android_id"));
    // Settings.Secure.getString() is allowed to answer null, and
    // GetStringUTFChars() on a null jstring aborts the VM instead of returning
    // null, so it never gets one.
    if (!str) {
        return "";
    }
    return env->GetStringUTFChars(str, 0);
}

bool isVipKey(const std::string& key) {
    return key.substr(0, 4) == "VIP-";
}

const char *GetDeviceModel(JNIEnv *env) {
    jclass buildClass = env->FindClass("android/os/Build");
    jfieldID modelId = env->GetStaticFieldID(buildClass, "MODEL", "Ljava/lang/String;");

    auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
    if (!str) {
        return "";
    }
    return env->GetStringUTFChars(str, 0);
}

const char *GetDeviceBrand(JNIEnv *env) {
    jclass buildClass = env->FindClass("android/os/Build");
    jfieldID modelId = env->GetStaticFieldID(buildClass, "BRAND", "Ljava/lang/String;");

    auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
    if (!str) {
        return "";
    }
    return env->GetStringUTFChars(str, 0);
}

const char *GetDeviceUniqueIdentifier(JNIEnv *env, const char *uuid) {
    jclass uuidClass = env->FindClass("java/util/UUID");

    auto len = strlen(uuid);

    jbyteArray myJByteArray = env->NewByteArray(len);
    env->SetByteArrayRegion(myJByteArray, 0, len, (jbyte *) uuid);

    jmethodID nameUUIDFromBytesMethod = env->GetStaticMethodID(uuidClass, "nameUUIDFromBytes", "([B)Ljava/util/UUID;");
    jmethodID toStringMethod = env->GetMethodID(uuidClass, "toString", "()Ljava/lang/String;");

    auto obj = env->CallStaticObjectMethod(uuidClass, nameUUIDFromBytesMethod, myJByteArray);
    auto str = (jstring) env->CallObjectMethod(obj, toStringMethod);
    if (!str) {
        return "";
    }
    return env->GetStringUTFChars(str, 0);
}

// CalcMD5() lived here and existed for exactly one caller: the shared-secret
// licence check (MD5 of "CODMDGR-<key>-<hwid>-<secret>"). Both are gone - the
// app verifies a signature now (System/Core/LicenceToken.h) - and the helper
// went with them so that the next person cannot quietly rebuild the old scheme.
// Tools::CalcMD5 in IL2CppSDKGenerator is unrelated and still there.

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) userp;
    
    mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        return 0;
    }
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

int ShowSoftKeyboardInput() {
    jint result;
    jint flags = 0;
    
    JNIEnv *env;
    jvm->AttachCurrentThread(&env, NULL);
    
    jclass looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
    env->CallStaticVoidMethod(looperClass, prepareMethod);
    
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    jclass contextClass = env->FindClass("android/content/Context");
    jfieldID fieldINPUT_METHOD_SERVICE = env->GetStaticFieldID(contextClass, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
    jobject INPUT_METHOD_SERVICE = env->GetStaticObjectField(contextClass, fieldINPUT_METHOD_SERVICE);
    jmethodID getSystemServiceMethod = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject callObjectMethod = env->CallObjectMethod(mInitialApplication, getSystemServiceMethod, INPUT_METHOD_SERVICE);
    
    jclass classInputMethodManager = env->FindClass("android/view/inputmethod/InputMethodManager");
    jmethodID toggleSoftInputId = env->GetMethodID(classInputMethodManager, "toggleSoftInput", "(II)V");
    
    if (result) {
        env->CallVoidMethod(callObjectMethod, toggleSoftInputId, 2, flags);
    } else {
        env->CallVoidMethod(callObjectMethod, toggleSoftInputId, flags, flags);
    }
    
    env->DeleteLocalRef(classInputMethodManager);
    env->DeleteLocalRef(callObjectMethod);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
    jvm->DetachCurrentThread();
    
    return result;
}

int PollUnicodeChars() {
    JNIEnv *env;
    jvm->AttachCurrentThread(&env, NULL);
    
    jclass looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
    env->CallStaticVoidMethod(looperClass, prepareMethod);
    
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    jclass keyEventClass = env->FindClass("android/view/KeyEvent");
    jmethodID getUnicodeCharMethod = env->GetMethodID(keyEventClass, "getUnicodeChar", "(I)I");
    
    ImGuiIO& io = ImGui::GetIO();
    
    int return_key = env->CallIntMethod(keyEventClass, getUnicodeCharMethod);
    
    env->DeleteLocalRef(keyEventClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
    jvm->DetachCurrentThread();
    
    return return_key;
}

std::string getClipboard() {
    std::string result;
    if (!jvm) {
        return result;
    }

    JNIEnv *env = nullptr;
    if (jvm->AttachCurrentThread(&env, NULL) != JNI_OK || !env) {
        return result;
    }
    
    auto looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
    env->CallStaticVoidMethod(looperClass, prepareMethod);
    
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    auto contextClass = env->FindClass("android/content/Context");
    auto getSystemServiceMethod = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    
    auto str = env->NewStringUTF("clipboard");
    auto clipboardManager = env->CallObjectMethod(mInitialApplication, getSystemServiceMethod, str);
    env->DeleteLocalRef(str);
    
    jclass ClipboardManagerClass = env->FindClass("android/content/ClipboardManager");
    auto getText = env->GetMethodID(ClipboardManagerClass, "getText", "()Ljava/lang/CharSequence;");

    jclass CharSequenceClass = env->FindClass("java/lang/CharSequence");
    auto toStringMethod = env->GetMethodID(CharSequenceClass, "toString", "()Ljava/lang/String;");

    auto text = env->CallObjectMethod(clipboardManager, getText);
    if (text) {
        str = (jstring) env->CallObjectMethod(text, toStringMethod);
        // An empty clipboard answers null; GetStringUTFChars() on that aborts
        // the VM, so it is checked before it is asked.
        if (str) {
            const char *chars = env->GetStringUTFChars(str, 0);
            if (chars) {
                result = chars;
                env->ReleaseStringUTFChars(str, chars);
            }
            env->DeleteLocalRef(str);
        }
        env->DeleteLocalRef(text);
    }
    env->DeleteLocalRef(CharSequenceClass);
    env->DeleteLocalRef(ClipboardManagerClass);
    env->DeleteLocalRef(clipboardManager);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
    jvm->DetachCurrentThread();
    // The std::string, not its c_str(): the local String dies with this
    // function, so returning the pointer handed back freed memory - a tap on
    // "paste" followed by anything that read the key crashed.
    return result;
}

std::string Login(const char *user_key) {
    if (!user_key) {
        return "No access key";
    }
    if (!jvm) {
        return "Secure channel unavailable";
    }

    JNIEnv *env = nullptr;
    if (jvm->AttachCurrentThread(&env, 0) != JNI_OK || !env) {
        return "Secure channel unavailable";
    }
    
    auto looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
    env->CallStaticVoidMethod(looperClass, prepareMethod);
    
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    // A null answer from any of the device getters used to be appended to a
    // std::string as a C string, which is undefined behaviour and crashed on
    // devices without an ANDROID_ID.
    const char *androidId = GetAndroidID(env, mInitialApplication);
    const char *model = GetDeviceModel(env);
    const char *brand = GetDeviceBrand(env);

    std::string hwid = user_key;
    hwid += androidId ? androidId : "";
    hwid += model ? model : "";
    hwid += brand ? brand : "";
    std::string UUID = GetDeviceUniqueIdentifier(env, hwid.c_str());
    jvm->DetachCurrentThread();
    std::string errMsg;
	usedKey = user_key;  
    
	if (isVipKey(user_key)) {  
        userType = "ETHNIR PREM";  
    } else {  
        userType = "ETHNIR FREE";  
    }  
	
    struct MemoryStruct chunk{};
    // calloc, not malloc: the reply is parsed as a C string below, and if curl
    // never writes (a failed connect, a TLS error, an empty body) the buffer has
    // to read as empty rather than as whatever the allocator handed back.
    // json::parse() over that uninitialised byte - and the catch block that then
    // appended it to the error message - is what took the app down the moment
    // "sign in" was tapped.
    chunk.memory = (char *) calloc(1, 1);
    chunk.size = 0;
    if (!chunk.memory) {
        return "Out of memory";
    }
    
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    
    if (curl) {
        std::string api_url = oxorany("https://ethnir.x10.mx/connect");
        curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        // This runs on the render thread, inside the frame that drew the button,
        // so it must never block forever: with no timeout a dead host froze the
        // app until Android killed it, which reads as a crash to the user.
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        char data[4096];
        const int dataLen = snprintf(data, sizeof(data), "game=CODMGR&user_key=%s&serial=%s",
                                     user_key, UUID.c_str());
        // A key long enough to overflow this buffer used to run off the stack.
        if (dataLen <= 0 || (size_t)dataLen >= sizeof(data)) {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            return "Access key is too long";
        }
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        res = curl_easy_perform(curl);
        if (res == CURLE_OK && chunk.size == 0) {
            // Nothing came back: there is no JSON to parse.
            errMsg = "No response from the licence server";
        } else if (res == CURLE_OK) {
            try {
                json result = json::parse(chunk.memory);
                if (result["status"] == true) {
                    // The panel signs a payload; this build verifies it against
                    // the public key compiled in (System/Core/LicenceToken.h).
                    // The endpoint is no longer the authority - the signature is.
                    // A swapped panel can answer, but it cannot sign, so its
                    // tokens land in the "not issued by this service" branch.
                    //
                    // The MD5 shared secret that used to be here is gone on
                    // purpose: it was in the APK, so any panel (or no panel at
                    // all) could reproduce a token this build would accept.
                    const std::string payload = result["data"].value("payload", std::string());
                    const std::string sig     = result["data"].value("sig",     std::string());

                    const licence::Result verdict =
                        licence::CheckSession(payload, sig, user_key, UUID);

                    if (verdict.ok) {
                        bValid = true;
                        g_Token = payload;      // the signed payload, for diagnostics
                        g_Auth.clear();
                        expiryTimestamp = verdict.expires;

                        struct tm when{};
                        localtime_r(&expiryTimestamp, &when);
                        char date[32] = {};
                        strftime(date, sizeof date, "%Y-%m-%d", &when);
                        EXP = date;
                    } else {
                        errMsg = verdict.reason;
                    }
                } else {
                    errMsg = result["reason"].get<std::string>();
                }
            } catch (json::exception &e) {
                // Only ever read the bytes curl actually wrote, and keep the
                // result short enough for the card's one-line error row.
                errMsg = e.what();
                errMsg += " | ";
                errMsg.append(chunk.memory, chunk.size < 120 ? chunk.size : 120);
            }
        } else {
            errMsg = curl_easy_strerror(res);
        }
    }
    curl_easy_cleanup(curl);
    free(chunk.memory);            // the reply leaked on every attempt
    jvm->DetachCurrentThread();
    return bValid ? "OK" : (errMsg.empty() ? "Login failed" : errMsg);
}
