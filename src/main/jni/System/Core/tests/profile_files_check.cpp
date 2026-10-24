#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>
#include "System/Core/ProfileFiles.h"
std::string androidFilesDir;
int main() {
 char temp[]="/tmp/ethnir_profile_checks_XXXXXX";
 char* base=mkdtemp(temp);
 assert(base);
 androidFilesDir=std::string(base)+"/";
 assert(ValidProfileName("F") && !ValidProfileName("../bad") && !ValidProfileName("a/b"));
 assert(GetAutoLoadProfile().empty());

 mkdir((androidFilesDir+"configs").c_str(), 0700);
 {std::ofstream f(ProfilePath("F")); f<<"{\"test\":true}";}
 assert(ProfileExists("F"));
 auto names=ListProfiles();
 assert(names.size()==1 && names[0]=="F");
 assert(SetAutoLoadProfile("F"));
 assert(GetAutoLoadProfile()=="F");
 assert(!RenameProfile("F", "../invalid"));
 assert(RenameProfile("F", "My Profile"));
 assert(!ProfileExists("F") && ProfileExists("My Profile"));
 assert(GetAutoLoadProfile()=="My Profile");
 {std::ifstream file(ProfilePath("My Profile")); std::string contents; std::getline(file, contents); assert(contents=="{\"test\":true}");}
 assert(DeleteProfile("My Profile"));
 assert(GetAutoLoadProfile().empty());
 assert(ListProfiles().empty());
 std::remove(AutoLoadPath().c_str());
 rmdir((androidFilesDir+"configs").c_str());
 rmdir(base);
 puts("PASS: exact profile names, listing, rename, delete, auto-load selection, traversal protection");
}
