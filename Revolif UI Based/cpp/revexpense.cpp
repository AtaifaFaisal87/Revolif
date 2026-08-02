#include "revexpense.h"

RevExpense::RevExpense(QObject *parent) : QObject(parent) {}

void RevExpense::setExpenseID(int id) { if (m_expenseID != id) { m_expenseID = id; emit expenseIDChanged(); } }
void RevExpense::setTitle(const QString &t) { if (m_title != t) { m_title = t; emit titleChanged(); } }
void RevExpense::setAmount(double a) { if (m_amount != a) { m_amount = a; emit amountChanged(); } }
void RevExpense::setCategory(const QString &c) { if (m_category != c) { m_category = c; emit categoryChanged(); } }
void RevExpense::setDate(const QDate &d) { if (m_date != d) { m_date = d; emit dateChanged(); } }
void RevExpense::setDescription(const QString &d) { if (m_description != d) { m_description = d; emit descriptionChanged(); } }
