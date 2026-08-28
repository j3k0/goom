//
//  LocalAchievementsManager.cpp
//  uothello_iphone
//
//  Created by Florent Boudet on 28/10/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include <sstream>
#include "LocalAchievementsManager.h"
#include "GTPreferences.h"

using namespace gametools;

LocalAchievementsManager::LocalAchievementsManager(const char *profileName) {
    if (profileName != NULL)
        m_profileName = profileName;
}

void LocalAchievementsManager::declareAchievement(const char *identifier, double percentComplete)
{
    std::string skey = "ach.";
    skey += m_profileName + ".";
    skey += identifier;
    
    SetIntPreference(skey.c_str(), percentComplete * 1000);
    // Keep or update the cache
    m_cachedAchievements[skey] = percentComplete;
}

double LocalAchievementsManager::getAchievement(const char *identifier)
{
    std::string skey = "ach.";
    skey += m_profileName + ".";
    skey += identifier;
    // Return from cache if available
    std::map<std::string, double>::iterator cached = m_cachedAchievements.find(skey);
    if (cached != m_cachedAchievements.end())
        return cached->second;
    // else get from preferences, then cache it
    float dval = (float)GetIntPreference(skey.c_str(), -1000.0f) / 1000.0f;
    m_cachedAchievements[skey] = dval;
    return dval;
}

void LocalAchievementsManager::resetAchievements()
{
    for (std::map<std::string, AchievementDescription>::iterator iter = m_achievementDescriptions.begin() ;
         iter != m_achievementDescriptions.end() ; ++iter) {
        declareAchievement(iter->second.identifier.c_str(), -1);
    }
    m_cachedAchievements.clear();
}

bool LocalAchievementsManager::getSyncFlag(const char *identifier)
{
    std::string skey = "ach.";
    skey += m_profileName + ".";
    skey += identifier;
    skey += ".sync";
    return (GetIntPreference(skey.c_str(), 1) != 0);
    // std::string value = m_preferencesManager->getPreference(skey.c_str(), "1");
    // return (atoi(value.c_str()) != 0);
}

void LocalAchievementsManager::markSyncFlag(const char *identifier, bool sync)
{
    std::string skey = "ach.";
    skey += m_profileName + ".";
    skey += identifier;
    skey += ".sync";
    SetIntPreference(skey.c_str(), sync?1:0);
    // m_preferencesManager->setPreference(skey.c_str(), sync?"1":"0");
}


void LocalAchievementsManager::declareAchievementDescription(AchievementDescription desc)
{
    m_achievementDescriptions[desc.identifier] = desc;
}

