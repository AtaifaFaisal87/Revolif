#include "goallistmodel.h"
#include <QDate>

GoalListModel::GoalListModel(QObject *parent) : QAbstractListModel(parent) {}

int GoalListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_goals.size();
}

QVariant GoalListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_goals.size()) return QVariant();
    RevGoal *goal = m_goals.at(index.row());
    switch (role) {
    case GoalObjectRole: return QVariant::fromValue(goal);
    case GoalIDRole: return goal->goalID();
    case TitleRole: return goal->title();
    case DescriptionRole: return goal->description();
    case CategoryRole: return goal->category();
    case DeadlineRole: return goal->deadline();
    case StatusRole: return goal->status();
    case DisplayStatusRole: return goal->displayStatus();
    case IsOverdueRole: return goal->isOverdue();
    case IsCompletedRole: return goal->isCompleted();
    }
    return QVariant();
}

QHash<int, QByteArray> GoalListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[GoalObjectRole] = "goalObject";
    roles[GoalIDRole] = "goalID";
    roles[TitleRole] = "title";
    roles[DescriptionRole] = "description";
    roles[CategoryRole] = "category";
    roles[DeadlineRole] = "deadline";
    roles[StatusRole] = "status";
    roles[DisplayStatusRole] = "displayStatus";
    roles[IsOverdueRole] = "isOverdue";
    roles[IsCompletedRole] = "isCompleted";
    return roles;
}

int GoalListModel::pendingCount() const
{
    int c = 0;
    for (auto *g : m_goals) if (g->status() != "Completed" && !g->isOverdue()) c++;
    return c;
}

int GoalListModel::completedCount() const
{
    int c = 0;
    for (auto *g : m_goals) if (g->status() == "Completed") c++;
    return c;
}

int GoalListModel::overdueCount() const
{
    int c = 0;
    for (auto *g : m_goals) if (g->status() != "Completed" && g->isOverdue()) c++;
    return c;
}

RevGoal* GoalListModel::get(int index) const
{
    if (index < 0 || index >= m_goals.size()) return nullptr;
    return m_goals.at(index);
}

void GoalListModel::addGoal(RevGoal *goal)
{
    beginInsertRows(QModelIndex(), m_goals.size(), m_goals.size());
    m_goals.append(goal);
    endInsertRows();
    emit countChanged();
    emit countsChanged();
}

void GoalListModel::removeGoal(int index)
{
    if (index < 0 || index >= m_goals.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_goals.at(index)->deleteLater();
    m_goals.removeAt(index);
    endRemoveRows();
    emit countChanged();
    emit countsChanged();
}

void GoalListModel::clear()
{
    if (m_goals.isEmpty()) return;
    beginResetModel();
    for (auto *g : m_goals) g->deleteLater();
    m_goals.clear();
    endResetModel();
    emit countChanged();
    emit countsChanged();
}

int GoalListModel::indexOfGoal(int goalID) const
{
    for (int i = 0; i < m_goals.size(); ++i)
        if (m_goals.at(i)->goalID() == goalID) return i;
    return -1;
}

void GoalListModel::setGoals(const QList<RevGoal*> &goals)
{
    beginResetModel();
    for (auto *g : m_goals) g->deleteLater();
    m_goals = goals;
    endResetModel();
    emit countChanged();
    emit countsChanged();
}
