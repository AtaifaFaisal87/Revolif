#include "revolifcontroller.h"
#include "../core/revolif_backend.cpp"
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QDate>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <cmath>

RevolifController::RevolifController(QObject *parent)
    : QObject(parent), system(nullptr), currentUser(nullptr), adminMode(false)
{
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    system = new System();
    m_pageTitle = "Dashboard";

    m_focusTimer = new QTimer(this);
    m_focusTimer->setInterval(1000);
    connect(m_focusTimer, &QTimer::timeout, this, [this]() { emit focusTick(); });

    // Currency is an app-wide preference (not tied to a user account), saved
    // locally so it's remembered the next time the app is opened. Falls back
    // to PKR if nothing was saved yet or the saved value isn't recognized.
    QSettings settings("Revolif", "Revolif");
    QString savedCode = settings.value("currencyCode", "PKR").toString();
    if (savedCode == "INR" || savedCode == "PKR" || savedCode == "USD")
        m_currencyCode = savedCode;

    // Dark mode is also an app-wide preference, remembered across launches.
    m_darkMode = settings.value("darkMode", false).toBool();
}

RevolifController::~RevolifController()
{
    delete system;
}

bool RevolifController::isLoggedIn() const { return currentUser != nullptr; }
bool RevolifController::isAdmin() const { return adminMode; }

QString RevolifController::currentUserName() const {
    return currentUser ? QString::fromStdString(currentUser->getDisplayName()) : "";
}

QString RevolifController::currentUserTitle() const {
    return currentUser ? QString::fromStdString(currentUser->getTitle()) : "";
}

QString RevolifController::currentUserEmail() const {
    return currentUser ? QString::fromStdString(currentUser->getEmail()) : "";
}

// ---- Name of whichever achievement the user has pinned/featured on the
// Achievements page (currentUser->getDisplayedAchievementID()). This is
// deliberately separate from currentUserTitle(), which reflects an
// unrelated, automatically-earned streak title -- using that property for
// the profile badge meant pinning an achievement never actually changed
// what appeared next to the username. Returns an empty string when
// nothing is pinned (-1 sentinel) or the id no longer matches an
// achievement. ----
QString RevolifController::featuredAchievementName() const {
    if (!currentUser) return QString();
    int id = currentUser->getDisplayedAchievementID();
    if (id == -1) return QString();
    Achievement *ach = system->getAchievementByID(id);
    return ach ? QString::fromStdString(ach->getName()) : QString();
}

int RevolifController::currentStreak() const {
    return currentUser ? currentUser->getCurrentStreak() : 0;
}

int RevolifController::bestStreak() const {
    return currentUser ? currentUser->getBestStreak() : 0;
}

int RevolifController::lifeScore() const {
    return currentUser ? system->calculateLifeScore(*currentUser) : 0;
}

QString RevolifController::lifeScoreLabel() const {
    return currentUser ? QString::fromStdString(system->getLifeScoreLabel(system->calculateLifeScore(*currentUser))) : "";
}

int RevolifController::pendingTasks() const {
    return currentUser ? currentUser->getTaskManager().countPendingTasks() : 0;
}

int RevolifController::completedTasks() const {
    return currentUser ? currentUser->getTaskManager().countCompletedTasks() : 0;
}

int RevolifController::overdueTasks() const {
    return currentUser ? currentUser->getTaskManager().countOverdueTasks() : 0;
}

int RevolifController::pendingGoals() const {
    return currentUser ? currentUser->getGoalManager().countPendingGoals() : 0;
}

int RevolifController::completedGoals() const {
    return currentUser ? currentUser->getGoalManager().countCompletedGoals() : 0;
}

double RevolifController::totalExpenses() const {
    return currentUser ? currentUser->getExpenseManager().calculateTotalExpense() : 0.0;
}

QString RevolifController::pageTitle() const { return m_pageTitle; }

void RevolifController::setPageTitle(const QString &title) {
    if (m_pageTitle != title) {
        m_pageTitle = title;
        emit pageTitleChanged();
    }
}

QString RevolifController::currencySymbol() const {
    if (m_currencyCode == "INR") return "₹";
    if (m_currencyCode == "USD") return "$";
    return "Rs."; // PKR (default)
}

bool RevolifController::setCurrency(const QString &code) {
    if (code != "INR" && code != "PKR" && code != "USD") {
        emit errorOccurred("Unsupported currency.");
        return false;
    }
    if (m_currencyCode == code) return true;
    m_currencyCode = code;
    QSettings settings("Revolif", "Revolif");
    settings.setValue("currencyCode", m_currencyCode);
    emit currencyChanged();
    emit statsChanged();
    return true;
}

bool RevolifController::setDarkMode(bool enabled) {
    if (m_darkMode == enabled) return true;
    m_darkMode = enabled;
    QSettings settings("Revolif", "Revolif");
    settings.setValue("darkMode", m_darkMode);
    emit darkModeChanged();
    return true;
}

QVariantList RevolifController::getCurrencyOptions() const {
    QVariantList list;
    auto addOption = [&list](const QString &code, const QString &label, const QString &symbol) {
        QVariantMap m;
        m["code"] = code;
        m["label"] = label;
        m["symbol"] = symbol;
        list.append(m);
    };
    addOption("INR", "Indian Rupee (₹)", "₹");
    addOption("PKR", "Pakistani Rupee (Rs.)", "Rs.");
    addOption("USD", "US Dollar ($)", "$");
    return list;
}

bool RevolifController::login(const QString &username, const QString &password) {
    User *user = nullptr;
    LoginResult result = system->getAuth().attemptLogin(username.toStdString(), password.toStdString(), user);

    switch (result) {
    case LOGIN_ADMIN:
        emit loginChimeRequested();
        adminMode = true;
        currentUser = nullptr;
        emit isLoggedInChanged();
        emit isAdminChanged();
        emit currentUserChanged();
        return true;
    case LOGIN_SUCCESS:
        emit loginChimeRequested();
        adminMode = false;
        currentUser = user;
        system->prepareUserSession(user);
        currentUser->checkAndUpdateTitle();
        loadFocusHistory();
        emit isLoggedInChanged();
        emit isAdminChanged();
        emit currentUserChanged();
        emit statsChanged();
        return true;
    case LOGIN_SUSPENDED: {
        bool canRestore = user ? user->getDeactivatedBySelf() : false;
        emit accountSuspended(username, canRestore);
        return false;
    }
    case LOGIN_NO_ACCOUNTS:
        emit errorOccurred("No accounts registered yet.");
        return false;
    case LOGIN_USER_NOT_FOUND:
        emit errorOccurred("User not found.");
        return false;
    case LOGIN_WRONG_PASSWORD:
        emit errorOccurred("Invalid password.");
        return false;
    }
    return false;
}

bool RevolifController::restoreAccount(const QString &username, const QString &password) {
    User *u = system->getAuth().lookupUserByUsername(username.toStdString());
    if (!u) {
        emit errorOccurred("User not found.");
        return false;
    }
    if (u->getIsActive()) {
        emit errorOccurred("Account is not suspended.");
        return false;
    }
    if (!system->getAuth().verifyPassword(u, password.toStdString())) {
        emit errorOccurred("Wrong password.");
        return false;
    }
    if (!u->getDeactivatedBySelf()) {
        emit errorOccurred("This account was suspended by an admin and can't be self-restored. Please contact an admin.");
        return false;
    }

    u->setActive(true);
    u->setDeactivatedBySelf(false);
    system->rewriteUsersFile();

    emit loginChimeRequested();

    adminMode = false;
    currentUser = u;
    system->prepareUserSession(u);
    currentUser->checkAndUpdateTitle();
    loadFocusHistory();
    emit isLoggedInChanged();
    emit isAdminChanged();
    emit currentUserChanged();
    emit statsChanged();
    emit successMessage("Account restored. Welcome back!");
    return true;
}

bool RevolifController::permanentlyDeleteSuspendedAccount(const QString &username, const QString &password, const QString &reason) {
    User *u = system->getAuth().lookupUserByUsername(username.toStdString());
    if (!u) {
        emit errorOccurred("User not found.");
        return false;
    }
    if (u->getIsActive()) {
        emit errorOccurred("Account is not suspended.");
        return false;
    }
    if (!system->getAuth().verifyPassword(u, password.toStdString())) {
        emit errorOccurred("Wrong password.");
        return false;
    }

    QString finalReason = reason.trimmed().isEmpty() ? QString("Not specified") : reason;
    system->permanentlyDeleteUser(u, finalReason.toStdString());
    emit statsChanged();
    emit successMessage("Account permanently deleted.");
    return true;
}

bool RevolifController::registerUser(const QString &name, const QString &username,
                                      const QString &dob, const QString &email,
                                      const QString &password) {
    if (name.trimmed().isEmpty()) {
        emit errorOccurred("Full name is required.");
        return false;
    }
    if (username.trimmed().isEmpty()) {
        emit errorOccurred("Username is required.");
        return false;
    }
    if (email.trimmed().isEmpty() || !email.contains('@') || !email.contains('.')) {
        emit errorOccurred("Enter a valid email address.");
        return false;
    }
    if (password.length() < 6) {
        emit errorOccurred("Password must be at least 6 characters.");
        return false;
    }
    try {
        QStringList parts = dob.split('/');
        if (parts.size() != 3) {
            emit errorOccurred("Invalid date format. Use DD/MM/YYYY");
            return false;
        }
        Date dobDate(parts[0].toInt(), parts[1].toInt(), parts[2].toInt());

        if (system->getAuth().lookupUserByUsername(username.toStdString()) != nullptr) {
            emit errorOccurred("Username already exists.");
            return false;
        }

        if (system->getAuth().emailExists(email.toStdString())) {
            emit errorOccurred("Email already registered.");
            return false;
        }

        User *newUser = new User(name.toStdString(), username.toStdString(), dobDate, password.toStdString(), email.toStdString());
        system->getUsers().push_back(newUser);
        system->saveUserToFile(newUser);
        logAdminActivity("registration", name + " (@" + username + ") registered.");
        emit successMessage("Account created successfully!");
        return true;
    } catch (const std::exception &e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

void RevolifController::logout() {
    if (m_focusRunning || m_focusPaused) {
        finalizeAndSaveFocusSession();
    }
    m_focusHistory.clear();
    m_focusTimer->stop();
    m_focusRunning = false;
    m_focusPaused = false;
    m_focusAccumulatedSecs = 0;
    emit focusStateChanged();
    emit focusTick();

    currentUser = nullptr;
    adminMode = false;
    emit isLoggedInChanged();
    emit isAdminChanged();
    emit currentUserChanged();
    emit statsChanged();
}

QVariantList RevolifController::getTasks() {
    QVariantList list;
    if (!currentUser) return list;

    const std::vector<Task*> &tasks = currentUser->getTaskManager().getTasks();
    for (Task *t : tasks) {
        QVariantMap m;
        m["id"] = t->getTaskID();
        m["title"] = QString::fromStdString(t->getTitle());
        m["description"] = QString::fromStdString(t->getDescription());
        m["category"] = QString::fromStdString(t->getCategory());
        m["type"] = t->getTaskType() == 1 ? "Academic" : "Daily";
        m["status"] = QString::fromStdString(t->getStatus());
        m["priority"] = QString::fromStdString(t->getPriority());
        m["deadline"] = QString::fromStdString(t->getDeadline().toString());
        m["time"] = QString::fromStdString(t->getDeadlineTime().toString());
        m["isRecurring"] = t->getIsRecurring();
        m["recurrenceInterval"] = QString::fromStdString(t->getRecurrenceInterval());
        m["overdue"] = (t->getStatus() == "Pending" && t->getDeadline().isPastDate());
        list.append(m);
    }
    return list;
}

bool RevolifController::addTask(int type, const QString &title, const QString &description,
                                 int day, int month, int year, int hour, int minute,
                                 const QString &meridiem, const QString &priority,
                                 bool recurring, const QString &interval,
                                 const QString &category) {
    if (!currentUser) return false;
    if (title.trimmed().isEmpty()) {
        emit errorOccurred("Task title is required.");
        return false;
    }
    try {
        Date deadline(day, month, year);
        Time deadlineTime(hour, minute, meridiem.toStdString());
        Task *task;
        if (type == 1)
            task = new AcademicTask(title.toStdString(), description.toStdString(), deadline, deadlineTime);
        else
            task = new DailyTask(title.toStdString(), description.toStdString(), deadline, deadlineTime);
        if (!category.trimmed().isEmpty())
            task->setCategory(category.toStdString());
        task->setPriority(priority.toStdString());
        task->setRecurring(recurring, interval.toStdString());
        currentUser->getTaskManager().loadTask(task);
        system->saveTaskToFile(task, currentUser->getUID());
        system->logActivity(currentUser, "task_added", task->getTitle(), task->getCategory());
        emit statsChanged();
        emit successMessage("Task added successfully!");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to add task.");
        return false;
    }
}

bool RevolifController::updateTask(int id, const QString &field, const QVariant &value) {
    if (!currentUser) return false;
    Task *t = currentUser->getTaskManager().searchByID(id);
    if (!t) return false;

    std::string f = field.toStdString();
    if (f == "title") t->setTitle(value.toString().toStdString());
    else if (f == "description") t->setDescription(value.toString().toStdString());
    else if (f == "category") t->setCategory(value.toString().toStdString());
    else if (f == "priority") t->setPriority(value.toString().toStdString());
    else if (f == "status") {
        if (value.toString() == "Completed") t->markCompleted();
        else t->markPending();
    }

    system->rewriteTasksFile();
    emit statsChanged();
    return true;
}

bool RevolifController::deleteTask(int id) {
    if (!currentUser) return false;
    currentUser->getTaskManager().deleteTaskById(id);
    system->rewriteTasksFile();
    emit statsChanged();
    return true;
}

bool RevolifController::completeTask(int id) {
    if (!currentUser) return false;
    Task *t = currentUser->getTaskManager().searchByID(id);
    currentUser->getTaskManager().completeTaskById(id);
    system->rewriteTasksFile();
    if (t) system->logActivity(currentUser, "task_completed", t->getTitle(), t->getCategory());
    emit statsChanged();
    return true;
}

QVariantList RevolifController::getGoals() {
    QVariantList list;
    if (!currentUser) return list;

    const std::vector<Goal*> &goals = currentUser->getGoalManager().getGoals();
    for (Goal *g : goals) {
        QVariantMap m;
        m["id"] = g->getGoalID();
        m["title"] = QString::fromStdString(g->getTitle());
        m["description"] = QString::fromStdString(g->getDescription());
        m["category"] = QString::fromStdString(g->getCategory());
        m["status"] = QString::fromStdString(g->getStatus());
        m["displayStatus"] = QString::fromStdString(g->getDisplayStatus());
        m["deadline"] = QString::fromStdString(g->getDeadline().toString());
        m["overdue"] = (g->getStatus() != "Completed" && g->getDeadline().isPastDate());
        list.append(m);
    }
    return list;
}

bool RevolifController::addGoal(const QString &title, const QString &description,
                                 const QString &category, int day, int month, int year) {
    if (!currentUser) return false;
    if (title.trimmed().isEmpty()) {
        emit errorOccurred("Goal title is required.");
        return false;
    }
    try {
        Date deadline(day, month, year);
        Goal *goal = new Goal(title.toStdString(), description.toStdString(), category.toStdString(), deadline);
        currentUser->getGoalManager().loadGoal(goal);
        system->saveGoalToFile(goal, currentUser->getUID());
        system->logActivity(currentUser, "goal_added", goal->getTitle(), goal->getCategory());
        emit statsChanged();
        emit successMessage("Goal added successfully!");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to add goal.");
        return false;
    }
}

bool RevolifController::updateGoal(int id, const QString &field, const QVariant &value) {
    if (!currentUser) return false;
    Goal *g = currentUser->getGoalManager().searchByID(id);
    if (!g) return false;

    std::string f = field.toStdString();
    if (f == "title") g->setTitle(value.toString().toStdString());
    else if (f == "description") g->setDescription(value.toString().toStdString());
    else if (f == "category") g->setCategory(value.toString().toStdString());

    system->rewriteGoalsFile();
    emit statsChanged();
    return true;
}

bool RevolifController::deleteGoal(int id) {
    if (!currentUser) return false;
    currentUser->getGoalManager().deleteGoalById(id);
    system->rewriteGoalsFile();
    emit statsChanged();
    return true;
}

bool RevolifController::completeGoal(int id) {
    if (!currentUser) return false;
    Goal *g = currentUser->getGoalManager().searchByID(id);
    currentUser->getGoalManager().completeGoalById(id);
    system->checkAchievements(currentUser);
    currentUser->checkAndUpdateTitle();
    system->rewriteGoalsFile();
    system->rewriteUsersFile();
    if (g) system->logActivity(currentUser, "goal_completed", g->getTitle(), g->getCategory());
    emit statsChanged();
    emit currentUserChanged();
    return true;
}

QVariantList RevolifController::getExpenses() {
    QVariantList list;
    if (!currentUser) return list;

    const std::vector<Expense*> &expenses = currentUser->getExpenseManager().getExpenses();
    for (Expense *e : expenses) {
        QVariantMap m;
        m["id"] = e->getExpenseID();
        m["title"] = QString::fromStdString(e->getTitle());
        m["amount"] = e->getAmount();
        m["category"] = QString::fromStdString(e->getCategory());
        m["date"] = QString::fromStdString(e->getDate().toString());
        m["description"] = QString::fromStdString(e->getDescription());
        list.append(m);
    }
    return list;
}

bool RevolifController::addExpense(const QString &title, double amount, const QString &category,
                                    int day, int month, int year, const QString &description) {
    if (!currentUser) return false;
    if (title.trimmed().isEmpty()) {
        emit errorOccurred("Expense title is required.");
        return false;
    }
    if (std::isnan(amount) || amount <= 0.0) {
        emit errorOccurred("Enter a valid expense amount greater than 0.");
        return false;
    }
    try {
        Date date(day, month, year);
        Expense *expense = new Expense(title.toStdString(), amount, category.toStdString(), date, description.toStdString());
        currentUser->getExpenseManager().loadExpense(expense);
        system->saveExpenseToFile(expense, currentUser->getUID());
        system->logActivity(currentUser, "expense_added", expense->getTitle(), expense->getCategory());
        emit statsChanged();
        emit successMessage("Expense added successfully!");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to add expense.");
        return false;
    }
}

bool RevolifController::updateExpense(int id, const QString &field, const QVariant &value) {
    if (!currentUser) return false;
    Expense *e = currentUser->getExpenseManager().searchByID(id);
    if (!e) return false;

    std::string f = field.toStdString();
    if (f == "title") e->setTitle(value.toString().toStdString());
    else if (f == "amount") e->setAmount(value.toDouble());
    else if (f == "category") e->setCategory(value.toString().toStdString());
    else if (f == "description") e->setDescription(value.toString().toStdString());

    system->rewriteExpensesFile();
    emit statsChanged();
    return true;
}

bool RevolifController::deleteExpense(int id) {
    if (!currentUser) return false;
    currentUser->getExpenseManager().deleteExpenseById(id);
    system->rewriteExpensesFile();
    emit statsChanged();
    return true;
}

QVariantList RevolifController::getActivityHeatmap(int weeks) {
    QVariantList result;
    if (!currentUser) return result;
    if (weeks < 1) weeks = 1;
    if (weeks > 53) weeks = 53;

    int totalDays = weeks * 7;
    Date today = Date::getToday();
    long todayNum = today.toDayNumber();
    long startNum = todayNum - (totalDays - 1);

    // Count activity entries per day within the visible window.
    std::vector<int> counts(totalDays, 0);
    const std::vector<ActivityLog*>& entries = currentUser->getActivityLogManager().getEntries();
    for (ActivityLog* a : entries) {
        long dn = a->getDate().toDayNumber();
        long idx = dn - startNum;
        if (idx >= 0 && idx < totalDays)
            counts[idx]++;
    }

    int maxCount = 0;
    for (int c : counts) maxCount = std::max(maxCount, c);

    for (int i = 0; i < totalDays; i++) {
        Date d = Date::fromDayNumber(startNum + i);
        QVariantMap day;
        day["date"] = QString::fromStdString(d.toString());
        day["count"] = counts[i];
        // Intensity level 0-4, like LeetCode/GitHub graphs, scaled to this
        // user's own busiest day so the graph stays meaningful at any volume.
        int level = 0;
        if (counts[i] > 0 && maxCount > 0) {
            double ratio = (double)counts[i] / (double)maxCount;
            level = 1 + (int)(ratio * 3.0001); // 1..4
            if (level > 4) level = 4;
        }
        day["level"] = level;
        day["weekday"] = (int)(((startNum + i) % 7 + 7) % 7); // 0=Mon per toDayNumber epoch
        result.append(day);
    }
    return result;
}

QVariantMap RevolifController::getDashboardData() {
    QVariantMap data;
    if (!currentUser) return data;

    TaskManager& tm = currentUser->getTaskManager();
    GoalManager& gm = currentUser->getGoalManager();
    ExpenseManager& em = currentUser->getExpenseManager();

    data["userName"] = QString::fromStdString(currentUser->getDisplayName());
    data["title"] = QString::fromStdString(currentUser->getTitle());
    int score = system->calculateLifeScore(*currentUser);
    data["lifeScore"] = score;
    data["lifeScoreLabel"] = QString::fromStdString(system->getLifeScoreLabel(score));
    data["currentStreak"] = currentUser->getCurrentStreak();
    data["bestStreak"] = currentUser->getBestStreak();
    data["pendingTasks"] = tm.countPendingTasks();
    data["completedTasks"] = tm.countCompletedTasks();
    data["overdueTasks"] = tm.countOverdueTasks();
    data["dueSoonTasks"] = tm.countDueSoonTasks(3);
    data["pendingGoals"] = gm.countPendingGoals();
    data["completedGoals"] = gm.countCompletedGoals();
    data["overdueGoals"] = gm.countOverdueGoals();
    data["totalExpenses"] = em.calculateTotalExpense();

    auto topCat = em.getTopCategoryInfo();
    data["topCategory"] = QString::fromStdString(topCat.first);
    data["topCategoryAmount"] = topCat.second;
    data["nextTitle"] = QString::fromStdString(gm.getNextTitleName());
    data["goalsUntilNextTitle"] = gm.goalsUntilNextTitle();

    QVariantList focusTasks;
    std::vector<Task*> pending;
    for (Task* t : tm.getTasks()) {
        if (t->getStatus() == "Pending") pending.push_back(t);
    }
    std::sort(pending.begin(), pending.end(), [](Task* a, Task* b) {
        int wa = (a->getPriority() == "High") ? 1 : (a->getPriority() == "Medium") ? 2 : 3;
        int wb = (b->getPriority() == "High") ? 1 : (b->getPriority() == "Medium") ? 2 : 3;
        if (wa != wb) return wa < wb;
        return a->getDeadline().toComparable() < b->getDeadline().toComparable();
    });
    for (size_t i = 0; i < pending.size() && i < 3; i++) {
        QVariantMap t;
        t["id"] = pending[i]->getTaskID();
        t["title"] = QString::fromStdString(pending[i]->getTitle());
        t["category"] = QString::fromStdString(pending[i]->getCategory());
        t["priority"] = QString::fromStdString(pending[i]->getPriority());
        t["deadline"] = QString::fromStdString(pending[i]->getDeadline().toString());
        focusTasks.append(t);
    }
    data["focusTasks"] = focusTasks;

    QVariantList upcoming;
    std::vector<Task*> upcomingSorted = pending;
    std::sort(upcomingSorted.begin(), upcomingSorted.end(), [](Task* a, Task* b) {
        long da = a->getDeadline().toComparable();
        long db = b->getDeadline().toComparable();
        if (da != db) return da < db;
        auto minutesOf = [](Task* t) {
            int h = t->getDeadlineTime().getHour() % 12;
            if (t->getDeadlineTime().getMeridiem() == "PM") h += 12;
            return h * 60 + t->getDeadlineTime().getMinute();
        };
        return minutesOf(a) < minutesOf(b);
    });
    for (size_t i = 0; i < upcomingSorted.size() && i < 5; i++) {
        QVariantMap t;
        t["id"] = upcomingSorted[i]->getTaskID();
        t["title"] = QString::fromStdString(upcomingSorted[i]->getTitle());
        t["deadline"] = QString::fromStdString(upcomingSorted[i]->getDeadline().toString());
        t["time"] = QString::fromStdString(upcomingSorted[i]->getDeadlineTime().toString());
        upcoming.append(t);
    }
    data["upcomingTasks"] = upcoming;

    // ---- Today's summary cards: Tasks / Goals / Expenses ----
    Date today = Date::getToday();
    double expensesThisMonth = 0.0;
    for (Expense* e : em.getExpenses()) {
        if (e->getDate().getMonth() == today.getMonth() && e->getDate().getYear() == today.getYear())
            expensesThisMonth += e->getAmount();
    }
    data["expensesThisMonth"] = expensesThisMonth;

    // ---- Small monthly expense bar chart: this month's spend split into
    // ~weekly buckets (Week 1..5), straight from the expense records. ----
    QVariantList expenseChart;
    {
        double weekTotals[5] = {0, 0, 0, 0, 0};
        for (Expense* e : em.getExpenses()) {
            if (e->getDate().getMonth() == today.getMonth() && e->getDate().getYear() == today.getYear()) {
                int wk = (e->getDate().getDay() - 1) / 7; // 0..4
                if (wk > 4) wk = 4;
                weekTotals[wk] += e->getAmount();
            }
        }
        for (int w = 0; w < 5; w++) {
            QVariantMap bucket;
            bucket["label"] = QString("W%1").arg(w + 1);
            bucket["amount"] = weekTotals[w];
            expenseChart.append(bucket);
        }
    }
    data["monthlyExpenseChart"] = expenseChart;

    // ---- GitHub-style contribution calendar for the current month, built
    // from the real activity log (task/goal/expense events). ----
    QVariantList calendarDays;
    {
        ActivityLogManager& alm = currentUser->getActivityLogManager();
        int y = today.getYear();
        int m = today.getMonth();
        int daysInThisMonth = QDate(y, m, 1).daysInMonth();
        Date first(1, m, y);
        // Sunday = 0 .. Saturday = 6 (civil day-number epoch 1970-01-01 was a Thursday)
        int firstWeekday = (int)(((first.toDayNumber() % 7) + 4 + 7) % 7);

        for (int d = 1; d <= daysInThisMonth; d++) {
            Date day(d, m, y);
            int count = alm.countOnDate(day);
            int level = count == 0 ? 0 : (count == 1 ? 1 : (count <= 3 ? 2 : 3));
            QVariantMap cell;
            cell["day"] = d;
            cell["count"] = count;
            cell["level"] = level;
            cell["isToday"] = (d == today.getDay());
            calendarDays.append(cell);
        }
        data["calendarFirstWeekday"] = firstWeekday;
    }
    data["contributionCalendar"] = calendarDays;

    // ---- Recent activity timeline: last 8 logged events, newest first ----
    QVariantList recentActivity;
    {
        ActivityLogManager& alm = currentUser->getActivityLogManager();
        std::vector<ActivityLog*> recent = alm.getRecent(8);
        for (ActivityLog* a : recent) {
            QVariantMap item;
            item["type"] = QString::fromStdString(a->getType());
            item["title"] = QString::fromStdString(a->getTitle());
            item["category"] = QString::fromStdString(a->getCategory());
            item["date"] = QString::fromStdString(a->getDate().toString());
            item["time"] = QString::fromStdString(a->getTime().toString());
            recentActivity.append(item);
        }
    }
    data["recentActivity"] = recentActivity;

    return data;
}

QVariantMap RevolifController::getProfileData() {
    QVariantMap data;
    if (!currentUser) return data;
    data["uid"] = currentUser->getUID();
    data["username"] = QString::fromStdString(currentUser->getUsername());
    data["name"] = QString::fromStdString(currentUser->getName());
    data["email"] = QString::fromStdString(currentUser->getEmail());
    data["dob"] = QString::fromStdString(currentUser->getDOB().toString());
    data["registrationDate"] = QString::fromStdString(currentUser->getRegistrationDate().toString());
    data["lastLogin"] = currentUser->getLastLogin().toComparable() == 20000101 
                         ? "Never" : QString::fromStdString(currentUser->getLastLogin().toString());
    data["status"] = currentUser->getIsActive() ? "Active" : "Inactive";
    data["achievements"] = currentUser->getUnlockedCount();
    data["streak"] = currentUser->getCurrentStreak();
    data["bestStreak"] = currentUser->getBestStreak();
    return data;
}

bool RevolifController::updateProfile(const QString &field, const QVariant &value) {
    if (!currentUser) return false;
    std::string f = field.toStdString();
    QString v = value.toString().trimmed();

    if (f == "name") {
        if (v.isEmpty()) {
            emit errorOccurred("Name cannot be empty.");
            return false;
        }
        currentUser->setName(v.toStdString());
    } else if (f == "email") {
        if (v.isEmpty() || !v.contains('@') || !v.contains('.')) {
            emit errorOccurred("Enter a valid email address.");
            return false;
        }
        if (system->getAuth().emailExists(v.toStdString(), currentUser)) {
            emit errorOccurred("That email is already in use by another account.");
            return false;
        }
        currentUser->setEmail(v.toStdString());
    } else {
        return false;
    }

    system->rewriteUsersFile();
    emit currentUserChanged();
    emit successMessage("Profile updated successfully!");
    return true;
}

bool RevolifController::changePassword(const QString &oldPass, const QString &newPass) {
    if (!currentUser) return false;
    try {
        if (!system->getAuth().verifyPassword(currentUser, oldPass.toStdString())) {
            emit errorOccurred("Wrong current password.");
            return false;
        }
        if (newPass.length() < 6) {
            emit errorOccurred("Password must be at least 6 characters.");
            return false;
        }
        currentUser->setPasswordHash(simpleHash(newPass.toStdString()));
        system->rewriteUsersFile();
        emit successMessage("Password changed successfully.");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to change password.");
        return false;
    }
}

bool RevolifController::deleteMyAccount(const QString &password) {
    if (!currentUser) return false;
    if (!system->getAuth().verifyPassword(currentUser, password.toStdString())) {
        emit errorOccurred("Wrong password.");
        return false;
    }
    currentUser->setActive(false);
    currentUser->setDeactivatedBySelf(true);
    system->rewriteUsersFile();
    logout();
    emit successMessage("Account deleted successfully.");
    return true;
}

bool RevolifController::generateMonthlyReport() {
    if (!currentUser) return false;
    try {
        system->generateMonthlyReport(*currentUser);
        QString filename = QString::fromStdString(currentUser->getUsername()) + "_report.txt";
        QString finalPath = moveReportToFolder(filename);
        emit successMessage(finalPath.isEmpty()
            ? "Report generated successfully."
            : "Report saved to Documents > Revolif Reports.");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to generate report.");
        return false;
    }
}

bool RevolifController::openReportsFolder() {
    return QDesktopServices::openUrl(QUrl::fromLocalFile(reportsFolderPath()));
}

QString RevolifController::getReportsFolderPath() const {
    return QDir::toNativeSeparators(reportsFolderPath());
}

QVariantList RevolifController::getAchievements() {
    QVariantList list;
    const std::vector<Achievement> &achs = system->getAchievements();
    std::vector<int> unlocked = currentUser ? currentUser->getUnlockedAchievementIDs() : std::vector<int>();

    for (const Achievement &a : achs) {
        QVariantMap m;
        m["id"] = a.getAchievementID();
        m["name"] = QString::fromStdString(a.getName());
        m["description"] = QString::fromStdString(a.getDescription());
        m["requiredGoals"] = a.getRequiredGoals();
        m["isDefault"] = a.getIsDefault();
        m["unlocked"] = std::find(unlocked.begin(), unlocked.end(), a.getAchievementID()) != unlocked.end();
        list.append(m);
    }
    return list;
}

bool RevolifController::setDisplayedAchievement(int id) {
    if (!currentUser) return false;
    currentUser->setDisplayedAchievementID(id);
    system->rewriteUsersFile();
    emit currentUserChanged();
    return true;
}

int RevolifController::getDisplayedAchievementId() {
    return currentUser ? currentUser->getDisplayedAchievementID() : -1;
}

QVariantList RevolifController::getBudgets() {
    QVariantList list;
    if (!currentUser) return list;

    const std::map<std::string, double> &budgets = currentUser->getExpenseManager().getCategoryBudgets();
    for (const auto &pair : budgets) {
        double spent = currentUser->getExpenseManager().getCategoryTotal(pair.first);
        QVariantMap m;
        m["category"] = QString::fromStdString(pair.first);
        m["limit"] = pair.second;
        m["spent"] = spent;
        m["overBudget"] = pair.second > 0.0 && spent > pair.second;
        list.append(m);
    }
    return list;
}

bool RevolifController::setBudget(const QString &category, double limit) {
    if (!currentUser) {
        emit errorOccurred("You must be logged in to set a budget.");
        return false;
    }
    QString trimmedCategory = category.trimmed();
    if (trimmedCategory.isEmpty()) {
        emit errorOccurred("Category is required.");
        return false;
    }
    if (std::isnan(limit) || limit < 0.0) {
        emit errorOccurred("Budget limit must be zero or greater.");
        return false;
    }
    currentUser->getExpenseManager().setBudgetGUI(trimmedCategory.toStdString(), limit);
    emit statsChanged();
    emit successMessage("Budget set for " + trimmedCategory + ".");
    return true;
}

QVariantMap RevolifController::getSpendingByCategory() {
    QVariantMap data;
    if (!currentUser) return data;

    const std::vector<Expense*> &expenses = currentUser->getExpenseManager().getExpenses();
    std::map<std::string, double> totals;
    for (Expense *e : expenses) {
        totals[e->getCategory()] += e->getAmount();
    }
    for (auto &p : totals) {
        data[QString::fromStdString(p.first)] = p.second;
    }
    return data;
}

bool RevolifController::isCurrentUserAdmin() const {
    return adminMode;
}

QVariantList RevolifController::getAllUsers() {
    QVariantList list;
    const std::vector<User*> &users = system->getUsers();
    for (User *u : users) {
        QVariantMap m;
        m["uid"] = u->getUID();
        m["username"] = QString::fromStdString(u->getUsername());
        m["name"] = QString::fromStdString(u->getName());
        m["email"] = QString::fromStdString(u->getEmail());
        m["active"] = u->getIsActive();
        m["deactivatedBySelf"] = u->getDeactivatedBySelf();
        m["streak"] = u->getCurrentStreak();
        m["bestStreak"] = u->getBestStreak();
        m["goalsCompleted"] = u->getGoalManager().countCompletedGoals();
        Date reg = u->getRegistrationDate();
        m["regDate"] = QString("%1/%2/%3")
                           .arg(reg.getDay(), 2, 10, QChar('0'))
                           .arg(reg.getMonth(), 2, 10, QChar('0'))
                           .arg(reg.getYear());
        list.append(m);
    }
    return list;
}

bool RevolifController::suspendUser(const QString &username) {
    try {
        User *u = system->getAuth().lookupUserByUsername(username.toStdString());
        if (!u) {
            emit errorOccurred("User not found.");
            return false;
        }
        u->setActive(false);
        u->setDeactivatedBySelf(false);
        system->rewriteUsersFile();
        logAdminActivity("suspension", QString::fromStdString(u->getName()) + " (@" + username + ") was suspended.");
        emit statsChanged();
        emit successMessage("User suspended.");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to suspend user.");
        return false;
    }
}

bool RevolifController::unsuspendUser(const QString &username) {
    try {
        User *u = system->getAuth().lookupUserByUsername(username.toStdString());
        if (!u) {
            emit errorOccurred("User not found.");
            return false;
        }
        u->setActive(true);
        u->setDeactivatedBySelf(false);
        system->rewriteUsersFile();
        logAdminActivity("reactivation", QString::fromStdString(u->getName()) + " (@" + username + ") was reactivated.");
        emit statsChanged();
        emit successMessage("User unsuspended.");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to unsuspend user.");
        return false;
    }
}

bool RevolifController::permanentlyDeleteUser(const QString &username, const QString &reason) {
    try {
        User *u = system->getAuth().lookupUserByUsername(username.toStdString());
        if (!u) {
            emit errorOccurred("User not found.");
            return false;
        }
        if (u->getIsActive()) {
            emit errorOccurred("User must be suspended before they can be permanently deleted.");
            return false;
        }
        QString name = QString::fromStdString(u->getName());
        system->permanentlyDeleteUser(u, reason.toStdString());
        logAdminActivity("deletion", name + " (@" + username + ") was permanently deleted.");
        emit statsChanged();
        emit successMessage("User permanently deleted.");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to delete user.");
        return false;
    }
}

QVariantList RevolifController::getPermanentlyDeletedUsers() {
    QVariantList list;
    std::ifstream file("permanently_deleted_users.dat", std::ios::binary);
    if (!file) return list;

    PermanentDeletedUserRecord pdr;
    while (file.read((char*)&pdr, sizeof(pdr))) {
        QVariantMap m;
        m["uid"] = pdr.uid;
        m["username"] = QString::fromStdString(std::string(pdr.username));
        m["name"] = QString::fromStdString(std::string(pdr.name));
        m["date"] = QString("%1/%2/%3").arg(pdr.deletion_day).arg(pdr.deletion_month).arg(pdr.deletion_year);
        m["reason"] = QString::fromStdString(std::string(pdr.reason));
        list.append(m);
    }
    file.close();
    return list;
}

QStringList RevolifController::getDeletionReasonOptions() {
    QStringList options;
    for (const std::string &reason : System::getDeletionReasonsList())
        options << QString::fromStdString(reason);
    return options;
}

QVariantList RevolifController::getDeletionReasonStats() {
    QVariantList list;
    int permanentlyDeletedCount = 0;
    std::map<std::string, int> counts = system->getDeletionReasonCounts(permanentlyDeletedCount);
    const std::vector<std::string> &reasons = System::getDeletionReasonsList();

    // Same "sort by count, highest first" order as the console bar chart.
    std::vector<int> order(reasons.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = (int)i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return counts[reasons[a]] > counts[reasons[b]];
    });

    for (int idx : order) {
        const std::string &reason = reasons[idx];
        int count = counts[reason];
        double percent = permanentlyDeletedCount > 0
                              ? (double)count / permanentlyDeletedCount * 100.0
                              : 0.0;
        QVariantMap m;
        m["reason"] = QString::fromStdString(reason);
        m["count"] = count;
        m["percent"] = percent;
        list.append(m);
    }
    return list;
}

QVariantMap RevolifController::getSystemStatistics() {
    QVariantMap data;
    const std::vector<User*> &users = system->getUsers();
    int active = 0, inactive = 0, suspended = 0, totalGoals = 0;
    for (User *u : users) {
        if (u->getIsActive()) active++;
        else if (u->getDeactivatedBySelf()) inactive++;
        else suspended++;
        totalGoals += u->getGoalManager().countCompletedGoals();
    }
    data["totalUsers"] = (int)users.size();
    data["activeUsers"] = active;
    data["inactiveUsers"] = inactive;
    data["suspendedUsers"] = suspended + inactive;
    data["totalGoals"] = totalGoals;
    data["totalAchievements"] = (int)system->getAchievements().size();
    data["permanentlyDeletedUsers"] = (int)getPermanentlyDeletedUsers().size();
    return data;
}

void RevolifController::logAdminActivity(const QString &type, const QString &detail) {
    QVariantMap entry;
    entry["type"] = type;
    entry["detail"] = detail;
    entry["timestamp"] = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm");
    m_adminActivity.prepend(entry);
    while (m_adminActivity.size() > 30) m_adminActivity.removeLast();
}

QVariantList RevolifController::getRecentAdminActivity() {
    return m_adminActivity;
}

bool RevolifController::generateSystemReport() {
    try {
        system->generateSystemReport();
        QString finalPath = moveReportToFolder("system_report.txt");
        emit successMessage(finalPath.isEmpty()
            ? "System report generated."
            : "System report saved to Documents > Revolif Reports.");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to generate report.");
        return false;
    }
}

bool RevolifController::adminChangePassword(const QString &oldPass, const QString &newPass) {
    try {
        if (!system->getAuth().verifyAdminPassword(oldPass.toStdString())) {
            emit errorOccurred("Wrong current password.");
            return false;
        }
        if (newPass.length() < 6) {
            emit errorOccurred("Password must be at least 6 characters.");
            return false;
        }
        system->getAuth().changeAdminPassword(newPass.toStdString());
        system->rewriteAdminFile();
        emit successMessage("Admin password changed.");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to change password.");
        return false;
    }
}

bool RevolifController::addAchievement(const QString &name, const QString &description, int requiredGoals) {
    if (name.trimmed().isEmpty()) {
        emit errorOccurred("Achievement name is required.");
        return false;
    }
    try {
        system->getAchievements().push_back(Achievement(name.toStdString(), description.toStdString(), requiredGoals, false));
        system->rewriteAchievementsFile();
        logAdminActivity("achievement", "Achievement \"" + name + "\" was added.");
        emit statsChanged();
        emit successMessage("Achievement added.");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to add achievement.");
        return false;
    }
}

bool RevolifController::removeAchievement(int id) {
    try {
        Achievement *ach = system->getAchievementByID(id);
        if (!ach) {
            emit errorOccurred("Achievement not found.");
            return false;
        }
        if (ach->getIsDefault()) {
            emit errorOccurred("Cannot remove default achievements.");
            return false;
        }
        auto &achs = system->getAchievements();
        for (size_t i = 0; i < achs.size(); i++) {
            if (achs[i].getAchievementID() == id) {
                QString name = QString::fromStdString(achs[i].getName());
                achs.erase(achs.begin() + i);
                system->rewriteAchievementsFile();
                logAdminActivity("achievement", "Achievement \"" + name + "\" was deleted.");
                emit statsChanged();
                emit successMessage("Achievement removed.");
                return true;
            }
        }
        return false;
    } catch (...) {
        emit errorOccurred("Failed to remove achievement.");
        return false;
    }
}

bool RevolifController::updateAchievement(int id, const QString &field, const QVariant &value) {
    try {
        Achievement *ach = system->getAchievementByID(id);
        if (!ach) {
            emit errorOccurred("Achievement not found.");
            return false;
        }
        if (ach->getIsDefault()) {
            emit errorOccurred("Cannot update default achievements.");
            return false;
        }
        std::string f = field.toStdString();
        if (f == "name") ach->setName(value.toString().toStdString());
        else if (f == "description") ach->setDescription(value.toString().toStdString());
        else if (f == "requiredGoals") ach->setRequiredGoals(value.toInt());
        system->rewriteAchievementsFile();
        logAdminActivity("achievement", "Achievement \"" + QString::fromStdString(ach->getName()) + "\" was updated.");
        emit statsChanged();
        emit successMessage("Achievement updated.");
        return true;
    } catch (...) {
        emit errorOccurred("Failed to update achievement.");
        return false;
    }
}

QString RevolifController::getErrorMessage() const {
    return m_errorMessage;
}

// ================================================================
//  FOCUS SESSION FEATURE
//  Self-contained addition: timer state lives in this controller,
//  history is persisted as a small per-user JSON file so it doesn't
//  require touching the legacy System/User storage format.
// ================================================================

// ================================================================
//  REPORT EXPORTS (.txt)
//  The backend (System::generateMonthlyReport / generateSystemReport)
//  writes its plain-text report into the current working directory,
//  same as the console build. That directory is the app's own install
//  folder though, which the user has no easy reason to go looking in.
//  So once the backend has written the file, we move it into a
//  "Revolif Reports" folder under the user's Documents, where it's
//  easy to find, and expose a button to jump straight there.
// ================================================================

QString RevolifController::reportsFolderPath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Revolif Reports";
    QDir().mkpath(dir);
    return dir;
}

QString RevolifController::moveReportToFolder(const QString &generatedFileName) {
    QString source = QDir::currentPath() + "/" + generatedFileName;
    QString destDir = reportsFolderPath();
    QString dest = destDir + "/" + generatedFileName;

    if (!QFile::exists(source)) return QString();

    QFile::remove(dest);
    if (!QFile::rename(source, dest)) {
        // Cross-volume moves can fail with rename(); fall back to copy+remove.
        if (QFile::copy(source, dest)) {
            QFile::remove(source);
        } else {
            return source; // couldn't relocate it, but it was still generated
        }
    }
    return dest;
}

QString RevolifController::focusDataFilePath() const {
    if (!currentUser) return QString();
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QString username = QString::fromStdString(currentUser->getUsername());
    return dir + "/focus_sessions_" + username + ".json";
}

void RevolifController::loadFocusHistory() {
    m_focusHistory.clear();
    m_focusRunning = false;
    m_focusPaused = false;
    m_focusAccumulatedSecs = 0;
    m_focusTimer->stop();

    QString path = focusDataFilePath();
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isArray()) return;

    for (const QJsonValue &v : doc.array()) {
        QJsonObject o = v.toObject();
        RevFocusRecord rec;
        rec.date = QDate::fromString(o.value("date").toString(), Qt::ISODate);
        rec.startTime = QDateTime::fromString(o.value("start").toString(), Qt::ISODate);
        rec.endTime = QDateTime::fromString(o.value("end").toString(), Qt::ISODate);
        rec.durationSeconds = static_cast<qint64>(o.value("duration").toDouble());
        m_focusHistory.append(rec);
    }

    emit focusStateChanged();
    emit focusTick();
    emit focusHistoryChanged();
}

void RevolifController::saveFocusHistory() {
    QString path = focusDataFilePath();
    if (path.isEmpty()) return;

    QJsonArray arr;
    for (const RevFocusRecord &rec : m_focusHistory) {
        QJsonObject o;
        o["date"] = rec.date.toString(Qt::ISODate);
        o["start"] = rec.startTime.toString(Qt::ISODate);
        o["end"] = rec.endTime.toString(Qt::ISODate);
        o["duration"] = static_cast<double>(rec.durationSeconds);
        arr.append(o);
    }

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        f.close();
    }
}

void RevolifController::finalizeAndSaveFocusSession() {
    if (m_focusRunning) {
        m_focusAccumulatedSecs += m_focusRunStart.secsTo(QDateTime::currentDateTime());
    }
    if (m_focusAccumulatedSecs > 0 && m_focusSessionStart.isValid()) {
        RevFocusRecord rec;
        rec.date = m_focusSessionStart.date();
        rec.startTime = m_focusSessionStart;
        rec.endTime = QDateTime::currentDateTime();
        rec.durationSeconds = m_focusAccumulatedSecs;
        m_focusHistory.prepend(rec);
        saveFocusHistory();
        emit focusHistoryChanged();
    }
}

QString RevolifController::formatHm(qint64 seconds) {
    qint64 h = seconds / 3600;
    qint64 m = (seconds % 3600) / 60;
    if (h > 0) {
        return QString("%1h %2m").arg(h).arg(m, 2, 10, QChar('0'));
    }
    return QString("%1m").arg(m);
}

int RevolifController::focusElapsedSeconds() const {
    if (m_focusRunning) {
        return static_cast<int>(m_focusAccumulatedSecs + m_focusRunStart.secsTo(QDateTime::currentDateTime()));
    }
    return static_cast<int>(m_focusAccumulatedSecs);
}

QString RevolifController::focusElapsedFormatted() const {
    int total = focusElapsedSeconds();
    int h = total / 3600;
    int m = (total % 3600) / 60;
    int s = total % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

void RevolifController::focusStart() {
    if (!currentUser) return;
    if (m_focusRunning || m_focusPaused) return; // a session is already active; use Pause/Resume/Stop

    m_focusAccumulatedSecs = 0;
    m_focusSessionStart = QDateTime::currentDateTime();
    m_focusRunStart = m_focusSessionStart;
    m_focusRunning = true;
    m_focusPaused = false;
    m_focusTimer->start();
    emit focusStateChanged();
    emit focusTick();
}

void RevolifController::focusPause() {
    if (!m_focusRunning) return;
    m_focusAccumulatedSecs += m_focusRunStart.secsTo(QDateTime::currentDateTime());
    m_focusRunning = false;
    m_focusPaused = true;
    m_focusTimer->stop();
    emit focusStateChanged();
    emit focusTick();
}

void RevolifController::focusResume() {
    if (!m_focusPaused) return;
    m_focusRunStart = QDateTime::currentDateTime();
    m_focusRunning = true;
    m_focusPaused = false;
    m_focusTimer->start();
    emit focusStateChanged();
    emit focusTick();
}

void RevolifController::focusStop() {
    if (!m_focusRunning && !m_focusPaused) return;
    finalizeAndSaveFocusSession();

    m_focusRunning = false;
    m_focusPaused = false;
    m_focusAccumulatedSecs = 0;
    m_focusTimer->stop();
    emit focusStateChanged();
    emit focusTick();
}

void RevolifController::focusReset() {
    // Discards the current segment without saving it as a session.
    m_focusRunning = false;
    m_focusPaused = false;
    m_focusAccumulatedSecs = 0;
    m_focusTimer->stop();
    emit focusStateChanged();
    emit focusTick();
}

QVariantMap RevolifController::getFocusStats() {
    QVariantMap stats;
    qint64 todaySecs = 0, weekSecs = 0, totalSecs = 0, longestSecs = 0;

    QDate today = QDate::currentDate();
    QDate weekStart = today.addDays(-(today.dayOfWeek() - 1)); // Monday of this week

    QSet<QString> sessionDates;
    for (const RevFocusRecord &rec : m_focusHistory) {
        totalSecs += rec.durationSeconds;
        if (rec.durationSeconds > longestSecs) longestSecs = rec.durationSeconds;
        if (rec.date == today) todaySecs += rec.durationSeconds;
        if (rec.date >= weekStart && rec.date <= today) weekSecs += rec.durationSeconds;
        sessionDates.insert(rec.date.toString(Qt::ISODate));
    }

    // Include the currently running/paused session in "today" and "total" so
    // the stats feel live while a session is in progress.
    int liveSecs = focusElapsedSeconds();
    if (liveSecs > 0) {
        todaySecs += liveSecs;
        weekSecs += liveSecs;
        totalSecs += liveSecs;
        if (liveSecs > longestSecs) longestSecs = liveSecs;
    }

    // Streak: consecutive days with at least one session, counting back from
    // today (or from yesterday if today has no session logged yet).
    QDate cursor = today;
    if (!sessionDates.contains(cursor.toString(Qt::ISODate)) && liveSecs == 0) {
        cursor = cursor.addDays(-1);
    }
    int streak = 0;
    while (sessionDates.contains(cursor.toString(Qt::ISODate)) ||
           (cursor == today && liveSecs > 0)) {
        streak++;
        cursor = cursor.addDays(-1);
    }

    stats["todaySeconds"] = static_cast<qlonglong>(todaySecs);
    stats["todayFormatted"] = formatHm(todaySecs);
    stats["weekSeconds"] = static_cast<qlonglong>(weekSecs);
    stats["weekFormatted"] = formatHm(weekSecs);
    stats["totalSeconds"] = static_cast<qlonglong>(totalSecs);
    stats["totalFormatted"] = formatHm(totalSecs);
    stats["longestSeconds"] = static_cast<qlonglong>(longestSecs);
    stats["longestFormatted"] = formatHm(longestSecs);
    stats["streakDays"] = streak;
    return stats;
}

QVariantList RevolifController::getFocusHistory() {
    QVariantList list;
    for (const RevFocusRecord &rec : m_focusHistory) {
        QVariantMap m;
        m["date"] = rec.date.toString("MMM d, yyyy");
        m["startTime"] = rec.startTime.toString("h:mm AP");
        m["endTime"] = rec.endTime.toString("h:mm AP");
        m["duration"] = formatHm(rec.durationSeconds);
        list.append(m);
    }
    return list;
}
