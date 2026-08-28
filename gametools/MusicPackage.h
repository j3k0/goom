/*
 *  MusicPackage.h
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 11/8/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef MusicPackage_H
#define MusicPackage_H

#include <string>
#include <map>
#include <vector>

namespace gametools {

class MusicPackage
{
public:
	MusicPackage(const char *musicFile) : m_file(musicFile) {}
	
	int getMusicPosition(const char *command) const;
	void registerMusicPosition(const char *command, int position);
	
	const std::string &getFileName() const { return m_file; }
	
    void registerMusicStressLayer(int layer) { m_stressLayers.push_back(layer); }
    void registerMusicRelaxLayer(int layer) { m_relaxLayers.push_back(layer); }
    
    typedef std::vector<int> MusicLayerList;
    const MusicLayerList &getMusicStressLayers() const { return m_stressLayers; }
    const MusicLayerList &getMusicRelaxLayers() const  { return m_relaxLayers;  }
    
private:
	typedef std::map<std::string, int> MusicPositionMap;
	
	std::string m_file;
	MusicPositionMap m_positions;
    
    MusicLayerList m_stressLayers;
    MusicLayerList m_relaxLayers;
};

}
#endif