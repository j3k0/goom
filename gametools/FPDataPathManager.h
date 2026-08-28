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

#ifndef _PUYODATAPATHMANAGER_H
#define _PUYODATAPATHMANAGER_H

#include "DataPathManager.h"
#include "ios_filepath.h"
#include <vector>

namespace gametools {

class FPDataPathManager;

class FPDataPackage : public DataPackage {
public:
    FPDataPackage(FPDataPathManager *owner,
                  const char *packagePath,
                  int packageNumber);
    bool hasFile(const char *shortPath) const;
    std::string getPath(const char *shortPath) const;

    virtual DataInputStream *openDataInputStream(const char *shortPath) const;
    virtual bool hasDataInputStream(const char *shortPath) const { return hasFile(shortPath); }
    virtual std::string getName() const;
private:
    FPDataPathManager *m_owner;
    int m_packageNumber;
    std::string m_name;
};

class FPDataInputStream : public DataInputStream {
public:
    FPDataInputStream(const char *fname);
    virtual ~FPDataInputStream();
    virtual int streamRead(void *buffer, int size);
    virtual int streamSkip(int size);
private:
    FILE *m_f;
};

class FPDataPathManager : public MultiPackageDataPathManager {
public:
    FPDataPathManager(ios_fc::String coreDataPath);

    bool hasFile(ios_fc::String shortPath) const;
    ios_fc::String getPath(ios_fc::String shortPath) const;
    ios_fc::String getPathInPack(ios_fc::String shortPath, int packPathIndex) const;
    ios_fc::SelfVector<ios_fc::String> getEntriesAtPath(ios_fc::String shortPath) const;
    int getNumPacks() const { return m_dataPaths.size(); }
    void setMaxPackNumber(int maxPackNumber);
	ios_fc::SelfVector<ios_fc::String> getPlatformFiles(ios_fc::String path) const;

    // Data Provider
    virtual DataInputStream *openDataInputStream(const char *shortPath) const;
    virtual bool hasDataInputStream(const char *shortPath) const { return hasFile(shortPath); }

    // Multi Package
    virtual int numPackages() const { return m_packages.size(); }
    virtual DataPackage &getPackage(int i) { return m_packages[i]; }
	
private:
	ios_fc::FilePath m_coreDataPath;
    ios_fc::SelfVector<ios_fc::FilePath> m_dataPaths;
    std::vector<FPDataPackage> m_packages;
};

}

#endif // _PUYODATAPATHMANAGER_H

