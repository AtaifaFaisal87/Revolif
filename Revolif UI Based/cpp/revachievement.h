#ifndef REVACHIEVEMENT_H
#define REVACHIEVEMENT_H

#include <QObject>
#include <QString>

class RevAchievement : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int achievementID READ achievementID WRITE setAchievementID NOTIFY achievementIDChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(int requiredGoals READ requiredGoals WRITE setRequiredGoals NOTIFY requiredGoalsChanged)
    Q_PROPERTY(bool isDefault READ isDefault WRITE setIsDefault NOTIFY isDefaultChanged)
    Q_PROPERTY(bool unlocked READ unlocked WRITE setUnlocked NOTIFY unlockedChanged)

public:
    explicit RevAchievement(QObject *parent = nullptr);

    int achievementID() const { return m_achievementID; }
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    int requiredGoals() const { return m_requiredGoals; }
    bool isDefault() const { return m_isDefault; }
    bool unlocked() const { return m_unlocked; }

    void setAchievementID(int id);
    void setName(const QString &n);
    void setDescription(const QString &d);
    void setRequiredGoals(int r);
    void setIsDefault(bool d);
    void setUnlocked(bool u);

signals:
    void achievementIDChanged();
    void nameChanged();
    void descriptionChanged();
    void requiredGoalsChanged();
    void isDefaultChanged();
    void unlockedChanged();

private:
    int m_achievementID = 0;
    QString m_name;
    QString m_description;
    int m_requiredGoals = 0;
    bool m_isDefault = false;
    bool m_unlocked = false;
};

#endif // REVACHIEVEMENT_H
