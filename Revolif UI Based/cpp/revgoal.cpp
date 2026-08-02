#include "revgoal.h"

RevGoal::RevGoal(QObject *parent) : QObject(parent) {}

QString RevGoal::displayStatus() const
{
    if (m_status == "Completed") return "Completed";
    if (m_deadline < QDate::currentDate()) return "Overdue";
    return "Pending";
}

bool RevGoal::isOverdue() const
{
    return m_status != "Completed" && m_deadline < QDate::currentDate();
}

void RevGoal::setGoalID(int id) { if (m_goalID != id) { m_goalID = id; emit goalIDChanged(); } }
void RevGoal::setTitle(const QString &t) { if (m_title != t) { m_title = t; emit titleChanged(); } }
void RevGoal::setDescription(const QString &d) { if (m_description != d) { m_description = d; emit descriptionChanged(); } }
void RevGoal::setCategory(const QString &c) { if (m_category != c) { m_category = c; emit categoryChanged(); } }
void RevGoal::setDeadline(const QDate &d) { if (m_deadline != d) { m_deadline = d; emit deadlineChanged(); emit displayStatusChanged(); emit overdueChanged(); } }
void RevGoal::setStatus(const QString &s) { if (m_status != s) { m_status = s; emit statusChanged(); emit displayStatusChanged(); emit overdueChanged(); } }

void RevGoal::completeGoal() { setStatus("Completed"); }
