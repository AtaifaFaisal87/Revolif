#include "revachievement.h"

RevAchievement::RevAchievement(QObject *parent) : QObject(parent) {}

void RevAchievement::setAchievementID(int id) { if (m_achievementID != id) { m_achievementID = id; emit achievementIDChanged(); } }
void RevAchievement::setName(const QString &n) { if (m_name != n) { m_name = n; emit nameChanged(); } }
void RevAchievement::setDescription(const QString &d) { if (m_description != d) { m_description = d; emit descriptionChanged(); } }
void RevAchievement::setRequiredGoals(int r) { if (m_requiredGoals != r) { m_requiredGoals = r; emit requiredGoalsChanged(); } }
void RevAchievement::setIsDefault(bool d) { if (m_isDefault != d) { m_isDefault = d; emit isDefaultChanged(); } }
void RevAchievement::setUnlocked(bool u) { if (m_unlocked != u) { m_unlocked = u; emit unlockedChanged(); } }
