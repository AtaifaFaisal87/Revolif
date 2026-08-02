#ifndef GOALLISTMODEL_H
#define GOALLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "../revgoal.h"

class GoalListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY countsChanged)
    Q_PROPERTY(int completedCount READ completedCount NOTIFY countsChanged)
    Q_PROPERTY(int overdueCount READ overdueCount NOTIFY countsChanged)

public:
    enum GoalRoles {
        GoalObjectRole = Qt::UserRole + 1,
        GoalIDRole, TitleRole, DescriptionRole, CategoryRole,
        DeadlineRole, StatusRole, DisplayStatusRole, IsOverdueRole, IsCompletedRole
    };

    explicit GoalListModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_goals.size(); }
    int pendingCount() const;
    int completedCount() const;
    int overdueCount() const;

    Q_INVOKABLE RevGoal* get(int index) const;
    Q_INVOKABLE void addGoal(RevGoal *goal);
    Q_INVOKABLE void removeGoal(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int indexOfGoal(int goalID) const;
    QList<RevGoal*> goals() const { return m_goals; }
    void setGoals(const QList<RevGoal*> &goals);

signals:
    void countChanged();
    void countsChanged();

private:
    QList<RevGoal*> m_goals;
};

#endif // GOALLISTMODEL_H
