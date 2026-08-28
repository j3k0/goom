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

#include <sstream>
#include "GTPak.h"
#include "GTPlatform.h"
#include "GTLog.h"

using namespace ios_fc;
using namespace gametools;

#define isnum(X) ((X>='0') && (X<='9'))

#pragma mark Data Input Stream

PakDataInputStream::PakDataInputStream(DataInputStream *f, int size) {
    m_f = f;
    m_remainingBytes = size;
}

PakDataInputStream::~PakDataInputStream() {
    if (m_f) delete m_f;
}

int PakDataInputStream::streamSkip(int size) {
    if (size > m_remainingBytes)
        size = m_remainingBytes;
    m_remainingBytes -= size;
    if (m_f == NULL || size == 0) return 0;
    return m_f->streamSkip(size);
}

int PakDataInputStream::streamRead(void *buffer, int size) {
    if (size > m_remainingBytes)
        size = m_remainingBytes;
    m_remainingBytes -= size;
    if (m_f == NULL || size == 0) return 0;
    return m_f->streamRead(buffer, size);
}

#pragma mark Data Package

PakDataPackage::PakDataPackage(PakDataPathManager *owner, DataPackage &p) : m_isPak(false), m_owner(owner), m_package(p)
{
    if (m_package.hasDataInputStream("index.txt") && m_package.hasDataInputStream("data.pak")) {
        GTLogf("  >> Package %s is a PAK, loading index...", m_package.getName().c_str());
        m_isPak = true;
        DataInputStream *tmp = m_package.openDataInputStream("index.txt");
        BufferedStream index(*tmp); delete tmp;
        char  line[1024];
        int   fileStart = 0;
        while (index.gets(line, 1024)) {
            int len = strlen(line);
            if (line[len-1] == '\n') {
                line[len-1] = '\0';
                --len;
            }
            char fileName[1024];
            int  fileSize;
            sscanf(line, "%s %d", fileName, &fileSize);
            PakMapEntry entry;
            entry.package = this; 
            entry.fileStart = fileStart;
            entry.fileSize  = fileSize;
            m_owner->registerMapEntry(fileName, entry);
            m_files[fileName] = entry;
            fileStart += fileSize;
        }
    }
}

DataInputStream *PakDataPackage::openPak() const {
    return m_package.openDataInputStream("data.pak");
}

DataInputStream *PakDataPackage::openDataInputStream(const char *shortPath) const
{
    if (m_isPak) {
        DataInputStream *pak = m_package.openDataInputStream("data.pak");
        PakMap::const_iterator it = m_files.find(shortPath);
        if (it == m_files.end()) {
            GTLogf("ERROR: Could not find file %s in PAK", shortPath);
            return NULL;
        }
        const PakMapEntry &entry = it->second;
        pak->streamSkip(entry.fileStart);
        return new PakDataInputStream(pak, entry.fileSize);
    }
    else
        return m_package.openDataInputStream(shortPath);
}

bool PakDataPackage::hasDataInputStream(const char *shortPath) const {
    if (m_isPak)
        return m_files.find(shortPath) != m_files.end();
    else
        return m_package.hasDataInputStream(shortPath);
}

std::string PakDataPackage::getName() const
{
    return m_package.getName();
}

#pragma mark Data Path Manager

PakDataPathManager::PakDataPathManager(MultiPackageDataPathManager &manager)
    : m_dp(manager)
{
    for (int i=0; i<m_dp.numPackages(); ++i) {
        DataPackage &p = m_dp.getPackage(i);
        m_packages.push_back(new PakDataPackage(this, p));
    }
}

PakDataPathManager::~PakDataPathManager()
{
    for (int i=0; i<m_packages.size(); ++i)
        delete m_packages[i];
}

bool PakDataPathManager::hasDataInputStream(const char *shortPath) const
{
    if (m_files.find(shortPath) != m_files.end()) return true;
    for (int i = 0 ; i < m_packages.size() ; i++)
        if (m_packages[i]->hasDataInputStream(shortPath))
            return true;
    return false;
}

DataInputStream *PakDataPathManager::openDataInputStream(const char *shortPath) const
{
    PakMap::const_iterator it = m_files.find(shortPath);
    if (it != m_files.end()) {
        DataInputStream    *pak = it->second.package->openPak();
        pak->streamSkip(it->second.fileStart);
        return new PakDataInputStream(pak, it->second.fileSize);
    }
    for (int i = 0 ; i < m_packages.size() ; i++)
        if (m_packages[i]->hasDataInputStream(shortPath))
            return m_packages[i]->openDataInputStream(shortPath);
    return NULL;
}
