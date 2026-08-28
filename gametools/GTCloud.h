//
//  GTCloud.h
//  Project
//
//  Created by Jean-Christophe Hoelt on 4/16/12.
//  Copyright (c) 2012 Fovea.cc. All rights reserved.
//

#ifndef Project_GTCloud_h
#define Project_GTCloud_h

#include "ios_memory.h"

namespace gametools {

// Per platform distributed file accesses.
void GTCloudSetRepository(const char *local, const char *remote);
    
ios_fc::VoidBuffer GTCloudLoad(const char *fileName);
void GTCloudSave(const char *fileName, ios_fc::VoidBuffer buffer);
void GTCloudPush(const char *fileName);
void GTCloudPull(const char *fileName);
void GTCloudRevertPull(const char *fileName);

void GTCloudDisableNetwork();

}

#endif
