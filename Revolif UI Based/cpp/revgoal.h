#ifndef REVGOAL_H
#define REVGOAL_H

#include <QObject>
#include <QString>
#include <QDate>

class RevGoal : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int goalID READ goalID WRITE setGoalID NOTIFY goalIDChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    Q_PROPERTY(QDate deadline READ deadline WRITE setDeadline NOTIFY deadlineChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QString displayStatus READ displayStatus NOTIFY displayStatusChanged)
    Q_PROPERTY(bool isOverdue READ isOverdue NOTIFY overdueChanged)
    Q_PROPERTY(bool isCompleted READ isCompleted NOTIFY statusChanged)

public:
    explicit RevGoal(QObject *parent = nullptr);

    int goalID() const { return m_goalID; }
    QString title() const { return m_title; }
    QString description() const { return m_description; }
    QString category() const { return m_category; }
    QDate deadline() const { return m_deadline; }
    QString status() const { return m_status; }
    QString displayStatus() const;
    bool isOverdue() const;
    bool isCompleted() const { return m_status == "Completed"; }

    void setGoalID(int id);
    void setTitle(const QString &t);
    void setDescription(const QString &d);
    void setCategory(const QString &c);
    void setDeadline(const QDate &d);
    void setStatus(const QString &s);

    Q_INVOKABLE void completeGoal();

signals:
    void goalIDChanged();
    void titleChanged();
    void descriptionChanged();
    void categoryChanged();
    void deadlineChanged();
    void statusChanged();
    void displayStatusChanged();
    void overdueChanged();

private:
    int m_goalID = 0;
    QString m_title;
    QString m_description;
    QString m_category;
    QDate m_deadline;
    QString m_status = "Incomplete";
};

#endif // REVGOAL_H
