/*
 *  GTPreferences.h
 *  
 *
 *  Created by Guillaume Borios on 03/08/04.
 *  Copyright 2004 iOS. All rights reserved.
 *
 */

#ifndef GTPREF_H
#define GTPREF_H

namespace gametools {

void ChangePreferenceFile(const char *name);
    
/* Set preferences */
void SetBoolPreference(const char *name, bool value);
void SetIntPreference (const char *name, int value);
void SetStrPreference (const char *name, const char *value);
void SetFloatPreference(const char *name, float value);

/* Get preferences */
bool GetBoolPreference(const char *name, bool defaut);
int  GetIntPreference (const char *name, int defaut);
void GetStrPreference (const char *name, char *out, const char *defaut, const int bufferSize = 256);
float GetFloatPreference(const char *name, float defaut);
    
}
	
#endif
