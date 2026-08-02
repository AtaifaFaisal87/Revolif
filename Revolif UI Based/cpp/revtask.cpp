#include "revtask.h"
#include <QDate>

RevTask::RevTask(QObject *parent) : QObject(parent) {}

bool RevTask::isOverdue() const
{
    return m_status == "Pending" && m_deadline < QDate::currentDate();
}

bool RevTask::isDueSoon() const
{
    if (m_status != "Pending") return false;
    int days = QDate::currentDate().daysTo(m_deadline);
    return days >= 0 && days <= 3;
}

void RevTask::setTaskID(int id) { if (m_taskID != id) { m_taskID = id; emit taskIDChanged(); } }
void RevTask::setTitle(const QString &t) { if (m_title != t) { m_title = t; emit titleChanged(); } }
void RevTask::setDescription(const QString &d) { if (m_description != d) { m_description = d; emit descriptionChanged(); } }
void RevTask::setDeadline(const QDate &d) { if (m_deadline != d) { m_deadline = d; emit deadlineChanged(); emit overdueChanged(); emit dueSoonChanged(); } }
void RevTask::setDeadlineTime(const QTime &t) { if (m_deadlineTime != t) { m_deadlineTime = t; emit deadlineTimeChanged(); } }
void RevTask::setCategory(const QString &c) { if (m_category != c) { m_category = c; emit categoryChanged(); } }
void RevTask::setStatus(const QString &s) { if (m_status != s) { m_status = s; emit statusChanged(); emit overdueChanged(); emit dueSoonChanged(); } }
void RevTask::setPriority(const QString &p) { if (m_priority != p) { m_priority = p; emit priorityChanged(); } }
void RevTask::setIsRecurring(bool r) { if (m_isRecurring != r) { m_isRecurring = r; emit isRecurringChanged(); } }
void RevTask::setRecurrenceInterval(const QString &i) { if (m_recurrenceInterval != i) { m_recurrenceInterval = i; emit recurrenceIntervalChanged(); } }
void RevTask::setTaskType(int type) { if (m_taskType != type) { m_taskType = type; emit taskTypeChanged(); } }

void RevTask::markCompleted() { setStatus("Completed"); }
void RevTask::markPending() { setStatus("Pending"); }

int RevTask::priorityWeight(const QString &p)
{
    if (p == "High") return 1;
    if (p == "Medium") return 2;
    return 3;
}
