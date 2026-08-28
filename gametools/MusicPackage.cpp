/*
 *  MusicPackage.cpp
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 11/8/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "MusicPackage.h"

using namespace gametools;

int MusicPackage::getMusicPosition(const char *command) const
{
	MusicPositionMap::const_iterator existing = m_positions.find(command);
    if (existing == m_positions.end()) {
		return 0;
    }
	else {
		return existing->second;
	}

}

void MusicPackage::registerMusicPosition(const char *command, int position)
{
	MusicPositionMap::const_iterator existing = m_positions.find(command);
    if (existing == m_positions.end()) {
		m_positions[command] = position;
    }
}
