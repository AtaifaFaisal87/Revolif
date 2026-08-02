#include "expenselistmodel.h"

ExpenseListModel::ExpenseListModel(QObject *parent) : QAbstractListModel(parent) {}

int ExpenseListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_expenses.size();
}

QVariant ExpenseListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_expenses.size()) return QVariant();
    RevExpense *exp = m_expenses.at(index.row());
    switch (role) {
    case ExpenseObjectRole: return QVariant::fromValue(exp);
    case ExpenseIDRole: return exp->expenseID();
    case TitleRole: return exp->title();
    case AmountRole: return exp->amount();
    case CategoryRole: return exp->category();
    case DateRole: return exp->date();
    case DescriptionRole: return exp->description();
    }
    return QVariant();
}

QHash<int, QByteArray> ExpenseListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ExpenseObjectRole] = "expenseObject";
    roles[ExpenseIDRole] = "expenseID";
    roles[TitleRole] = "title";
    roles[AmountRole] = "amount";
    roles[CategoryRole] = "category";
    roles[DateRole] = "date";
    roles[DescriptionRole] = "description";
    return roles;
}

double ExpenseListModel::totalAmount() const
{
    double total = 0.0;
    for (auto *e : m_expenses) total += e->amount();
    return total;
}

RevExpense* ExpenseListModel::get(int index) const
{
    if (index < 0 || index >= m_expenses.size()) return nullptr;
    return m_expenses.at(index);
}

void ExpenseListModel::addExpense(RevExpense *expense)
{
    beginInsertRows(QModelIndex(), m_expenses.size(), m_expenses.size());
    m_expenses.append(expense);
    endInsertRows();
    emit countChanged();
    emit totalAmountChanged();
}

void ExpenseListModel::removeExpense(int index)
{
    if (index < 0 || index >= m_expenses.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_expenses.at(index)->deleteLater();
    m_expenses.removeAt(index);
    endRemoveRows();
    emit countChanged();
    emit totalAmountChanged();
}

void ExpenseListModel::clear()
{
    if (m_expenses.isEmpty()) return;
    beginResetModel();
    for (auto *e : m_expenses) e->deleteLater();
    m_expenses.clear();
    endResetModel();
    emit countChanged();
    emit totalAmountChanged();
}

int ExpenseListModel::indexOfExpense(int expenseID) const
{
    for (int i = 0; i < m_expenses.size(); ++i)
        if (m_expenses.at(i)->expenseID() == expenseID) return i;
    return -1;
}

QVariantMap ExpenseListModel::categoryTotals() const
{
    QVariantMap totals;
    for (auto *e : m_expenses) {
        QString cat = e->category();
        totals[cat] = totals[cat].toDouble() + e->amount();
    }
    return totals;
}

QVariantMap ExpenseListModel::topCategory() const
{
    QVariantMap totals = categoryTotals();
    QString topCat;
    double topAmt = 0.0;
    for (auto it = totals.begin(); it != totals.end(); ++it) {
        if (it.value().toDouble() > topAmt) {
            topAmt = it.value().toDouble();
            topCat = it.key();
        }
    }
    QVariantMap result;
    result["category"] = topCat;
    result["amount"] = topAmt;
    return result;
}

void ExpenseListModel::setExpenses(const QList<RevExpense*> &expenses)
{
    beginResetModel();
    for (auto *e : m_expenses) e->deleteLater();
    m_expenses = expenses;
    endResetModel();
    emit countChanged();
    emit totalAmountChanged();
}
