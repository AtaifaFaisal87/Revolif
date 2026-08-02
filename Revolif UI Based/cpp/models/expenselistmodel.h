#ifndef EXPENSELISTMODEL_H
#define EXPENSELISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "../revexpense.h"

class ExpenseListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(double totalAmount READ totalAmount NOTIFY totalAmountChanged)

public:
    enum ExpenseRoles {
        ExpenseObjectRole = Qt::UserRole + 1,
        ExpenseIDRole, TitleRole, AmountRole, CategoryRole, DateRole, DescriptionRole
    };

    explicit ExpenseListModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_expenses.size(); }
    double totalAmount() const;

    Q_INVOKABLE RevExpense* get(int index) const;
    Q_INVOKABLE void addExpense(RevExpense *expense);
    Q_INVOKABLE void removeExpense(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int indexOfExpense(int expenseID) const;
    Q_INVOKABLE QVariantMap categoryTotals() const;
    Q_INVOKABLE QVariantMap topCategory() const;
    QList<RevExpense*> expenses() const { return m_expenses; }
    void setExpenses(const QList<RevExpense*> &expenses);

signals:
    void countChanged();
    void totalAmountChanged();

private:
    QList<RevExpense*> m_expenses;
};

#endif // EXPENSELISTMODEL_H
