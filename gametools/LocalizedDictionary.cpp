/* FloboPuyo
 * Copyright (C) 2004
 *   Florent Boudet        <flobo@ios-software.com>,
 *   Jean-Christophe Hoelt <jeko@ios-software.com>,
 *   Guillaume Borios      <gyom@ios-software.com>
 *
 * iOS Software <http://www.ios-software.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 */

/* This class is not thread safe */
#ifdef MACOSX
 #include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef WIN32
 /* Stupid trick for WIN32 */
 #ifdef DATADIR
  #undef DATADIR
 #endif
 #include "windows.h"
#endif

#ifdef SYMBIAN
#include <QLocale>
#endif

#include "config.h"
#include "LocalizedDictionary.h"
#include "GTPreferences.h"
#include "GTLog.h"
#include "ios_memory.h"
#include <stdio.h>
#include HASH_MAP_H
#include <cstring>
#include <string>
#include <stdint.h>

using namespace ios_fc;

namespace gametools {

/*************************** CUSTOM HASHMAP STUFF *********************************/
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
|| defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif
	
#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
+(uint32_t)(((const uint8_t *)(d))[0]) )
#endif
	
	// by Paul Hsieh
	// http://www.azillionmonkeys.com/qed/hash.html
	struct SuperFastHashString {
		size_t operator()(std::string datas) const {
			const char * data = datas.c_str();
			uint32_t len = datas.size();
			uint32_t hash = len, tmp;
			int rem;
			
			if (len <= 0 || data == NULL) return 0;
			
			rem = len & 3;
			len >>= 2;
			
			/* Main loop */
			for (;len > 0; len--) {
				hash  += get16bits (data);
				tmp    = (get16bits (data+2) << 11) ^ hash;
				hash   = (hash << 16) ^ tmp;
				data  += 2*sizeof (uint16_t);
				hash  += hash >> 11;
			}
			
			/* Handle end cases */
			switch (rem) {
				case 3: hash += get16bits (data);
					hash ^= hash << 16;
					hash ^= data[sizeof (uint16_t)] << 18;
					hash += hash >> 11;
					break;
				case 2: hash += get16bits (data);
					hash ^= hash << 11;
					hash += hash >> 17;
					break;
				case 1: hash += *data;
					hash ^= hash << 10;
					hash += hash >> 1;
			}
			
			/* Force "avalanching" of final 127 bits */
			hash ^= hash << 3;
			hash += hash >> 5;
			hash ^= hash << 4;
			hash += hash >> 17;
			hash ^= hash << 25;
			hash += hash >> 6;
			
			return hash;
		}
	};
	
	struct EqualString {
		bool operator()(const std::string s1, const std::string s2) const {
			return (s1 == s2);
		}
	};
	
        typedef HASH_NAMESPACE::hash_map<std::string, void *, SuperFastHashString, EqualString> str_dictionnary;
	
	typedef struct {
		str_dictionnary * dictionary;
		int refcount;
	} dictionaryEntry;
	
	static str_dictionnary dictionaries;
	

static bool readLine(DataInputStream *dictionaryFile, String &lineRead)
{
    bool result = true;
    char newChar[2];
    newChar[1] = 0;
    String newLineRead;
    do {
        if (dictionaryFile->streamRead(newChar, 1) != 1)
            result = false;
		if (result && (newChar[0] != 10) && (newChar[0] != 13)) // Ignore CR and LF
            newLineRead += newChar;
	} while (result && (newChar[0] != 10)); // Stop reading on CR only

    // Converting the escape sequences
    if (result) {
        const char *text = newLineRead;
        lineRead = "";
        char previousChar = text[0];
        for (unsigned int i = 0 ; i < strlen(text) ; i++) {
            char texti = text[i];
            switch (previousChar) {
                case '\\':
                    switch (texti) {
                        case 'n':
                            lineRead += '\n';
                            break;
                        default:
                            lineRead += texti;
                    }
                    break;
                default:
                    switch (texti) {
                        case '\\':
                            break;
                        default:
                            lineRead += texti;
                    }
            }
            previousChar = texti;
        }
    }
    return result;
}
	
static bool readLinesTillBlank(DataInputStream *dictionaryFile, String &fullLine) {
    bool fileOk;
	bool first = true;
	String line;
	do {
		fileOk = readLine(dictionaryFile, line);
		if (line != "") {
			if (first) {
				fullLine = line;
				first = false;
			}
			else {
				fullLine += '\n';
				fullLine += line;
			}
		}
		else break;
	} while (fileOk);
	return fileOk;
}


/*************************************************************************************/

/* English is the default */
#define kGTDefaultPreferedLanguage "en"
/* Additionnaly we'll try at most 10 user languages */
#define kGTMaxPreferedLanguage 2

// +1 for the default one, +1 for the $LANG one, +2 for the non-checked windows algorigthm.
char *PreferedLocales[kGTMaxPreferedLanguage+4];

int   PreferedLocalesCount = 0;
static bool  systemInitiated = false;
static const char *gDefaultLocale = NULL;

void Locales_SetDefault(const char *defaultLocale) {
    if (defaultLocale)
        gDefaultLocale = strdup(defaultLocale);
}

void Locales_Init(const char *forceLocale)
{
  if (forceLocale == NULL) {
	  static char storedLocale[16];
	  GetStrPreference("ForceLocale", storedLocale, "", 16);
	  if (storedLocale[0] != '\0')
		  forceLocale = &storedLocale[0];
  }
	
  if (forceLocale != NULL) {
	  SetStrPreference("ForceLocale", forceLocale);
	  systemInitiated = false;
	  PreferedLocales[0] = strdup(forceLocale);
	  PreferedLocalesCount = 1;
	  dictionaries.clear();
  }

  if (!systemInitiated) {

#ifdef DEBUG
	GTLog("Languages detection.");
#endif

#if defined(MACOSX)

    CFStringRef localeIdentifier;
    CFArrayRef prefArray;
    char canonicalLocale[32];

    prefArray = (CFArrayRef) CFPreferencesCopyValue(CFSTR("AppleLanguages"), kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
    int nPrefLocales = (prefArray ? CFArrayGetCount(prefArray) : 0);

	int i = 0;
	for (i=0; i < nPrefLocales; i++) {
        localeIdentifier = (CFStringRef)CFArrayGetValueAtIndex(prefArray, i);
        CFStringGetCString(localeIdentifier, canonicalLocale, 32, kCFStringEncodingUTF8);
        if (canonicalLocale[0] < 'a') canonicalLocale[0] += 'a'-'A';
        if (canonicalLocale[1] < 'a') canonicalLocale[1] += 'a'-'A';
        canonicalLocale[2] = 0;
        PreferedLocales[PreferedLocalesCount] = strdup(canonicalLocale);
		PreferedLocalesCount++;
		if (PreferedLocalesCount >= kGTMaxPreferedLanguage)
			break;
	}

#elif defined(WIN32)

#define WinKnownLangsNb 18
    static const char * WinKnownLangsNames[WinKnownLangsNb] = {"fr","en","ja","de","es","it","nl","sv","da","pt","fi","no","ru","ar","el","he","ca","zh"};
    static const WORD WinKnownLangsCodes[WinKnownLangsNb] = {LANG_FRENCH, LANG_ENGLISH, LANG_JAPANESE, LANG_GERMAN, LANG_SPANISH, LANG_ITALIAN, LANG_DUTCH, LANG_SWEDISH, LANG_DANISH, LANG_PORTUGUESE, LANG_FINNISH, LANG_NORWEGIAN, LANG_RUSSIAN, LANG_ARABIC, LANG_GREEK, LANG_HEBREW, LANG_CATALAN, LANG_CHINESE};

    WORD winLang;

    winLang = PRIMARYLANGID(GetUserDefaultLangID());
	int i = 0;
    for (; i<WinKnownLangsNb; i++)
    {
      if (WinKnownLangsCodes[i] == winLang)
      {
		if (PreferedLocalesCount >= kGTMaxPreferedLanguage) break;
		PreferedLocales[PreferedLocalesCount] = strdup(WinKnownLangsNames[i]);
        PreferedLocalesCount++;
        break;
      }
    }

    winLang = PRIMARYLANGID(GetSystemDefaultLangID());
    for (; i<WinKnownLangsNb; i++)
    {
      if (WinKnownLangsCodes[i] == winLang)
	  {
		if (PreferedLocalesCount >= kGTMaxPreferedLanguage) break;
		PreferedLocales[PreferedLocalesCount] = strdup(WinKnownLangsNames[i]);
        PreferedLocalesCount++;
        break;
      }
    }

	char * my_lang = getenv("LANG");
	if ((my_lang != NULL) && (strlen(my_lang) >= 2) && (PreferedLocalesCount < kGTMaxPreferedLanguage)) {
	  PreferedLocales[PreferedLocalesCount] = strdup(my_lang);
	  PreferedLocales[PreferedLocalesCount][2] = 0;
	  PreferedLocalesCount++;
	}

#elif defined(SYMBIAN)

#define SymbianKnownLangsNb 18
	static const char *SymbianKnownLangsNames[SymbianKnownLangsNb] = {"en","fr","ar","de","es","it","ja","nl","sv","da","pt","fi","no","ru","el","he","ca","zh"};

	QLocale qLocale;
	QString qLanguage = qLocale.name().left(2).toLower();
	PreferedLocales[PreferedLocalesCount++] = strdup(qLanguage.toAscii().data());
	for (int i=0; i<SymbianKnownLangsNb; ++i) {
		if (qLanguage != SymbianKnownLangsNames[i])
			PreferedLocales[PreferedLocalesCount++] = strdup(SymbianKnownLangsNames[i]);
		if (PreferedLocalesCount >= kGTMaxPreferedLanguage)
			break;
	}

#else // Default

    const char * my_lang = (gDefaultLocale != NULL ? gDefaultLocale : getenv("LANG"));
    if ((my_lang != NULL) && (strlen(my_lang) >= 2)) {
        if (strncmp(my_lang, "zh", 2) == 0)
            PreferedLocales[PreferedLocalesCount] = strdup("cn");
        else {
            PreferedLocales[PreferedLocalesCount] = strdup(my_lang);
            PreferedLocales[PreferedLocalesCount][2] = 0;
        }
        PreferedLocalesCount++;
    }

#endif

    /* Finally set the default language (the reason for the +1 in PreferedLocales declaration) */
    bool defaultIsFound = false;
	for (int j=0; j<PreferedLocalesCount; ++j) {
		if (strcmp(PreferedLocales[j],kGTDefaultPreferedLanguage) == 0) {
			defaultIsFound = true;
			break;
		}
	}
	if (!defaultIsFound) {
		PreferedLocales[PreferedLocalesCount] = strdup(kGTDefaultPreferedLanguage);
		PreferedLocalesCount++;
	}

	if (PreferedLocalesCount == 0) {
		PreferedLocalesCount = 1;
		PreferedLocales[0] = strdup(kGTDefaultPreferedLanguage);
	}
	else if (strcmp(PreferedLocales[0], kGTDefaultPreferedLanguage) == 0) {
		PreferedLocalesCount = 1;
	}
#ifdef DEBUG
	for (int i = 0; i < PreferedLocalesCount; i++) {
		char tmp[512];
		sprintf(tmp, "User prefered language %d: '%s'", i, PreferedLocales[i]);
		GTLog(tmp);
	}
#endif

    systemInitiated = true;
  }
}

/*************************************************************************************/

LocalizedDictionary::LocalizedDictionary(const DataPathManager &datapathManager, const char *dictionaryDirectory, const char *dictionaryName) : dictionary(NULL), datapathManager(datapathManager)
{
  signed int i;

  /* First create the prefered languages list whenever needed */
  Locales_Init();
	
  stdName = FilePath::combine(dictionaryDirectory, dictionaryName);
  dictionaryEntry * myDictEntry = (dictionaryEntry *)dictionaries[std::string((const char *)stdName)];

  if (myDictEntry == NULL)
  {
    myDictEntry = (dictionaryEntry *)malloc(sizeof(dictionaryEntry));
    myDictEntry->dictionary = new str_dictionnary;
    myDictEntry->refcount=0;
    dictionaries[std::string((const char *)stdName)] = (void *)myDictEntry;
	  
    //printf("++Loading %s\n", stdName.c_str());

    /* Get the first matching dictionary */
    bool found = false;
	
    for (i = PreferedLocalesCount - 1; i >= 0 ; i--) {

        /* try to open the dictionary for the selected locale */
        String locale(PreferedLocales[i]);
        String directoryName = FilePath::combine(dictionaryDirectory, locale);
        String dictFilePath = FilePath::combine(directoryName, dictionaryName) + ".dic";
        DataInputStream *dictionaryFile = NULL;
        if (datapathManager.hasDataInputStream(dictFilePath))
            dictionaryFile = datapathManager.openDataInputStream(dictFilePath);
        // try { dictionaryFile = fopen(datapathManager.getPath(dictFilePath), "r"); }
		// catch (Exception &e) {} // Catch getPath() exceptions.
		
        if (dictionaryFile != NULL)
        {
            /* Read all the entries in the dictionary file */
            String keyString, valueString;
            bool fileOk;
            fileOk = readLine(dictionaryFile, keyString);
            while (fileOk) {
                fileOk = readLinesTillBlank(dictionaryFile, valueString);
                if (fileOk) {
                    std::string key((const char *)keyString);
                    void * old = (*(myDictEntry->dictionary))[key];
                    if (old != NULL) free(old);
                    char * newstring = strdup(valueString);
/* #ifdef DEBUG
					char tmp[2048];
					sprintf(tmp, "dict[%s]=%s",key.c_str(),newstring);
					GTLog(tmp);
#endif */
                    (*(myDictEntry->dictionary))[key] = (void *)newstring;
                    do {
                        fileOk = readLine(dictionaryFile, keyString);
                    } while (fileOk && (keyString == ""));
                }
            }
            delete dictionaryFile; dictionaryFile = NULL;
#ifdef DEBUG
			GTLogf("Found dictionary %s",(const char *)dictFilePath);
#endif
            found = true;
        }
    }
    // Should we look for any eligible dictionnary now?
    // By now we don't bother since english (en) should be there or we return the original strings anyway.
#ifdef DEBUG
    if (!found) GTLogf("No dictionary found in %s for %s", dictionaryDirectory, dictionaryName);
#endif
  }

  myDictEntry->refcount++;
  dictionary = static_cast<void*>(myDictEntry->dictionary);
  //fprintf(stderr,"-----Refcount++ = %d (%s)\n",myDictEntry->refcount,(const char *)stdName);
#ifdef DEBUG
  GTLog("LocalizedDictionary::LocalizedDictionary() done");
#endif
}

LocalizedDictionary::~LocalizedDictionary()
{
  str_dictionnary::iterator iter = dictionaries.find(std::string((const char *)stdName));

  if (iter != dictionaries.end())
  {
    dictionaryEntry * myDictEntry = (dictionaryEntry *)(iter->second);
    myDictEntry->refcount--;
    //fprintf(stderr,"-----Refcount-- = %d (%s)\n",myDictEntry->refcount,(const char *)stdName);
    if (myDictEntry->refcount <= 0) {
      //fprintf(stderr,"-----Destroying %s.\n",(const char *)stdName);

        for (str_dictionnary::iterator it = myDictEntry->dictionary->begin(); it != myDictEntry->dictionary->end(); it++)
      {
          free(it->second);
      }
      myDictEntry->dictionary->clear();

      delete myDictEntry->dictionary;

      free(myDictEntry);

        dictionaries.erase(iter);
        /* clean up a bit before leaving
        fprintf(stderr,"-----Languages cleanup...\n");
        for (int i = 0; i < PreferedLocalesCount; i++) {
            free(PreferedLocales[i]);
        }
        */
    }
  }
  else
  {
    GTLogf("FATAL ERROR dictionary %s destroyed too early.",(const char *)stdName);
    exit(-1);
  }
}
	
static const char * replaceArabicNumbers(const char *original) {
	if (PreferedLocales[0][0] == 'a' && PreferedLocales[0][1] == 'r') {
		static char txt[8][2048];
        static int txti = 0;
        txti = (txti + 1) % 8;
		int l = strlen(original);
		if (l > 2048) return original;
		int j = 0;
		bool found = false;
		for (int i=0; i<l; ++i) {
			txt[txti][j] = original[i];
			if (original[i] >= '0' && original[i] <= '9') {
				txt[txti][j++] = 0xd9;
				txt[txti][j] = original[i] - '0' + 0xa0;
				found = true;
			}
			++j;
		}
		if (found) {
			txt[txti][j] = 0;
			return txt[txti];
		}
	}
	return original;
}

const char * LocalizedDictionary::getLocalizedString(const char * originalString, bool copyIfNotThere)
{
    std::string sOriginalString = originalString;
    char * result = (char *) ((*static_cast<str_dictionnary *>(dictionary))[sOriginalString]);
    if (result != NULL) {
        return result;
    } else if (copyIfNotThere) {
      char * A = strdup(originalString);
      (*static_cast<str_dictionnary *>(dictionary))[sOriginalString] = (void *)A;
	  return replaceArabicNumbers(A);
	}
    return replaceArabicNumbers(originalString);
}

void LocalizedDictionary::setLocalizedString(const char *keyString, const char *valueString)
{
    std::string key(keyString);
    void *old = (*static_cast<str_dictionnary *>(dictionary))[key];
    if (old != NULL) free(old);
    char *newstring = strdup(valueString);
    (*static_cast<str_dictionnary *>(dictionary))[key] = (void *)newstring;
}

}
