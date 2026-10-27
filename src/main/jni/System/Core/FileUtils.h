#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <jni.h>
#include "Includes/Logger.h"
#include "Includes/Jni_Stuff.h"
#include "System/Core/LoginRemember.h"
#include "System/Core/DataRoot.h"

extern JavaVM* VM;
extern char logintext[4096];

// Asks the Application for one of its directories and returns its absolute
// path: getFilesDir() for the private directory, getExternalFilesDir(null) for
// the external one. PassNullString selects the latter, whose signature takes a
// (null) type argument. Both return "" on any failure, and every caller treats
// that as "do not use this location" rather than as an empty path.
static std::string ApplicationDirPath(JavaVM* vm, const char* method,
                                      const char* signature, bool passNullString) {
    JNIEnv* env = AttachCurrentThread3(vm);
    if (!env) {
        LOGE("Failed to get JNIEnv");
        return "";
    }

    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    if (!activityThreadClass) {
        LOGE("Failed to find android/app/ActivityThread");
        return "";
    }

    jobject application = nullptr;
    jmethodID currentApplicationMethod = env->GetStaticMethodID(
        activityThreadClass, "currentApplication", "()Landroid/app/Application;");
    if (currentApplicationMethod) {
        application = env->CallStaticObjectMethod(activityThreadClass, currentApplicationMethod);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            application = nullptr;
        }
    }

    jobject activityThread = nullptr;
    if (!application) {
        jmethodID currentActivityThreadMethod = env->GetStaticMethodID(
            activityThreadClass, "currentActivityThread", "()Landroid/app/ActivityThread;");
        if (currentActivityThreadMethod) {
            activityThread = env->CallStaticObjectMethod(activityThreadClass, currentActivityThreadMethod);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                activityThread = nullptr;
            }
        }
        if (!activityThread) {
            LOGE("Failed to get current ActivityThread");
            return "";
        }

        jmethodID getApplicationMethod = env->GetMethodID(
            activityThreadClass, "getApplication", "()Landroid/app/Application;");
        application = env->CallObjectMethod(activityThread, getApplicationMethod);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            application = nullptr;
        }
    }

    if (!application) {
        LOGE("Failed to get Application instance");
        return "";
    }

    jclass contextClass = env->GetObjectClass(application);
    jmethodID getDirMethod = env->GetMethodID(contextClass, method, signature);
    if (!getDirMethod) {
        LOGE("Failed to find %s", method);
        env->ExceptionClear();
        return "";
    }
    jobject filesDir = passNullString
        ? env->CallObjectMethod(application, getDirMethod, (jstring) nullptr)
        : env->CallObjectMethod(application, getDirMethod);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        filesDir = nullptr;
    }
    if (!filesDir) {
        LOGE("Failed to get %s", method);
        return "";
    }

    jclass fileClass = env->GetObjectClass(filesDir);
    jmethodID getAbsolutePathMethod = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    jstring absolutePath = static_cast<jstring>(env->CallObjectMethod(filesDir, getAbsolutePathMethod));
    if (!absolutePath) {
        LOGE("Failed to get absolute path");
        return "";
    }

    const char* pathChars = env->GetStringUTFChars(absolutePath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(absolutePath, pathChars);

    if (activityThread) env->DeleteLocalRef(activityThread);
    env->DeleteLocalRef(application);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(filesDir);
    env->DeleteLocalRef(fileClass);
    env->DeleteLocalRef(absolutePath);

    return path;
}

std::string GetFilesDirPath(JavaVM* vm) {
    return ApplicationDirPath(vm, "getFilesDir", "()Ljava/io/File;", false);
}

std::string GetExternalFilesDirPath(JavaVM* vm) {
    return ApplicationDirPath(vm, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;", true);
}

// One resolved storage root for the mod's own files - profiles, autoload and
// the log the menu points at. Candidates are tried in order:
//
//   1. the game's external directory, which is where this mod's profiles have
//      always lived on a plain install and is ours to create;
//   2. what the Application itself reports as external storage, which is the
//      same path on a plain install and the container's mount of it inside
//      GSpace or ParallelSpace;
//   3. the two container layouts reported from real devices, accepted only when
//      they are really there - another app's directory is not ours to create;
//   4. the app's private directory, which always exists and is always writable.
//
// A plain install therefore lands exactly where it always did, and a container
// install lands on something that works instead of on a path that was never
// created. The choice is logged once so a device report can say where the files
// went.
std::string GetDataRoot() {
    static std::string root;
    static bool resolved = false;
    if (resolved) return root;

    std::vector<data_root::Candidate> candidates;
    candidates.emplace_back(data_root::kGameExternal, true);

    const std::string external = GetExternalFilesDirPath(VM);
    if (!external.empty()) candidates.emplace_back(external, true);

    const std::vector<data_root::Candidate> containers = data_root::ContainerCandidates();
    candidates.insert(candidates.end(), containers.begin(), containers.end());

    const std::string internal = GetFilesDirPath(VM);
    if (!internal.empty()) candidates.emplace_back(internal, true);

    root = data_root::Select(candidates);
    if (root.empty()) root = data_root::kGameExternal;   // never empty: today's path
    resolved = true;
    LOGI("[data] storage root: %s", root.c_str());
    return root;
}

bool SaveLoginTextToFile(const char* logintext) {
    std::string filesDir = GetFilesDirPath(VM);
    if (filesDir.empty()) {
        return false;
    }

    std::string textfile = filesDir + "/codm.ini";
    std::ofstream file(textfile.c_str(), std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file << logintext;
    file.close();
    return true;
}

// The saved key is only kept while the "Remember me" box asks for it, so the two
// reads and writes below always go through the same names and the same rule.
bool SaveLoginRememberToFile(bool remember) {
    std::string filesDir = GetFilesDirPath(VM);
    if (filesDir.empty()) {
        return false;
    }

    std::ofstream file((filesDir + "/" + login_remember::kFlagFile).c_str(),
                       std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file << login_remember::StoredFlagText(remember);
    file.close();
    return true;
}

bool LoadLoginRememberFromFile() {
    std::string filesDir = GetFilesDirPath(VM);
    if (filesDir.empty()) {
        return true;
    }

    std::ifstream file((filesDir + "/" + login_remember::kFlagFile).c_str());
    if (!file.is_open()) {
        return true;
    }

    std::string flag;
    std::getline(file, flag);
    file.close();
    return login_remember::FlagFromStoredText(flag);
}

bool ForgetLoginTextFile() {
    std::string filesDir = GetFilesDirPath(VM);
    if (filesDir.empty()) {
        return false;
    }

    std::ofstream file((filesDir + "/" + login_remember::kKeyFile).c_str(),
                       std::ios::out | std::ios::trunc);
    return file.is_open();
}

bool LoadTextFromFile() {
    std::string filesDir = GetFilesDirPath(VM);
    if (filesDir.empty()) {
        return false;
    }

    std::string textfile = filesDir + "/codm.ini";
    std::ifstream file(textfile.c_str());
    std::stringstream buffer;

    if (file.is_open()) {
        buffer << file.rdbuf();
        file.close();
        strncpy(logintext, buffer.str().c_str(), sizeof(logintext));
        logintext[sizeof(logintext) - 1] = '\0';
        return true;
    }

    return false;
}

#endif

