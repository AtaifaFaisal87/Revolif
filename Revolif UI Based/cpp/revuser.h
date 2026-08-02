#ifndef REVUSER_H
#define REVUSER_H

#include <QObject>
#include <QString>
#include <QDate>
#include <QList>

class RevUser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int uid READ uid WRITE setUid NOTIFY uidChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QDate dob READ dob WRITE setDob NOTIFY dobChanged)
    Q_PROPERTY(QDate registrationDate READ registrationDate WRITE setRegistrationDate NOTIFY registrationDateChanged)
    Q_PROPERTY(QDate lastLogin READ lastLogin WRITE setLastLogin NOTIFY lastLoginChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(bool isActive READ isActive WRITE setIsActive NOTIFY isActiveChanged)
    Q_PROPERTY(bool deactivatedBySelf READ deactivatedBySelf WRITE setDeactivatedBySelf NOTIFY deactivatedBySelfChanged)
    Q_PROPERTY(int currentStreak READ currentStreak WRITE setCurrentStreak NOTIFY currentStreakChanged)
    Q_PROPERTY(int bestStreak READ bestStreak WRITE setBestStreak NOTIFY bestStreakChanged)
    Q_PROPERTY(int displayedAchievementID READ displayedAchievementID WRITE setDisplayedAchievementID NOTIFY displayedAchievementIDChanged)
    Q_PROPERTY(int unlockedCount READ unlockedCount NOTIFY unlockedCountChanged)
    Q_PROPERTY(bool welcomeEmailSent READ welcomeEmailSent WRITE setWelcomeEmailSent NOTIFY welcomeEmailSentChanged)
    Q_PROPERTY(QDate lastSummaryEmailDate READ lastSummaryEmailDate WRITE setLastSummaryEmailDate NOTIFY lastSummaryEmailDateChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString passwordHash READ passwordHash WRITE setPasswordHash NOTIFY passwordHashChanged)
    Q_PROPERTY(int pendingTasks READ pendingTasks NOTIFY pendingTasksChanged)
    Q_PROPERTY(int completedTasks READ completedTasks NOTIFY completedTasksChanged)
    Q_PROPERTY(int overdueTasks READ overdueTasks NOTIFY overdueTasksChanged)
    Q_PROPERTY(int pendingGoals READ pendingGoals NOTIFY pendingGoalsChanged)
    Q_PROPERTY(int completedGoals READ completedGoals NOTIFY completedGoalsChanged)
    Q_PROPERTY(int overdueGoals READ overdueGoals NOTIFY overdueGoalsChanged)
    Q_PROPERTY(double totalExpenses READ totalExpenses NOTIFY totalExpensesChanged)
    Q_PROPERTY(int lifeScore READ lifeScore NOTIFY lifeScoreChanged)
    Q_PROPERTY(QString lifeScoreLabel READ lifeScoreLabel NOTIFY lifeScoreChanged)

public:
    explicit RevUser(QObject *parent = nullptr);

    int uid() const { return m_uid; }
    QString name() const { return m_name; }
    QString username() const { return m_username; }
    QString email() const { return m_email; }
    QDate dob() const { return m_dob; }
    QDate registrationDate() const { return m_registrationDate; }
    QDate lastLogin() const { return m_lastLogin; }
    QString title() const { return m_title; }
    bool isActive() const { return m_isActive; }
    bool deactivatedBySelf() const { return m_deactivatedBySelf; }
    int currentStreak() const { return m_currentStreak; }
    int bestStreak() const { return m_bestStreak; }
    int displayedAchievementID() const { return m_displayedAchievementID; }
    int unlockedCount() const { return m_unlockedAchievementIDs.size(); }
    bool welcomeEmailSent() const { return m_welcomeEmailSent; }
    QDate lastSummaryEmailDate() const { return m_lastSummaryEmailDate; }
    QString displayName() const;
    QString passwordHash() const { return m_passwordHash; }

    void setUid(int id);
    void setName(const QString &n);
    void setUsername(const QString &u);
    void setEmail(const QString &e);
    void setDob(const QDate &d);
    void setRegistrationDate(const QDate &d);
    void setLastLogin(const QDate &d);
    void setTitle(const QString &t);
    void setIsActive(bool a);
    void setDeactivatedBySelf(bool d);
    void setCurrentStreak(int s);
    void setBestStreak(int s);
    void setDisplayedAchievementID(int id);
    void setWelcomeEmailSent(bool s);
    void setLastSummaryEmailDate(const QDate &d);
    void setPasswordHash(const QString &h);

    Q_INVOKABLE void recordLogin();
    Q_INVOKABLE void addUnlockedAchievement(int achID);
    Q_INVOKABLE bool hasAchievement(int achID) const;
    Q_INVOKABLE void clearUnlockedAchievements();
    QList<int> unlockedAchievementIDs() const { return m_unlockedAchievementIDs; }
    void setUnlockedAchievementIDs(const QList<int> &ids);

    // These are computed from the system models, but we cache values here for QML binding
    int pendingTasks() const { return m_pendingTasks; }
    int completedTasks() const { return m_completedTasks; }
    int overdueTasks() const { return m_overdueTasks; }
    int pendingGoals() const { return m_pendingGoals; }
    int completedGoals() const { return m_completedGoals; }
    int overdueGoals() const { return m_overdueGoals; }
    double totalExpenses() const { return m_totalExpenses; }
    int lifeScore() const;
    QString lifeScoreLabel() const;

    void setTaskStats(int pending, int completed, int overdue);
    void setGoalStats(int pending, int completed, int overdue);
    void setTotalExpenses(double total);

signals:
    void uidChanged();
    void nameChanged();
    void usernameChanged();
    void emailChanged();
    void dobChanged();
    void registrationDateChanged();
    void lastLoginChanged();
    void titleChanged();
    void isActiveChanged();
    void deactivatedBySelfChanged();
    void currentStreakChanged();
    void bestStreakChanged();
    void displayedAchievementIDChanged();
    void unlockedCountChanged();
    void welcomeEmailSentChanged();
    void lastSummaryEmailDateChanged();
    void displayNameChanged();
    void passwordHashChanged();
    void pendingTasksChanged();
    void completedTasksChanged();
    void overdueTasksChanged();
    void pendingGoalsChanged();
    void completedGoalsChanged();
    void overdueGoalsChanged();
    void totalExpensesChanged();
    void lifeScoreChanged();

private:
    int m_uid = 0;
    QString m_name;
    QString m_username;
    QString m_email;
    QDate m_dob;
    QDate m_registrationDate;
    QDate m_lastLogin;
    QString m_title;
    bool m_isActive = true;
    bool m_deactivatedBySelf = false;
    int m_currentStreak = 0;
    int m_bestStreak = 0;
    int m_displayedAchievementID = -1;
    QList<int> m_unlockedAchievementIDs;
    bool m_welcomeEmailSent = false;
    QDate m_lastSummaryEmailDate;
    QString m_passwordHash;

    int m_pendingTasks = 0;
    int m_completedTasks = 0;
    int m_overdueTasks = 0;
    int m_pendingGoals = 0;
    int m_completedGoals = 0;
    int m_overdueGoals = 0;
    double m_totalExpenses = 0.0;
};

#endif // REVUSER_H
