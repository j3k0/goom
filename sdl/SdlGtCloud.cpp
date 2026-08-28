//
// Local-only GTCloud backend for desktop builds.
//
// Mirrors iphone/xcodeproject/Classes/iPhoneGtCloud.mm minus the
// network push/pull (fovea.cc cloud service): saves go to the user
// directory, loads fall back to the read-only resources copy.
//

#include "GTCloud.h"
#include "GTLog.h"
#include "ios_fc.h"
#include "ios_mutex.h"
#include <fstream>
#include <string>
#include <cstdio>

namespace gametools {

static ios_fc::Mutex *g_cloudMutex = NULL;
#define SCOPED_CLOUD_LOCK \
    if (g_cloudMutex == NULL) g_cloudMutex = new ios_fc::Mutex(); \
    ios_fc::Lock cloudLock(*g_cloudMutex)

static std::string g_localRepository;
static std::string g_remoteRepository;
static std::string g_userDir;      // writable, e.g. SDL_GetPrefPath()
static std::string g_resourcesDir; // read-only bundle resources

void SdlGtCloudSetPaths(const char *userDir, const char *resourcesDir) {
    g_userDir = userDir;
    g_resourcesDir = resourcesDir;
}

void GTCloudDisableNetwork() {
}

void GTCloudSetRepository(const char *local, const char *remote) {
    SCOPED_CLOUD_LOCK;
    g_localRepository  = local;
    g_remoteRepository = remote;
}

static std::string cloudGetUserFilePath(const char *fileName) {
    return g_userDir + "/" + g_remoteRepository + "-Cloud-" + fileName;
}

static std::string cloudGetResourcesFilePath(const char *fileName) {
    return g_resourcesDir + "/" + g_localRepository + "-Cloud/" + fileName;
}

static ios_fc::VoidBuffer cloudReadFile(std::ifstream &is) {
    ios_fc::VoidBuffer output;
    is.seekg(0, std::ios::end);
    int length = is.tellg();
    is.seekg(0, std::ios::beg);
    output.realloc(length);
    is.read(output, length);
    is.close();
    return output;
}

ios_fc::VoidBuffer GTCloudLoad(const char *fileName) {
    SCOPED_CLOUD_LOCK;
    GTLogf("Trying to load %s", fileName);
    if (g_localRepository == "") return ios_fc::VoidBuffer();
    std::string rwPath = cloudGetUserFilePath(fileName);
    std::ifstream rwFile(rwPath.c_str(), std::ios::binary);
    if (rwFile) return cloudReadFile(rwFile);
    std::string roPath = cloudGetResourcesFilePath(fileName);
    std::ifstream roFile(roPath.c_str(), std::ios::binary);
    if (roFile) return cloudReadFile(roFile);
    GTLogf("FILE NOT FOUND: %s", roPath.c_str());
    throw ios_fc::Exception("Could not find file");
}

void GTCloudSave(const char *fileName, ios_fc::VoidBuffer buffer) {
    SCOPED_CLOUD_LOCK;
    if (g_localRepository == "") return;
    std::string rwPath = cloudGetUserFilePath(fileName);
    std::ofstream rwFile(rwPath.c_str(), std::ios::binary);
    rwFile.write(buffer, buffer.size());
    rwFile.close();
}

void GTCloudPush(const char *fileName) {
}

void GTCloudPull(const char *fileName) {
}

void GTCloudRevertPull(const char *fileName) {
    SCOPED_CLOUD_LOCK;
    if (g_localRepository == "") return;
    std::string currentFileName = cloudGetUserFilePath(fileName);
    std::string backupFileName  = currentFileName + ".bak";
    bool fileexists = false;
    {
        std::ifstream ifFile(backupFileName.c_str(), std::ios::binary);
        if (ifFile) fileexists = true;
    }
    std::remove(currentFileName.c_str());
    if (fileexists)
        std::rename(backupFileName.c_str(), currentFileName.c_str());
}

}
