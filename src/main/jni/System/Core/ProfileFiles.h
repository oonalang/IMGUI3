#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>

extern std::string androidFilesDir;

inline bool ValidProfileName(const std::string& name)
{
    if (name.empty() || name.size() > 63 || name == "." || name == "..") return false;
    if (name.front() == ' ' || name.back() == ' ') return false;
    for (unsigned char ch : name)
        if (!(std::isalnum(ch) || ch == '_' || ch == '-' || ch == ' ')) return false;
    return true;
}

inline std::string ProfilePath(const std::string& name)
{
    return ValidProfileName(name) ? androidFilesDir + "configs/" + name + ".json" : std::string();
}

inline bool ProfileExists(const std::string& name)
{
    const std::string path = ProfilePath(name);
    if (path.empty()) return false;
    struct stat st{};
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

inline std::string AutoLoadPath() { return androidFilesDir + "configs/autoload.profile"; }

inline std::string GetAutoLoadProfile()
{
    std::ifstream file(AutoLoadPath());
    std::string name;
    if (!file.is_open() || !std::getline(file, name) || !ValidProfileName(name))
        return std::string();
    return name;
}

inline bool SetAutoLoadProfile(const std::string& name)
{
    if (!name.empty() && !ProfileExists(name)) return false;
    const std::string dir = androidFilesDir + "configs";
    mkdir(dir.c_str(), 0775);
    const std::string tmp = AutoLoadPath() + ".tmp";
    {
        std::ofstream file(tmp, std::ios::trunc);
        if (!file.is_open()) return false;
        if (!name.empty()) file << name << '\n';
        file.flush();
        if (!file.good()) return false;
    }
    if (std::rename(tmp.c_str(), AutoLoadPath().c_str()) != 0)
    {
        std::remove(tmp.c_str());
        return false;
    }
    return true;
}

inline bool RenameProfile(const std::string& from, const std::string& to)
{
    if (!ProfileExists(from) || !ValidProfileName(to) || ProfileExists(to)) return false;
    const bool selected = GetAutoLoadProfile() == from;
    if (std::rename(ProfilePath(from).c_str(), ProfilePath(to).c_str()) != 0) return false;
    if (selected && !SetAutoLoadProfile(to))
    {
        std::rename(ProfilePath(to).c_str(), ProfilePath(from).c_str());
        return false;
    }
    return true;
}

inline bool DeleteProfile(const std::string& name)
{
    if (!ProfileExists(name)) return false;

    const bool selected = GetAutoLoadProfile() == name;
    if (std::remove(ProfilePath(name).c_str()) != 0) return false;
    if (selected) SetAutoLoadProfile("");
    return true;
}

inline std::vector<std::string> ListProfiles()
{
    std::vector<std::string> out;
    std::string dir = androidFilesDir + "configs";
    DIR* d = opendir(dir.c_str());
    if (!d) return out;
    struct dirent* e;
    while ((e = readdir(d)) != nullptr)
    {
        std::string name = e->d_name;
        if (name.size() > 5 && name.compare(name.size() - 5, 5, ".json") == 0 &&
            ValidProfileName(name.substr(0, name.size() - 5)))
            out.push_back(name.substr(0, name.size() - 5));
    }
    closedir(d);
    std::sort(out.begin(), out.end());
    return out;
}

