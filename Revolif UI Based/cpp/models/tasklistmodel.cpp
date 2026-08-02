#include "tasklistmodel.h"
#include <QDate>

TaskListModel::TaskListModel(QObject *parent) : QAbstractListModel(parent) {}

int TaskListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_tasks.size();
}

QVariant TaskListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tasks.size()) return QVariant();
    RevTask *task = m_tasks.at(index.row());
    switch (role) {
    case TaskObjectRole: return QVariant::fromValue(task);
    case TaskIDRole: return task->taskID();
    case TitleRole: return task->title();
    case DescriptionRole: return task->description();
    case DeadlineRole: return task->deadline();
    case DeadlineTimeRole: return task->deadlineTime();
    case CategoryRole: return task->category();
    case StatusRole: return task->status();
    case PriorityRole: return task->priority();
    case IsRecurringRole: return task->isRecurring();
    case RecurrenceIntervalRole: return task->recurrenceInterval();
    case TaskTypeRole: return task->taskType();
    case IsOverdueRole: return task->isOverdue();
    case IsDueSoonRole: return task->isDueSoon();
    }
    return QVariant();
}

QHash<int, QByteArray> TaskListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TaskObjectRole] = "taskObject";
    roles[TaskIDRole] = "taskID";
    roles[TitleRole] = "title";
    roles[DescriptionRole] = "description";
    roles[DeadlineRole] = "deadline";
    roles[DeadlineTimeRole] = "deadlineTime";
    roles[CategoryRole] = "category";
    roles[StatusRole] = "status";
    roles[PriorityRole] = "priority";
    roles[IsRecurringRole] = "isRecurring";
    roles[RecurrenceIntervalRole] = "recurrenceInterval";
    roles[TaskTypeRole] = "taskType";
    roles[IsOverdueRole] = "isOverdue";
    roles[IsDueSoonRole] = "isDueSoon";
    return roles;
}

int TaskListModel::pendingCount() const
{
    int c = 0;
    for (auto *t : m_tasks) if (t->status() == "Pending") c++;
    return c;
}

int TaskListModel::completedCount() const
{
    int c = 0;
    for (auto *t : m_tasks) if (t->status() == "Completed") c++;
    return c;
}

int TaskListModel::overdueCount() const
{
    int c = 0;
    for (auto *t : m_tasks) if (t->status() == "Pending" && t->deadline() < QDate::currentDate()) c++;
    return c;
}

RevTask* TaskListModel::get(int index) const
{
    if (index < 0 || index >= m_tasks.size()) return nullptr;
    return m_tasks.at(index);
}

void TaskListModel::addTask(RevTask *task)
{
    beginInsertRows(QModelIndex(), m_tasks.size(), m_tasks.size());
    m_tasks.append(task);
    endInsertRows();
    emit countChanged();
    emit countsChanged();
}

void TaskListModel::removeTask(int index)
{
    if (index < 0 || index >= m_tasks.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_tasks.at(index)->deleteLater();
    m_tasks.removeAt(index);
    endRemoveRows();
    emit countChanged();
    emit countsChanged();
}

void TaskListModel::clear()
{
    if (m_tasks.isEmpty()) return;
    beginResetModel();
    for (auto *t : m_tasks) t->deleteLater();
    m_tasks.clear();
    endResetModel();
    emit countChanged();
    emit countsChanged();
}

void TaskListModel::sortByPriorityAndDeadline()
{
    beginResetModel();
    std::sort(m_tasks.begin(), m_tasks.end(), [](RevTask *a, RevTask *b) {
        int wa = RevTask::priorityWeight(a->priority());
        int wb = RevTask::priorityWeight(b->priority());
        if (wa != wb) return wa < wb;
        return a->deadline() < b->deadline();
    });
    endResetModel();
}

int TaskListModel::indexOfTask(int taskID) const
{
    for (int i = 0; i < m_tasks.size(); ++i)
        if (m_tasks.at(i)->taskID() == taskID) return i;
    return -1;
}

void TaskListModel::setTasks(const QList<RevTask*> &tasks)
{
    beginResetModel();
    for (auto *t : m_tasks) t->deleteLater();
    m_tasks = tasks;
    endResetModel();
    emit countChanged();
    emit countsChanged();
}
