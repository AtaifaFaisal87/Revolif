#ifndef REVTASK_H
#define REVTASK_H

#include <QObject>
#include <QString>
#include <QDate>
#include <QTime>

class RevTask : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int taskID READ taskID WRITE setTaskID NOTIFY taskIDChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QDate deadline READ deadline WRITE setDeadline NOTIFY deadlineChanged)
    Q_PROPERTY(QTime deadlineTime READ deadlineTime WRITE setDeadlineTime NOTIFY deadlineTimeChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QString priority READ priority WRITE setPriority NOTIFY priorityChanged)
    Q_PROPERTY(bool isRecurring READ isRecurring WRITE setIsRecurring NOTIFY isRecurringChanged)
    Q_PROPERTY(QString recurrenceInterval READ recurrenceInterval WRITE setRecurrenceInterval NOTIFY recurrenceIntervalChanged)
    Q_PROPERTY(int taskType READ taskType WRITE setTaskType NOTIFY taskTypeChanged)
    Q_PROPERTY(bool isOverdue READ isOverdue NOTIFY overdueChanged)
    Q_PROPERTY(bool isDueSoon READ isDueSoon NOTIFY dueSoonChanged)

public:
    explicit RevTask(QObject *parent = nullptr);

    int taskID() const { return m_taskID; }
    QString title() const { return m_title; }
    QString description() const { return m_description; }
    QDate deadline() const { return m_deadline; }
    QTime deadlineTime() const { return m_deadlineTime; }
    QString category() const { return m_category; }
    QString status() const { return m_status; }
    QString priority() const { return m_priority; }
    bool isRecurring() const { return m_isRecurring; }
    QString recurrenceInterval() const { return m_recurrenceInterval; }
    int taskType() const { return m_taskType; }
    bool isOverdue() const;
    bool isDueSoon() const;

    void setTaskID(int id);
    void setTitle(const QString &t);
    void setDescription(const QString &d);
    void setDeadline(const QDate &d);
    void setDeadlineTime(const QTime &t);
    void setCategory(const QString &c);
    void setStatus(const QString &s);
    void setPriority(const QString &p);
    void setIsRecurring(bool r);
    void setRecurrenceInterval(const QString &i);
    void setTaskType(int type);

    Q_INVOKABLE void markCompleted();
    Q_INVOKABLE void markPending();

    static int priorityWeight(const QString &p);

signals:
    void taskIDChanged();
    void titleChanged();
    void descriptionChanged();
    void deadlineChanged();
    void deadlineTimeChanged();
    void categoryChanged();
    void statusChanged();
    void priorityChanged();
    void isRecurringChanged();
    void recurrenceIntervalChanged();
    void taskTypeChanged();
    void overdueChanged();
    void dueSoonChanged();

private:
    int m_taskID = 0;
    QString m_title;
    QString m_description;
    QDate m_deadline;
    QTime m_deadlineTime;
    QString m_category = "Academic";
    QString m_status = "Pending";
    QString m_priority = "Medium";
    bool m_isRecurring = false;
    QString m_recurrenceInterval = "None";
    int m_taskType = 1;
};

#endif // REVTASK_H
