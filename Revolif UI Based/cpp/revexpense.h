#ifndef REVEXPENSE_H
#define REVEXPENSE_H

#include <QObject>
#include <QString>
#include <QDate>

class RevExpense : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int expenseID READ expenseID WRITE setExpenseID NOTIFY expenseIDChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(double amount READ amount WRITE setAmount NOTIFY amountChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    Q_PROPERTY(QDate date READ date WRITE setDate NOTIFY dateChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)

public:
    explicit RevExpense(QObject *parent = nullptr);

    int expenseID() const { return m_expenseID; }
    QString title() const { return m_title; }
    double amount() const { return m_amount; }
    QString category() const { return m_category; }
    QDate date() const { return m_date; }
    QString description() const { return m_description; }

    void setExpenseID(int id);
    void setTitle(const QString &t);
    void setAmount(double a);
    void setCategory(const QString &c);
    void setDate(const QDate &d);
    void setDescription(const QString &d);

signals:
    void expenseIDChanged();
    void titleChanged();
    void amountChanged();
    void categoryChanged();
    void dateChanged();
    void descriptionChanged();

private:
    int m_expenseID = 0;
    QString m_title;
    double m_amount = 0.0;
    QString m_category;
    QDate m_date;
    QString m_description;
};

#endif // REVEXPENSE_H
