//
//  LocalAchievementsManager.h
//  uothello_iphone
//
//  Created by Florent Boudet on 28/10/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef _LOCAL_ACHIEVEMENTS_MANAGER_H_
#define _LOCAL_ACHIEVEMENTS_MANAGER_H_

#include "AchievementsManager.h"
#include <map>

namespace gametools {

class LocalAchievementsManager : public AchievementsManager
{
public:
    LocalAchievementsManager(const char *profileName = NULL);
public:
    virtual void declareAchievement(const char *identifier, double percentComplete);
    virtual double getAchievement(const char *identifier);
    virtual void resetAchievements();
    virtual void displayAchievements() {}
    virtual void displayLeaderboard(const char *identifier) {}
    virtual bool hasDisplayAchievements() const { return false; }
    virtual void declareScore(const char *identifier, double score) {}
public:
    bool getSyncFlag(const char *identifier);
    void markSyncFlag(const char *identifier, bool sync);
public:
    void declareAchievementDescription(AchievementDescription desc);
private:
    std::string m_profileName;
    std::map<std::string, double> m_cachedAchievements;
    std::map<std::string, AchievementDescription> m_achievementDescriptions;
};

}

#endif // _LOCAL_ACHIEVEMENTS_MANAGER_H_
