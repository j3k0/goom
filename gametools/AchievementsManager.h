//
//  Achievement.h
//  uothello_iphone
//
//  Created by Florent Boudet on 12/10/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef _ACHIEVEMENTS_MANAGER_H_
#define _ACHIEVEMENTS_MANAGER_H_

#include <string>

namespace gametools {

struct AchievementDescription
{
    AchievementDescription(std::string identifier,
                           std::string title = "",
                           std::string notAchievedDescription = "",
                           std::string achievedDescription = "")
    : identifier(identifier), title(title),
    notAchievedDescription(notAchievedDescription),
    achievedDescription(achievedDescription) {}
    AchievementDescription() {}
    
    std::string identifier;
    std::string title;
    std::string notAchievedDescription;
    std::string achievedDescription;
};

class AchievementsNotifier {
public:
    virtual ~AchievementsNotifier() {}
    virtual void notifyAchievement(AchievementDescription &desc) = 0;
};

class AchievementsManager {
public:
    virtual ~AchievementsManager() {}
    virtual void declareAchievement(const char *identifier, double percentComplete) = 0;
    virtual double getAchievement(const char *identifier) = 0;
    virtual void resetAchievements() = 0;
    virtual bool hasDisplayAchievements() const = 0;
    virtual void displayAchievements() = 0;
    virtual void displayLeaderboard(const char *identifier) = 0;
    virtual void setAchievementsNotifier(AchievementsNotifier *notifier) {}

    // Leaderboard management.
    virtual void declareScore(const char *identifier, double score) = 0;
};

}

#endif // _ACHIEVEMENTS_MANAGER_H_
