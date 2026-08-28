//
//  GTLog.cpp
//  Project
//
//  Created by Jean-Christophe Hoelt on 9/30/11.
//  Copyright 2011 Fovea.cc. All rights reserved.
//

#include "GTLog.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

namespace gametools {
    
void textToLines(const char *text, StringList &lines) {
    //Here is some code to split the text that we have been
    //given into a set of lines.  
    //This could be made much neater by using
    //a regular expression library such as the one avliable from
    //boost.org (I've only done it out by hand to avoid complicating
    //this tutorial with unnecessary library dependencies).
    const char *start_line=text;
    const char *c;
    for(c=text;*c;c++) {
        if(*c=='\n') {
            std::stringstream line;
            for(const char *n=start_line;n<c;n++) line << *n;
            lines.push_back(line.str());
            start_line=c+1;
        }
    }
    if(start_line) {
        std::stringstream line;
        for(const char*n=start_line;n<c;n++) line << *n;
        lines.push_back(line.str());
    }
}
    
void GTLogf(const char *format, ...) {
    char buffer1k[1024];
    
    va_list args;
    va_start (args, format);

    int need = 1 + vsnprintf(buffer1k, sizeof(buffer1k), format, args);

    if (need > sizeof(buffer1k)) {
        char *buffer = (char*)malloc(need);
        vsnprintf(buffer, need, format, args);
        GTLog(buffer);
        free(buffer);
    }
    else {
        GTLog(buffer1k);
    }
    
    va_end(args);
}
    
const char *formatNiceInt(int p1) {
    static char txt[32];
    int p1k = p1 / 1000;
    int p1m = p1k / 1000;
    int p1M = p1m / 1000;
    p1  -= p1k * 1000;
    p1k -= p1m * 1000;
    p1m -= p1M * 1000;
    if (p1M > 0)
        sprintf(txt, "%d,%03d,%03d,%03d", p1M, p1m, p1k, p1);
    else if (p1m > 0) {
        sprintf(txt, "%d,%03d,%03d", p1m, p1k, p1);
    }
    else if (p1k > 0) {
        sprintf(txt, "%d,%03d", p1k, p1);
    }
    else {
        sprintf(txt, "%d", p1);
    }
    return txt;
}

}