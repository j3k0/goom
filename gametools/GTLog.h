//
//  GTLog.h
//  Birdy Nam Nam
//
//  Created by Jean-Christophe Hoelt on 8/29/11.
//  Copyright 2011 Fovea.cc. All rights reserved.
//

#ifndef GTLog_H
#define GTLog_H

#include <string>
#include <list>

namespace gametools {
    void GTLog(const char *);
    void GTLogf(const char *format, ...);
    void GTLogCheckpoint(const char *);

    const char *formatNiceInt(int p1);

    typedef std::list<std::string> StringList;
    void textToLines(const char *text, StringList &lines);
}

#endif