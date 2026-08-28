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
#include "FPDataPathManager.h"
#include "GTPlatform.h"

using namespace ios_fc;
using namespace gametools;

#define isnum(X) ((X>='0') && (X<='9'))

FPDataInputStream::FPDataInputStream(const char *fname)
{
    m_f = fopen(fname, "r");
}

FPDataInputStream::~FPDataInputStream()
{
    fclose(m_f);
}

int FPDataInputStream::streamRead(void *buffer, int size)
{
    return fread(buffer, 1, size, m_f);
}

int FPDataInputStream::streamSkip(int size)
{
    return fseek(m_f, size, SEEK_CUR);
}

FPDataPackage::FPDataPackage(FPDataPathManager *owner,
                             const char *packagePath,
                             int packageNumber)
    : m_owner(owner), m_packageNumber(packageNumber)
{
    // std::stringstream sstream;
    // sstream << packagePath;
    // name = sstream.str();
    m_name = packagePath;
}

std::string FPDataPackage::getPath(const char *shortPath) const
{
    return (const char *)(m_owner->getPathInPack(shortPath, m_packageNumber));
}

bool FPDataPackage::hasFile(const char *shortPath) const
{
    String ret = (m_owner->getPathInPack(shortPath, m_packageNumber));
    return (ret != "");
}

DataInputStream *FPDataPackage::openDataInputStream(const char *shortPath) const
{
    String ret = (m_owner->getPathInPack(shortPath, m_packageNumber));
    return new FPDataInputStream(ret.c_str());
}

std::string FPDataPackage::getName() const
{
    return m_name;
}

FPDataPathManager::FPDataPathManager(String coreDataPath)
    : m_coreDataPath(coreDataPath)
{
    SelfVector<String> dataFiles = m_coreDataPath.listFiles();
    SelfVector<String> wellFormattedNames;
    AdvancedBuffer<int> wellFormattedNumbers;
    for (int i = 0 ; i < dataFiles.size() ; i++) {
        String currentFile = dataFiles[i];
		printf("=======> %s\n", currentFile.c_str());
		
        int len = currentFile.length();
        if ((len > 3) && (currentFile[len - 4] == '.')
                && isnum(currentFile[len - 3])
                && isnum(currentFile[len - 2])
                && isnum(currentFile[len - 1])) {
            wellFormattedNames.add(currentFile);
            wellFormattedNumbers.add(atoi(currentFile.substring(currentFile.length() - 3)));
        }
    }
    // Now let's sort the names found
    while (wellFormattedNames.size() > 0) {
        int biggestFileIndex = -1;
        int biggestFileNumber = -1;
        for (int i = 0 ; i < wellFormattedNumbers.size() ; i++) {
            if (wellFormattedNumbers[i] > biggestFileNumber) {
                biggestFileNumber = wellFormattedNumbers[i];
                biggestFileIndex = i;
            }
        }
        if (biggestFileIndex != -1) {
            m_dataPaths.add(new FilePath(m_coreDataPath.combine(wellFormattedNames[biggestFileIndex])));
            m_packages.push_back(FPDataPackage(this, m_dataPaths[m_dataPaths.size()-1].getPathString(), m_packages.size()));
            wellFormattedNumbers.removeAt(biggestFileIndex);
            wellFormattedNames.removeAt(biggestFileIndex);
        }
    }
}

String FPDataPathManager::getPath(String shortPath0) const
{
	SelfVector<String> v = getPlatformFiles(shortPath0);
	for (int vIndex = 0; vIndex < v.size(); ++vIndex) {
		String shortPath = v[vIndex];
		for (int i = 0 ; i < m_dataPaths.size() ; i++) {
			FilePath testPath(m_dataPaths[i].combine(shortPath));
			if (testPath.exists()) {
				return testPath.getPathString();
			}
		}
	}
	printf("File Not Found: %s\n", shortPath0.c_str());
    throw Exception(String("File ") + shortPath0 + " not found !");
}

bool FPDataPathManager::hasFile(String shortPath0) const
{
	SelfVector<String> v = getPlatformFiles(shortPath0);
	for (int vIndex = 0; vIndex < v.size(); ++vIndex) {
		String shortPath = v[vIndex];
		for (int i = 0 ; i < m_dataPaths.size() ; i++) {
			FilePath testPath(m_dataPaths[i].combine(shortPath));
			if (testPath.exists())
                return true;
		}
	}
    return false;
}

String FPDataPathManager::getPathInPack(String shortPath, int packPathIndex) const
{
    FilePath testPath(m_dataPaths[packPathIndex].combine(shortPath));
    if (testPath.exists())
        return testPath.getPathString();
    else
        return "";
    // throw Exception(String("File ") + shortPath + " not found !");
}

SelfVector<String> FPDataPathManager::getEntriesAtPath(String shortPath) const
{
    FilePath rshortPath(shortPath);
    SelfVector<String> result;
    for (int i = 0 ; i < m_dataPaths.size() ; i++) {
        FilePath testPath(m_dataPaths[i].combine(shortPath));
        if (testPath.exists()) {
            SelfVector<String> existingFilesInPack = testPath.listFiles();
            for (int j = 0 ; j < existingFilesInPack.size() ; j++)
            {
                result.add(rshortPath.combine(existingFilesInPack[j]));
            }
        }
    }
    return result;
}

void FPDataPathManager::setMaxPackNumber(int maxPackNumber)
{
    for (int i = m_dataPaths.size() - 1 ; i >= 0 ; i--) {
        const String &currentFile = m_dataPaths[i].getPathString();
        int currentNumber = atoi(currentFile.substring(currentFile.length() - 3));
        if (currentNumber > maxPackNumber)
            m_dataPaths.removeAtKeepOrder(i);
    }
}

ios_fc::SelfVector<ios_fc::String> FPDataPathManager::getPlatformFiles(String path) const {
	
	ios_fc::SelfVector<ios_fc::String> v;
#if 0
	int dot = path.length() - 1;
	while (path[path.length() - dot] != '.') dot--;
	ios_fc::String base = path.substring(0, path.length() - dot);
	ios_fc::String ext = path.substring(path.length() - dot);
	
    std::vector<const char *>platforms = GTGetPlatformStrings();
	for (unsigned int i = 0; i < platforms.size() && i < 2; ++i) {
        v.add(base + ios_fc::String("-") + ios_fc::String(platforms[i]) + ext);
    }
#endif
	v.add(path);
#if 0
	for (unsigned int i = 2; i < platforms.size(); ++i) {
        v.add(base + ios_fc::String("-") + ios_fc::String(platforms[i]) + ext);
    }
#endif
	
	return v;
}

DataInputStream *FPDataPathManager::openDataInputStream(const char *shortPath) const
{
    String path = getPath(shortPath);
    return new FPDataInputStream(path);
}
