#include "revuser.h"

RevUser::RevUser(QObject *parent) : QObject(parent)
{
    m_registrationDate = QDate::currentDate();
}

QString RevUser::displayName() const
{
    if (m_title.isEmpty()) return m_name;
    return m_name + " (" + m_title + ")";
}

void RevUser::recordLogin()
{
    QDate today = QDate::currentDate();
    if (!m_lastLogin.isValid() || m_lastLogin.isNull()) {
        m_currentStreak = 1;
    } else {
        int diff = m_lastLogin.daysTo(today);
        if (diff == 1) m_currentStreak++;
        else if (diff > 1) m_currentStreak = 1;
    }
    if (m_currentStreak > m_bestStreak) m_bestStreak = m_currentStreak;
    m_lastLogin = today;
    emit currentStreakChanged();
    emit bestStreakChanged();
    emit lastLoginChanged();
}

void RevUser::addUnlockedAchievement(int achID)
{
    if (!m_unlockedAchievementIDs.contains(achID)) {
        m_unlockedAchievementIDs.append(achID);
        emit unlockedCountChanged();
    }
}

bool RevUser::hasAchievement(int achID) const
{
    return m_unlockedAchievementIDs.contains(achID);
}

void RevUser::clearUnlockedAchievements()
{
    m_unlockedAchievementIDs.clear();
    emit unlockedCountChanged();
}

void RevUser::setUnlockedAchievementIDs(const QList<int> &ids)
{
    m_unlockedAchievementIDs = ids;
    emit unlockedCountChanged();
}

int RevUser::lifeScore() const
{
    int totalTasks = m_pendingTasks + m_completedTasks;
    double taskScore = (totalTasks > 0) ? ((double)m_completedTasks / totalTasks) * 100.0 : 100.0;
    int totalGoals = m_completedGoals + m_pendingGoals + m_overdueGoals;
    double goalScore = (totalGoals > 0) ? ((double)m_completedGoals / totalGoals) * 100.0 : 100.0;
    double overduePenalty = (m_overdueTasks * 5.0) + (m_overdueGoals * 5.0);
    double rawScore = (taskScore * 0.4) + (goalScore * 0.4) + 20.0 - overduePenalty;
    if (rawScore > 100) rawScore = 100;
    if (rawScore < 0) rawScore = 0;
    return (int)rawScore;
}

QString RevUser::lifeScoreLabel() const
{
    int score = lifeScore();
    if (score >= 90) return "Outstanding";
    if (score >= 75) return "Excellent";
    if (score >= 60) return "Good";
    if (score >= 40) return "Fair";
    return "Needs Improvement";
}

void RevUser::setTaskStats(int pending, int completed, int overdue)
{
    m_pendingTasks = pending;
    m_completedTasks = completed;
    m_overdueTasks = overdue;
    emit pendingTasksChanged();
    emit completedTasksChanged();
    emit overdueTasksChanged();
    emit lifeScoreChanged();
}

void RevUser::setGoalStats(int pending, int completed, int overdue)
{
    m_pendingGoals = pending;
    m_completedGoals = completed;
    m_overdueGoals = overdue;
    emit pendingGoalsChanged();
    emit completedGoalsChanged();
    emit overdueGoalsChanged();
    emit lifeScoreChanged();
}

void RevUser::setTotalExpenses(double total)
{
    m_totalExpenses = total;
    emit totalExpensesChanged();
}

void RevUser::setUid(int id) { if (m_uid != id) { m_uid = id; emit uidChanged(); } }
void RevUser::setName(const QString &n) { if (m_name != n) { m_name = n; emit nameChanged(); emit displayNameChanged(); } }
void RevUser::setUsername(const QString &u) { if (m_username != u) { m_username = u; emit usernameChanged(); } }
void RevUser::setEmail(const QString &e) { if (m_email != e) { m_email = e; emit emailChanged(); } }
void RevUser::setDob(const QDate &d) { if (m_dob != d) { m_dob = d; emit dobChanged(); } }
void RevUser::setRegistrationDate(const QDate &d) { if (m_registrationDate != d) { m_registrationDate = d; emit registrationDateChanged(); } }
void RevUser::setLastLogin(const QDate &d) { if (m_lastLogin != d) { m_lastLogin = d; emit lastLoginChanged(); } }
void RevUser::setTitle(const QString &t) { if (m_title != t) { m_title = t; emit titleChanged(); emit displayNameChanged(); } }
void RevUser::setIsActive(bool a) { if (m_isActive != a) { m_isActive = a; emit isActiveChanged(); } }
void RevUser::setDeactivatedBySelf(bool d) { if (m_deactivatedBySelf != d) { m_deactivatedBySelf = d; emit deactivatedBySelfChanged(); } }
void RevUser::setCurrentStreak(int s) { if (m_currentStreak != s) { m_currentStreak = s; emit currentStreakChanged(); } }
void RevUser::setBestStreak(int s) { if (m_bestStreak != s) { m_bestStreak = s; emit bestStreakChanged(); } }
void RevUser::setDisplayedAchievementID(int id) { if (m_displayedAchievementID != id) { m_displayedAchievementID = id; emit displayedAchievementIDChanged(); } }
void RevUser::setWelcomeEmailSent(bool s) { if (m_welcomeEmailSent != s) { m_welcomeEmailSent = s; emit welcomeEmailSentChanged(); } }
void RevUser::setLastSummaryEmailDate(const QDate &d) { if (m_lastSummaryEmailDate != d) { m_lastSummaryEmailDate = d; emit lastSummaryEmailDateChanged(); } }
void RevUser::setPasswordHash(const QString &h) { if (m_passwordHash != h) { m_passwordHash = h; emit passwordHashChanged(); } }
