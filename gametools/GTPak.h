/* GameTools
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

#ifndef _PAKDATAPATHMANAGER_H
#define _PAKDATAPATHMANAGER_H

#include "DataPathManager.h"
#include <vector>
#include <map>

namespace gametools {

class PakDataPathManager;
class PakDataPackage;

struct PakMapEntry {
    PakDataPackage *package;
    int fileStart;
    int fileSize;
};
typedef std::map<std::string, PakMapEntry> PakMap;

class PakDataPackage : public DataPackage {
public:
    PakDataPackage(PakDataPathManager *owner, DataPackage &package);

    // virtual bool hasFile(const char *shortPath) const;
    // virtual std::string getPath(const char *shortPath) const;

    virtual std::string getName() const;
    virtual bool hasDataInputStream(const char *shortPath) const;
    virtual DataInputStream *openDataInputStream(const char *shortPath) const;

    DataInputStream *openPak() const;
private:
    bool                m_isPak;
    PakDataPathManager *m_owner;
    DataPackage        &m_package;
    PakMap              m_files;
};

class PakDataInputStream : public DataInputStream {
public:
    PakDataInputStream(DataInputStream *stream, int size);
    virtual ~PakDataInputStream();
    virtual int streamRead(void *buffer, int size);
    virtual int streamSkip(int size);
private:
    DataInputStream *m_f;
    int              m_remainingBytes;
};

class PakDataPathManager : public MultiPackageDataPathManager {
public:
    PakDataPathManager(MultiPackageDataPathManager &dataPathManager);
    ~PakDataPathManager();

    // virtual ios_fc::String getPath(ios_fc::String shortPath) const;
    // virtual ios_fc::SelfVector<ios_fc::String> getEntriesAtPath(ios_fc::String shortPath) const;

    // Data Provider
    virtual bool hasDataInputStream(const char *shortPath) const;
    virtual DataInputStream *openDataInputStream(const char *shortPath) const;

    // MultiPackageDataPathManager
    virtual int numPackages() const { return m_packages.size(); }
    virtual DataPackage &getPackage(int i) { return *m_packages[i]; }

    // Fast access to files.
    void registerMapEntry(const char *name, PakMapEntry entry) { m_files[name] = entry; }
	
private:
    MultiPackageDataPathManager &m_dp;
    std::vector<PakDataPackage*> m_packages;
    PakMap                       m_files;
};

}

#endif // _PUYODATAPATHMANAGER_H

