/*
 *  preferences.c
 *  
 *  Created by Guillaume Borios on 03/08/04.
 *  Copyright 2004 iOS. All rights reserved.
 *
 */

#include "GTPreferences.h"
#include "GTLog.h"
#include "config.h"
#include "ios_mutex.h"

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>

namespace gametools {

static std::string g_prefFile = "";
void ChangePreferenceFile(const char *name) {
    g_prefFile = name;
}

static ios_fc::Mutex *g_prefMutex = NULL;

void SetIntPreference(const char *pname, int value)
{
    std::string name = g_prefFile + "." + pname;
    if (g_prefMutex == NULL) g_prefMutex = new ios_fc::Mutex();
    ios_fc::Lock prefLock(*g_prefMutex);
    
    CFStringRef nom = CFStringCreateWithCString (NULL,name.c_str(),CFStringGetSystemEncoding());
    if (nom != NULL)
    {
        CFNumberRef aValue = CFNumberCreate(NULL,kCFNumberIntType,&value);
        if (aValue != NULL)
        {
            CFPreferencesSetAppValue (nom,aValue,kCFPreferencesCurrentApplication);
            (void)CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
            CFRelease(aValue);
        }
        CFRelease(nom);
    }
}

void SetBoolPreference(const char * pname, bool value)
{
    std::string name = g_prefFile + "." + pname;
    if (g_prefMutex == NULL) g_prefMutex = new ios_fc::Mutex();
    ios_fc::Lock prefLock(*g_prefMutex);
    
    CFStringRef nom = CFStringCreateWithCString (NULL,name.c_str(),CFStringGetSystemEncoding());
    if (nom != NULL)
    {
        CFPreferencesSetAppValue (nom,value?kCFBooleanTrue:kCFBooleanFalse,kCFPreferencesCurrentApplication);
        (void)CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
        CFRelease(nom);
    }
}

int GetIntPreference(const char * pname, int defaut)
{
    std::string name = g_prefFile + "." + pname;
    if (g_prefMutex == NULL) g_prefMutex = new ios_fc::Mutex();
    ios_fc::Lock prefLock(*g_prefMutex);

    int ret = defaut;
    CFStringRef key = CFStringCreateWithCString (NULL,name.c_str(),CFStringGetSystemEncoding());
    
    if (key != NULL)
    {
        Boolean keyExistsAndHasValidFormat = false;
        CFIndex val = CFPreferencesGetAppIntegerValue(key, kCFPreferencesCurrentApplication, &keyExistsAndHasValidFormat);
        if (keyExistsAndHasValidFormat == true) ret = (int)(val);
        CFRelease(key);
    }
    return ret;
}

bool GetBoolPreference(const char *pname, bool defaut)
{
    std::string name = g_prefFile + "." + pname;
    if (g_prefMutex == NULL) g_prefMutex = new ios_fc::Mutex();
    ios_fc::Lock prefLock(*g_prefMutex);

    bool ret = defaut;
    CFStringRef key = CFStringCreateWithCString (NULL,name.c_str(),CFStringGetSystemEncoding());
    
    if (key != NULL)
    {
        Boolean keyExistsAndHasValidFormat = false;
        Boolean val = CFPreferencesGetAppBooleanValue(key, kCFPreferencesCurrentApplication, &keyExistsAndHasValidFormat);
        if (keyExistsAndHasValidFormat == true) ret = (bool)(val);
        CFRelease(key);
    }
    return ret;
}

void SetStrPreference (const char *pname, const char *value)
{
    std::string name = g_prefFile + "." + pname;
    if (g_prefMutex == NULL) g_prefMutex = new ios_fc::Mutex();
    ios_fc::Lock prefLock(*g_prefMutex);

    CFStringRef nom = CFStringCreateWithCString (NULL,name.c_str(),CFStringGetSystemEncoding());
    if (nom != NULL)
    {
        CFStringRef val = CFStringCreateWithCString (NULL,value,CFStringGetSystemEncoding());
        if (val != NULL)
        {
            CFPreferencesSetAppValue (nom,val,kCFPreferencesCurrentApplication);
            (void)CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
            CFRelease(val);
        }
        CFRelease(nom);
    }
}

void GetStrPreference (const char *pname, char *out, const char *defaut, const int bufferSize)
{
    if (g_prefMutex == NULL) g_prefMutex = new ios_fc::Mutex();
    ios_fc::Lock prefLock(*g_prefMutex);

    if ((out==NULL) || (pname==NULL)) return;
    std::string name = g_prefFile + "." + pname;

    if (defaut != NULL)
    {
        strncpy(out,defaut,bufferSize-1);
        out[bufferSize-1]=0;
    }
    else out[0]=0;

    CFStringRef nom = CFStringCreateWithCString (NULL,name.c_str(),CFStringGetSystemEncoding());
    
    if (nom != NULL)
    {
        CFStringRef value = (CFStringRef)CFPreferencesCopyAppValue(nom,kCFPreferencesCurrentApplication);

        if (value != NULL)
        {
            if (CFGetTypeID(value) == CFStringGetTypeID ())
            {
                if ((!CFStringGetCString (value, out, bufferSize, CFStringGetSystemEncoding())) && (defaut != NULL))
                    strcpy(out,defaut);
            }
            CFRelease(value);
        }
        CFRelease(nom);
    }
}
	
}

#elif defined(ANDROID) /* Not __APPLE__, ANDROID */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include "GTLog.h"

namespace gametools {

static std::string g_prefFile = "";
void ChangePreferenceFile(const char *name) {
    g_prefFile = name;
}

typedef std::map<std::string, std::string> StrStrMap;
static StrStrMap m_cachedPrefs;

void SetBoolPreference(const char * name, bool value) {
    SetIntPreference(name, (int)value);
}
void SetIntPreference(const char *name, int value) {
  char var[256];
  sprintf(var,"%d",value);
  SetStrPreference(name,var);
}
bool GetBoolPreference(const char * name, bool defaut) {
    return GetIntPreference(name,defaut)?true:false;
}
int GetIntPreference(const char *name, int defaut) {
  char var[256];
  var[0] = 0;
  GetStrPreference(name,var,NULL,256);
  if (var[0]) return atoi(var);
  return defaut;
}

void Android_SetStrPreference(const char *name, const char *value);
void Android_GetStrPreference(const char *name, char *out, const char *defaut, const int bufferSize);

void SetStrPreference (const char *pname, const char *value) {
    std::string name = g_prefFile + "." + pname;
    m_cachedPrefs[name] = value;
    Android_SetStrPreference(name.c_str(), value);
}

void GetStrPreference (const char *pname, char *out, const char *defaut, const int bufferSize) {
    std::string name = g_prefFile + "." + pname;
    StrStrMap::iterator it = m_cachedPrefs.find(name);
    if (it == m_cachedPrefs.end()) {
        Android_GetStrPreference(name.c_str(), out, defaut, bufferSize);
        m_cachedPrefs[name] = out;
    }
    else
        strncpy(out, it->second.c_str(), bufferSize);
}

}


#else /* Not __APPLE__, Not ANDROID */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <map>
#include <string>

#ifdef SYMBIAN
#include <QApplication>
#endif

namespace gametools {

static std::string g_prefFile = "";
void ChangePreferenceFile(const char *name) {
    g_prefFile = name;
}

static const char * prefsfile = ".fovea.cfg";
static const char * sep = "\n\r";

static char * file = NULL;
static char * home = NULL;

static bool getPrefsPath(void)
{
    if (home == NULL)
    {
#ifdef SYMBIAN
		char * h = strdup(QApplication::applicationDirPath().toAscii().data());
#else
#ifndef _WIN32
        char * h = getenv("HOME");
#else
        char * h = ".";
#endif
#endif
		if (h==NULL) return false;
        home = (char *)malloc(strlen(h)+strlen(prefsfile)+2);
        if (home == NULL) return false;
        strcpy(home,h);
        strcat(home,"/");
        strcat(home,prefsfile);
#ifdef SYMBIAN
		free(h);
#endif
    }
    return true;
}

static void fetchFile(void)
{
    FILE * prefs;
    struct stat myStat;
    
    // If we already have loaded the file, ignore the request
    if (file != NULL) return;
    
    // Set up a fake file if the file path cannot be determined
    if (getPrefsPath() == false)
    {
      file = (char *)malloc((size_t)1);
      file[0]=0;
      return;
    }

    // Else try to read the file
    prefs = fopen(home, "r");

    // Set up a fake file if the real one cannot be read
    if (prefs == NULL)
    {
      file = (char *)malloc((size_t)1);
      file[0]=0;
      return;
    }

    // Check the file size and try to read it or set up a fake file if problem
    if (stat(home,&myStat) == 0)
    {
        file = (char *)malloc((size_t)(myStat.st_size)+1);
        int l = fread(file, 1,(size_t)(myStat.st_size), prefs);
        file[l]=0;
    }
    else
    {
      file = (char *)malloc((size_t)1);
      file[0]=0;
    }

    // Close the file
    fclose(prefs);
}

static void storeFile(void)
{    
    // Ensure we know where to write
    getPrefsPath();

    // Ignore the request if the file path cannot be determined or the memory image doesn't exist
    if ( (file == NULL) || (home == NULL) )
    {
      return;
    }

    GTLogf("Saving preferences to %s", home);
    // Try to open the file to write it
    FILE * prefs = fopen(home, "w");
    if (prefs == NULL) return;

    // Store the image and close the file
    //fprintf(stderr,"Writing to %s\n%s",home,file);
    fprintf(prefs,"%s",file);
    fclose(prefs);
}

void SetBoolPreference(const char * name, bool value)
{
    SetIntPreference(name, (int)value);
}

void SetIntPreference(const char *name, int value)
{
  char var[256];
  sprintf(var,"%d",value);
  SetStrPreference(name,var);
}


bool GetBoolPreference(const char * name, bool defaut)
{
    return GetIntPreference(name,defaut)?true:false;
}

int GetIntPreference(const char *name, int defaut)
{
  char var[256];
  var[0] = 0;
  GetStrPreference(name,var,NULL,256);
  if (var[0]) return atoi(var);
  return defaut;
}

static void System_SetStrPreference (const char *name, const char *value)
{
    char * key;
    char * prefs;
    
    // if no name given, ignore
    if (name == NULL) return;
    
    // if no value given, use an empty string
    if (value == NULL) value = "";
    
    // Read current value to check if file update is really needed
    int valueLen = strlen(value);
    char * oldValue = (char *)malloc(valueLen+2);
    if (oldValue != NULL)
    {
      // If equal, ignore request
      GetStrPreference(name, oldValue, "core.preferences.fakeoldvalue", valueLen+2);
      if (!strcmp(oldValue,value) && strlen(oldValue) == strlen(value)) {
	free(oldValue);
	return;
      }
      free(oldValue);
    }

    // Not equal... we should update the memory image and the file

    // No memory image available, return...
    if (file == NULL) return;

    // Allocate a new mem image or die
    prefs = (char*)malloc(strlen(file) + strlen(name) + strlen(value) + strlen("=\n") + 1);
    if (prefs == NULL) return;
    
    // Copy the old image to the new, updating the right line...
    prefs[0]=0;
    int l = 0;
    for (key = strtok(file, sep); key; key = strtok(NULL, sep))
    {
        if (strstr(key,name) != key)
        {
          sprintf(prefs+l,"%s\n",key);
          l += strlen(key)+1;
        }
    }
    sprintf(prefs+l,"%s=%s\n",name,value);
    free(file);
    file = prefs;

    // Finally try to store the file
    storeFile();
}


static void System_GetStrPreference (const char *name, char *out, const char *defaut, const int bufferSize)
{
    char * key, *copiedfile;
    int tmplen;

    if ((out==NULL) || (name==NULL)) return;
    
    if (defaut != NULL)
    {
        strncpy(out,defaut,bufferSize-1);
        out[bufferSize-1]=0;
    }
    else out[0]=0;
    
    fetchFile();
    if (file==NULL) return;
    
    char tmp[256];
    sprintf(tmp,"%s=",name);
    tmplen = strlen(tmp);

    copiedfile = strdup(file);
    if (copiedfile == NULL) return;

    for (key = strtok(copiedfile, sep); key; key = strtok(NULL, sep))
    {
        if (strncmp(key, tmp, tmplen) == 0)
        {
            strncpy(out, key+tmplen, bufferSize-1);
            out[bufferSize-1] = 0;
            break;
        }
    }
    free(copiedfile);
    
    return;
}

typedef std::map<std::string, std::string> StrStrMap;
static StrStrMap m_cachedPrefs;

void SetStrPreference (const char *pname, const char *value) {
    std::string name = g_prefFile + "." + pname;
    m_cachedPrefs[name] = value;
    System_SetStrPreference(name.c_str(), value);
}

void GetStrPreference (const char *pname, char *out, const char *defaut, const int bufferSize) {
    std::string name = g_prefFile + "." + pname;
    StrStrMap::iterator it = m_cachedPrefs.find(name);
    if (it == m_cachedPrefs.end()) {
        System_GetStrPreference(name.c_str(), out, defaut, bufferSize);
        m_cachedPrefs[name] = out;
    }
    else
        strncpy(out, it->second.c_str(), bufferSize);
}

}

#endif /* Not __APPLE__, Not ANDROID */

namespace gametools {

#define fltCoef 65536
    
    float GetFloatPreference(const char *name, float defaut) {
        return (float)GetIntPreference(name, defaut * fltCoef) / fltCoef;
    }
    
    void SetFloatPreference(const char *name, float value) {
        SetIntPreference(name, value * fltCoef);
    }

}