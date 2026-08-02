#ifndef TASKLISTMODEL_H
#define TASKLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "../revtask.h"

class TaskListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY countsChanged)
    Q_PROPERTY(int completedCount READ completedCount NOTIFY countsChanged)
    Q_PROPERTY(int overdueCount READ overdueCount NOTIFY countsChanged)

public:
    enum TaskRoles {
        TaskObjectRole = Qt::UserRole + 1,
        TaskIDRole, TitleRole, DescriptionRole, DeadlineRole, DeadlineTimeRole,
        CategoryRole, StatusRole, PriorityRole, IsRecurringRole,
        RecurrenceIntervalRole, TaskTypeRole, IsOverdueRole, IsDueSoonRole
    };

    explicit TaskListModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_tasks.size(); }
    int pendingCount() const;
    int completedCount() const;
    int overdueCount() const;

    Q_INVOKABLE RevTask* get(int index) const;
    Q_INVOKABLE void addTask(RevTask *task);
    Q_INVOKABLE void removeTask(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void sortByPriorityAndDeadline();
    Q_INVOKABLE int indexOfTask(int taskID) const;
    QList<RevTask*> tasks() const { return m_tasks; }
    void setTasks(const QList<RevTask*> &tasks);

signals:
    void countChanged();
    void countsChanged();

private:
    QList<RevTask*> m_tasks;
};

#endif // TASKLISTMODEL_H
