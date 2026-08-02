// ============================================================
//              REVOLIF - LIFE MANAGER (VERSION 2.2)
// ============================================================
// This version includes everything from before, PLUS:
//   - Task priority levels (High/Medium/Low) with smart sorting
//   - Recurring tasks (Daily/Weekly/Monthly auto-reset)
//   - Login streak tracking (current + best streak)
//   - Life Score: a single composite score of overall progress
//   - Category budgets with overspending alerts
//   - ASCII bar chart for spending by category
//   - Hashed passwords (no more plaintext storage)
//   - Unique email enforcement across all users
//   - Exportable monthly report (.txt file)
//   - Account status system (soft delete):
//       Active --(user deletes account)--> Inactive --> Restore OR Permanently Delete
//       Active --(admin suspends account)--> Suspended --> Permanently Delete OR Logout only
//     (Inactive == self-deactivated, restorable at next login. Suspended ==
//     admin-deactivated, NOT restorable by the user.) A suspended/inactive
//     username may also be reused as a brand-new account via registration.
//   - Permanent deletion audit trail: permanently deleting an account wipes
//     it from every binary file (users, tasks, goals, expenses) and writes
//     a minimal, un-restorable audit record (UID, username, date, reason)
//     to permanently_deleted_users.dat, viewable by the admin for history
// ============================================================

// Default admin login: admin / admin123
// (Change the admin password immediately after first login.)

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <utility>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "libcurl.lib")
#endif

// ---- REVOLIF EMAIL SYSTEM: requires libcurl (with SMTP + TLS/SSL support
// enabled, which the standard curl builds have) for the SMTPS (SMTP-over-
// TLS) connection to Gmail. On MSYS2/MinGW-w64 (UCRT64), install with:
//   pacman -S mingw-w64-ucrt-x86_64-curl
// and link with -lcurl. On Linux/macOS, link with -lcurl. ----
#include <curl/curl.h>


using namespace std;

// ============================================================
//                  CUSTOM EXCEPTION CLASSES
// ============================================================

// covers: File could not be opened for reading or writing
class FileException : public exception
{
    string message;

public:
    FileException(string msg) : message(msg) {}
    const char *what() const noexcept override { return message.c_str(); }
};

// covers: Task/Goal/Expense/Achievement/User not found in manager
class NotFoundException : public exception
{
    string message;

public:
    NotFoundException(string msg) : message(msg) {}
    const char *what() const noexcept override { return message.c_str(); }
};

// covers: User not found, duplicate username/email
class UserException : public exception
{
    string message;

public:
    UserException(string msg) : message(msg) {}
    const char *what() const noexcept override { return message.c_str(); }
};

// covers: Wrong username or password
class AuthException : public exception
{
    string message;

public:
    AuthException(string msg) : message(msg) {}
    const char *what() const noexcept override { return message.c_str(); }
};

// covers: Invalid input data, invalid state, or invalid menu choice
class ValidationException : public exception
{
    string message;

public:
    ValidationException(string msg) : message(msg) {}
    const char *what() const noexcept override { return message.c_str(); }
};

// ============================================================
//                       RUNTIME LOGGER
// ============================================================

// Controls which runtime messages are printed during sensitive operations (e.g. file loading).
// Prevents statements like TASK ADDED from firing mid-load.
class RuntimeLogger
{
private:
    static bool suppressMessages;

public:
    static void enableSilentMode()
    {
        suppressMessages = true;
    }

    static void disableSilentMode()
    {
        suppressMessages = false;
    }

    static bool isSilent()
    {
        return suppressMessages;
    }
};

// Initialize static member
bool RuntimeLogger::suppressMessages = false;

// ============================================================
// STRUCTS - used for file I/O to flatten complex objects into plain data
// ============================================================

struct UserRecord
{
    int uid;
    char username[50];
    char name[50];
    int dob_day, dob_month, dob_year;
    char password_hash[50];
    char email[50];
    int reg_day, reg_month, reg_year;
    int lastLogin_day, lastLogin_month, lastLogin_year;
    bool isActive; // ACCOUNT STATUS (soft delete): true = Active, false = Inactive/Suspended. The
                    // record is never physically removed from users.dat just for being inactive.
    bool deactivatedBySelf; // true only if the USER deleted their own account (not an admin
                             // suspension) -- controls whether "Restore Account" is offered at login.
    int currentStreak;
    int bestStreak;
    char title[50];
    int displayedAchievementID;
    int unlockedCount;
    int unlockedIDs[100];
    bool welcomeEmailSent;
    int lastSummaryEmail_day, lastSummaryEmail_month, lastSummaryEmail_year;
};

struct TaskRecord
{
    int taskID;
    int userID;
    char title[100];
    char description[200];
    int deadline_day, deadline_month, deadline_year;
    int deadline_hour, deadline_minute;
    char deadline_meridiem[3];
    char category[20];
    char status[20];
    char priority[10];
    bool isRecurring;
    char recurrenceInterval[10];
    int taskType; // 1 = Academic, 2 = Daily
};

struct ExpenseRecord
{
    int expenseID;
    int userID;
    char title[100];
    double amount;
    char category[50];
    int date_day, date_month, date_year;
    char description[200];
};

struct GoalRecord
{
    int goalID;
    int userID;
    char title[100];
    char description[200];
    char category[50];
    int deadline_day, deadline_month, deadline_year;
    char status[20];
};

// ---- Append-only log entry for the dashboard's activity feed and
// contribution calendar. Written whenever the user adds/completes a task,
// completes a goal, or logs an expense. Never rewritten record-by-record --
// only ever appended to or fully rewritten (rewriteActivityLogFile), same
// pattern as every other .dat file in this app. ----
struct ActivityRecord
{
    int activityID;
    int userID;
    char type[24];     // "task_added","task_completed","goal_added","goal_completed","expense_added"
    char title[100];
    char category[50];
    int date_day, date_month, date_year;
    int time_hour, time_minute;
    char time_meridiem[3];
};

struct AchievementRecord
{
    int achievementID;
    char name[100];
    char description[200];
    int requiredGoals;
    bool isDefault;
};

struct AdminRecord
{
    char username[50];
    char password_hash[50];
};

// ---- Audit-only record for a permanently deleted account. Written to
// permanently_deleted_users.dat at the moment of permanent deletion, once
// every other trace of the user has been wiped from users/tasks/goals/
// expenses.dat. These records are history -- they are never read back
// into memory as live objects and can never be restored. ----
struct PermanentDeletedUserRecord
{
    int uid;
    char username[50];
    int deletion_day, deletion_month, deletion_year;
    char reason[200];
    char name[100];
};

// ============================================================
//                     GLOBAL HELPER FUNCTIONS
// ============================================================

#ifdef _WIN32
// Path to a small marker file the TTS script creates the instant it has
// finished loading/configuring the speech engine -- i.e. right before it
// starts actually speaking. This lets us wait on the real init delay
// instead of guessing at a duration.
string getTtsMarkerPath()
{
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    return string(tempPath) + "revolif_tts_ready.tmp";
}

bool fileExistsOnDisk(const string &path)
{
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES);
}

void speak(const string &message)
{
    string marker = getTtsMarkerPath();
    DeleteFileA(marker.c_str());

    // ---- Uses the native SAPI.SpVoice COM object rather than the
    // System.Speech .NET assembly. Add-Type + SelectVoice used to force
    // PowerShell to JIT-load System.Speech.dll and enumerate installed
    // voices on every single call -- that was the dominant contributor to
    // the multi-second delay before any sound played. SAPI.SpVoice is a
    // lightweight COM object with no CLR to warm up, so it starts
    // speaking almost as soon as the process launches. ----
    string args =
        "-WindowStyle Hidden -Command \""
        "$speak = New-Object -ComObject SAPI.SpVoice; "

        // Voice settings
        "$speak.Rate = -2; "
        "$speak.Volume = 80; "

        // Marker file
        "New-Item -Path '" +
        marker + "' -ItemType File -Force | Out-Null; "

                 // Speak the provided message
                 "$speak.Speak('" +
        message + "');\"";

    ShellExecuteA(
        NULL,
        "open",
        "powershell.exe",
        args.c_str(),
        NULL,
        SW_HIDE);
}
#endif

// ---- Lightweight loading bar ----
// Reflects real progress only; the caller decides when to advance it.
void showLoadingStep(const string &message, int percent)
{
    const int barWidth = 30;
    int filled = (barWidth * percent) / 100;
    bool isComplete = (percent >= 100);

    // ANSI color codes for a modern, clean look. The bar is cyan while
    // work is still in progress, and switches to green once it reaches
    // 100% ("Ready") so completion reads as a distinct, positive state.
    const string COLOR_FILL = isComplete ? "\033[92m" : "\033[36m"; // Green when done, cyan while loading
    const string COLOR_EMPTY = "\033[90m";                         // Bright black (gray) for empty
    const string COLOR_PCT = isComplete ? "\033[92m" : "\033[97m"; // Green percentage once done
    const string COLOR_MSG = isComplete ? "\033[92m" : "\033[37m"; // Green message once done
    const string COLOR_RESET = "\033[0m";                          // Reset

    cout << "\r" << COLOR_EMPTY << "[" << COLOR_RESET;
    for (int i = 0; i < barWidth; i++)
    {
        if (i < filled)
            cout << COLOR_FILL << "\u2588" << COLOR_RESET;
        else
            cout << COLOR_EMPTY << "\u2588" << COLOR_RESET;
    }
    cout << COLOR_EMPTY << "]" << COLOR_RESET
         << " " << COLOR_PCT << setw(3) << percent << "%" << COLOR_RESET
         << "  " << COLOR_MSG << message << COLOR_RESET
         << string(40, ' ') << flush;
}

string toLowerStr(string s)
{
    for (int i = 0; i < (int)s.size(); i++)
    {
        s[i] = tolower((unsigned char)s[i]);
    }
    return s;
}

bool containsIgnoreCase(const string &haystack, const string &needle)
{
    string h = toLowerStr(haystack);
    string n = toLowerStr(needle);
    return h.find(n) != string::npos;
}

// ---- SIMPLE PASSWORD HASH (djb2 algorithm) ----
// NOTE: This is a lightweight hash for demonstration purposes so that
// passwords are never stored or compared as plain text in memory/files.
string simpleHash(const string &input)
{
    unsigned long hash = 5381;
    for (int i = 0; i < (int)input.size(); i++)
        hash = ((hash << 5) + hash) + (unsigned char)input[i];

    stringstream ss;
    ss << hex << hash;
    return ss.str();
}

// ---- PRIORITY WEIGHT (for sorting tasks) ----
int priorityWeight(string p)
{
    if (p == "High")
        return 1;
    if (p == "Medium")
        return 2;
    return 3; // Low
}

// ============================================================
//              INPUT VALIDATION FUNCTIONS
// ============================================================

// ---- NON EMPTY STRING ----
string getNonEmptyLine(string prompt)
{
    string value;
    while (true)
    {
        cout << prompt;
        getline(cin, value);
        if (value.empty())
        {
            cout << "THIS FIELD CANNOT BE EMPTY! PLEASE TRY AGAIN.\n";
            continue;
        }
        return value;
    }
}

// ---- INTEGER INPUT ----
int getValidInt(string prompt)
{
    int value;
    while (true)
    {
        cout << prompt;
        cin >> value;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "INVALID INPUT! PLEASE ENTER A NUMBER." << endl;
        }
        else
        {
            cin.ignore(1000, '\n');
            return value;
        }
    }
}

// ---- INTEGER IN RANGE ----
int getValidIntInRange(string prompt, int min, int max)
{
    int value;
    while (true)
    {
        value = getValidInt(prompt);

        if (value < min || value > max)
            cout << "PLEASE ENTER A NUMBER BETWEEN " << min << " AND " << max << "." << endl;
        else
            return value;
    }
}

// ---- NON-NEGATIVE DOUBLE ----
double getNonNegativeAmount(string prompt)
{
    double amount;
    while (true)
    {
        cout << prompt;
        cin >> amount;
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "INVALID AMOUNT! PLEASE ENTER A NUMBER.\n";
            continue;
        }
        cin.ignore(1000, '\n');
        if (amount < 0)
        {
            cout << "AMOUNT CANNOT BE NEGATIVE! PLEASE TRY AGAIN.\n";
            continue;
        }
        return amount;
    }
}

// ---- LETTERS ONLY (for names) ----
string getValidAlphaString(string prompt)
{
    string value;
    while (true)
    {
        cout << prompt;
        getline(cin, value);

        if (value.empty())
        {
            cout << "INPUT CANNOT BE EMPTY!" << endl;
            continue;
        }

        bool valid = true;
        for (int i = 0; i < (int)value.size(); i++)
        {
            if (!isalpha(value[i]) && value[i] != ' ')
            {
                valid = false;
                break;
            }
        }

        if (!valid)
            cout << "ONLY LETTERS AND SPACES ALLOWED!" << endl;
        else
            return value;
    }
}

// ---- USERNAME (alphanumeric, underscores, 3-20 chars) ----
string getValidUsername(string prompt)
{
    string value;
    while (true)
    {
        cout << prompt;
        getline(cin, value);

        if (value.empty())
        {
            cout << "USERNAME CANNOT BE EMPTY!" << endl;
            continue;
        }

        if (value.size() < 3 || value.size() > 20)
        {
            cout << "USERNAME MUST BE BETWEEN 3 AND 20 CHARACTERS!" << endl;
            continue;
        }

        bool valid = true;
        for (int i = 0; i < (int)value.size(); i++)
        {
            if (!isalnum(value[i]) && value[i] != '_')
            {
                valid = false;
                break;
            }
        }

        if (!valid)
            cout << "USERNAME CAN ONLY CONTAIN LETTERS, NUMBERS, AND UNDERSCORES!" << endl;
        else
            return value;
    }
}

// ---- EMAIL ----
string getValidEmail(string prompt)
{
    string value;
    while (true)
    {
        cout << prompt;
        getline(cin, value);

        if (value.empty())
        {
            cout << "EMAIL CANNOT BE EMPTY!" << endl;
            continue;
        }

        bool hasAt = false;
        bool hasDot = false;
        int atPos = -1;

        for (int i = 0; i < (int)value.size(); i++)
        {
            if (value[i] == '@')
            {
                hasAt = true;
                atPos = i;
            }
            if (value[i] == '.' && hasAt && i > atPos + 1)
            {
                hasDot = true;
            }
        }

        bool valid = hasAt && hasDot;
        if (valid)
        {
            if (value.find(' ') != string::npos)
                valid = false;
            if (atPos == 0 || atPos == (int)value.size() - 1)
                valid = false;
            if (value[value.size() - 1] == '.')
                valid = false;
        }

        if (!valid)
            cout << "INVALID EMAIL! MUST BE A VALID EMAIL FORMAT (e.g., user@example.com)" << endl;
        else
            return value;
    }
}

// ---- MASKED INPUT (echoes '*' instead of the typed characters) ----
string getMaskedInput(string prompt)
{
    cout << prompt;
    string value;

#ifdef _WIN32
    char ch;
    while ((ch = _getch()) != '\r' && ch != '\n')
    {
        if (ch == '\b' || ch == 127) // backspace
        {
            if (!value.empty())
            {
                value.pop_back();
                cout << "\b \b";
            }
        }
        else
        {
            value.push_back(ch);
            cout << '*';
        }
    }
#else
    termios oldSettings, newSettings;
    tcgetattr(STDIN_FILENO, &oldSettings);
    newSettings = oldSettings;
    newSettings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

    // NOTE: we read through cin (not a raw read() on the fd) so that this
    // stays in sync with cin's internal buffer, which is also used by all
    // the other getline()/cin>> calls throughout the program.
    char ch;
    while (cin.get(ch) && ch != '\n')
    {
        if (ch == 127 || ch == '\b') // backspace
        {
            if (!value.empty())
            {
                value.pop_back();
                cout << "\b \b" << flush;
            }
        }
        else
        {
            value.push_back(ch);
            cout << '*' << flush;
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
#endif

    cout << endl;
    return value;
}

// ---- MASKED, NON-EMPTY INPUT (used for existing-password prompts) ----
string getMaskedNonEmptyLine(string prompt)
{
    string value;
    while (true)
    {
        value = getMaskedInput(prompt);
        if (value.empty())
        {
            cout << "THIS FIELD CANNOT BE EMPTY! PLEASE TRY AGAIN.\n";
            continue;
        }
        return value;
    }
}

// ---- PASSWORD (min 6 characters, masked input) ----
string getValidPassword(string prompt)
{
    string value;
    while (true)
    {
        value = getMaskedInput(prompt);

        if (value.empty())
        {
            cout << "PASSWORD CANNOT BE EMPTY!" << endl;
            continue;
        }

        if (value.size() < 6)
            cout << "PASSWORD TOO SHORT! MINIMUM 6 CHARACTERS." << endl;
        else
            return value;
    }
}

// ---- CONFIRM PASSWORD ----
string getConfirmedPassword(string prompt)
{
    string password, confirmPassword;
    while (true)
    {
        password = getValidPassword(prompt);
        confirmPassword = getValidPassword("CONFIRM PASSWORD: ");
        if (password != confirmPassword)
        {
            cout << "PASSWORDS DO NOT MATCH! PLEASE TRY AGAIN.\n";
            continue;
        }
        return password;
    }
}

// ---- VISIBLE INPUT (echoes typed characters normally, used for registration) ----
string getVisibleInput(string prompt)
{
    cout << prompt;
    string value;
    getline(cin, value);
    return value;
}

// ---- VISIBLE PASSWORD (min 6 characters, visible input, used for registration) ----
string getValidVisiblePassword(string prompt)
{
    string value;
    while (true)
    {
        value = getVisibleInput(prompt);

        if (value.empty())
        {
            cout << "PASSWORD CANNOT BE EMPTY!" << endl;
            continue;
        }

        if (value.size() < 6)
            cout << "PASSWORD TOO SHORT! MINIMUM 6 CHARACTERS." << endl;
        else
            return value;
    }
}

// ---- CONFIRM VISIBLE PASSWORD (used for registration) ----
string getConfirmedVisiblePassword(string prompt)
{
    string password, confirmPassword;
    while (true)
    {
        password = getValidVisiblePassword(prompt);
        confirmPassword = getValidVisiblePassword("CONFIRM PASSWORD: ");
        if (password != confirmPassword)
        {
            cout << "PASSWORDS DO NOT MATCH! PLEASE TRY AGAIN.\n";
            continue;
        }
        return password;
    }
}

// ---- CONFIRM ACTION ----
bool confirmAction(string message)
{
    int choice;
    while (true)
    {
        cout << "\n"
             << message << "\n";
        cout << "1. Yes\n";
        cout << "2. No\n";
        cout << "ENTER CHOICE: ";
        cin >> choice;
        cin.ignore(1000, '\n');
        if (choice == 1)
            return true;
        if (choice == 2)
            return false;
        cout << "INVALID CHOICE!\n";
    }
}

// ---- DATE (DD/MM/YYYY format) ----
string getValidDateString(string prompt)
{
    string value;
    while (true)
    {
        cout << prompt;
        getline(cin, value);

        bool valid = true;

        if (value.size() != 10)
            valid = false;
        else if (value[2] != '/' || value[5] != '/')
            valid = false;
        else
        {
            for (int i = 0; i < 10; i++)
            {
                if (i == 2 || i == 5)
                    continue;
                if (!isdigit(value[i]))
                {
                    valid = false;
                    break;
                }
            }

            if (valid)
            {
                int day = stoi(value.substr(0, 2));
                int month = stoi(value.substr(3, 2));
                int year = stoi(value.substr(6, 4));

                if (day < 1 || day > 31)
                    valid = false;
                if (month < 1 || month > 12)
                    valid = false;
                if (year < 1900 || year > 2100)
                    valid = false;

                int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
                bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
                if (isLeap)
                    daysInMonth[1] = 29;
                if (month >= 1 && month <= 12 && day > daysInMonth[month - 1])
                    valid = false;
            }
        }

        if (!valid)
            cout << "INVALID DATE! FORMAT MUST BE: DD/MM/YYYY (e.g., 15/07/2026)" << endl;
        else
            return value;
    }
}

// ---- TIME (HH:MM format, 12-hour with AM/PM) ----
string getValidTimeString(string prompt)
{
    string value;
    string period;
    while (true)
    {
        cout << prompt;
        getline(cin, value);

        bool valid = true;

        if (value.size() != 5)
            valid = false;
        else if (value[2] != ':')
            valid = false;
        else
        {
            for (int i = 0; i < 5; i++)
            {
                if (i == 2)
                    continue;
                if (!isdigit(value[i]))
                {
                    valid = false;
                    break;
                }
            }

            int hours = stoi(value.substr(0, 2));
            int minutes = stoi(value.substr(3, 2));

            if (hours < 1 || hours > 12)
                valid = false;
            if (minutes < 0 || minutes > 59)
                valid = false;
        }

        if (!valid)
        {
            cout << "INVALID TIME! FORMAT MUST BE: HH:MM (1-12)" << endl;
            continue;
        }

        cout << "ENTER AM OR PM: ";
        getline(cin, period);
        if (period != "AM" && period != "PM" && period != "am" && period != "pm")
        {
            cout << "INVALID! ENTER AM OR PM" << endl;
            continue;
        }

        if (period == "am")
            period = "AM";
        if (period == "pm")
            period = "PM";

        int hours = stoi(value.substr(0, 2));
        int minutes = stoi(value.substr(3, 2));

        if (period == "AM" && hours == 12)
            hours = 0;
        if (period == "PM" && hours != 12)
            hours += 12;

        char buf[6];
        sprintf(buf, "%02d:%02d", hours, minutes);
        return string(buf);
    }
}

// ---- PUBLISHER (letters, spaces, dots, & allowed) ----
string getValidPublisher(string prompt)
{
    string value;
    while (true)
    {
        cout << prompt;
        getline(cin, value);

        if (value.empty())
        {
            cout << "INPUT CANNOT BE EMPTY!" << endl;
            continue;
        }

        bool valid = true;
        for (int i = 0; i < (int)value.size(); i++)
        {
            if (!isalpha(value[i]) && value[i] != ' ' &&
                value[i] != '.' && value[i] != '&' && value[i] != '-')
            {
                valid = false;
                break;
            }
        }

        if (!valid)
            cout << "INVALID INPUT! ONLY LETTERS, SPACES, '.', '&', '-' ALLOWED." << endl;
        else
            return value;
    }
}

// ---- ISBN (10 or 13 digits, hyphens allowed) ----
string getValidISBN(string prompt)
{
    string value;
    while (true)
    {
        cout << prompt;
        getline(cin, value);

        if (value.empty())
        {
            cout << "INPUT CANNOT BE EMPTY!" << endl;
            continue;
        }

        string digitsOnly = "";
        bool valid = true;

        for (int i = 0; i < (int)value.size(); i++)
        {
            if (isdigit(value[i]))
                digitsOnly += value[i];
            else if (value[i] != '-')
            {
                valid = false;
                break;
            }
        }

        if (!valid || (digitsOnly.size() != 10 && digitsOnly.size() != 13))
            cout << "INVALID ISBN! MUST BE 10 OR 13 DIGITS (HYPHENS OPTIONAL)." << endl;
        else
            return value;
    }
}

// ---- CNIC (xxxxx-xxxxxxx-x format) ----
string getValidCNIC(string prompt)
{
    string value;
    while (true)
    {
        cout << prompt;
        getline(cin, value);

        bool valid = true;

        if (value.size() != 15)
            valid = false;
        else
        {
            for (int i = 0; i < 15; i++)
            {
                if (i == 5 || i == 13)
                {
                    if (value[i] != '-')
                    {
                        valid = false;
                        break;
                    }
                }
                else
                {
                    if (!isdigit(value[i]))
                    {
                        valid = false;
                        break;
                    }
                }
            }
        }

        if (!valid)
            cout << "INVALID CNIC! FORMAT MUST BE: xxxxx-xxxxxxx-x" << endl;
        else
            return value;
    }
}

// ---- PHONE (03xxxxxxxxx, 11 digits) ----
string getValidPhone(string prompt)
{
    string value;
    while (true)
    {
        cout << prompt;
        getline(cin, value);

        bool valid = true;

        if (value.size() != 11)
            valid = false;
        else if (value[0] != '0' || value[1] != '3')
            valid = false;
        else
        {
            for (int i = 0; i < 11; i++)
            {
                if (!isdigit(value[i]))
                {
                    valid = false;
                    break;
                }
            }
        }

        if (!valid)
            cout << "INVALID PHONE! FORMAT MUST BE: 03xxxxxxxxx (11 digits)" << endl;
        else
            return value;
    }
}

// ---- POSITIVE INTEGER ----
int getPositiveInt(string prompt)
{
    int value;
    while (true)
    {
        value = getValidInt(prompt);
        if (value <= 0)
            cout << "VALUE MUST BE GREATER THAN 0!" << endl;
        else
            return value;
    }
}

// ============================================================
//                          DATE CLASS
// ============================================================

class Date
{
private:
    int day;
    int month;
    int year;

    static bool isLeapYear(int y)
    {
        return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    }

    static int daysInMonth(int m, int y)
    {
        int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (m == 2 && isLeapYear(y))
            return 29;
        return days[m - 1];
    }

public:
    Date() : day(1), month(1), year(2000) {}
    Date(int d, int m, int y) : day(d), month(m), year(y) {}

    int getDay() const { return day; }
    int getMonth() const { return month; }
    int getYear() const { return year; }

    static Date getToday()
    {
        time_t now = time(nullptr);
        tm *ltm = localtime(&now);
        return Date(ltm->tm_mday, ltm->tm_mon + 1, ltm->tm_year + 1900);
    }

    long toComparable() const
    {
        return (long)year * 10000 + (long)month * 100 + (long)day;
    }

    bool isPastDate() const
    {
        return toComparable() < getToday().toComparable();
    }

    long toDayNumber() const
    {
        int y = year;
        int m = month;
        int d = day;
        y -= (m <= 2) ? 1 : 0;
        long era = (y >= 0 ? y : y - 399) / 400;
        long yoe = y - era * 400;
        long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
        long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097 + doe - 719468;
    }

    // ---- Inverse of toDayNumber(): builds a Date back from a day count ----
    static Date fromDayNumber(long z)
    {
        z += 719468;
        long era = (z >= 0 ? z : z - 146096) / 146097;
        unsigned long doe = (unsigned long)(z - era * 146097);
        unsigned long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        long y = (long)yoe + era * 400;
        unsigned long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        unsigned long mp = (5 * doy + 2) / 153;
        unsigned long d = doy - (153 * mp + 2) / 5 + 1;
        unsigned long m = mp + (mp < 10 ? 3 : -9);
        y += (m <= 2);
        return Date((int)d, (int)m, (int)y);
    }

    // ---- Returns a new Date shifted forward by 'days' days ----
    Date addDays(int days) const
    {
        return fromDayNumber(toDayNumber() + days);
    }

    long daysUntil(const Date &other) const
    {
        return other.toDayNumber() - toDayNumber();
    }

    void inputDate(bool restrictPast = false)
    {
        while (true)
        {
            int d = getValidIntInRange("ENTER DAY(1-31): ", 1, 31);
            int m = getValidIntInRange("ENTER MONTH(1-12): ", 1, 12);
            int y = getValidIntInRange("ENTER YEAR(1900-2100): ", 1900, 2100);

            int maxDay = daysInMonth(m, y);
            if (d < 1 || d > maxDay)
            {
                cout << "INVALID DAY! DAY MUST BE BETWEEN 1-" << maxDay
                     << " FOR THE SELECTED MONTH.\n";
                continue;
            }

            day = d;
            month = m;
            year = y;

            if (restrictPast && isPastDate())
            {
                cout << "ERROR! THIS DATE HAS ALREADY PASSED.\n";
                cout << "PLEASE ENTER TODAY'S DATE OR A FUTURE DATE.\n";
                continue;
            }
            break;
        }
    }

    void displayDate() const
    {
        cout << (day < 10 ? "0" : "")
             << day << "/"
             << (month < 10 ? "0" : "")
             << month << "/"
             << year;
    }

    friend ostream &operator<<(ostream &out, const Date &d)
    {
        out << (d.day < 10 ? "0" : "")
            << d.day << "/"
            << (d.month < 10 ? "0" : "")
            << d.month << "/"
            << d.year;
        return out;
    }

    bool isEqual(Date d) const
    {
        return day == d.day && month == d.month && year == d.year;
    }

    // GUI helper (defined out-of-line near end of file)
    std::string toString() const;
};

// ============================================================
//                          TIME CLASS
// ============================================================

class Time
{
private:
    int hour;
    int minute;
    string meridiem;

public:
    Time() : hour(12), minute(0), meridiem("PM") {}
    Time(int h, int m, string mer) : hour(h), minute(m), meridiem(mer) {}

    int getHour() const { return hour; }
    int getMinute() const { return minute; }
    string getMeridiem() const { return meridiem; }

    void inputTime()
    {
        while (true)
        {
            int h = getValidIntInRange("ENTER HOUR(1-12): ", 1, 12);
            int m = getValidIntInRange("ENTER MINUTE(0-59): ", 0, 59);

            cout << "SELECT AM/PM:\n";
            cout << "1. AM\n";
            cout << "2. PM\n";
            int merChoice = getValidIntInRange("ENTER CHOICE: ", 1, 2);
            string mer = (merChoice == 1) ? "AM" : "PM";

            hour = h;
            minute = m;
            meridiem = mer;
            break;
        }
    }

    friend ostream &operator<<(ostream &out, const Time &t)
    {
        out << (t.hour < 10 ? "0" : "")
            << t.hour << ":"
            << (t.minute < 10 ? "0" : "")
            << t.minute << " "
            << t.meridiem;
        return out;
    }

    // GUI helper (defined out-of-line near end of file)
    std::string toString() const;
};

// ============================================================
//         RECORD DISPLAY FORMATTING HELPERS (BOXED LAYOUT)
// ============================================================
// These helpers are used ONLY by record display functions such as
// displayTask(), displayGoal(), displayExpense(), displayAchievement().
// They do not affect any menus, prompts, or input/navigation screens.

const int RECORD_BOX_WIDTH = 72;   // total width of the box, borders included
const int RECORD_LABEL_WIDTH = 13; // width reserved for the label before ':'

// ---- Converts any streamable value (int, double, Date, Time, ...) to a string ----
template <typename T>
string toDisplayString(const T &value)
{
    ostringstream oss;
    oss << value;
    return oss.str();
}

// ---- Formats a monetary amount with 2 decimal places ----
string formatAmountForDisplay(double amount)
{
    ostringstream oss;
    oss << fixed << setprecision(2) << amount;
    return oss.str();
}

// ---- Prints a horizontal border line, e.g. "======...======" ----
void printBorder(int width = RECORD_BOX_WIDTH)
{
    cout << string(width, '=') << "\n";
}

// ---- Prints a blank line inside the box, e.g. "|          |" ----
void printEmptyLine(int width = RECORD_BOX_WIDTH)
{
    cout << "|" << string(width - 2, ' ') << "|\n";
}

// ---- Splits text into lines no wider than 'width', breaking only at spaces ----
vector<string> wrapText(const string &text, int width)
{
    vector<string> lines;
    if (width < 1)
        width = 1;

    istringstream iss(text);
    string word;
    string currentLine;

    while (iss >> word)
    {
        if (currentLine.empty())
        {
            currentLine = word;
        }
        else if ((int)(currentLine.size() + 1 + word.size()) <= width)
        {
            currentLine += " " + word;
        }
        else
        {
            lines.push_back(currentLine);
            currentLine = word;
        }
    }

    if (!currentLine.empty())
        lines.push_back(currentLine);
    if (lines.empty())
        lines.push_back("");

    return lines;
}

// ---- Shortens text to fit exactly within 'width' columns, appending "..."
//      when it had to cut something off. Used for single-line, fixed-width
//      table/chart columns (e.g. category or reason names) where wrapping
//      onto multiple lines isn't an option and free-text values could
//      otherwise overflow the column and break alignment. ----
string truncateForColumn(const string &text, int width)
{
    if (width < 1)
        width = 1;
    if ((int)text.size() <= width)
        return text;
    if (width <= 3)
        return text.substr(0, width);
    return text.substr(0, width - 3) + "...";
}

// ---- Builds a horizontal divider line of exactly 'width' characters ----
string makeDivider(int width, char ch = '-')
{
    if (width < 1)
        width = 1;
    return string(width, ch);
}

// ---- Builds a "===== TITLE =====" banner line, exactly 'width' characters
//      wide, with the title centered. Used by chart headers so the banner
//      always matches the actual table width instead of a hand-counted
//      literal string that can drift out of alignment. ----
string makeTitleBar(const string &title, int width, char ch = '=')
{
    if (width < 1)
        width = 1;
    string padded = " " + title + " ";
    if ((int)padded.size() >= width)
        return padded.substr(0, width);

    int totalFill = width - (int)padded.size();
    int leftFill = totalFill / 2;
    int rightFill = totalFill - leftFill;
    return string(leftFill, ch) + padded + string(rightFill, ch);
}

// ---- Prints one label:value field, wrapping long values across multiple
//      continuation lines that align perfectly under the value column ----
void printWrappedField(const string &label, const string &value,
                       int boxWidth = RECORD_BOX_WIDTH, int labelWidth = RECORD_LABEL_WIDTH)
{
    ostringstream prefixStream;
    prefixStream << "   " << left << setw(labelWidth) << label << ": ";
    string prefix = prefixStream.str();

    int valueWidth = boxWidth - 2 - (int)prefix.size();
    vector<string> lines = wrapText(value, valueWidth);

    for (size_t i = 0; i < lines.size(); i++)
    {
        string content = (i == 0) ? (prefix + lines[i]) : (string(prefix.size(), ' ') + lines[i]);
        int padding = boxWidth - 2 - (int)content.size();
        if (padding < 0)
            padding = 0;
        cout << "|" << content << string(padding, ' ') << "|\n";
    }
}

// ---- Prints a single-line label:value field (short values that never wrap) ----
void printField(const string &label, const string &value,
                int boxWidth = RECORD_BOX_WIDTH, int labelWidth = RECORD_LABEL_WIDTH)
{
    printWrappedField(label, value, boxWidth, labelWidth);
}

// ---- Renders a full record as a bordered box from a list of label/value pairs ----
void displayRecord(const vector<pair<string, string>> &fields,
                   int boxWidth = RECORD_BOX_WIDTH, int labelWidth = RECORD_LABEL_WIDTH)
{
    printBorder(boxWidth);
    printEmptyLine(boxWidth);
    for (size_t i = 0; i < fields.size(); i++)
        printWrappedField(fields[i].first, fields[i].second, boxWidth, labelWidth);
    printEmptyLine(boxWidth);
    printBorder(boxWidth);
}

// ============================================================
//                         TASK CLASS
// ============================================================

class Task
{
protected:
    int taskID;
    static int nextTaskID;
    string title;
    string description;
    Date deadline;
    Time deadlineTime;
    string category;
    string status;
    string priority;           // "High" / "Medium" / "Low"
    bool isRecurring;          // whether this task repeats
    string recurrenceInterval; // "None" / "Daily" / "Weekly" / "Monthly"
    int taskType;               // 1 = Academic, 2 = Daily -- independent of category

public:
    Task(string t, string desc, Date d, Time tm, string c, int type = 1)
    {
        taskID = nextTaskID++;
        title = t;
        description = desc;
        deadline = d;
        deadlineTime = tm;
        category = c;
        status = "Pending";
        priority = "Medium";
        isRecurring = false;
        recurrenceInterval = "None";
        taskType = type;
    }

    virtual ~Task() {}

    void setTaskID(int id) { taskID = id; }
    static void setNextTaskID(int id) { nextTaskID = id; }

    int getTaskID() const { return taskID; }
    string getTitle() const { return title; }
    string getDescription() const { return description; }
    Date getDeadline() const { return deadline; }
    Time getDeadlineTime() const { return deadlineTime; }
    string getCategory() const { return category; }
    int getTaskType() const { return taskType; }
    string getStatus() const { return status; }
    string getPriority() const { return priority; }
    bool getIsRecurring() const { return isRecurring; }
    string getRecurrenceInterval() const { return recurrenceInterval; }

    void setTitle(string t) { title = t; }
    void setDescription(string d) { description = d; }
    void setDeadline(Date d) { deadline = d; }
    void setDeadlineTime(Time t) { deadlineTime = t; }
    void setCategory(string c) { category = c; }
    void setPriority(string p) { priority = p; }
    void setRecurring(bool r, string interval = "None")
    {
        isRecurring = r;
        recurrenceInterval = r ? interval : "None";
    }

    void markCompleted() { status = "Completed"; }
    void markPending() { status = "Pending"; }

    virtual void displayTask() const = 0;
};

int Task::nextTaskID = 1;

// ============================================================
//                       ACADEMIC TASK CLASS
// ============================================================

class AcademicTask : public Task
{
public:
    AcademicTask(string t, string desc, Date d, Time tm)
        : Task(t, desc, d, tm, "Academic", 1) {}

    static string getAcademicTitle()
    {
        cout << "\n========== ACADEMIC TASK ==========\n";
        cout << "1. Exam\n";
        cout << "2. Assignment\n";
        cout << "3. Project\n";
        cout << "4. Midterm\n";
        cout << "5. Test\n";
        cout << "6. Submission\n";
        cout << "7. Viva\n";
        cout << "8. Presentation\n";
        cout << "9. Other\n";
        int choice = getValidIntInRange("ENTER CHOICE: ", 1, 9);

        switch (choice)
        {
        case 1:
            return "Exam";
        case 2:
            return "Assignment";
        case 3:
            return "Project";
        case 4:
            return "Midterm";
        case 5:
            return "Test";
        case 6:
            return "Submission";
        case 7:
            return "Viva";
        case 8:
            return "Presentation";
        default:
            return getNonEmptyLine("ENTER TASK TITLE: ");
        }
    }

    void displayTask() const override
    {
        vector<pair<string, string>> fields;
        fields.push_back(make_pair("TASK ID", toDisplayString(taskID)));
        fields.push_back(make_pair("CATEGORY", category));
        fields.push_back(make_pair("TITLE", title));
        fields.push_back(make_pair("DESCRIPTION", description));
        fields.push_back(make_pair("DEADLINE", toDisplayString(deadline) + "  " + toDisplayString(deadlineTime)));
        fields.push_back(make_pair("PRIORITY", priority));
        fields.push_back(make_pair("STATUS", status));
        if (isRecurring)
            fields.push_back(make_pair("RECURRING", recurrenceInterval));
        displayRecord(fields);
    }
};

// ============================================================
//                         DAILY TASK CLASS
// ============================================================

class DailyTask : public Task
{
public:
    DailyTask(string t, string desc, Date d, Time tm)
        : Task(t, desc, d, tm, "Daily", 2) {}

    static string getDailyTitle()
    {
        cout << "\n========== DAILY TASK ==========\n";
        cout << "1. Laundry\n";
        cout << "2. Cleaning House\n";
        cout << "3. Washing Dishes\n";
        cout << "4. Exercise\n";
        cout << "5. Grocery Shopping\n";
        cout << "6. Cooking\n";
        cout << "7. Reading\n";
        cout << "8. Other\n";
        int choice = getValidIntInRange("ENTER CHOICE: ", 1, 8);

        switch (choice)
        {
        case 1:
            return "Laundry";
        case 2:
            return "Cleaning House";
        case 3:
            return "Washing Dishes";
        case 4:
            return "Exercise";
        case 5:
            return "Grocery Shopping";
        case 6:
            return "Cooking";
        case 7:
            return "Reading";
        default:
            return getNonEmptyLine("ENTER TASK TITLE: ");
        }
    }

    void displayTask() const override
    {
        vector<pair<string, string>> fields;
        fields.push_back(make_pair("TASK ID", toDisplayString(taskID)));
        fields.push_back(make_pair("CATEGORY", category));
        fields.push_back(make_pair("TITLE", title));
        fields.push_back(make_pair("DESCRIPTION", description));
        fields.push_back(make_pair("DEADLINE", toDisplayString(deadline) + "  " + toDisplayString(deadlineTime)));
        fields.push_back(make_pair("PRIORITY", priority));
        fields.push_back(make_pair("STATUS", status));
        if (isRecurring)
            fields.push_back(make_pair("RECURRING", recurrenceInterval));
        displayRecord(fields);
    }
};

// ============================================================
//                       TASK MANAGER CLASS
// ============================================================

class TaskManager
{
private:
    vector<Task *> tasks;

public:
    ~TaskManager()
    {
        for (int i = 0; i < (int)tasks.size(); i++)
            delete tasks[i];
    }

    Task *addTask()
    {
        cout << "\n========== ADD TASK ==========\n";
        cout << "1. Academic Task\n";
        cout << "2. Daily Task\n";
        int choice = getValidIntInRange("ENTER CHOICE: ", 1, 2);

        string title = (choice == 1) ? AcademicTask::getAcademicTitle()
                                     : DailyTask::getDailyTitle();
        string description = getNonEmptyLine("ENTER DESCRIPTION: ");
        Date deadline;
        cout << "\nENTER DEADLINE DATE:\n";
        deadline.inputDate(true);
        Time deadlineTime;
        cout << "\nENTER DEADLINE TIME:\n";
        deadlineTime.inputTime();

        cout << "\nSELECT PRIORITY:\n";
        cout << "1. High\n";
        cout << "2. Medium\n";
        cout << "3. Low\n";
        int pChoice = getValidIntInRange("ENTER CHOICE: ", 1, 3);
        string priority = (pChoice == 1) ? "High" : (pChoice == 2) ? "Medium"
                                                                   : "Low";

        bool recurring = confirmAction("IS THIS A RECURRING TASK?");
        string interval = "None";
        if (recurring)
        {
            cout << "\nSELECT RECURRENCE INTERVAL:\n";
            cout << "1. Daily\n";
            cout << "2. Weekly\n";
            cout << "3. Monthly\n";
            int rChoice = getValidIntInRange("ENTER CHOICE: ", 1, 3);
            interval = (rChoice == 1) ? "Daily" : (rChoice == 2) ? "Weekly"
                                                                 : "Monthly";
        }

        Task *task;
        if (choice == 1)
        {
            task = new AcademicTask(title, description, deadline, deadlineTime);
            cout << "ACADEMIC TASK ADDED SUCCESSFULLY!\n";
        }
        else
        {
            task = new DailyTask(title, description, deadline, deadlineTime);
            cout << "DAILY TASK ADDED SUCCESSFULLY!\n";
        }

        task->setPriority(priority);
        task->setRecurring(recurring, interval);
        tasks.push_back(task);
        return task;
    }

    void loadTask(Task *task) { tasks.push_back(task); }
    const vector<Task *> &getTasks() const { return tasks; }

    // ---- Wipes every task belonging to this manager. Used when an inactive
    // account is reused during registration, so the "new" account starts
    // exactly like a brand-new one. ----
    void clearAllTasks()
    {
        for (int i = 0; i < (int)tasks.size(); i++)
            delete tasks[i];
        tasks.clear();
    }

    void displayAllTasks() const
    {
        if (tasks.empty())
        {
            cout << "NO TASKS AVAILABLE!\n";
            return;
        }
        cout << "\n========== ALL TASKS ==========\n";
        for (int i = 0; i < (int)tasks.size(); i++)
        {
            cout << "\n"
                 << (i + 1) << ".\n";
            tasks[i]->displayTask();
        }
    }

    void displayCompletedTasks() const
    {
        cout << "\n========== COMPLETED TASKS ==========\n";
        bool found = false;
        int number = 0;
        for (int i = 0; i < (int)tasks.size(); i++)
        {
            if (tasks[i]->getStatus() == "Completed")
            {
                cout << "\n"
                     << (++number) << ".\n";
                tasks[i]->displayTask();
                found = true;
            }
        }
        if (!found)
            cout << "NO COMPLETED TASKS!\n";
    }

    // Now sorted by priority (High -> Medium -> Low), then by nearest deadline
    void displayPendingTasks() const
    {
        vector<Task *> pending;
        for (int i = 0; i < (int)tasks.size(); i++)
            if (tasks[i]->getStatus() == "Pending")
                pending.push_back(tasks[i]);

        cout << "\n========== PENDING TASKS (SORTED BY PRIORITY) ==========\n";
        if (pending.empty())
        {
            cout << "NO PENDING TASKS!\n";
            return;
        }

        sort(pending.begin(), pending.end(), [](Task *a, Task *b)
             {
            int wa = priorityWeight(a->getPriority());
            int wb = priorityWeight(b->getPriority());
            if (wa != wb) return wa < wb;
            return a->getDeadline().toComparable() < b->getDeadline().toComparable(); });

        for (int i = 0; i < (int)pending.size(); i++)
        {
            cout << "\n"
                 << (i + 1) << ".\n";
            pending[i]->displayTask();
        }
    }

    int countPendingTasks() const
    {
        int count = 0;
        for (int i = 0; i < (int)tasks.size(); i++)
            if (tasks[i]->getStatus() == "Pending")
                count++;
        return count;
    }

    int countOverdueTasks() const
    {
        int count = 0;
        for (int i = 0; i < (int)tasks.size(); i++)
            if (tasks[i]->getStatus() == "Pending" &&
                tasks[i]->getDeadline().isPastDate())
                count++;
        return count;
    }

    int countCompletedTasks() const
    {
        int count = 0;
        for (int i = 0; i < (int)tasks.size(); i++)
            if (tasks[i]->getStatus() == "Completed")
                count++;
        return count;
    }

    int countDueSoonTasks(int days) const
    {
        int count = 0;
        Date today = Date::getToday();
        for (int i = 0; i < (int)tasks.size(); i++)
        {
            if (tasks[i]->getStatus() == "Pending")
            {
                long diff = today.daysUntil(tasks[i]->getDeadline());
                if (diff >= 0 && diff <= days)
                    count++;
            }
        }
        return count;
    }

    // ---- Auto-resets completed recurring tasks once their cycle has passed ----
    void processRecurringTasks()
    {
        Date today = Date::getToday();
        int regenerated = 0;

        for (int i = 0; i < (int)tasks.size(); i++)
        {
            if (tasks[i]->getIsRecurring() &&
                tasks[i]->getStatus() == "Completed" &&
                tasks[i]->getDeadline().isPastDate())
            {
                int stepDays = 1;
                if (tasks[i]->getRecurrenceInterval() == "Weekly")
                    stepDays = 7;
                else if (tasks[i]->getRecurrenceInterval() == "Monthly")
                    stepDays = 30;

                Date newDeadline = tasks[i]->getDeadline();
                while (newDeadline.toComparable() <= today.toComparable())
                    newDeadline = newDeadline.addDays(stepDays);

                tasks[i]->setDeadline(newDeadline);
                tasks[i]->markPending();
                regenerated++;
            }
        }

        if (regenerated > 0)
            cout << "\n[RECURRING TASKS] " << regenerated
                 << " recurring task(s) have been reset for their next cycle.\n";
    }

    Task *searchByID(int id)
    {
        for (int i = 0; i < (int)tasks.size(); i++)
            if (tasks[i]->getTaskID() == id)
                return tasks[i];
        return nullptr;
    }

    vector<Task *> searchByTitle(string title)
    {
        vector<Task *> result;
        for (int i = 0; i < (int)tasks.size(); i++)
            if (containsIgnoreCase(tasks[i]->getTitle(), title))
                result.push_back(tasks[i]);
        return result;
    }

    vector<Task *> searchByCategory(string category)
    {
        vector<Task *> result;
        for (int i = 0; i < (int)tasks.size(); i++)
            if (containsIgnoreCase(tasks[i]->getCategory(), category))
                result.push_back(tasks[i]);
        return result;
    }

    vector<Task *> searchByDeadline(Date date)
    {
        vector<Task *> result;
        for (int i = 0; i < (int)tasks.size(); i++)
            if (tasks[i]->getDeadline().isEqual(date))
                result.push_back(tasks[i]);
        return result;
    }

    vector<Task *> searchByStatus(string status)
    {
        vector<Task *> result;
        for (int i = 0; i < (int)tasks.size(); i++)
            if (containsIgnoreCase(tasks[i]->getStatus(), status))
                result.push_back(tasks[i]);
        return result;
    }

    void displaySearchResults(vector<Task *> result)
    {
        if (result.empty())
        {
            cout << "NO TASK FOUND!\n";
            return;
        }
        for (int i = 0; i < (int)result.size(); i++)
            result[i]->displayTask();
    }

    void completeTaskFlow()
    {
        displayPendingTasks();
        if (countPendingTasks() == 0)
            return;
        int id = getValidInt("ENTER TASK ID TO MARK AS COMPLETED: ");
        Task *task = searchByID(id);
        if (task == nullptr)
        {
            throw NotFoundException("TASK NOT FOUND WITH ID: " + to_string(id));
        }
        task->markCompleted();
        cout << "TASK MARKED AS COMPLETED!\n";
    }

    void deleteTaskFlow()
    {
        displayAllTasks();
        if (tasks.empty())
            return;
        int id = getValidInt("ENTER TASK ID TO DELETE: ");
        for (int i = 0; i < (int)tasks.size(); i++)
        {
            if (tasks[i]->getTaskID() == id)
            {
                if (!confirmAction("ARE YOU SURE YOU WANT TO DELETE THIS ITEM?"))
                {
                    cout << "DELETE CANCELLED!\n";
                    return;
                }
                delete tasks[i];
                tasks.erase(tasks.begin() + i);
                cout << "TASK DELETED SUCCESSFULLY!\n";
                return;
            }
        }
        throw NotFoundException("TASK NOT FOUND WITH ID: " + to_string(id));
    }

    // GUI helpers (defined out-of-line near end of file)
    void deleteTaskById(int id);
    void completeTaskById(int id);

    void updateTaskFlow()
    {
        displayAllTasks();
        if (tasks.empty())
            return;
        int id = getValidInt("ENTER TASK ID TO UPDATE: ");
        Task *task = searchByID(id);
        if (task == nullptr)
        {
            cout << "TASK NOT FOUND!\n";
            return;
        }
        cout << "\n========== UPDATE TASK ==========\n";
        cout << "1. Title\n";
        cout << "2. Description\n";
        cout << "3. Deadline\n";
        cout << "4. Time\n";
        cout << "5. Status\n";
        cout << "6. Priority\n";
        cout << "7. Recurring Settings\n";
        cout << "8. Cancel\n";
        int fieldChoice = getValidIntInRange("ENTER CHOICE: ", 1, 8);

        switch (fieldChoice)
        {
        case 1:
            task->setTitle(getNonEmptyLine("ENTER NEW TITLE: "));
            break;
        case 2:
            task->setDescription(getNonEmptyLine("ENTER NEW DESCRIPTION: "));
            break;
        case 3:
        {
            Date newDeadline;
            cout << "ENTER NEW DEADLINE DATE:\n";
            newDeadline.inputDate(true);
            task->setDeadline(newDeadline);
            break;
        }
        case 4:
        {
            Time newTime;
            cout << "ENTER NEW DEADLINE TIME:\n";
            newTime.inputTime();
            task->setDeadlineTime(newTime);
            break;
        }
        case 5:
        {
            cout << "1. Pending\n";
            cout << "2. Completed\n";
            int statusChoice = getValidIntInRange("ENTER CHOICE: ", 1, 2);
            if (statusChoice == 1)
                task->markPending();
            else
                task->markCompleted();
            break;
        }
        case 6:
        {
            cout << "1. High\n2. Medium\n3. Low\n";
            int pChoice = getValidIntInRange("ENTER CHOICE: ", 1, 3);
            string priority = (pChoice == 1) ? "High" : (pChoice == 2) ? "Medium"
                                                                       : "Low";
            task->setPriority(priority);
            break;
        }
        case 7:
        {
            bool recurring = confirmAction("SHOULD THIS TASK BE RECURRING?");
            string interval = "None";
            if (recurring)
            {
                cout << "1. Daily\n2. Weekly\n3. Monthly\n";
                int rChoice = getValidIntInRange("ENTER CHOICE: ", 1, 3);
                interval = (rChoice == 1) ? "Daily" : (rChoice == 2) ? "Weekly"
                                                                     : "Monthly";
            }
            task->setRecurring(recurring, interval);
            break;
        }
        case 8:
            cout << "UPDATE CANCELLED!\n";
            return;
        }
        cout << "TASK UPDATED SUCCESSFULLY!\n";
    }
};

// ============================================================
//                         EXPENSE CLASS
// ============================================================

class Expense
{
private:
    int expenseID;
    static int nextExpenseID;
    string title;
    double amount;
    string category;
    Date date;
    string description;

public:
    Expense(string t, double a, string c, Date d, string desc)
    {
        expenseID = nextExpenseID++;
        title = t;
        amount = a;
        category = c;
        date = d;
        description = desc;
    }

    int getExpenseID() const { return expenseID; }
    string getTitle() const { return title; }
    double getAmount() const { return amount; }
    string getCategory() const { return category; }
    Date getDate() const { return date; }
    string getDescription() const { return description; }

    void setTitle(string t) { title = t; }
    void setAmount(double a) { amount = a; }
    void setCategory(string c) { category = c; }
    void setDate(Date d) { date = d; }
    void setDescription(string d) { description = d; }
    void setExpenseID(int id) { expenseID = id; }
    static void setNextExpenseID(int id) { nextExpenseID = id; }

    void displayExpense() const
    {
        vector<pair<string, string>> fields;
        fields.push_back(make_pair("EXPENSE ID", toDisplayString(expenseID)));
        fields.push_back(make_pair("TITLE", title));
        fields.push_back(make_pair("AMOUNT", formatAmountForDisplay(amount)));
        fields.push_back(make_pair("CATEGORY", category));
        fields.push_back(make_pair("DATE", toDisplayString(date)));
        fields.push_back(make_pair("DESCRIPTION", description));
        displayRecord(fields);
    }
};

int Expense::nextExpenseID = 1;

// ============================================================
//                     EXPENSE MANAGER CLASS
// ============================================================

class ExpenseManager
{
private:
    vector<Expense *> expenses;
    map<string, double> categoryBudgets;

public:
    ~ExpenseManager()
    {
        for (int i = 0; i < (int)expenses.size(); i++)
            delete expenses[i];
    }

    string selectCategory()
    {
        cout << "\n========== EXPENSE CATEGORY ==========\n";
        cout << "1. Food\n";
        cout << "2. Transport\n";
        cout << "3. Education\n";
        cout << "4. Shopping\n";
        cout << "5. Bills\n";
        cout << "6. Entertainment\n";
        cout << "7. Health\n";
        cout << "8. Other\n";
        int choice = getValidIntInRange("ENTER CHOICE: ", 1, 8);

        switch (choice)
        {
        case 1:
            return "Food";
        case 2:
            return "Transport";
        case 3:
            return "Education";
        case 4:
            return "Shopping";
        case 5:
            return "Bills";
        case 6:
            return "Entertainment";
        case 7:
            return "Health";
        default:
            return getNonEmptyLine("ENTER CATEGORY NAME: ");
        }
    }

    Expense *addExpense()
    {
        cout << "\n========== ADD EXPENSE ==========\n";
        string category = selectCategory();
        string title = getNonEmptyLine("ENTER TITLE (e.g. \"KFC Dinner\", \"Uber Ride\"): ");
        double amount = getNonNegativeAmount("ENTER AMOUNT: ");
        Date date;
        cout << "ENTER DATE:\n";
        date.inputDate();
        string description = getNonEmptyLine("ENTER DESCRIPTION: ");
        Expense *expense = new Expense(title, amount, category, date, description);
        expenses.push_back(expense);
        cout << "EXPENSE ADDED SUCCESSFULLY!\n";

        checkBudgetAlert(category);
        return expense;
    }

    void loadExpense(Expense *expense) { expenses.push_back(expense); }

    // ---- Wipes every expense and category budget belonging to this manager.
    // Used when an inactive account is reused during registration, so the
    // "new" account starts exactly like a brand-new one. ----
    void clearAllExpenses()
    {
        for (int i = 0; i < (int)expenses.size(); i++)
            delete expenses[i];
        expenses.clear();
        categoryBudgets.clear();
    }
    const vector<Expense *> &getExpenses() const { return expenses; }

    void displayAllExpenses() const
    {
        if (expenses.empty())
        {
            cout << "NO EXPENSES AVAILABLE!\n";
            return;
        }
        cout << "\n========== ALL EXPENSES ==========\n";
        for (int i = 0; i < (int)expenses.size(); i++)
        {
            cout << "\n"
                 << (i + 1) << ".\n";
            expenses[i]->displayExpense();
        }
    }

    Expense *searchByID(int id)
    {
        for (int i = 0; i < (int)expenses.size(); i++)
            if (expenses[i]->getExpenseID() == id)
                return expenses[i];
        return nullptr;
    }

    vector<Expense *> searchByTitle(string title)
    {
        vector<Expense *> result;
        for (int i = 0; i < (int)expenses.size(); i++)
            if (containsIgnoreCase(expenses[i]->getTitle(), title))
                result.push_back(expenses[i]);
        return result;
    }

    vector<Expense *> searchByCategory(string category)
    {
        vector<Expense *> result;
        for (int i = 0; i < (int)expenses.size(); i++)
            if (containsIgnoreCase(expenses[i]->getCategory(), category))
                result.push_back(expenses[i]);
        return result;
    }

    vector<Expense *> searchByDate(Date date)
    {
        vector<Expense *> result;
        for (int i = 0; i < (int)expenses.size(); i++)
            if (expenses[i]->getDate().isEqual(date))
                result.push_back(expenses[i]);
        return result;
    }

    void displaySearchResults(vector<Expense *> result)
    {
        if (result.empty())
        {
            cout << "NO EXPENSE FOUND!\n";
            return;
        }
        for (int i = 0; i < (int)result.size(); i++)
            result[i]->displayExpense();
    }

    void deleteExpenseFlow()
    {
        displayAllExpenses();
        if (expenses.empty())
            return;
        int id = getValidInt("ENTER EXPENSE ID TO DELETE: ");
        for (int i = 0; i < (int)expenses.size(); i++)
        {
            if (expenses[i]->getExpenseID() == id)
            {
                if (!confirmAction("ARE YOU SURE YOU WANT TO DELETE THIS ITEM?"))
                {
                    cout << "DELETE CANCELLED!\n";
                    return;
                }
                delete expenses[i];
                expenses.erase(expenses.begin() + i);
                cout << "EXPENSE DELETED SUCCESSFULLY!\n";
                return;
            }
        }
        throw NotFoundException("EXPENSE NOT FOUND WITH ID: " + to_string(id));
    }

    // GUI helper (defined out-of-line near end of file)
    void deleteExpenseById(int id);

    void updateExpenseFlow()
    {
        displayAllExpenses();
        if (expenses.empty())
            return;
        int id = getValidInt("ENTER EXPENSE ID TO UPDATE: ");
        Expense *expense = searchByID(id);
        if (expense == nullptr)
        {
            cout << "EXPENSE NOT FOUND!\n";
            return;
        }
        cout << "\n========== UPDATE EXPENSE ==========\n";
        cout << "1. Title\n";
        cout << "2. Amount\n";
        cout << "3. Category\n";
        cout << "4. Date\n";
        cout << "5. Description\n";
        cout << "6. Cancel\n";
        int fieldChoice = getValidIntInRange("ENTER CHOICE: ", 1, 6);

        switch (fieldChoice)
        {
        case 1:
            expense->setTitle(getNonEmptyLine("ENTER NEW TITLE: "));
            break;
        case 2:
            expense->setAmount(getNonNegativeAmount("ENTER NEW AMOUNT: "));
            break;
        case 3:
            expense->setCategory(selectCategory());
            break;
        case 4:
        {
            Date newDate;
            cout << "ENTER NEW DATE:\n";
            newDate.inputDate();
            expense->setDate(newDate);
            break;
        }
        case 5:
            expense->setDescription(getNonEmptyLine("ENTER NEW DESCRIPTION: "));
            break;
        case 6:
            cout << "UPDATE CANCELLED!\n";
            return;
        }
        cout << "EXPENSE UPDATED SUCCESSFULLY!\n";
    }

    double calculateTotalExpense()
    {
        double total = 0;
        for (int i = 0; i < (int)expenses.size(); i++)
            total += expenses[i]->getAmount();
        return total;
    }

    pair<string, double> getTopCategoryInfo() const
    {
        if (expenses.empty())
            return make_pair("", 0.0);

        vector<string> categories;
        vector<double> totals;
        for (int i = 0; i < (int)expenses.size(); i++)
        {
            string cat = expenses[i]->getCategory();
            double amt = expenses[i]->getAmount();
            bool found = false;
            for (int j = 0; j < (int)categories.size(); j++)
            {
                if (categories[j] == cat)
                {
                    totals[j] += amt;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                categories.push_back(cat);
                totals.push_back(amt);
            }
        }
        int bestIndex = 0;
        for (int i = 1; i < (int)totals.size(); i++)
            if (totals[i] > totals[bestIndex])
                bestIndex = i;
        return make_pair(categories[bestIndex], totals[bestIndex]);
    }

    // ---- BUDGETS ----
    void setBudget()
    {
        cout << "\n========== SET CATEGORY BUDGET ==========\n";
        string category = selectCategory();
        double limit = getNonNegativeAmount("ENTER MONTHLY BUDGET LIMIT FOR THIS CATEGORY: ");
        categoryBudgets[category] = limit;
        cout << "BUDGET SET FOR " << category << ": " << limit << endl;
    }

    // ---- GUI helper: sets/updates a category budget directly, without any
    // console I/O. Caller (RevolifController) is responsible for validating
    // that category is non-empty and limit is >= 0 before calling this.
    // Like the console version, budgets are kept in memory only for the
    // current session and are not persisted to disk. ----
    void setBudgetGUI(const string &category, double limit)
    {
        categoryBudgets[category] = limit;
    }

    // ---- GUI helper: read-only access to the category -> limit map, so the
    // GUI can list every budget the user has set, alongside how much has
    // been spent in each category (via getCategoryTotal). ----
    const map<string, double> &getCategoryBudgets() const { return categoryBudgets; }

    double getCategoryTotal(string category) const
    {
        double total = 0;
        for (int i = 0; i < (int)expenses.size(); i++)
            if (expenses[i]->getCategory() == category)
                total += expenses[i]->getAmount();
        return total;
    }

    void checkBudgetAlert(string category)
    {
        if (categoryBudgets.find(category) == categoryBudgets.end())
            return;

        double limit = categoryBudgets[category];
        double spent = getCategoryTotal(category);

        if (spent > limit)
            cout << "\n[BUDGET ALERT] You have EXCEEDED your " << category
                 << " budget! (Spent: " << spent << " / Limit: " << limit << ")\n";
        else if (limit > 0 && spent > limit * 0.8)
            cout << "\n[BUDGET WARNING] You are close to your " << category
                 << " budget! (Spent: " << spent << " / Limit: " << limit << ")\n";
    }

    void viewBudgets() const
    {
        if (categoryBudgets.empty())
        {
            cout << "NO BUDGETS SET YET!\n";
            return;
        }
        cout << "\n========== BUDGET OVERVIEW ==========\n";
        for (map<string, double>::const_iterator it = categoryBudgets.begin();
             it != categoryBudgets.end(); ++it)
        {
            double spent = getCategoryTotal(it->first);
            cout << left << setw(15) << it->first
                 << " Spent: " << setw(10) << spent
                 << " Limit: " << it->second;
            if (spent > it->second)
                cout << "  [OVER BUDGET]";
            cout << endl;
        }
        cout << "======================================\n";
    }

    // ---- SPENDING CHART (bar chart by category, sorted, with % share) ----
    void displaySpendingChart() const
    {
        if (expenses.empty())
        {
            cout << "NO EXPENSES TO DISPLAY!\n";
            return;
        }

        vector<string> categories;
        vector<double> totals;
        for (int i = 0; i < (int)expenses.size(); i++)
        {
            string cat = expenses[i]->getCategory();
            double amt = expenses[i]->getAmount();
            bool found = false;
            for (int j = 0; j < (int)categories.size(); j++)
            {
                if (categories[j] == cat)
                {
                    totals[j] += amt;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                categories.push_back(cat);
                totals.push_back(amt);
            }
        }

        // Sort categories by spend, highest first
        vector<int> order((int)categories.size());
        for (int i = 0; i < (int)order.size(); i++)
            order[i] = i;
        sort(order.begin(), order.end(), [&](int a, int b)
             { return totals[a] > totals[b]; });

        double maxVal = 0, grandTotal = 0;
        for (int i = 0; i < (int)totals.size(); i++)
        {
            if (totals[i] > maxVal)
                maxVal = totals[i];
            grandTotal += totals[i];
        }

        const int labelWidth = 14;
        const int maxBarLength = 25;
        const int barColWidth = maxBarLength + 2; // includes the [ and ] brackets
        const int amountColWidth = 12;
        const int percentColWidth = 9; // includes the trailing '%'
        const int totalWidth = labelWidth + 1 + barColWidth + amountColWidth + percentColWidth;

        cout << "\n" << makeTitleBar("SPENDING BY CATEGORY", totalWidth) << "\n";
        cout << left << setw(labelWidth) << "CATEGORY" << " " << left << setw(barColWidth) << "SHARE"
             << right << setw(amountColWidth) << "AMOUNT" << setw(percentColWidth) << "PERCENT" << "\n";
        cout << makeDivider(totalWidth) << "\n";

        for (int k = 0; k < (int)order.size(); k++)
        {
            int i = order[k];
            int barLength = (maxVal > 0) ? (int)((totals[i] / maxVal) * maxBarLength + 0.5) : 0;
            double percent = (grandTotal > 0) ? (totals[i] / grandTotal) * 100.0 : 0.0;

            string colorCode;
            if (percent <= 50.0)
                colorCode = "\033[32m"; // green
            else if (percent <= 80.0)
                colorCode = "\033[33m"; // yellow
            else
                colorCode = "\033[31m"; // red

            string filledBlocks;
            for (int b = 0; b < barLength; b++)
                filledBlocks += "\u2588";

            string bar = "[" + colorCode + filledBlocks + "\033[0m" + string(maxBarLength - barLength, '-') + "]";

            cout << left << setw(labelWidth) << truncateForColumn(categories[i], labelWidth)
                 << " " << bar
                 << right << setw(amountColWidth) << fixed << setprecision(2) << totals[i]
                 << setw(percentColWidth - 1) << fixed << setprecision(1) << percent << "%"
                 << "\n";
        }

        cout << makeDivider(totalWidth) << "\n";
        cout << left << setw(labelWidth) << "TOTAL" << " " << setw(barColWidth) << ""
             << right << setw(amountColWidth) << fixed << setprecision(2) << grandTotal
             << setw(percentColWidth) << "100.0%\n";
        cout << makeDivider(totalWidth, '=') << "\n";
    }
};

// ============================================================
//                    ACTIVITY LOG CLASS
// ============================================================
// Lightweight, append-only record of "things the user did" -- used only
// to power the GUI dashboard's recent-activity timeline and its
// GitHub-style contribution calendar. Deliberately minimal: no update/
// delete flow, no console menu, since nothing in the app needs to edit
// history after the fact.

class ActivityLog
{
private:
    int activityID;
    static int nextActivityID;
    string type;     // "task_added" | "task_completed" | "goal_added" | "goal_completed" | "expense_added"
    string title;
    string category;
    Date date;
    Time time;

public:
    ActivityLog(string ty, string ti, string cat, Date d, Time t)
    {
        activityID = nextActivityID++;
        type = ty;
        title = ti;
        category = cat;
        date = d;
        time = t;
    }

    int getActivityID() const { return activityID; }
    string getType() const { return type; }
    string getTitle() const { return title; }
    string getCategory() const { return category; }
    Date getDate() const { return date; }
    Time getTime() const { return time; }

    void setActivityID(int id) { activityID = id; }
    static void setNextActivityID(int id) { nextActivityID = id; }
};

int ActivityLog::nextActivityID = 1;

// ============================================================
//                 ACTIVITY LOG MANAGER CLASS
// ============================================================

class ActivityLogManager
{
private:
    vector<ActivityLog *> entries;

public:
    ~ActivityLogManager()
    {
        for (int i = 0; i < (int)entries.size(); i++)
            delete entries[i];
    }

    void loadEntry(ActivityLog *entry) { entries.push_back(entry); }

    ActivityLog *addEntry(string type, string title, string category, Date date, Time time)
    {
        ActivityLog *entry = new ActivityLog(type, title, category, date, time);
        entries.push_back(entry);
        return entry;
    }

    const vector<ActivityLog *> &getEntries() const { return entries; }

    // ---- Wipes every logged entry. Used when an inactive account is
    // reused during registration, so the "new" account starts clean. ----
    void clearAllEntries()
    {
        for (int i = 0; i < (int)entries.size(); i++)
            delete entries[i];
        entries.clear();
    }

    // ---- How many logged activities fall on a given calendar date --
    // used to shade a single cell of the contribution calendar. ----
    int countOnDate(Date d) const
    {
        int count = 0;
        for (int i = 0; i < (int)entries.size(); i++)
            if (entries[i]->getDate().isEqual(d))
                count++;
        return count;
    }

    // ---- Every entry that falls within the given month, oldest first --
    // used to build the whole contribution calendar in one pass. ----
    vector<ActivityLog *> getEntriesInMonth(int month, int year) const
    {
        vector<ActivityLog *> result;
        for (int i = 0; i < (int)entries.size(); i++)
            if (entries[i]->getDate().getMonth() == month && entries[i]->getDate().getYear() == year)
                result.push_back(entries[i]);
        return result;
    }

    // ---- Most recent N entries, newest first -- feeds the recent-
    // activity timeline. ----
    vector<ActivityLog *> getRecent(int n) const
    {
        vector<ActivityLog *> sorted = entries;
        std::sort(sorted.begin(), sorted.end(), [](ActivityLog *a, ActivityLog *b) {
            long da = a->getDate().toComparable();
            long db = b->getDate().toComparable();
            if (da != db)
                return da > db;
            return a->getActivityID() > b->getActivityID();
        });
        if ((int)sorted.size() > n)
            sorted.resize(n);
        return sorted;
    }
};

// ============================================================
//                         GOAL CLASS
// ============================================================

class Goal
{
private:
    int goalID;
    static int nextGoalID;
    string title;
    string description;
    string category;
    Date deadline;
    string status;

public:
    Goal(string t, string desc, string c, Date d)
    {
        goalID = nextGoalID++;
        title = t;
        description = desc;
        category = c;
        deadline = d;
        status = "Incomplete";
    }

    int getGoalID() const { return goalID; }
    string getTitle() const { return title; }
    string getDescription() const { return description; }
    string getCategory() const { return category; }
    Date getDeadline() const { return deadline; }
    string getStatus() const { return status; }

    string getDisplayStatus() const
    {
        if (status == "Completed")
            return "Completed";
        if (deadline.isPastDate())
            return "Overdue";
        return "Pending";
    }

    void setTitle(string t) { title = t; }
    void setDescription(string d) { description = d; }
    void setCategory(string c) { category = c; }
    void setDeadline(Date d) { deadline = d; }
    void setGoalID(int id) { goalID = id; }
    static void setNextGoalID(int id) { nextGoalID = id; }

    void completeGoal() { status = "Completed"; }

    void displayGoal() const
    {
        vector<pair<string, string>> fields;
        fields.push_back(make_pair("GOAL ID", toDisplayString(goalID)));
        fields.push_back(make_pair("TITLE", title));
        fields.push_back(make_pair("CATEGORY", category));
        fields.push_back(make_pair("DEADLINE", toDisplayString(deadline)));
        fields.push_back(make_pair("STATUS", getDisplayStatus()));
        fields.push_back(make_pair("DESCRIPTION", description));
        displayRecord(fields);
    }
};

int Goal::nextGoalID = 1;

// ============================================================
//                      GOAL MANAGER CLASS
// ============================================================

class GoalManager
{
private:
    vector<Goal *> goals;
    vector<string> badges;

public:
    ~GoalManager()
    {
        for (int i = 0; i < (int)goals.size(); i++)
            delete goals[i];
    }

    string selectCategory()
    {
        cout << "\n========== GOAL CATEGORY ==========\n";
        cout << "1. Academic\n";
        cout << "2. Career\n";
        cout << "3. Health\n";
        cout << "4. Personal Development\n";
        cout << "5. Financial\n";
        cout << "6. Other\n";
        int choice = getValidIntInRange("ENTER CHOICE: ", 1, 6);

        switch (choice)
        {
        case 1:
            return "Academic";
        case 2:
            return "Career";
        case 3:
            return "Health";
        case 4:
            return "Personal Development";
        case 5:
            return "Financial";
        default:
            return getNonEmptyLine("ENTER CATEGORY NAME: ");
        }
    }

    Goal *addGoal()
    {
        cout << "\n========== ADD GOAL ==========\n";
        string title = getNonEmptyLine("ENTER GOAL TITLE: ");
        string category = selectCategory();
        Date deadline;
        cout << "ENTER DEADLINE:\n";
        deadline.inputDate(true);
        string description = getNonEmptyLine("ENTER DESCRIPTION: ");
        Goal *goal = new Goal(title, description, category, deadline);
        goals.push_back(goal);
        cout << "GOAL ADDED SUCCESSFULLY!\n";
        return goal;
    }

    void loadGoal(Goal *goal) { goals.push_back(goal); }

    // ---- Wipes every goal and unlocked badge belonging to this manager.
    // Used when an inactive account is reused during registration, so the
    // "new" account starts exactly like a brand-new one. ----
    void clearAllGoals()
    {
        for (int i = 0; i < (int)goals.size(); i++)
            delete goals[i];
        goals.clear();
        badges.clear();
    }
    const vector<Goal *> &getGoals() const { return goals; }

    void displayAllGoals() const
    {
        if (goals.empty())
        {
            cout << "NO GOALS AVAILABLE!\n";
            return;
        }
        cout << "\n========== ALL GOALS ==========\n";
        for (int i = 0; i < (int)goals.size(); i++)
        {
            cout << "\n"
                 << (i + 1) << ".\n";
            goals[i]->displayGoal();
        }
    }

    void displayCompletedGoals() const
    {
        bool found = false;
        int number = 0;
        cout << "\n========== COMPLETED GOALS ==========\n";
        for (int i = 0; i < (int)goals.size(); i++)
        {
            if (goals[i]->getStatus() == "Completed")
            {
                cout << "\n"
                     << (++number) << ".\n";
                goals[i]->displayGoal();
                found = true;
            }
        }
        if (!found)
            cout << "NO COMPLETED GOALS!\n";
    }

    void displayIncompleteGoals() const
    {
        bool found = false;
        int number = 0;
        cout << "\n========== PENDING / OVERDUE GOALS ==========\n";
        for (int i = 0; i < (int)goals.size(); i++)
        {
            if (goals[i]->getStatus() != "Completed")
            {
                cout << "\n"
                     << (++number) << ".\n";
                goals[i]->displayGoal();
                found = true;
            }
        }
        if (!found)
            cout << "NO PENDING GOALS!\n";
    }

    Goal *searchByID(int id)
    {
        for (int i = 0; i < (int)goals.size(); i++)
            if (goals[i]->getGoalID() == id)
                return goals[i];
        return nullptr;
    }

    vector<Goal *> searchByTitle(string title)
    {
        vector<Goal *> result;
        for (int i = 0; i < (int)goals.size(); i++)
            if (containsIgnoreCase(goals[i]->getTitle(), title))
                result.push_back(goals[i]);
        return result;
    }

    vector<Goal *> searchByCategory(string category)
    {
        vector<Goal *> result;
        for (int i = 0; i < (int)goals.size(); i++)
            if (containsIgnoreCase(goals[i]->getCategory(), category))
                result.push_back(goals[i]);
        return result;
    }

    vector<Goal *> searchByDeadline(Date date)
    {
        vector<Goal *> result;
        for (int i = 0; i < (int)goals.size(); i++)
            if (goals[i]->getDeadline().isEqual(date))
                result.push_back(goals[i]);
        return result;
    }

    vector<Goal *> searchByStatus(string status)
    {
        vector<Goal *> result;
        for (int i = 0; i < (int)goals.size(); i++)
            if (containsIgnoreCase(goals[i]->getStatus(), status))
                result.push_back(goals[i]);
        return result;
    }

    void displaySearchResults(vector<Goal *> result)
    {
        if (result.empty())
        {
            cout << "NO GOAL FOUND!\n";
            return;
        }
        for (int i = 0; i < (int)result.size(); i++)
            result[i]->displayGoal();
    }

    int countCompletedGoals() const
    {
        int count = 0;
        for (int i = 0; i < (int)goals.size(); i++)
            if (goals[i]->getStatus() == "Completed")
                count++;
        return count;
    }

    int countPendingGoals() const
    {
        int count = 0;
        for (int i = 0; i < (int)goals.size(); i++)
            if (goals[i]->getStatus() != "Completed" &&
                !goals[i]->getDeadline().isPastDate())
                count++;
        return count;
    }

    int countOverdueGoals() const
    {
        int count = 0;
        for (int i = 0; i < (int)goals.size(); i++)
            if (goals[i]->getStatus() != "Completed" &&
                goals[i]->getDeadline().isPastDate())
                count++;
        return count;
    }

    int goalsUntilNextTitle() const
    {
        int count = countCompletedGoals();
        static const int milestoneCounts[] = {5, 10, 20, 30, 40, 50};
        for (int i = 0; i < 6; i++)
            if (count < milestoneCounts[i])
                return milestoneCounts[i] - count;
        return 0;
    }

    string getNextTitleName() const
    {
        int count = countCompletedGoals();
        static const int milestoneCounts[] = {5, 10, 20, 30, 40, 50};
        static const string milestoneTitles[] = {
            "Goal Beginner", "Goal Enthusiast", "Goal Achiever",
            "Goal Master", "Goal Champion", "Goal Legend"};
        for (int i = 0; i < 6; i++)
            if (count < milestoneCounts[i])
                return milestoneTitles[i];
        return "";
    }

    bool completeGoal(int id)
    {
        Goal *goal = searchByID(id);
        if (goal == nullptr)
        {
            throw NotFoundException("GOAL NOT FOUND WITH ID: " + to_string(id));
        }
        if (goal->getStatus() == "Completed")
        {
            throw ValidationException("GOAL ALREADY COMPLETED!");
        }
        goal->completeGoal();
        updateBadges();
        cout << "GOAL COMPLETED SUCCESSFULLY!\n";
        return true;
    }

    bool completeGoalFlow()
    {
        displayIncompleteGoals();
        if (goals.empty())
            return false;
        int id = getValidInt("ENTER GOAL ID TO MARK AS COMPLETED: ");
        cin.ignore(1000, '\n');
        return completeGoal(id);
    }

    void updateBadges()
    {
        int count = countCompletedGoals();
        static const int milestoneCounts[] = {5, 10, 20, 30, 40, 50};
        static const string milestoneTitles[] = {
            "Goal Beginner", "Goal Enthusiast", "Goal Achiever",
            "Goal Master", "Goal Champion", "Goal Legend"};
        int totalMilestones = 6;
        // Backfill every milestone the user actually qualifies for, not just
        // the next single tier. Previously this only awarded one badge per
        // call, so a user who jumped several tiers at once (e.g. goals loaded
        // from a save file) would get stuck on an early title like
        // "Goal Enthusiast" until they ground out enough additional goals to
        // catch up one tier at a time.
        while ((int)badges.size() < totalMilestones &&
               count >= milestoneCounts[badges.size()])
        {
            badges.push_back(milestoneTitles[badges.size()]);
        }
    }

    string getLatestTitle() const
    {
        if (badges.empty())
            return "";
        return badges.back();
    }

    void viewAchievements() const
    {
        cout << "\n========== ACHIEVEMENTS ==========\n";
        cout << "GOALS COMPLETED : " << countCompletedGoals() << endl;
        if (badges.empty())
        {
            cout << "NO TITLES EARNED YET! COMPLETE 5 GOALS TO EARN YOUR FIRST TITLE.\n";
            return;
        }
        for (int i = 0; i < (int)badges.size(); i++)
            cout << "[TITLE UNLOCKED] " << badges[i] << endl;
    }

    void updateGoalFlow()
    {
        displayAllGoals();
        if (goals.empty())
            return;
        int id = getValidInt("ENTER GOAL ID TO UPDATE: ");
        Goal *goal = searchByID(id);
        if (goal == nullptr)
        {
            cout << "GOAL NOT FOUND!\n";
            return;
        }
        cout << "\n========== UPDATE GOAL ==========\n";
        cout << "1. Title\n";
        cout << "2. Description\n";
        cout << "3. Category\n";
        cout << "4. Deadline\n";
        cout << "5. Cancel\n";
        int fieldChoice = getValidIntInRange("ENTER CHOICE: ", 1, 5);

        switch (fieldChoice)
        {
        case 1:
            goal->setTitle(getNonEmptyLine("ENTER NEW TITLE: "));
            break;
        case 2:
            goal->setDescription(getNonEmptyLine("ENTER NEW DESCRIPTION: "));
            break;
        case 3:
            goal->setCategory(selectCategory());
            break;
        case 4:
        {
            Date newDeadline;
            cout << "ENTER NEW DEADLINE:\n";
            newDeadline.inputDate(true);
            goal->setDeadline(newDeadline);
            break;
        }
        case 5:
            cout << "UPDATE CANCELLED!\n";
            return;
        }
        cout << "GOAL UPDATED SUCCESSFULLY!\n";
    }

    void deleteGoalFlow()
    {
        displayAllGoals();
        if (goals.empty())
            return;
        int id = getValidInt("ENTER GOAL ID TO DELETE: ");
        for (int i = 0; i < (int)goals.size(); i++)
        {
            if (goals[i]->getGoalID() == id)
            {
                if (!confirmAction("ARE YOU SURE YOU WANT TO DELETE THIS ITEM?"))
                {
                    cout << "DELETE CANCELLED!\n";
                    return;
                }
                delete goals[i];
                goals.erase(goals.begin() + i);
                cout << "GOAL DELETED SUCCESSFULLY!\n";
                return;
            }
        }
        cout << "GOAL NOT FOUND!\n";
    }

    // GUI helpers (defined out-of-line near end of file)
    void deleteGoalById(int id);
    void completeGoalById(int id);
};

// ============================================================
//                     ACHIEVEMENT CLASS
// ============================================================

class Achievement
{
private:
    int achievementID;
    static int nextAchievementID;
    string name;
    string description;
    int requiredGoals;
    bool isDefault;

public:
    Achievement(string n, string desc, int reqGoals, bool def = false)
    {
        achievementID = nextAchievementID++;
        name = n;
        description = desc;
        requiredGoals = reqGoals;
        isDefault = def;
    }

    int getAchievementID() const { return achievementID; }
    string getName() const { return name; }
    string getDescription() const { return description; }
    int getRequiredGoals() const { return requiredGoals; }
    bool getIsDefault() const { return isDefault; }

    void setName(string n) { name = n; }
    void setDescription(string d) { description = d; }
    void setRequiredGoals(int req) { requiredGoals = req; }
    void setAchievementID(int id) { achievementID = id; }
    static void setNextAchievementID(int id) { nextAchievementID = id; }

    void displayAchievement() const
    {
        vector<pair<string, string>> fields;
        fields.push_back(make_pair("ACHIEVEMENT ID", toDisplayString(achievementID)));
        fields.push_back(make_pair("NAME", name));
        fields.push_back(make_pair("DESCRIPTION", description));
        fields.push_back(make_pair("REQUIREMENT", "Complete " + toDisplayString(requiredGoals) + " Goals"));
        fields.push_back(make_pair("TYPE", isDefault ? "Default (Protected)" : "Custom"));
        displayRecord(fields);
    }

    void displayBrief() const
    {
        cout << "[" << achievementID << "] " << name
             << " (Complete " << requiredGoals << " Goals)";
        if (isDefault)
            cout << " [DEFAULT]";
        cout << endl;
    }
};

int Achievement::nextAchievementID = 1;

// ============================================================
//                      ADMIN CLASS
// ============================================================

class Admin
{
private:
    string username;
    string password; // stored hashed

public:
    Admin(string u = "admin", string p = "admin123")
        : username(u), password(simpleHash(p)) {}

    string getUsername() const { return username; }

    // ---- Password hash accessors (used exclusively by the Authentication class) ----
    string getPasswordHash() const { return password; }
    void setPasswordHash(const string &hash) { password = hash; }
};

// Forward declaration of System (needed for User class)
class System;

// ============================================================
//                     USER CLASS (UPDATED)
// ============================================================

class User
{
private:
    string name;
    string username;
    int UID;
    static int next_id;
    Date DOB;
    string password; // stored hashed
    string email;
    Date registrationDate;

    string title;

    bool isActive;
    bool deactivatedBySelf; // true only when the USER deleted their own account
    Date lastLogin;

    int currentStreak;
    int bestStreak;

    vector<int> unlockedAchievementIDs;
    int displayedAchievementID;

    // ---- Email system state: whether the one-time Welcome Email has been
    // sent, and the last calendar date a Daily Summary Email went out (so
    // at most one summary is ever sent per day, no matter how many times
    // the user logs in). lastSummaryEmailDate defaults to the same
    // "never" sentinel (2000-01-01) used elsewhere in this file. ----
    bool welcomeEmailSent;
    Date lastSummaryEmailDate;

    TaskManager taskManager;
    ExpenseManager expenseManager;
    GoalManager goalManager;
    ActivityLogManager activityLogManager;

    friend class System;

public:
    User()
        : name("NONE"),
          username(""),
          UID(next_id++),
          password(""),
          email(""),
          title(""),
          isActive(true),
          deactivatedBySelf(false),
          currentStreak(0),
          bestStreak(0),
          displayedAchievementID(-1),
          welcomeEmailSent(false)
    {
        registrationDate = Date::getToday();
        lastLogin = Date();
        lastSummaryEmailDate = Date();
    }

    User(string n, string uname, Date dob, string pass, string em = "")
        : name(n),
          username(uname),
          UID(next_id++),
          DOB(dob),
          password(simpleHash(pass)),
          email(em),
          title(""),
          isActive(true),
          deactivatedBySelf(false),
          currentStreak(0),
          bestStreak(0),
          displayedAchievementID(-1),
          welcomeEmailSent(false)
    {
        registrationDate = Date::getToday();
        lastLogin = Date();
        lastSummaryEmailDate = Date();
    }

    string getName() const { return name; }
    string getUsername() const { return username; }
    int getUID() const { return UID; }
    Date getDOB() const { return DOB; }
    string getEmail() const { return email; }
    Date getRegistrationDate() const { return registrationDate; }
    string getTitle() const { return title; }
    bool getIsActive() const { return isActive; }
    bool getDeactivatedBySelf() const { return deactivatedBySelf; }
    Date getLastLogin() const { return lastLogin; }
    int getDisplayedAchievementID() const { return displayedAchievementID; }
    vector<int> getUnlockedAchievementIDs() const { return unlockedAchievementIDs; }
    int getCurrentStreak() const { return currentStreak; }
    int getBestStreak() const { return bestStreak; }
    bool getWelcomeEmailSent() const { return welcomeEmailSent; }
    Date getLastSummaryEmailDate() const { return lastSummaryEmailDate; }

    void setWelcomeEmailSent(bool sent) { welcomeEmailSent = sent; }
    void setLastSummaryEmailDate(Date d) { lastSummaryEmailDate = d; }

    void setName(string n) { name = n; }
    void setEmail(string e) { email = e; }
    void setDOB(Date d) { DOB = d; }
    void setRegistrationDate(Date d) { registrationDate = d; }
    void setActive(bool active) { isActive = active; }
    void setDeactivatedBySelf(bool bySelf) { deactivatedBySelf = bySelf; }
    void setLastLogin(Date d) { lastLogin = d; }
    void setCurrentStreak(int s) { currentStreak = s; }
    void setBestStreak(int s) { bestStreak = s; }
    void setTitle(string t) { title = t; }

    void setDisplayedAchievementID(int id) { displayedAchievementID = id; }

    // ---- Records a login and updates the daily streak ----
    void recordLogin()
    {
        Date today = Date::getToday();

        bool neverLoggedInBefore = (lastLogin.toComparable() == 20000101);

        if (neverLoggedInBefore)
        {
            currentStreak = 1;
        }
        else
        {
            long diff = lastLogin.daysUntil(today);
            if (diff == 1)
                currentStreak++;
            else if (diff > 1)
                currentStreak = 1;
            // diff == 0 (same-day re-login) leaves streak unchanged
        }

        if (currentStreak > bestStreak)
            bestStreak = currentStreak;

        lastLogin = today;
    }

    void addUnlockedAchievement(int achID)
    {
        for (int i = 0; i < (int)unlockedAchievementIDs.size(); i++)
            if (unlockedAchievementIDs[i] == achID)
                return;
        unlockedAchievementIDs.push_back(achID);
    }

    // ---- Wipes every unlocked achievement. Used when an inactive account is
    // reused during registration, so the "new" account starts exactly like a
    // brand-new one. ----
    void clearUnlockedAchievements()
    {
        unlockedAchievementIDs.clear();
    }

    bool hasAchievement(int achID) const
    {
        for (int i = 0; i < (int)unlockedAchievementIDs.size(); i++)
            if (unlockedAchievementIDs[i] == achID)
                return true;
        return false;
    }

    int getUnlockedCount() const
    {
        return unlockedAchievementIDs.size();
    }

    string getDisplayName() const
    {
        if (title.empty())
            return name;
        return name + " (" + title + ")";
    }

    void setUID(int id) { UID = id; }
    static void setNextID(int id) { next_id = id; }

    TaskManager &getTaskManager() { return taskManager; }
    ExpenseManager &getExpenseManager() { return expenseManager; }
    GoalManager &getGoalManager() { return goalManager; }
    ActivityLogManager &getActivityLogManager() { return activityLogManager; }

    // ---- Password hash accessors (used exclusively by the Authentication class) ----
    string getPasswordHash() const { return password; }
    void setPasswordHash(const string &hash) { password = hash; }

    // ---- Uses the same wrapText/printWrappedField boxed layout as
    // displayTask/displayGoal/displayExpense/displayAchievement, so a long
    // email or display name wraps instead of overflowing the line. ----
    void displayProfile() const
    {
        string lastLoginStr = (lastLogin.toComparable() == 20000101) ? "Never" : toDisplayString(lastLogin);

        cout << "\n================ USER PROFILE ================\n";
        vector<pair<string, string>> fields;
        fields.push_back(make_pair("UID", toDisplayString(UID)));
        fields.push_back(make_pair("USERNAME", username));
        fields.push_back(make_pair("NAME", getDisplayName()));
        fields.push_back(make_pair("EMAIL", email.empty() ? "N/A" : email));
        fields.push_back(make_pair("DOB", toDisplayString(DOB)));
        fields.push_back(make_pair("REGISTRATION DATE", toDisplayString(registrationDate)));
        fields.push_back(make_pair("LAST LOGIN", lastLoginStr));
        fields.push_back(make_pair("ACCOUNT STATUS", isActive ? "Active" : "Inactive"));
        fields.push_back(make_pair("ACHIEVEMENTS", toDisplayString((int)unlockedAchievementIDs.size()) + " unlocked"));
        fields.push_back(make_pair("LOGIN STREAK", toDisplayString(currentStreak) + " day(s) (Best: " + toDisplayString(bestStreak) + ")"));

        displayRecord(fields, RECORD_BOX_WIDTH, 18); // 18 fits "REGISTRATION DATE"
    }

    void showWelcome(const vector<Achievement> &achievements) const
    {
        cout << "\n========== WELCOME " << name << " ==========\n";

        if (displayedAchievementID != -1)
        {
            string achName = "Unknown Achievement";
            for (int i = 0; i < (int)achievements.size(); i++)
            {
                if (achievements[i].getAchievementID() == displayedAchievementID)
                {
                    achName = achievements[i].getName();
                    break;
                }
            }
            cout << "\nFeatured Achievement:\n";
            cout << "   " << char(4) << " " << achName << "\n";
        }
        else
        {
            cout << "\nNo featured achievement selected.\n";
            cout << "Complete goals and select an achievement to display.\n";
        }

        cout << "\nGoals Completed: " << goalManager.countCompletedGoals() << endl;
        cout << "Tasks Pending: " << taskManager.countPendingTasks() << endl;
        cout << "Login Streak: " << currentStreak << " day(s) (Best: " << bestStreak << ")\n";
        cout << "==============================================\n";
    }

    void checkAndUpdateTitle()
    {
        string latest = goalManager.getLatestTitle();
        if (!latest.empty() && latest != title)
        {
            title = latest;
            cout << "\n*******************************************\n";
            cout << "  CONGRATULATIONS! NEW TITLE UNLOCKED: " << title << "\n";
            cout << "*******************************************\n";
        }
    }
};

int User::next_id = 1;

// ============================================================
//                      EMAIL MANAGER
// ============================================================
// Sends the two REVOLIF transactional emails (Welcome + Daily Summary)
// over Gmail SMTP using implicit TLS (SMTPS, port 465), via a direct
// OpenSSL BIO connection -- no external mail library required beyond
// libssl/libcrypto. All email activity (successes and failures) is
// appended to email_log.txt. A failure here is always swallowed by the
// caller: it must never stop the application from opening the dashboard.
// ============================================================

class EmailManager
{
private:
    static const string SMTP_HOST;
    static const int SMTP_PORT = 465;
    static const string SENDER_EMAIL;
    static const string SENDER_NAME;
    static const string CREDENTIALS_FILE;
    static const string LOG_FILE;

    // ---- Base64 encoding (RFC 4648), needed for AUTH LOGIN and for
    // RFC 2047 UTF-8 subject encoding. No external dependency. ----
    static string base64Encode(const string &input)
    {
        static const char table[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        string out;
        int val = 0, bits = -6;
        for (unsigned char c : input)
        {
            val = (val << 8) + c;
            bits += 8;
            while (bits >= 0)
            {
                out.push_back(table[(val >> bits) & 0x3F]);
                bits -= 6;
            }
        }
        if (bits > -6)
            out.push_back(table[((val << 8) >> (bits + 8)) & 0x3F]);
        while (out.size() % 4)
            out.push_back('=');
        return out;
    }

    // ---- Encodes a UTF-8 header value (e.g. the Subject line, or a
    // display name containing non-ASCII characters) per RFC 2047, so any
    // Unicode text survives correctly in the raw SMTP DATA headers. ----
    static string encodeHeaderUTF8(const string &value)
    {
        return "=?UTF-8?B?" + base64Encode(value) + "?=";
    }

    // ---- Minimal HTML-escaping for user-supplied strings (names, titles,
    // task/goal text) so they can never break out of the HTML markup. ----
    static string htmlEscape(const string &s)
    {
        string out;
        out.reserve(s.size());
        for (char c : s)
        {
            switch (c)
            {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            default:
                out += c;
            }
        }
        return out;
    }

    // ---- Appends a single timestamped line to email_log.txt. Logging
    // itself is best-effort and must never throw. ----
    static void logEvent(const string &line)
    {
        try
        {
            ofstream log(LOG_FILE, ios::app);
            if (!log)
                return;
            Date today = Date::getToday();
            log << "[" << toDisplayString(today) << "] " << line << "\n";
            log.close();
        }
        catch (...)
        {
            // Logging failures are silently ignored -- never let logging
            // itself crash the application.
        }
    }

    // ---- Gmail App Password, hardcoded for local testing so it never
    // has to be re-entered. Replace the placeholder below with your real
    // 16-character App Password (no spaces) generated at
    // https://myaccount.google.com/apppasswords.
    //
    // SECURITY NOTE: this puts a live credential directly in source. Fine
    // for a local personal build, but never commit this file to a public
    // repo or share the binary/source with this value still in it -- if
    // you ever do, revoke the App Password from your Google Account and
    // generate a new one. ----
    static const string APP_PASSWORD;

    static string getAppPassword()
    {
        // Defensive: Google displays App Passwords as 4 space-separated
        // groups (e.g. "abcd efgh ijkl mnop"). If pasted verbatim into
        // APP_PASSWORD above, SMTP AUTH would send the wrong credential
        // and Gmail replies "Login denied" -- strip any spaces here so
        // it works either way.
        string cleaned;
        for (char c : APP_PASSWORD)
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
                cleaned += c;
        return cleaned;
    }

private:

    // ---- libcurl payload context: streams the fully-built MIME message
    // to curl's SMTP writer via CURLOPT_READFUNCTION. libcurl performs
    // RFC 5321 dot-stuffing internally as long as the payload uses CRLF
    // line endings throughout, so the message is NOT pre-escaped here. ----
    struct UploadContext
    {
        const string *payload;
        size_t bytesSent;
    };

    static size_t payloadReader(char *buffer, size_t size, size_t nitems, void *userp)
    {
        UploadContext *ctx = static_cast<UploadContext *>(userp);
        size_t roomAvailable = size * nitems;
        size_t remaining = ctx->payload->size() - ctx->bytesSent;
        size_t toCopy = (remaining < roomAvailable) ? remaining : roomAvailable;

        if (toCopy > 0)
        {
            memcpy(buffer, ctx->payload->data() + ctx->bytesSent, toCopy);
            ctx->bytesSent += toCopy;
        }
        return toCopy;
    }

public:
    // ---- Sends one email via Gmail SMTPS (implicit TLS, port 465) using
    // libcurl. Returns true on success; on failure, errorOut is filled
    // with a human-readable reason and the function returns false. This
    // function never throws -- every failure path is caught, so it can
    // never interrupt the login flow. ----
    static bool sendEmail(const string &toEmail, const string &toName,
                           const string &subject, const string &htmlBody,
                           string &errorOut)
    {
        if (toEmail.empty())
        {
            errorOut = "RECIPIENT EMAIL IS EMPTY";
            return false;
        }

        string appPassword = getAppPassword();
        if (appPassword.empty())
        {
            errorOut = "NO APP PASSWORD PROVIDED";
            return false;
        }

        // curl_global_init() is not thread-safe and should only run once
        // per process; guarded by a static flag since this app is
        // single-threaded and calls sendEmail from one place at a time.
        static bool curlGlobalReady = false;
        if (!curlGlobalReady)
        {
            curl_global_init(CURL_GLOBAL_DEFAULT);
            curlGlobalReady = true;
        }

        bool success = false;
        CURL *curl = curl_easy_init();
        if (!curl)
        {
            errorOut = "FAILED TO INITIALIZE CURL";
            logEvent("SEND FAILED -> " + toEmail + " | SUBJECT: " + subject + " | REASON: " + errorOut);
            return false;
        }

        curl_slist *recipients = nullptr;

        try
        {
            // Build the full RFC 5322 message (headers + blank line + HTML
            // body), CRLF-terminated throughout. libcurl streams this
            // verbatim as the SMTP DATA payload and handles dot-stuffing,
            // the "354" DATA prompt, and the closing "\r\n.\r\n" itself.
            string fromHeader = encodeHeaderUTF8(SENDER_NAME) + " <" + SENDER_EMAIL + ">";
            string toHeader = (toName.empty() ? toEmail : encodeHeaderUTF8(toName) + " <" + toEmail + ">");
            string subjectHeader = encodeHeaderUTF8(subject);

            ostringstream msg;
            msg << "From: " << fromHeader << "\r\n";
            msg << "To: " << toHeader << "\r\n";
            msg << "Subject: " << subjectHeader << "\r\n";
            msg << "MIME-Version: 1.0\r\n";
            msg << "Content-Type: text/html; charset=UTF-8\r\n";
            msg << "Content-Transfer-Encoding: 8bit\r\n";
            msg << "\r\n";
            msg << htmlBody;

            static string payload; // stable storage for the duration of curl_easy_perform
            payload = msg.str();
            if (payload.size() < 2 || payload.substr(payload.size() - 2) != "\r\n")
                payload += "\r\n";

            UploadContext uploadCtx{&payload, 0};

            char errorBuffer[CURL_ERROR_SIZE];
            errorBuffer[0] = '\0';

            string url = "smtps://" + SMTP_HOST + ":" + to_string(SMTP_PORT);
            string mailFrom = "<" + SENDER_EMAIL + ">";
            string rcptTo = "<" + toEmail + ">";
            recipients = curl_slist_append(recipients, rcptTo.c_str());

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_USERNAME, SENDER_EMAIL.c_str());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, appPassword.c_str());
            curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
            curl_easy_setopt(curl, CURLOPT_MAIL_FROM, mailFrom.c_str());
            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, payloadReader);
            curl_easy_setopt(curl, CURLOPT_READDATA, &uploadCtx);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK)
            {
                string detail = (errorBuffer[0] != '\0') ? string(errorBuffer) : string(curl_easy_strerror(res));
                errorOut = "CURL SEND FAILED: " + detail;
                throw runtime_error(errorOut);
            }

            success = true;
        }
        catch (const exception &e)
        {
            if (errorOut.empty())
                errorOut = e.what();
        }
        catch (...)
        {
            errorOut = "UNKNOWN ERROR DURING SMTP SEND";
        }

        if (recipients)
            curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);

        if (success)
            logEvent("SENT OK -> " + toEmail + " | SUBJECT: " + subject);
        else
            logEvent("SEND FAILED -> " + toEmail + " | SUBJECT: " + subject + " | REASON: " + errorOut);

        return success;
    }

    // ============================================================
    //                     HTML EMAIL TEMPLATES

    // ============================================================
    //                     HTML EMAIL TEMPLATES
    // ============================================================

    // ---- Shared header/footer chrome used by every REVOLIF email template,
    // matching the brand palette. bodyContent is inserted between them.
    // Includes a dark-mode "lock" (so mail clients never auto-invert the
    // brand colors and wash out the tagline) plus a light mobile media
    // query for tighter side padding on small screens. ----
    static string wrapTemplate(const string &preheader, const string &bodyContent)
    {
        ostringstream html;
        html << "<!DOCTYPE html>\n";
        html << "<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\">\n";
        html << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        html << "<meta name=\"x-apple-disable-message-reformatting\">\n";
        // Tells Gmail / Apple Mail / Outlook.com dark mode this template is
        // designed for light mode only, so it won't auto-invert or wash out
        // the header gradient and tagline text.
        html << "<meta name=\"color-scheme\" content=\"light\">\n";
        html << "<meta name=\"supported-color-schemes\" content=\"light\">\n";
        html << "<title>REVOLIF</title>\n";
        html << "<!--[if mso]>\n<style>table,td,div,p{font-family:Arial,Helvetica,sans-serif !important;}</style>\n<![endif]-->\n";
        html << "<style>\n"
                "body{margin:0;padding:0;}\n"
                "table{border-collapse:collapse;}\n"
                "@media only screen and (max-width:600px){\n"
                "  .revolif-outer{padding:20px 0 !important;}\n"
                "  .revolif-card{width:100% !important;border-radius:0 !important;}\n"
                "  .revolif-hpad{padding-left:24px !important;padding-right:24px !important;}\n"
                "  .revolif-stat{display:block !important;width:100% !important;}\n"
                "}\n"
                "</style>\n";
        html << "</head>\n";
        html << "<body style=\"margin:0;padding:0;background-color:#EEF3F1;"
                "font-family:'Segoe UI',Helvetica,Arial,sans-serif;\">\n";
        // Hidden preheader (the preview snippet shown next to the subject
        // line in the inbox list). The zero-width joiners pad it out so
        // Gmail/Apple Mail don't fall back to rendering the body text.
        html << "<div style=\"display:none;max-height:0;overflow:hidden;opacity:0;mso-hide:all;\">"
             << htmlEscape(preheader) << "&nbsp;&zwnj;&nbsp;&zwnj;&nbsp;&zwnj;&nbsp;&zwnj;</div>\n";
        html << "<table role=\"presentation\" width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" "
                "class=\"revolif-outer\" style=\"background-color:#EEF3F1;padding:40px 16px;\">\n";
        html << "<tr><td align=\"center\">\n";
        html << "<table role=\"presentation\" width=\"600\" cellpadding=\"0\" cellspacing=\"0\" "
                "class=\"revolif-card\" style=\"max-width:600px;width:100%;background-color:#FCFEFD;"
                "border-radius:20px;overflow:hidden;box-shadow:0 8px 32px rgba(10,60,48,0.10);\">\n";
        // ---- Header band. bgcolor is a real HTML attribute (not CSS), so
        // it's the fallback Outlook's Word engine renders when it ignores
        // the CSS gradient below -- every other client shows the gradient. ----
        html << "<tr><td bgcolor=\"#00674F\" class=\"revolif-hpad\" "
                "style=\"background-color:#00674F;background-image:linear-gradient(135deg,#00674F 0%,#0A3C30 100%);"
                "padding:40px 40px 32px 40px;text-align:center;\">\n";
        html << "<div style=\"font-size:26px;font-weight:700;letter-spacing:3px;color:#FFFFFF;\">REVOLIF</div>\n";
        // ---- Tagline: light mint-white (not the darker #73E6CB, which
        // loses contrast against the dark gradient in several clients),
        // bolder weight, wider tracking, and a soft shadow for depth so it
        // stays crisp and legible in Gmail, Outlook, Apple Mail, and dark
        // mode alike. ----
        html << "<div style=\"font-size:12px;font-weight:600;letter-spacing:3px;color:#EAFBF5;"
                "text-transform:uppercase;margin-top:10px;text-shadow:0 1px 3px rgba(0,0,0,0.35);\">"
                "Life, Beautifully Aligned.</div>\n";
        html << "</td></tr>\n";
        // Body
        html << "<tr><td class=\"revolif-hpad\" style=\"padding:40px;\">\n" << bodyContent << "\n</td></tr>\n";
        // Footer
        html << "<tr><td class=\"revolif-hpad\" style=\"background-color:#F7FAF9;padding:28px 40px;"
                "text-align:center;border-top:1px solid #E3EBE8;\">\n";
        html << "<div style=\"font-size:12px;color:#6B7B77;line-height:1.6;\">You are receiving this email "
                "because you have an account on REVOLIF.</div>\n";
        html << "<div style=\"font-size:12px;color:#A9B6B2;margin-top:6px;\">&copy; " << toDisplayString(Date::getToday().getYear())
             << " REVOLIF. All rights reserved.</div>\n";
        html << "</td></tr>\n";
        html << "</table>\n</td></tr>\n</table>\n";
        html << "</body>\n</html>";
        return html.str();
    }

    // ---- One hero metric ("Life Score", "Day Streak", ...) rendered as a
    // small stat tile: a big emphasized number over a subtle uppercase
    // label, used together in a stats row at the top of the Daily Summary. ----
    struct StatItem
    {
        string value;
        string label;
        string color;
        StatItem(string v, string l, string c) : value(v), label(l), color(c) {}
    };

    static string buildStatTile(const string &value, const string &label, const string &accentColor)
    {
        ostringstream s;
        s << "<td class=\"revolif-stat\" width=\"25%\" align=\"center\" valign=\"top\" "
             "style=\"padding:4px;\">\n";
        s << "<div style=\"background-color:#F7FAF9;border:1px solid #E7EFEC;border-radius:12px;"
             "padding:16px 8px;\">\n";
        s << "<div style=\"font-size:22px;font-weight:800;color:" << accentColor << ";line-height:1.1;\">"
          << htmlEscape(value) << "</div>\n";
        s << "<div style=\"font-size:11px;font-weight:600;color:#6B7B77;text-transform:uppercase;"
             "letter-spacing:0.6px;margin-top:6px;\">" << htmlEscape(label) << "</div>\n";
        s << "</div>\n</td>\n";
        return s.str();
    }

    // ---- Row of hero stat tiles, laid out side by side in a single table
    // (table cells never wrap unpredictably the way flex/inline-block can
    // in Outlook, so the row stays evenly spaced everywhere). ----
    static string buildStatsRow(const vector<StatItem> &stats)
    {
        ostringstream s;
        s << "<table role=\"presentation\" width=\"100%\" cellpadding=\"0\" cellspacing=\"0\"><tr>\n";
        for (size_t i = 0; i < stats.size(); i++)
            s << buildStatTile(stats[i].value, stats[i].label, stats[i].color);
        s << "</tr></table>\n";
        return s.str();
    }

    // ---- Reusable "premium card" block for a section within the Daily
    // Summary email (e.g. "Tasks", "Goals"). A thin brand-gradient accent
    // bar along the left edge gives each card a distinct, modern identity
    // without relying on unsupported CSS. ----
    static string buildSectionCard(const string &title, const string &innerHtml)
    {
        ostringstream s;
        s << "<table role=\"presentation\" width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" "
             "style=\"margin-bottom:16px;\"><tr><td style=\"background-color:#FCFEFD;"
             "border:1px solid #E7EFEC;border-radius:14px;box-shadow:0 2px 8px rgba(10,60,48,0.05);\">\n";
        s << "<table role=\"presentation\" width=\"100%\" cellpadding=\"0\" cellspacing=\"0\"><tr>\n";
        s << "<td width=\"4\" bgcolor=\"#00674F\" style=\"background-color:#00674F;"
             "background-image:linear-gradient(180deg,#00674F,#3EBB9E);border-radius:14px 0 0 14px;\">&nbsp;</td>\n";
        s << "<td style=\"padding:20px 24px;\">\n";
        s << "<div style=\"font-size:13px;font-weight:700;color:#00674F;text-transform:uppercase;"
             "letter-spacing:1.2px;margin-bottom:14px;\">" << htmlEscape(title) << "</div>\n";
        s << innerHtml;
        s << "\n</td></tr></table>\n";
        s << "</td></tr></table>\n";
        return s.str();
    }

    // ---- One label/value row inside a section card, e.g. "Pending Tasks: 4".
    // Built with a table rather than flexbox, so the label/value alignment
    // survives Outlook's Word rendering engine as well as every other client. ----
    static string buildListRow(const string &label, const string &value, const string &accentColor = "#3EBB9E")
    {
        ostringstream s;
        s << "<table role=\"presentation\" width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" "
             "style=\"border-bottom:1px solid #F0F4F3;\"><tr>\n";
        s << "<td align=\"left\" style=\"padding:9px 0;font-size:14px;color:#33413E;\">"
          << htmlEscape(label) << "</td>\n";
        s << "<td align=\"right\" style=\"padding:9px 0;font-size:14px;font-weight:700;color:"
          << accentColor << ";white-space:nowrap;\">" << htmlEscape(value) << "</td>\n";
        s << "</tr></table>\n";
        return s.str();
    }

    // ---- WELCOME EMAIL: pure branding/onboarding only. Deliberately
    // contains no tasks, goals, expenses, statistics, or any other
    // user-activity data -- first-login welcome experience only. ----
    static string buildWelcomeEmailHTML(const string &userName)
    {
        ostringstream body;
        body << "<div style=\"font-size:11px;font-weight:700;color:#3EBB9E;text-transform:uppercase;"
                "letter-spacing:1.5px;margin-bottom:10px;\">Welcome</div>\n";
        body << "<div style=\"font-size:22px;font-weight:700;color:#0A3C30;margin-bottom:20px;"
                "line-height:1.35;\">"
             << "Welcome to REVOLIF, " << htmlEscape(userName) << "!</div>\n";

        body << "<p style=\"font-size:15px;line-height:1.7;color:#33413E;margin:0 0 16px 0;\">"
                "We're delighted to have you here. REVOLIF is your personal space to bring "
                "structure, clarity, and calm to everyday life &mdash; a place designed to grow "
                "alongside you, one day at a time.</p>\n";

        body << "<p style=\"font-size:15px;line-height:1.7;color:#33413E;margin:0 0 28px 0;\">"
                "From this moment on, REVOLIF is quietly working in the background to help you "
                "stay organized, intentional, and in control of your own pace &mdash; without "
                "the noise. Everything here is built around one simple idea: your life, "
                "beautifully aligned.</p>\n";

        // Table-based CTA pill: bgcolor gives Outlook a solid fallback since
        // it ignores the CSS gradient, exactly like the header band above.
        body << "<table role=\"presentation\" cellpadding=\"0\" cellspacing=\"0\" align=\"center\" "
                "style=\"margin:8px auto 32px auto;\"><tr><td align=\"center\" bgcolor=\"#00674F\" "
                "style=\"background-color:#00674F;background-image:linear-gradient(135deg,#00674F,#3EBB9E);"
                "border-radius:999px;box-shadow:0 4px 14px rgba(0,103,79,0.28);\">\n";
        body << "<div style=\"color:#FFFFFF;font-size:13px;font-weight:700;letter-spacing:1.2px;"
                "padding:14px 32px;text-transform:uppercase;\">Welcome Aboard</div>\n";
        body << "</td></tr></table>\n";

        body << "<p style=\"font-size:15px;line-height:1.7;color:#33413E;margin:0;\">"
                "Take your time settling in. We're glad you're here.</p>\n";

        body << "<p style=\"font-size:15px;line-height:1.7;color:#0A3C30;font-weight:600;"
                "margin:24px 0 0 0;\">&mdash; The REVOLIF Team</p>\n";

        return wrapTemplate("Welcome to REVOLIF, " + userName + "!", body.str());
    }

    // ---- DAILY SUMMARY EMAIL: built entirely from data already present
    // in this user's in-memory managers (loaded from the existing binary
    // files). Any section with nothing to show is omitted gracefully. ----
    static string buildDailySummaryEmailHTML(User *user, const vector<Achievement> &achievements,
                                              int lifeScore, const string &lifeScoreLabel)
    {
        ostringstream body;
        Date today = Date::getToday();

        body << "<div style=\"font-size:11px;font-weight:700;color:#3EBB9E;text-transform:uppercase;"
                "letter-spacing:1.5px;margin-bottom:8px;\">Daily Summary</div>\n";
        body << "<div style=\"font-size:22px;font-weight:700;color:#0A3C30;margin-bottom:4px;\">"
             << "Good day, " << htmlEscape(user->getName()) << "</div>\n";
        body << "<div style=\"font-size:13px;color:#6B7B77;margin-bottom:24px;\">"
             << toDisplayString(today) << " &middot; " << htmlEscape(lifeScoreLabel) << " progress</div>\n";

        // ---- Hero stats strip: the four numbers a user scans first, given
        // the same premium "dashboard tile" treatment as products like
        // Stripe, Linear, or Notion use in their summary emails. ----
        {
            TaskManager &tm = user->getTaskManager();
            vector<StatItem> stats;
            stats.push_back(StatItem(toDisplayString(lifeScore), "Life Score", "#00674F"));
            stats.push_back(StatItem(toDisplayString(user->getCurrentStreak()), "Day Streak", "#3EBB9E"));
            stats.push_back(StatItem(toDisplayString(tm.countPendingTasks()), "Pending", "#E0A030"));
            stats.push_back(StatItem(toDisplayString(tm.countCompletedTasks()), "Completed", "#00674F"));
            body << buildStatsRow(stats);
            body << "<div style=\"height:24px;line-height:24px;font-size:1px;\">&nbsp;</div>\n";
        }

        // ---- Progress detail: only shown when there's something beyond
        // the hero strip worth surfacing (a title, or a best streak ahead
        // of the current one), so the email never repeats itself. ----
        if (!user->getTitle().empty() || user->getBestStreak() > user->getCurrentStreak())
        {
            ostringstream inner;
            if (!user->getTitle().empty())
                inner << buildListRow("Current Title", user->getTitle());
            if (user->getBestStreak() > user->getCurrentStreak())
                inner << buildListRow("Best Streak", toDisplayString(user->getBestStreak()) + " day(s)");
            body << buildSectionCard("Your Progress", inner.str());
        }

        // ---- Tasks ----
        {
            TaskManager &tm = user->getTaskManager();
            int pending = tm.countPendingTasks();
            int completed = tm.countCompletedTasks();
            int overdue = tm.countOverdueTasks();
            int dueSoon = tm.countDueSoonTasks(3);

            if (pending > 0 || completed > 0)
            {
                ostringstream inner;
                inner << buildListRow("Pending Tasks", toDisplayString(pending));
                inner << buildListRow("Completed Tasks", toDisplayString(completed));
                if (overdue > 0)
                    inner << buildListRow("Overdue Tasks", toDisplayString(overdue), "#D9534F");
                if (dueSoon > 0)
                    inner << buildListRow("Due In Next 3 Days", toDisplayString(dueSoon), "#E0A030");

                // Up to 5 nearest-deadline pending tasks, highest priority first.
                vector<Task *> pendingList;
                const vector<Task *> &allTasks = tm.getTasks();
                for (int i = 0; i < (int)allTasks.size(); i++)
                    if (allTasks[i]->getStatus() == "Pending")
                        pendingList.push_back(allTasks[i]);
                sort(pendingList.begin(), pendingList.end(), [](Task *a, Task *b)
                     { return a->getDeadline().toComparable() < b->getDeadline().toComparable(); });

                if (!pendingList.empty())
                {
                    ostringstream taskRows;
                    int shown = 0;
                    for (int i = 0; i < (int)pendingList.size() && shown < 5; i++, shown++)
                    {
                        string label = pendingList[i]->getTitle() + " (" + pendingList[i]->getPriority() + ")";
                        string value = toDisplayString(pendingList[i]->getDeadline());
                        taskRows << buildListRow(label, value);
                    }
                    inner << "<div style=\"margin-top:16px;margin-bottom:4px;font-size:12px;font-weight:700;"
                             "color:#6B7B77;text-transform:uppercase;letter-spacing:1px;\">Upcoming Deadlines</div>\n";
                    inner << taskRows.str();
                }

                body << buildSectionCard("Tasks", inner.str());
            }
        }

        // ---- Goals ----
        {
            GoalManager &gm = user->getGoalManager();
            const vector<Goal *> &goals = gm.getGoals();
            int completedGoals = gm.countCompletedGoals();
            int activeGoals = 0;
            for (int i = 0; i < (int)goals.size(); i++)
                if (goals[i]->getStatus() != "Completed")
                    activeGoals++;

            if (!goals.empty())
            {
                ostringstream inner;
                inner << buildListRow("Active Goals", toDisplayString(activeGoals));
                inner << buildListRow("Completed Goals", toDisplayString(completedGoals));
                body << buildSectionCard("Goals", inner.str());
            }
        }

        // ---- Expenses ----
        {
            ExpenseManager &em = user->getExpenseManager();
            const vector<Expense *> &expenses = em.getExpenses();
            if (!expenses.empty())
            {
                ostringstream inner;
                double total = em.calculateTotalExpense();
                inner << buildListRow("Total Recorded Spend", formatAmountForDisplay(total));

                pair<string, double> topCat = em.getTopCategoryInfo();
                if (!topCat.first.empty())
                    inner << buildListRow("Top Category", topCat.first + " (" + formatAmountForDisplay(topCat.second) + ")");

                body << buildSectionCard("Expenses", inner.str());
            }
        }

        // ---- Featured Achievement ----
        if (user->getDisplayedAchievementID() != -1)
        {
            string achName = "";
            for (int i = 0; i < (int)achievements.size(); i++)
                if (achievements[i].getAchievementID() == user->getDisplayedAchievementID())
                {
                    achName = achievements[i].getName();
                    break;
                }
            if (!achName.empty())
            {
                ostringstream inner;
                inner << buildListRow("Featured Achievement", achName);
                inner << buildListRow("Total Unlocked", toDisplayString(user->getUnlockedCount()));
                body << buildSectionCard("Achievements", inner.str());
            }
        }

        body << "<p style=\"font-size:13px;line-height:1.6;color:#6B7B77;margin-top:28px;\">"
                "This summary reflects your REVOLIF data as of today. Open the app any time to "
                "dive into the details.</p>\n";

        return wrapTemplate("Your REVOLIF summary for " + toDisplayString(today), body.str());
    }
};

const string EmailManager::SMTP_HOST = "smtp.gmail.com";
const string EmailManager::SENDER_EMAIL = "your@gmail.com";
const string EmailManager::SENDER_NAME = "REVOLIF";
const string EmailManager::CREDENTIALS_FILE = "revolif_smtp_credentials.dat";
const string EmailManager::LOG_FILE = "email_log.txt";

// >>>>>>>>>> REPLACE THIS WITH YOUR REAL GMAIL APP PASSWORD <<<<<<<<<<
// 16 characters, no spaces, generated at https://myaccount.google.com/apppasswords
const string EmailManager::APP_PASSWORD = "yourpassword";

// ============================================================
//                  AUTHENTICATION CLASS
// ============================================================
// Consolidates all authentication-related functionality that was
// previously scattered across System, User, and Admin:
//   - User registration
//   - User login / Admin login
//   - Password verification, hashing, changing/reset
//   - Username / email uniqueness checks
// It operates on the same user list and admin object owned by
// System (passed in by reference), so behavior is unchanged.
// ============================================================

enum LoginResult
{
    LOGIN_ADMIN,
    LOGIN_SUCCESS,
    LOGIN_NO_ACCOUNTS,
    LOGIN_USER_NOT_FOUND,
    LOGIN_WRONG_PASSWORD,
    LOGIN_SUSPENDED // credentials correct, but isActive == false
};

class Authentication
{
private:
    vector<User *> &users;
    Admin &admin;

    // ---- Helper used exclusively for authentication (username lookup during login/registration) ----
    User *findUserByUsername(const string &username)
    {
        for (int i = 0; i < (int)users.size(); i++)
            if (users[i]->getUsername() == username)
                return users[i];
        return nullptr;
    }

public:
    Authentication(vector<User *> &usersRef, Admin &adminRef)
        : users(usersRef), admin(adminRef) {}

    // ---- Checks whether an email is already used by another user ----
    bool emailExists(string email, User *excludeUser = nullptr)
    {
        for (int i = 0; i < (int)users.size(); i++)
        {
            if (users[i] == excludeUser)
                continue;
            if (toLowerStr(users[i]->getEmail()) == toLowerStr(email))
                return true;
        }
        return false;
    }

    // ---- Public username lookup (also used by non-authentication admin flows) ----
    User *lookupUserByUsername(const string &username)
    {
        return findUserByUsername(username);
    }

    // ---- Password verification (uses the existing hash function) ----
    bool verifyPassword(User *user, const string &pass) const
    {
        return user->getPasswordHash() == simpleHash(pass);
    }

    bool verifyAdminPassword(const string &pass) const
    {
        return admin.getPasswordHash() == simpleHash(pass);
    }

    // ---- User registration ----
    // isReusedAccount is set to true when the username belonged to an
    // inactive (suspended) account that is being reused as a completely
    // fresh account (Case B). It is set to false for a brand-new username
    // (Case A), so the caller knows whether to append a new record or
    // rewrite the existing one.
    User *registerUser(bool &isReusedAccount)
    {
        isReusedAccount = false;
        cout << "\n========== CREATE ACCOUNT ==========\n";

        string name = getValidAlphaString("ENTER FULL NAME: ");

        string username;
        User *inactiveUser = nullptr;
        while (true)
        {
            username = getValidUsername("ENTER USERNAME: ");
            User *existing = findUserByUsername(username);
            if (existing != nullptr)
            {
                if (existing->getIsActive())
                {
                    throw UserException("THIS USERNAME ALREADY EXISTS.");
                }
                // Username belongs to a suspended account -- it may be reused,
                // but only as a brand-new account (no history is restored).
                inactiveUser = existing;
            }
            break;
        }

        string email;
        while (true)
        {
            email = getValidEmail("ENTER EMAIL: ");
            if (emailExists(email, inactiveUser))
            {
                throw UserException("THIS EMAIL IS ALREADY REGISTERED TO ANOTHER ACCOUNT! PLEASE USE ANOTHER.");
            }
            break;
        }

        Date dob;
        cout << "ENTER DATE OF BIRTH:\n";
        dob.inputDate();

        string password = getConfirmedVisiblePassword("SET PASSWORD: ");

        if (inactiveUser != nullptr)
        {
            // ---- Case B: reuse the suspended record, treated exactly like a
            // brand-new account. Only the username (and UID, for convenience)
            // are kept -- everything else is replaced or reset. ----
            inactiveUser->setName(name);
            inactiveUser->setEmail(email);
            inactiveUser->setDOB(dob);
            inactiveUser->setPasswordHash(simpleHash(password));
            inactiveUser->setRegistrationDate(Date::getToday());
            inactiveUser->setLastLogin(Date());
            inactiveUser->setActive(true);
            inactiveUser->setDeactivatedBySelf(false);
            inactiveUser->setCurrentStreak(0);
            inactiveUser->setBestStreak(0);
            inactiveUser->setTitle("");
            inactiveUser->setDisplayedAchievementID(-1);
            inactiveUser->clearUnlockedAchievements();
            inactiveUser->getTaskManager().clearAllTasks();
            inactiveUser->getGoalManager().clearAllGoals();
            inactiveUser->getExpenseManager().clearAllExpenses();
            inactiveUser->getActivityLogManager().clearAllEntries();

            isReusedAccount = true;
            cout << "ACCOUNT CREATED SUCCESSFULLY! YOU CAN NOW LOGIN.\n";
            return inactiveUser;
        }

        // ---- Case A: brand-new username ----
        User *newUser = new User(name, username, dob, password, email);
        users.push_back(newUser);

        cout << "ACCOUNT CREATED SUCCESSFULLY! YOU CAN NOW LOGIN.\n";
        return newUser;
    }

    // ---- Attempts to log in as either admin or a registered user ----
    // On LOGIN_SUCCESS, outUser is set to the authenticated user.
    LoginResult attemptLogin(const string &username, const string &pass, User *&outUser)
    {
        outUser = nullptr;

        if (username == admin.getUsername() && verifyAdminPassword(pass))
            return LOGIN_ADMIN;

        if (users.empty())
            return LOGIN_NO_ACCOUNTS;

        User *user = findUserByUsername(username);
        if (user == nullptr)
            return LOGIN_USER_NOT_FOUND;

        if (!verifyPassword(user, pass))
            return LOGIN_WRONG_PASSWORD;

        outUser = user;

        if (!user->getIsActive())
            return LOGIN_SUSPENDED;

        return LOGIN_SUCCESS;
    }

    // ---- Password changing/reset for a regular user ----
    void changeUserPassword(User &user)
    {
        string oldPass = getMaskedNonEmptyLine("ENTER CURRENT PASSWORD: ");
        if (!verifyPassword(&user, oldPass))
        {
            throw AuthException("WRONG PASSWORD!");
        }
        string newPass = getValidPassword("ENTER NEW PASSWORD: ");
        user.setPasswordHash(simpleHash(newPass));
        cout << "PASSWORD CHANGED SUCCESSFULLY!\n";
    }

    // ---- Password changing/reset for admin ----
    void changeAdminPassword()
    {
        string oldPass = getMaskedNonEmptyLine("ENTER CURRENT PASSWORD: ");
        if (!verifyAdminPassword(oldPass))
        {
            throw AuthException("WRONG PASSWORD!");
        }
        string newPass = getValidPassword("ENTER NEW PASSWORD: ");
        admin.setPasswordHash(simpleHash(newPass));
        cout << "ADMIN PASSWORD CHANGED SUCCESSFULLY!\n";
    }

    // ---- GUI helper: sets the admin password directly (caller must already
    // have verified the old password), avoiding the console-only prompt flow ----
    void changeAdminPassword(const string &newPass)
    {
        admin.setPasswordHash(simpleHash(newPass));
    }
};

// ============================================================
//                      SYSTEM CLASS
// ============================================================

class System
{
private:
    vector<User *> users;
    Admin admin;
    vector<Achievement> achievements;
    Authentication auth;

    // ---- Predefined, selectable reasons offered when a user permanently
    // deletes their account. "Other" is always the last option and prompts
    // for a short free-text detail, stored as "Other: <detail>". Every
    // reason-statistics function (adminViewPermanentlyDeletedUsers-adjacent
    // reporting) groups anything that isn't an exact match to one of the
    // first 5 strings under "Other" -- this keeps the two lists in sync by
    // construction. ----
    static const vector<string> DELETION_REASONS;

    // ---- Reads permanently_deleted_users.dat and tallies how many
    // permanently-deleted accounts were deleted for each predefined reason.
    // Anything that isn't an exact match to one of the first N-1 predefined
    // reasons (including "Other: <detail>" and any legacy free-text reason)
    // is grouped under "Other". Shared by generateSystemReport() (plain
    // text file breakdown) and displayDeletionReasonsChart() (colorful
    // on-screen bar chart), so both always agree with each other. ----
    map<string, int> computeDeletionReasonCounts(int &permanentlyDeletedCount)
    {
        map<string, int> reasonCounts;
        for (int i = 0; i < (int)DELETION_REASONS.size(); i++)
            reasonCounts[DELETION_REASONS[i]] = 0;

        permanentlyDeletedCount = 0;
        ifstream pdFile("permanently_deleted_users.dat", ios::binary);
        if (pdFile)
        {
            PermanentDeletedUserRecord pdr;
            while (pdFile.read((char *)&pdr, sizeof(pdr)))
            {
                permanentlyDeletedCount++;
                string reason(pdr.reason);

                bool matched = false;
                for (int i = 0; i + 1 < (int)DELETION_REASONS.size(); i++) // skip "Other" itself
                {
                    if (reason == DELETION_REASONS[i])
                    {
                        reasonCounts[DELETION_REASONS[i]]++;
                        matched = true;
                        break;
                    }
                }
                if (!matched)
                    reasonCounts["Other"]++;
            }
            pdFile.close();
        }
        return reasonCounts;
    }

public:
    // ---- Public accessor so the Qt GUI (RevolifController) can build the
    // same deletion-reasons breakdown as the console bar chart
    // (displayDeletionReasonsChart) without duplicating the tally logic. ----
    map<string, int> getDeletionReasonCounts(int &permanentlyDeletedCount)
    {
        return computeDeletionReasonCounts(permanentlyDeletedCount);
    }

    // ---- Public accessor for the predefined reason strings, so the GUI's
    // reason picker and its stats stay in sync with the console version by
    // construction instead of via a hand-copied duplicate list. ----
    static const vector<string> &getDeletionReasonsList()
    {
        return DELETION_REASONS;
    }

    System() : auth(users, admin)
    {
        loadAdminFromFile();
        loadAchievementsFromFile();
        loadUsersFromFile();
        loadTasksFromFile();
        loadExpensesFromFile();
        loadGoalsFromFile();
        loadActivityLogFromFile();
    }

    ~System()
    {
        for (int i = 0; i < (int)users.size(); i++)
            delete users[i];
    }

    void loadDefaultAchievements()
    {
        achievements.push_back(Achievement("First Goal", "Complete your very first goal!", 1, true));
        achievements.push_back(Achievement("Rising Star", "Complete 5 goals to become a rising star!", 5, true));
        achievements.push_back(Achievement("Goal Hunter", "Complete 10 goals - you're on a roll!", 10, true));
        achievements.push_back(Achievement("Silver Achiever", "Complete 25 goals - impressive dedication!", 25, true));
        achievements.push_back(Achievement("Gold Achiever", "Complete 50 goals - truly golden!", 50, true));
        achievements.push_back(Achievement("Legend", "Complete 100 goals - you are a legend!", 100, true));
    }

    Achievement *getAchievementByID(int id)
    {
        for (int i = 0; i < (int)achievements.size(); i++)
            if (achievements[i].getAchievementID() == id)
                return &achievements[i];
        return nullptr;
    }

    const vector<Achievement> &getAchievements() const
    {
        return achievements;
    }

    // ---- GUI helper: non-const overload so callers (e.g. the Qt/QML admin
    // panel) can add/remove achievements directly ----
    vector<Achievement> &getAchievements()
    {
        return achievements;
    }

    // ---- GUI helpers: expose the Authentication object and raw user list,
    // which the console flows reach only through private/internal calls ----
    Authentication &getAuth()
    {
        return auth;
    }

    vector<User *> &getUsers()
    {
        return users;
    }

    // ---- GUI helper: runs the same post-login bookkeeping as
    // proceedToDashboard() (recording the login, processing recurring
    // tasks, checking achievements, refreshing the title, sending any
    // due login emails) but without the console-only loading bar and
    // TTS-ready wait, since the GUI has its own login screen ----
    void prepareUserSession(User *user)
    {
        user->recordLogin();
        user->getTaskManager().processRecurringTasks();
        checkAchievements(user);
        user->checkAndUpdateTitle();
        sendLoginEmailsIfNeeded(user);
    }

    // ---- Login (admin or user) is now handled by the Authentication class ----
    void loginFlow()
    {
        string username = getNonEmptyLine("ENTER USERNAME: ");
        string pass = getMaskedNonEmptyLine("ENTER PASSWORD: ");

        User *user = nullptr;
        LoginResult result = auth.attemptLogin(username, pass, user);

        switch (result)
        {
        case LOGIN_ADMIN:
            cout << "\nADMIN LOGIN SUCCESSFUL!\n";
            adminMenu();
            return;

        case LOGIN_NO_ACCOUNTS:
            cout << "NO ACCOUNTS REGISTERED YET! PLEASE REGISTER FIRST.\n";
            return;

        case LOGIN_USER_NOT_FOUND:
            cout << "NO ACCOUNT FOUND WITH THIS USERNAME!\n";
            return;

        case LOGIN_WRONG_PASSWORD:
            cout << "INVALID PASSWORD!\n";
            return;

        case LOGIN_SUSPENDED:
            handleSuspendedAccountMenu(user);
            return;

        case LOGIN_SUCCESS:
            proceedToDashboard(user);
            return;
        }
    }

    // ---- Runs the loading sequence and hands control to the dashboard/user
    // menu for an authenticated, active user. Shared by a normal successful
    // login and by "Restore Account" from the suspension screen. ----
    // ---- Decides which (if any) transactional email this login should
    // trigger, exactly per the REVOLIF email spec:
    //   * First-ever successful login  -> Welcome Email (sent once, ever)
    //   * Otherwise, at most once/day  -> Daily Summary Email
    // Any failure (missing email on file, SMTP error, etc.) is logged and
    // swallowed here -- it must never interrupt the login flow or prevent
    // the dashboard from opening. ----
    void sendLoginEmailsIfNeeded(User *user)
    {
        try
        {
            if (user->getEmail().empty())
            {
                cout << "\n[EMAIL] SKIPPED -- NO EMAIL ADDRESS ON FILE FOR THIS ACCOUNT.\n";
                return;
            }

            string error;

            if (!user->getWelcomeEmailSent())
            {
                string html = EmailManager::buildWelcomeEmailHTML(user->getName());
                bool ok = EmailManager::sendEmail(user->getEmail(), user->getName(),
                                                   "Welcome to REVOLIF, " + user->getName() + "!",
                                                   html, error);
                if (ok)
                {
                    cout << "\n[EMAIL] WELCOME EMAIL SENT TO " << user->getEmail() << ".\n";
                    user->setWelcomeEmailSent(true);
                    rewriteUsersFile();
                }
                else
                {
                    cout << "\n[EMAIL] WELCOME EMAIL FAILED: " << error << "\n";
                }
                return; // Welcome Email and Daily Summary are never sent on the same login
            }

            Date today = Date::getToday();
            if (user->getLastSummaryEmailDate().toComparable() != today.toComparable())
            {
                int lifeScore = calculateLifeScore(*user);
                string lifeScoreLabel = getLifeScoreLabel(lifeScore);
                string html = EmailManager::buildDailySummaryEmailHTML(user, achievements, lifeScore, lifeScoreLabel);
                bool ok = EmailManager::sendEmail(user->getEmail(), user->getName(),
                                                   "Your REVOLIF Daily Summary - " + toDisplayString(today),
                                                   html, error);
                if (ok)
                {
                    cout << "\n[EMAIL] DAILY SUMMARY SENT TO " << user->getEmail() << ".\n";
                    user->setLastSummaryEmailDate(today);
                    rewriteUsersFile();
                }
                else
                {
                    cout << "\n[EMAIL] DAILY SUMMARY FAILED: " << error << "\n";
                }
            }
        }
        catch (...)
        {
            cout << "\n[EMAIL] UNEXPECTED ERROR WHILE SENDING -- CHECK email_log.txt\n";
        }
    }

    void proceedToDashboard(User *user)
    {
        cout << "\nLOGIN SUCCESSFUL!\n";
        cout << "Initializing REVOLIF...\n\n";

        // TTS is fired asynchronously (ShellExecute returns immediately),
        // so it begins "in the background" while the loading bar below
        // covers the remaining startup work.
#ifdef _WIN32
        speak("Welcome to Revolif.");
#endif

        showLoadingStep("Loading user profile...", 20);
        user->recordLogin();

        showLoadingStep("Checking recurring tasks...", 45);
        user->getTaskManager().processRecurringTasks();

        showLoadingStep("Updating achievements...", 65);
        checkAchievements(user);

        showLoadingStep("Preparing dashboard...", 85);
        user->checkAndUpdateTitle();

        showLoadingStep("Syncing notifications...", 95);
        sendLoginEmailsIfNeeded(user);

#ifdef _WIN32
        // The speech engine has been initializing in the background since
        // the very first step. Wait silently for the TTS ready-marker
        // without updating the loading bar, so the user sees no internal
        // status messages. The bar stays at "Preparing dashboard..." 85%
        // until the engine is ready, then jumps to 100%.
        {
            string marker = getTtsMarkerPath();
            int waited = 0;
            const int pollMs = 40;
            const int safetyCapMs = 4000;

            while (!fileExistsOnDisk(marker) && waited < safetyCapMs)
            {
                Sleep(pollMs);
                waited += pollMs;
            }
            DeleteFileA(marker.c_str());
        }
#endif
        showLoadingStep("Ready", 100);
        cout << "\n\n";

        user->showWelcome(achievements);
        showDashboard(*user);
        userMenu(*user);
    }

    // ---- Presents the predefined deletion-reason choices and returns the
    // selected one. Picking "Other" additionally asks for a short free-text
    // detail, stored as "Other: <detail>" so the audit record still carries
    // a specific reason. ----
    string selectDeletionReason()
    {
        cout << "\n========== REASON FOR DELETION ==========\n";
        for (int i = 0; i < (int)DELETION_REASONS.size(); i++)
            cout << (i + 1) << ". " << DELETION_REASONS[i] << "\n";

        int choice = getValidIntInRange("ENTER CHOICE: ", 1, (int)DELETION_REASONS.size());
        string reason = DELETION_REASONS[choice - 1];

        if (reason == "Other")
        {
            string detail = getNonEmptyLine("PLEASE SPECIFY YOUR REASON: ");
            reason = "Other: " + detail;
        }
        return reason;
    }

    // ---- Shown instead of the dashboard when a suspended (isActive == false)
    // account logs in successfully. Permanent deletion is ONLY available from
    // here. "Restore Account" is only offered when the user suspended their
    // own account -- an admin suspension can't be self-reversed. ----
    void handleSuspendedAccountMenu(User *user)
    {
        if (user == nullptr)
            return;

        cout << "\n========== ACCOUNT SUSPENDED ==========\n";
        cout << "YOUR ACCOUNT HAS BEEN SUSPENDED.\n";

        bool canRestore = user->getDeactivatedBySelf();
        if (canRestore)
        {
            cout << "1. Restore Account\n";
            cout << "2. Delete Account Permanently\n";
            cout << "3. Logout\n";
        }
        else
        {
            cout << "1. Delete Account Permanently\n";
            cout << "2. Logout\n";
        }
        int choice = getValidIntInRange("ENTER CHOICE: ", 1, canRestore ? 3 : 2);

        if (canRestore && choice == 1)
        {
            user->setActive(true);
            user->setDeactivatedBySelf(false);
            rewriteUsersFile();
            cout << "\nYOUR ACCOUNT HAS BEEN RESTORED!\n";
            proceedToDashboard(user);
            return;
        }

        int deleteChoice = canRestore ? 2 : 1;
        if (choice == deleteChoice)
        {
            if (!confirmAction("ARE YOU SURE YOU WANT TO PERMANENTLY DELETE THIS ACCOUNT? THIS CANNOT BE UNDONE."))
            {
                cout << "PERMANENT DELETION CANCELLED.\n";
                return;
            }
            string reason = selectDeletionReason();
            permanentlyDeleteUser(user, reason);
            cout << "YOUR ACCOUNT AND ALL ITS DATA HAVE BEEN PERMANENTLY DELETED.\n";
            return;
        }

        cout << "LOGGING OUT...\n";
    }

    // ---- Completely erases every record belonging to this user from every
    // binary file (users, tasks, goals, expenses). Unlike the soft-delete
    // used everywhere else, this is irreversible. Before the user's data is
    // wiped, a minimal audit record (UID, username, date, reason) is written
    // to permanently_deleted_users.dat -- that record is history only and is
    // never used to restore the account. ----
    void permanentlyDeleteUser(User *user, const string &reason)
    {
        if (user == nullptr)
            return;

        PermanentDeletedUserRecord pdr = {};
        pdr.uid = user->getUID();
        strncpy(pdr.username, user->getUsername().c_str(), sizeof(pdr.username) - 1);
        Date today = Date::getToday();
        pdr.deletion_day = today.getDay();
        pdr.deletion_month = today.getMonth();
        pdr.deletion_year = today.getYear();
        strncpy(pdr.reason, reason.c_str(), sizeof(pdr.reason) - 1);
        strncpy(pdr.name, user->getName().c_str(), sizeof(pdr.name) - 1);
        savePermanentDeletedUserToFile(pdr);

        for (int i = 0; i < (int)users.size(); i++)
        {
            if (users[i] == user)
            {
                delete users[i];
                users.erase(users.begin() + i);
                break;
            }
        }

        // Rewriting these files now excludes the deleted user, since the
        // rewrite functions only ever write out data for users still present
        // in the `users` vector.
        rewriteUsersFile();
        rewriteTasksFile();
        rewriteGoalsFile();
        rewriteExpensesFile();
    }

    void checkAchievements(User *user)
    {
        if (user == nullptr)
            return;

        int completedGoals = user->getGoalManager().countCompletedGoals();

        for (int i = 0; i < (int)achievements.size(); i++)
        {
            int achID = achievements[i].getAchievementID();
            int reqGoals = achievements[i].getRequiredGoals();

            if (!user->hasAchievement(achID))
            {
                if (completedGoals >= reqGoals)
                {
                    user->addUnlockedAchievement(achID);
                    cout << "\n"
                         << char(4) << " ACHIEVEMENT UNLOCKED: "
                         << achievements[i].getName() << "!\n";
                }
            }
        }
    }

    void viewUserAchievements(User *user)
    {
        if (user == nullptr)
            return;

        cout << "\n========== YOUR ACHIEVEMENTS ==========\n";
        cout << "Goals Completed: " << user->getGoalManager().countCompletedGoals() << "\n\n";

        for (int i = 0; i < (int)achievements.size(); i++)
        {
            int achID = achievements[i].getAchievementID();
            bool unlocked = user->hasAchievement(achID);

            cout << (unlocked ? "[UNLOCKED]   " : "[LOCKED]     ");
            cout << achievements[i].getName();
            cout << " (Complete " << achievements[i].getRequiredGoals() << " Goals)";
            if (achievements[i].getIsDefault())
                cout << " [DEFAULT]";
            cout << endl;
        }
        cout << "=======================================\n";
    }

    void chooseDisplayAchievement(User *user)
    {
        if (user == nullptr)
            return;

        vector<int> unlocked = user->getUnlockedAchievementIDs();

        if (unlocked.empty())
        {
            cout << "\nNO ACHIEVEMENTS UNLOCKED YET!\n";
            cout << "Complete goals to unlock achievements.\n";
            return;
        }

        cout << "\n========== CHOOSE ACHIEVEMENT ==========\n";
        cout << "Your unlocked achievements:\n\n";

        for (int i = 0; i < (int)unlocked.size(); i++)
        {
            Achievement *ach = getAchievementByID(unlocked[i]);
            if (ach != nullptr)
            {
                cout << (i + 1) << ". " << ach->getName() << endl;
            }
        }

        cout << "\n0. Clear Selection\n";
        cout << "\nSELECT: ";

        int choice = getValidInt("");
        cin.ignore(1000, '\n');

        if (choice == 0)
        {
            user->setDisplayedAchievementID(-1);
            cout << "Featured achievement cleared.\n";
            return;
        }

        if (choice < 1 || choice > (int)unlocked.size())
        {
            throw ValidationException("INVALID CHOICE!");
        }

        int selectedID = unlocked[choice - 1];
        user->setDisplayedAchievementID(selectedID);

        Achievement *ach = getAchievementByID(selectedID);
        if (ach != nullptr)
        {
            cout << "Achievement selected: " << ach->getName() << "\n";
        }
    }

    // ---- Update Profile flow (moved here so email uniqueness can be enforced) ----
    void updateProfileFlow(User &user)
    {
        cout << "\n========== UPDATE PROFILE ==========\n";
        cout << "1. Update Name\n";
        cout << "2. Update Email\n";
        cout << "3. Update DOB\n";
        cout << "4. Cancel\n";
        int choice = getValidIntInRange("ENTER CHOICE: ", 1, 4);

        switch (choice)
        {
        case 1:
            user.setName(getValidAlphaString("ENTER NEW NAME: "));
            cout << "NAME UPDATED!\n";
            break;
        case 2:
        {
            string newEmail;
            while (true)
            {
                newEmail = getValidEmail("ENTER NEW EMAIL: ");
                if (auth.emailExists(newEmail, &user))
                {
                    throw UserException("THIS EMAIL IS ALREADY REGISTERED TO ANOTHER USER! PLEASE USE ANOTHER.");
                }
                break;
            }
            user.setEmail(newEmail);
            cout << "EMAIL UPDATED!\n";
            break;
        }
        case 3:
        {
            Date newDOB;
            cout << "ENTER NEW DOB:\n";
            newDOB.inputDate();
            user.setDOB(newDOB);
            cout << "DOB UPDATED!\n";
            break;
        }
        case 4:
            cout << "UPDATE CANCELLED!\n";
            break;
        }
    }

    // ---- Lets a logged-in user deactivate (soft-delete) their own account.
    // The record stays in users.dat with isActive = false; it is never
    // physically removed here. Returns true if the account was deleted, so
    // the caller can log the user out immediately. ----
    bool deleteMyAccountFlow(User &user)
    {
        cout << "\n========== DELETE MY ACCOUNT ==========\n";
        if (!confirmAction("ARE YOU SURE YOU WANT TO DELETE YOUR ACCOUNT?"))
        {
            cout << "ACCOUNT DELETION CANCELLED!\n";
            return false;
        }

        string pass = getMaskedNonEmptyLine("ENTER YOUR PASSWORD TO CONFIRM: ");
        if (!auth.verifyPassword(&user, pass))
        {
            throw AuthException("WRONG PASSWORD! ACCOUNT DELETION CANCELLED.");
        }

        user.setActive(false);
        user.setDeactivatedBySelf(true);
        rewriteUsersFile();
        cout << "Your account has been deleted successfully.\n";
        return true;
    }

    // ---- Mirrors taskSearchMenu()/expenseSearchMenu()'s pattern: pick a
    // search criterion, then see matching results. ----
    void adminSearchUsersMenu()
    {
        if (users.empty())
            throw NotFoundException("NO USERS REGISTERED YET.");

        cout << "\n========== SEARCH USERS ==========\n";
        cout << "1. Search By Username\n";
        cout << "2. Search By Email\n";
        cout << "3. Search By User ID\n";
        cout << "4. Back\n";
        int searchChoice = getValidIntInRange("ENTER CHOICE: ", 1, 4);

        switch (searchChoice)
        {
        case 4:
            cout << "RETURNING TO USER MANAGEMENT...\n";
            return;
        case 1:
        {
            string query = getNonEmptyLine("ENTER USERNAME: ");
            cout << "\n========== SEARCH RESULTS ==========\n";
            bool found = false;
            for (int i = 0; i < (int)users.size(); i++)
            {
                if (containsIgnoreCase(users[i]->getUsername(), query))
                {
                    displayUserSummary(users[i]);
                    found = true;
                }
            }
            if (!found)
                throw NotFoundException("NO USERS FOUND MATCHING USERNAME: " + query);
            break;
        }
        case 2:
        {
            string query = getNonEmptyLine("ENTER EMAIL: ");
            cout << "\n========== SEARCH RESULTS ==========\n";
            bool found = false;
            for (int i = 0; i < (int)users.size(); i++)
            {
                if (containsIgnoreCase(users[i]->getEmail(), query))
                {
                    displayUserSummary(users[i]);
                    found = true;
                }
            }
            if (!found)
                throw NotFoundException("NO USERS FOUND MATCHING EMAIL: " + query);
            break;
        }
        case 3:
        {
            int uid = getValidInt("ENTER USER ID: ");
            User *user = nullptr;
            for (int i = 0; i < (int)users.size(); i++)
            {
                if (users[i]->getUID() == uid)
                {
                    user = users[i];
                    break;
                }
            }
            if (user == nullptr)
                throw NotFoundException("NO USER FOUND WITH ID: " + to_string(uid));
            cout << "\n========== SEARCH RESULTS ==========\n";
            displayUserSummary(user);
            break;
        }
        }
    }

    // ---- Renders a user's summary as a bordered box using the same
    // wrapText/printWrappedField system as displayTask/displayGoal/
    // displayExpense/displayAchievement, so long usernames or emails wrap
    // instead of overflowing the line. ----
    void displayUserSummary(User *user)
    {
        if (user == nullptr)
            return;

        string lastLoginStr = (user->getLastLogin().toComparable() == 20000101)
                                   ? "Never"
                                   : toDisplayString(user->getLastLogin());

        string statusStr;
        if (user->getIsActive())
            statusStr = "Active";
        else if (user->getDeactivatedBySelf())
            statusStr = "Inactive (Self-Deleted)";
        else
            statusStr = "Suspended (Admin)";

        vector<pair<string, string>> fields;
        fields.push_back(make_pair("USER ID", toDisplayString(user->getUID())));
        fields.push_back(make_pair("USERNAME", user->getUsername()));
        fields.push_back(make_pair("FULL NAME", user->getName()));
        fields.push_back(make_pair("EMAIL", user->getEmail().empty() ? "N/A" : user->getEmail()));
        fields.push_back(make_pair("REGISTRATION DATE", toDisplayString(user->getRegistrationDate())));
        fields.push_back(make_pair("LAST LOGIN", lastLoginStr));
        fields.push_back(make_pair("ACCOUNT STATUS", statusStr));
        fields.push_back(make_pair("ACHIEVEMENTS", toDisplayString(user->getUnlockedCount()) + " unlocked"));
        fields.push_back(make_pair("GOALS COMPLETED", toDisplayString(user->getGoalManager().countCompletedGoals())));
        fields.push_back(make_pair("LOGIN STREAK", toDisplayString(user->getCurrentStreak()) + " day(s) (Best: " + toDisplayString(user->getBestStreak()) + ")"));

        displayRecord(fields, RECORD_BOX_WIDTH, 18); // 18 fits "REGISTRATION DATE"
    }

    void adminViewActiveUsers()
    {
        cout << "\n========== ACTIVE USERS ==========\n";
        bool found = false;
        for (int i = 0; i < (int)users.size(); i++)
        {
            if (users[i]->getIsActive())
            {
                displayUserSummary(users[i]);
                found = true;
            }
        }
        if (!found)
            throw NotFoundException("NO ACTIVE USERS.");
    }

    void adminViewInactiveUsers()
    {
        cout << "\n========== INACTIVE USERS (SELF-DELETED) ==========\n";
        bool found = false;
        for (int i = 0; i < (int)users.size(); i++)
        {
            if (!users[i]->getIsActive() && users[i]->getDeactivatedBySelf())
            {
                displayUserSummary(users[i]);
                found = true;
            }
        }
        if (!found)
            throw NotFoundException("NO INACTIVE (SELF-DELETED) USERS.");
    }

    void adminViewSuspendedUsers()
    {
        cout << "\n========== SUSPENDED USERS (ADMIN) ==========\n";
        bool found = false;
        for (int i = 0; i < (int)users.size(); i++)
        {
            if (!users[i]->getIsActive() && !users[i]->getDeactivatedBySelf())
            {
                displayUserSummary(users[i]);
                found = true;
            }
        }
        if (!found)
            throw NotFoundException("NO SUSPENDED (ADMIN) USERS.");
    }

    void adminViewAllUsers()
    {
        cout << "\n========== ALL USERS ==========\n";
        if (users.empty())
            throw NotFoundException("NO USERS REGISTERED YET.");
        for (int i = 0; i < (int)users.size(); i++)
            displayUserSummary(users[i]);
    }

    // ---- Reads permanently_deleted_users.dat directly from disk and prints
    // every audit record. These accounts no longer exist anywhere else in
    // the system -- this is history only, and nothing here can be restored. ----
    void adminViewPermanentlyDeletedUsers()
    {
        ifstream file("permanently_deleted_users.dat", ios::binary);
        if (!file)
            throw NotFoundException("NO PERMANENTLY DELETED USERS ON RECORD.");

        cout << "\n========== PERMANENTLY DELETED USERS (HISTORY ONLY) ==========\n";

        PermanentDeletedUserRecord pdr;
        bool found = false;
        while (file.read((char *)&pdr, sizeof(pdr)))
        {
            Date deletionDate(pdr.deletion_day, pdr.deletion_month, pdr.deletion_year);

            vector<pair<string, string>> fields;
            fields.push_back(make_pair("USER ID", toDisplayString(pdr.uid)));
            fields.push_back(make_pair("USERNAME", string(pdr.username)));
            fields.push_back(make_pair("DATE OF DELETION", toDisplayString(deletionDate)));
            fields.push_back(make_pair("REASON", string(pdr.reason)));
            displayRecord(fields, RECORD_BOX_WIDTH, 18); // 18 fits "DATE OF DELETION"

            found = true;
        }
        file.close();

        if (!found)
            throw NotFoundException("NO PERMANENTLY DELETED USERS ON RECORD.");
    }

    // ---- Mirrors updateTaskFlow()/updateGoalFlow()'s pattern: show every
    // user, pick one by username, then act on it. An active user can only be
    // suspended from here (soft delete, admin-side -- not self-restorable).
    // An inactive/suspended user can only be permanently deleted from here,
    // which is the admin-side counterpart to handleSuspendedAccountMenu()'s
    // self-service permanent deletion -- same reason picker, same audit
    // trail. ----
    void adminUpdateUserFlow()
    {
        if (users.empty())
            throw NotFoundException("NO USERS REGISTERED YET.");

        cout << "\n========== UPDATE USER ==========\n";
        for (int i = 0; i < (int)users.size(); i++)
            displayUserSummary(users[i]);

        string username = getNonEmptyLine("ENTER USERNAME TO UPDATE: ");
        User *user = auth.lookupUserByUsername(username);
        if (user == nullptr)
            throw NotFoundException("USER NOT FOUND: " + username);

        if (user->getIsActive())
        {
            cout << "\n1. Suspend Account\n";
            cout << "2. Cancel\n";
            int choice = getValidIntInRange("ENTER CHOICE: ", 1, 2);
            if (choice == 1)
            {
                user->setActive(false);
                // An admin suspension is never user-restorable -- only a self-delete is.
                user->setDeactivatedBySelf(false);
                rewriteUsersFile();
                cout << "USER " << username << " HAS BEEN SUSPENDED.\n";
            }
            else
            {
                cout << "UPDATE CANCELLED!\n";
            }
        }
        else
        {
            cout << "\n1. Permanently Delete Account\n";
            cout << "2. Cancel\n";
            int choice = getValidIntInRange("ENTER CHOICE: ", 1, 2);
            if (choice == 1)
            {
                if (!confirmAction("ARE YOU SURE YOU WANT TO PERMANENTLY DELETE THIS ACCOUNT? THIS CANNOT BE UNDONE."))
                {
                    cout << "PERMANENT DELETION CANCELLED.\n";
                    return;
                }
                string reason = selectDeletionReason();
                permanentlyDeleteUser(user, reason);
                cout << "USER " << username << " HAS BEEN PERMANENTLY DELETED.\n";
            }
            else
            {
                cout << "UPDATE CANCELLED!\n";
            }
        }
    }

    void adminViewStatistics()
    {
        int totalUsers = users.size();
        int activeUsers = 0, inactiveUsers = 0, suspendedUsers = 0;
        int totalGoalsCompleted = 0;

        for (int i = 0; i < totalUsers; i++)
        {
            if (users[i]->getIsActive())
                activeUsers++;
            else if (users[i]->getDeactivatedBySelf())
                inactiveUsers++;
            else
                suspendedUsers++;
            totalGoalsCompleted += users[i]->getGoalManager().countCompletedGoals();
        }

        cout << "\n========== SYSTEM STATISTICS ==========\n";
        cout << "TOTAL USERS              : " << totalUsers << endl;
        cout << "ACTIVE USERS             : " << activeUsers << endl;
        cout << "INACTIVE USERS (SELF)    : " << inactiveUsers << endl;
        cout << "SUSPENDED USERS (ADMIN)  : " << suspendedUsers << endl;
        cout << "TOTAL ACHIEVEMENTS       : " << achievements.size() << endl;
        cout << "DEFAULT ACHIEVEMENTS     : ";
        int defCount = 0;
        for (int i = 0; i < (int)achievements.size(); i++)
            if (achievements[i].getIsDefault())
                defCount++;
        cout << defCount << endl;
        cout << "CUSTOM ACHIEVEMENTS      : " << ((int)achievements.size() - defCount) << endl;
        cout << "TOTAL GOALS COMPLETED    : " << totalGoalsCompleted << endl;
        cout << "=======================================\n";
    }

    // ---- Exports a snapshot of the whole system to a .txt file: user counts
    // by account state (Active/Inactive/Suspended/Permanently Deleted) plus a
    // breakdown of why permanently deleted accounts were deleted. Mirrors
    // generateMonthlyReport()'s plain layout -- deliberately NO raw ANSI
    // escape codes here, since a .txt file is normally opened in Notepad/a
    // text editor rather than a terminal, and those show the escape codes
    // as literal garbled text instead of rendering color. ----
    void generateSystemReport()
    {
        string filename = "system_report.txt";
        ofstream report(filename.c_str());
        if (!report.is_open())
        {
            throw FileException("ERROR! COULD NOT CREATE REPORT FILE: " + filename);
        }

        int totalUsers = (int)users.size();
        int activeUsers = 0, inactiveUsers = 0, suspendedUsers = 0;
        for (int i = 0; i < totalUsers; i++)
        {
            if (users[i]->getIsActive())
                activeUsers++;
            else if (users[i]->getDeactivatedBySelf())
                inactiveUsers++; // self-deleted, restorable
            else
                suspendedUsers++; // admin-suspended, not restorable
        }

        // Tally permanently deleted accounts + their reasons straight from
        // the audit file (shared with displayDeletionReasonsChart()).
        int permanentlyDeletedCount = 0;
        map<string, int> reasonCounts = computeDeletionReasonCounts(permanentlyDeletedCount);

        report << "====================================================\n";
        report << "              REVOLIF - ADMIN SYSTEM REPORT\n";
        report << "====================================================\n";
        report << "GENERATED ON: " << Date::getToday() << "\n";
        report << "----------------------------------------------------\n\n";

        report << "USER ACCOUNTS\n";
        report << "  Total Users               : " << totalUsers << "\n";
        report << "  Active Users              : " << activeUsers << "\n";
        report << "  Inactive Users (Self)     : " << inactiveUsers << "\n";
        report << "  Suspended Users (Admin)   : " << suspendedUsers << "\n";
        report << "  Permanently Deleted       : " << permanentlyDeletedCount << "\n\n";

        report << "ACCOUNT DELETION REASONS\n";
        if (permanentlyDeletedCount == 0)
        {
            report << "  No permanently deleted accounts on record.\n";
        }
        else
        {
            for (int i = 0; i < (int)DELETION_REASONS.size(); i++)
            {
                const string &reason = DELETION_REASONS[i];
                report << "  " << left << setw(32) << reason << ": " << reasonCounts[reason] << "\n";
            }
        }

        report << "\n====================================================\n";

        report.close();
        cout << "SYSTEM REPORT GENERATED SUCCESSFULLY! SAVED AS: " << filename << "\n";
    }

    // ---- ACCOUNT DELETION REASONS CHART (on-screen colorful bar chart,
    // same visual style as ExpenseManager::displaySpendingChart) ----
    void displayDeletionReasonsChart()
    {
        int permanentlyDeletedCount = 0;
        map<string, int> reasonCounts = computeDeletionReasonCounts(permanentlyDeletedCount);

        if (permanentlyDeletedCount == 0)
        {
            cout << "NO PERMANENTLY DELETED ACCOUNTS ON RECORD!\n";
            return;
        }

        // Keep DELETION_REASONS order, but sort by count (highest first)
        // for the chart, same approach as the spending chart.
        vector<string> reasons = DELETION_REASONS;
        vector<int> counts((int)reasons.size());
        for (int i = 0; i < (int)reasons.size(); i++)
            counts[i] = reasonCounts[reasons[i]];

        vector<int> order((int)reasons.size());
        for (int i = 0; i < (int)order.size(); i++)
            order[i] = i;
        sort(order.begin(), order.end(), [&](int a, int b)
             { return counts[a] > counts[b]; });

        int maxVal = 0;
        for (int i = 0; i < (int)counts.size(); i++)
            if (counts[i] > maxVal)
                maxVal = counts[i];

        const int labelWidth = 32; // fits the longest predefined reason ("No longer using the application")
        const int maxBarLength = 25;
        const int barColWidth = maxBarLength + 2; // includes the [ and ] brackets
        const int countColWidth = 9;
        const int percentColWidth = 9; // includes the trailing '%'
        const int totalWidth = labelWidth + 1 + barColWidth + countColWidth + percentColWidth;

        cout << "\n" << makeTitleBar("ACCOUNT DELETION REASONS", totalWidth) << "\n";
        cout << left << setw(labelWidth) << "REASON" << " " << left << setw(barColWidth) << "SHARE"
             << right << setw(countColWidth) << "COUNT" << setw(percentColWidth) << "PERCENT" << "\n";
        cout << makeDivider(totalWidth) << "\n";

        // One fixed color per reason (cycles if there are ever more than 6),
        // instead of a percent-based color -- with only a handful of
        // reasons, percent-based buckets often collapsed to the same color.
        static const vector<string> REASON_COLORS = {
            "\033[31m", // red
            "\033[32m", // green
            "\033[33m", // yellow
            "\033[34m", // blue
            "\033[35m", // magenta
            "\033[36m"  // cyan
        };

        for (int k = 0; k < (int)order.size(); k++)
        {
            int i = order[k];
            int barLength = (maxVal > 0) ? (int)(((double)counts[i] / maxVal) * maxBarLength + 0.5) : 0;
            double percent = (permanentlyDeletedCount > 0)
                                 ? ((double)counts[i] / permanentlyDeletedCount) * 100.0
                                 : 0.0;

            string colorCode = REASON_COLORS[i % REASON_COLORS.size()];

            string filledBlocks;
            for (int b = 0; b < barLength; b++)
                filledBlocks += "\u2588";

            string bar = "[" + colorCode + filledBlocks + "\033[0m" + string(maxBarLength - barLength, '-') + "]";

            cout << left << setw(labelWidth) << truncateForColumn(reasons[i], labelWidth)
                 << " " << bar
                 << right << setw(countColWidth) << counts[i]
                 << setw(percentColWidth - 1) << fixed << setprecision(1) << percent << "%"
                 << "\n";
        }

        cout << makeDivider(totalWidth) << "\n";
        cout << left << setw(labelWidth) << "TOTAL" << " " << setw(barColWidth) << ""
             << right << setw(countColWidth) << permanentlyDeletedCount
             << setw(percentColWidth) << "100.0%\n";
        cout << makeDivider(totalWidth, '=') << "\n";
    }

    void adminAddAchievement()
    {
        cout << "\n========== ADD CUSTOM ACHIEVEMENT ==========\n";
        string name = getNonEmptyLine("ENTER ACHIEVEMENT NAME: ");
        string desc = getNonEmptyLine("ENTER DESCRIPTION: ");
        int reqGoals = getPositiveInt("ENTER REQUIRED GOALS TO UNLOCK: ");

        achievements.push_back(Achievement(name, desc, reqGoals, false));
        cout << "CUSTOM ACHIEVEMENT ADDED SUCCESSFULLY!\n";
    }

    void adminRemoveAchievement()
    {
        cout << "\n========== REMOVE ACHIEVEMENT ==========\n";
        cout << "Available achievements:\n";
        for (int i = 0; i < (int)achievements.size(); i++)
        {
            achievements[i].displayBrief();
        }

        int id = getValidInt("ENTER ACHIEVEMENT ID TO REMOVE: ");
        cin.ignore(1000, '\n');

        Achievement *ach = getAchievementByID(id);
        if (ach == nullptr)
        {
            throw NotFoundException("ACHIEVEMENT NOT FOUND WITH ID: " + to_string(id));
        }

        if (ach->getIsDefault())
        {
            throw ValidationException("CANNOT REMOVE DEFAULT ACHIEVEMENTS!");
        }

        if (!confirmAction("ARE YOU SURE YOU WANT TO REMOVE THIS ACHIEVEMENT?"))
        {
            cout << "REMOVAL CANCELLED!\n";
            return;
        }

        for (int i = 0; i < (int)achievements.size(); i++)
        {
            if (achievements[i].getAchievementID() == id)
            {
                achievements.erase(achievements.begin() + i);
                cout << "ACHIEVEMENT REMOVED SUCCESSFULLY!\n";
                return;
            }
        }
    }

    void adminUpdateAchievement()
    {
        cout << "\n========== UPDATE ACHIEVEMENT ==========\n";
        cout << "Available achievements:\n";
        for (int i = 0; i < (int)achievements.size(); i++)
        {
            achievements[i].displayBrief();
        }

        int id = getValidInt("ENTER ACHIEVEMENT ID TO UPDATE: ");
        cin.ignore(1000, '\n');

        Achievement *ach = getAchievementByID(id);
        if (ach == nullptr)
        {
            throw NotFoundException("ACHIEVEMENT NOT FOUND WITH ID: " + to_string(id));
        }

        if (ach->getIsDefault())
        {
            throw ValidationException("CANNOT UPDATE DEFAULT ACHIEVEMENTS!");
        }

        cout << "\n1. Update Name\n";
        cout << "2. Update Description\n";
        cout << "3. Update Required Goals\n";
        cout << "4. Cancel\n";
        int fieldChoice = getValidIntInRange("ENTER CHOICE: ", 1, 4);

        switch (fieldChoice)
        {
        case 1:
            ach->setName(getNonEmptyLine("ENTER NEW NAME: "));
            cout << "NAME UPDATED!\n";
            break;
        case 2:
            ach->setDescription(getNonEmptyLine("ENTER NEW DESCRIPTION: "));
            cout << "DESCRIPTION UPDATED!\n";
            break;
        case 3:
        {
            int req = getPositiveInt("ENTER NEW REQUIRED GOALS: ");
            ach->setRequiredGoals(req);
            cout << "REQUIREMENT UPDATED!\n";
            break;
        }
        case 4:
            cout << "UPDATE CANCELLED!\n";
            return;
        }
    }

    void adminViewAchievements()
    {
        cout << "\n========== ALL ACHIEVEMENTS ==========\n";
        for (int i = 0; i < (int)achievements.size(); i++)
        {
            achievements[i].displayAchievement();
        }
        if (achievements.empty())
            throw NotFoundException("NO ACHIEVEMENTS DEFINED.");
    }

    void adminMenu()
    {
        int choice;
        do
        {
            cout << "\n================ ADMIN MENU ================\n";
            cout << "1. User Management\n";
            cout << "2. Achievement Management\n";
            cout << "3. Reports & Statistics\n";
            cout << "4. Change Admin Password\n";
            cout << "5. Logout\n";
            choice = getValidIntInRange("ENTER CHOICE: ", 1, 5);

            try
            {
                switch (choice)
                {
                case 1:
                    adminUserMenu();
                    break;
                case 2:
                    adminAchievementMenu();
                    break;
                case 3:
                    adminReportsMenu();
                    break;
                case 4:
                    auth.changeAdminPassword();
                    rewriteAdminFile();
                    break;
                case 5:
                    cout << "LOGGING OUT...\n";
                    break;
                }
            }
            catch (NotFoundException &e)
            {
                cout << e.what() << endl;
            }
            catch (ValidationException &e)
            {
                cout << e.what() << endl;
            }
            catch (AuthException &e)
            {
                cout << e.what() << endl;
            }
            catch (UserException &e)
            {
                cout << e.what() << endl;
            }
            catch (FileException &e)
            {
                cout << e.what() << endl;
            }
        } while (choice != 5);
    }

    // ---- Groups every user search/view/update action in one place, all
    // following the same display-then-act pattern used by the user-side
    // task/goal/expense managers (e.g. taskSearchMenu(), updateTaskFlow()). ----
    void adminUserMenu()
    {
        int choice;
        do
        {
            cout << "\n================ USER MANAGEMENT ================\n";
            cout << "1. Search Users\n";
            cout << "2. Display All Users\n";
            cout << "3. Display Active Users\n";
            cout << "4. Display Inactive Users (Self-Deleted)\n";
            cout << "5. Display Suspended Users (Admin)\n";
            cout << "6. Display Permanently Deleted Users\n";
            cout << "7. Update User\n";
            cout << "8. Back\n";
            choice = getValidIntInRange("ENTER CHOICE: ", 1, 8);

            try
            {
                switch (choice)
                {
                case 1:
                    adminSearchUsersMenu();
                    break;
                case 2:
                    adminViewAllUsers();
                    break;
                case 3:
                    adminViewActiveUsers();
                    break;
                case 4:
                    adminViewInactiveUsers();
                    break;
                case 5:
                    adminViewSuspendedUsers();
                    break;
                case 6:
                    adminViewPermanentlyDeletedUsers();
                    break;
                case 7:
                    adminUpdateUserFlow();
                    break;
                case 8:
                    cout << "RETURNING TO ADMIN MENU...\n";
                    break;
                }
            }
            catch (NotFoundException &e)
            {
                cout << e.what() << endl;
            }
            catch (ValidationException &e)
            {
                cout << e.what() << endl;
            }
            catch (AuthException &e)
            {
                cout << e.what() << endl;
            }
            catch (UserException &e)
            {
                cout << e.what() << endl;
            }
            catch (FileException &e)
            {
                cout << e.what() << endl;
            }
        } while (choice != 8);
    }

    // ---- Groups every achievement management action in one place. ----
    void adminAchievementMenu()
    {
        int choice;
        do
        {
            cout << "\n================ ACHIEVEMENT MANAGEMENT ================\n";
            cout << "1. View All Achievements\n";
            cout << "2. Add Custom Achievement\n";
            cout << "3. Remove Custom Achievement\n";
            cout << "4. Update Custom Achievement\n";
            cout << "5. Back\n";
            choice = getValidIntInRange("ENTER CHOICE: ", 1, 5);

            try
            {
                switch (choice)
                {
                case 1:
                    adminViewAchievements();
                    break;
                case 2:
                    adminAddAchievement();
                    rewriteAchievementsFile();
                    break;
                case 3:
                    adminRemoveAchievement();
                    rewriteAchievementsFile();
                    break;
                case 4:
                    adminUpdateAchievement();
                    rewriteAchievementsFile();
                    break;
                case 5:
                    cout << "RETURNING TO ADMIN MENU...\n";
                    break;
                }
            }
            catch (NotFoundException &e)
            {
                cout << e.what() << endl;
            }
            catch (ValidationException &e)
            {
                cout << e.what() << endl;
            }
        } while (choice != 5);
    }

    // ---- Groups system-wide statistics and report generation. ----
    void adminReportsMenu()
    {
        int choice;
        do
        {
            cout << "\n================ REPORTS & STATISTICS ================\n";
            cout << "1. View System Statistics\n";
            cout << "2. View Account Deletion Reasons Chart\n";
            cout << "3. Generate System Report\n";
            cout << "4. Back\n";
            choice = getValidIntInRange("ENTER CHOICE: ", 1, 4);

            try
            {
                switch (choice)
                {
                case 1:
                    adminViewStatistics();
                    break;
                case 2:
                    displayDeletionReasonsChart();
                    break;
                case 3:
                    generateSystemReport();
                    break;
                case 4:
                    cout << "RETURNING TO ADMIN MENU...\n";
                    break;
                }
            }
            catch (FileException &e)
            {
                cout << e.what() << endl;
            }
        } while (choice != 4);
    }

    // ---- LIFE SCORE: a single composite score of overall progress ----
    int calculateLifeScore(User &user)
    {
        TaskManager &tm = user.getTaskManager();
        GoalManager &gm = user.getGoalManager();

        int totalTasks = tm.countPendingTasks() + tm.countCompletedTasks();
        double taskScore = (totalTasks > 0)
                               ? ((double)tm.countCompletedTasks() / totalTasks) * 100.0
                               : 100.0;

        int totalGoals = gm.countCompletedGoals() + gm.countPendingGoals() + gm.countOverdueGoals();
        double goalScore = (totalGoals > 0)
                               ? ((double)gm.countCompletedGoals() / totalGoals) * 100.0
                               : 100.0;

        double overduePenalty = (tm.countOverdueTasks() * 5.0) + (gm.countOverdueGoals() * 5.0);

        double rawScore = (taskScore * 0.4) + (goalScore * 0.4) + 20.0 - overduePenalty;

        if (rawScore > 100)
            rawScore = 100;
        if (rawScore < 0)
            rawScore = 0;

        return (int)rawScore;
    }

    string getLifeScoreLabel(int score)
    {
        if (score >= 90)
            return "Outstanding";
        if (score >= 75)
            return "Excellent";
        if (score >= 60)
            return "Good";
        if (score >= 40)
            return "Fair";
        return "Needs Improvement";
    }

    // ---- Exports a snapshot report of the user's progress to a .txt file ----
    void generateMonthlyReport(User &user)
    {
        string filename = user.getUsername() + "_report.txt";
        ofstream report(filename.c_str());
        if (!report.is_open())
        {
            throw FileException("ERROR! COULD NOT CREATE REPORT FILE: " + filename);
        }

        TaskManager &tm = user.getTaskManager();
        GoalManager &gm = user.getGoalManager();
        ExpenseManager &em = user.getExpenseManager();

        report << "====================================================\n";
        report << "                 REVOLIF - MONTHLY REPORT\n";
        report << "====================================================\n";
        report << "USER        : " << user.getDisplayName() << "\n";
        report << "GENERATED ON: " << Date::getToday() << "\n";
        report << "----------------------------------------------------\n\n";

        report << "TASKS\n";
        report << "  Pending   : " << tm.countPendingTasks() << "\n";
        report << "  Completed : " << tm.countCompletedTasks() << "\n";
        report << "  Overdue   : " << tm.countOverdueTasks() << "\n\n";

        report << "GOALS\n";
        report << "  Completed : " << gm.countCompletedGoals() << "\n";
        report << "  Pending   : " << gm.countPendingGoals() << "\n";
        report << "  Overdue   : " << gm.countOverdueGoals() << "\n\n";

        report << "EXPENSES\n";
        report << "  Total Spent      : " << fixed << setprecision(2) << em.calculateTotalExpense() << "\n";
        pair<string, double> topCat = em.getTopCategoryInfo();
        if (!topCat.first.empty())
            report << "  Highest Category : " << topCat.first << " (" << topCat.second << ")\n";

        report << "\nSTREAK\n";
        report << "  Current Streak : " << user.getCurrentStreak() << " day(s)\n";
        report << "  Best Streak    : " << user.getBestStreak() << " day(s)\n";

        int lifeScore = calculateLifeScore(user);
        report << "\nLIFE SCORE: " << lifeScore << "/100 (" << getLifeScoreLabel(lifeScore) << ")\n";
        report << "====================================================\n";

        report.close();
        cout << "REPORT GENERATED SUCCESSFULLY! SAVED AS: " << filename << "\n";
    }

    void showDashboard(User &user)
    {
        TaskManager &tm = user.getTaskManager();
        GoalManager &gm = user.getGoalManager();
        ExpenseManager &em = user.getExpenseManager();

        cout << "\n================ YOUR DASHBOARD ================\n";
        cout << "WELCOME BACK, " << user.getDisplayName() << "\n";

        int overdueTasks = tm.countOverdueTasks();
        int dueSoonTasks = tm.countDueSoonTasks(3);
        int pendingTasks = tm.countPendingTasks();
        int completedTasks = tm.countCompletedTasks();

        cout << "\nTASKS\n";
        cout << "  - " << overdueTasks << " OVERDUE, "
             << dueSoonTasks << " DUE WITHIN THE NEXT 3 DAYS\n";
        cout << "  - " << pendingTasks << " PENDING TOTAL, "
             << completedTasks << " COMPLETED TOTAL\n";

        int overdueGoals = gm.countOverdueGoals();
        int pendingGoals = gm.countPendingGoals();
        int completedGoals = gm.countCompletedGoals();

        cout << "\nGOALS\n";
        cout << "  - " << overdueGoals << " OVERDUE, "
             << pendingGoals << " PENDING\n";
        cout << "  - " << completedGoals << " COMPLETED LIFETIME";

        string nextTitle = gm.getNextTitleName();
        if (!nextTitle.empty())
        {
            int remaining = gm.goalsUntilNextTitle();
            cout << " -- " << remaining << " MORE TO REACH \""
                 << nextTitle << "\"";
        }
        else
        {
            cout << " -- MAXIMUM TITLE ALREADY UNLOCKED!";
        }
        cout << "\n";

        double totalSpent = em.calculateTotalExpense();
        cout << "\nEXPENSES\n";
        cout << "  - TOTAL SPENT: " << totalSpent << "\n";

        pair<string, double> topCategory = em.getTopCategoryInfo();
        if (!topCategory.first.empty())
        {
            cout << "  - HIGHEST CATEGORY: " << topCategory.first
                 << " (" << topCategory.second << ")\n";
        }

        if (!user.getTitle().empty())
        {
            cout << "\nRECENTLY EARNED TITLE: " << user.getTitle() << "\n";
        }

        int lifeScore = calculateLifeScore(user);
        cout << "\nLIFE SCORE: " << lifeScore << "/100 (" << getLifeScoreLabel(lifeScore) << ")\n";
        cout << "LOGIN STREAK: " << user.getCurrentStreak() << " day(s) (Best: "
             << user.getBestStreak() << ")\n";

        cout << "==================================================\n";

        if (pendingTasks > 0)
        {
            cout << "\nFOLLOWING ARE YOUR PENDING TASKS:\n";
            tm.displayPendingTasks();
        }
    }

    void userMenu(User &user)
    {
        int choice;
        do
        {
            cout << "\n================ USER MENU (" << user.getDisplayName() << ") ================\n";
            cout << "1. View Profile\n";
            cout << "2. Update Profile\n";
            cout << "3. Task Manager\n";
            cout << "4. Goal Manager\n";
            cout << "5. Expense Manager\n";
            cout << "6. View Achievements\n";
            cout << "7. Choose Display Achievement\n";
            cout << "8. Change Password\n";
            cout << "9. Generate Monthly Report\n";
            cout << "10. Delete My Account\n";
            cout << "11. Logout\n";
            choice = getValidIntInRange("ENTER CHOICE: ", 1, 11);

            try
            {
                switch (choice)
                {
                case 1:
                    user.displayProfile();
                    break;
                case 2:
                    updateProfileFlow(user);
                    rewriteUsersFile();
                    break;
                case 3:
                    taskMenu(user);
                    break;
                case 4:
                    goalMenu(user);
                    break;
                case 5:
                    expenseMenu(user);
                    break;
                case 6:
                    viewUserAchievements(&user);
                    break;
                case 7:
                    chooseDisplayAchievement(&user);
                    break;
                case 8:
                    auth.changeUserPassword(user);
                    rewriteUsersFile();
                    break;
                case 9:
                    generateMonthlyReport(user);
                    break;
                case 10:
                    if (deleteMyAccountFlow(user))
                        choice = 11; // account deleted -- log out immediately
                    break;
                case 11:
                    cout << "LOGGING OUT...\n";
                    break;
                }
            }
            catch (NotFoundException &e)
            {
                cout << e.what() << endl;
            }
            catch (ValidationException &e)
            {
                cout << e.what() << endl;
            }
            catch (AuthException &e)
            {
                cout << e.what() << endl;
            }
            catch (UserException &e)
            {
                cout << e.what() << endl;
            }
            catch (FileException &e)
            {
                cout << e.what() << endl;
            }
        } while (choice != 11);
    }

    void taskMenu(User &user)
    {
        int choice;
        do
        {
            cout << "\n================ TASK MANAGER ================\n";
            cout << "1. Add Task\n";
            cout << "2. Display All Tasks\n";
            cout << "3. Display Completed Tasks\n";
            cout << "4. Display Pending Tasks (Sorted By Priority)\n";
            cout << "5. Search Task\n";
            cout << "6. Mark Task As Completed\n";
            cout << "7. Update Task\n";
            cout << "8. Delete Task\n";
            cout << "9. Back\n";
            choice = getValidIntInRange("ENTER CHOICE: ", 1, 9);

            try
            {
                switch (choice)
                {
                case 1:
                {
                    Task *t = user.getTaskManager().addTask();
                    if (t)
                        saveTaskToFile(t, user.getUID());
                }
                break;
                case 2:
                    user.getTaskManager().displayAllTasks();
                    break;
                case 3:
                    user.getTaskManager().displayCompletedTasks();
                    break;
                case 4:
                    user.getTaskManager().displayPendingTasks();
                    break;
                case 5:
                    taskSearchMenu(user);
                    break;
                case 6:
                    user.getTaskManager().completeTaskFlow();
                    rewriteTasksFile();
                    break;
                case 7:
                    user.getTaskManager().updateTaskFlow();
                    rewriteTasksFile();
                    break;
                case 8:
                    user.getTaskManager().deleteTaskFlow();
                    rewriteTasksFile();
                    break;
                case 9:
                    cout << "RETURNING TO USER MENU...\n";
                    break;
                }
            }
            catch (NotFoundException &e)
            {
                cout << e.what() << endl;
            }
            catch (ValidationException &e)
            {
                cout << e.what() << endl;
            }
        } while (choice != 9);
    }

    void taskSearchMenu(User &user)
    {
        if (user.getTaskManager().getTasks().empty())
            throw NotFoundException("NO TASKS AVAILABLE!");

        cout << "\n========== SEARCH TASK ==========\n";
        cout << "1. Search By ID\n";
        cout << "2. Search By Title\n";
        cout << "3. Search By Category\n";
        cout << "4. Search By Deadline\n";
        cout << "5. Search By Status\n";
        cout << "6. Back\n";
        int searchChoice = getValidIntInRange("ENTER CHOICE: ", 1, 6);

        switch (searchChoice)
        {
        case 6:
            cout << "RETURNING TO TASK MANAGER...\n";
            return;
        case 1:
        {
            int id = getValidInt("ENTER TASK ID: ");
            Task *task = user.getTaskManager().searchByID(id);
            if (task)
                task->displayTask();
            else
                throw NotFoundException("TASK NOT FOUND WITH ID: " + to_string(id));
            break;
        }
        case 2:
        {
            string title = getNonEmptyLine("ENTER TITLE: ");
            vector<Task *> result = user.getTaskManager().searchByTitle(title);
            user.getTaskManager().displaySearchResults(result);
            break;
        }
        case 3:
        {
            string category = getNonEmptyLine("ENTER CATEGORY (Academic/Daily): ");
            vector<Task *> result = user.getTaskManager().searchByCategory(category);
            user.getTaskManager().displaySearchResults(result);
            break;
        }
        case 4:
        {
            Date date;
            cout << "ENTER DEADLINE:\n";
            date.inputDate();
            vector<Task *> result = user.getTaskManager().searchByDeadline(date);
            user.getTaskManager().displaySearchResults(result);
            break;
        }
        case 5:
        {
            string status = getNonEmptyLine("ENTER STATUS (Pending/Completed): ");
            vector<Task *> result = user.getTaskManager().searchByStatus(status);
            user.getTaskManager().displaySearchResults(result);
            break;
        }
        }
    }

    void goalMenu(User &user)
    {
        int choice;
        do
        {
            cout << "\n================ GOAL MANAGER ================\n";
            cout << "1. Add Goal\n";
            cout << "2. Display All Goals\n";
            cout << "3. Display Completed Goals\n";
            cout << "4. Display Pending/Overdue Goals\n";
            cout << "5. Search Goal\n";
            cout << "6. Mark Goal As Completed\n";
            cout << "7. View Titles (Old System)\n";
            cout << "8. Update Goal\n";
            cout << "9. Delete Goal\n";
            cout << "10. Back\n";
            choice = getValidIntInRange("ENTER CHOICE: ", 1, 10);

            try
            {
                switch (choice)
                {
                case 1:
                {
                    Goal *g = user.getGoalManager().addGoal();
                    if (g)
                        saveGoalToFile(g, user.getUID());
                }
                break;
                case 2:
                    user.getGoalManager().displayAllGoals();
                    break;
                case 3:
                    user.getGoalManager().displayCompletedGoals();
                    break;
                case 4:
                    user.getGoalManager().displayIncompleteGoals();
                    break;
                case 5:
                    goalSearchMenu(user);
                    break;
                case 6:
                {
                    bool completed = user.getGoalManager().completeGoalFlow();
                    if (completed)
                    {
                        user.checkAndUpdateTitle();
                        checkAchievements(&user);
                        rewriteGoalsFile();
                        rewriteUsersFile();
                    }
                    break;
                }
                case 7:
                    user.getGoalManager().viewAchievements();
                    break;
                case 8:
                    user.getGoalManager().updateGoalFlow();
                    rewriteGoalsFile();
                    break;
                case 9:
                    user.getGoalManager().deleteGoalFlow();
                    rewriteGoalsFile();
                    break;
                case 10:
                    cout << "RETURNING TO USER MENU...\n";
                    break;
                }
            }
            catch (NotFoundException &e)
            {
                cout << e.what() << endl;
            }
            catch (ValidationException &e)
            {
                cout << e.what() << endl;
            }
        } while (choice != 10);
    }

    void goalSearchMenu(User &user)
    {
        if (user.getGoalManager().getGoals().empty())
            throw NotFoundException("NO GOALS AVAILABLE!");

        cout << "\n========== SEARCH GOAL ==========\n";
        cout << "1. Search By ID\n";
        cout << "2. Search By Title\n";
        cout << "3. Search By Category\n";
        cout << "4. Search By Deadline\n";
        cout << "5. Search By Status\n";
        cout << "6. Back\n";
        int searchChoice = getValidIntInRange("ENTER CHOICE: ", 1, 6);

        switch (searchChoice)
        {
        case 6:
            cout << "RETURNING TO GOAL MANAGER...\n";
            return;
        case 1:
        {
            int id = getValidInt("ENTER GOAL ID: ");
            Goal *goal = user.getGoalManager().searchByID(id);
            if (goal)
                goal->displayGoal();
            else
                throw NotFoundException("GOAL NOT FOUND WITH ID: " + to_string(id));
            break;
        }
        case 2:
        {
            string title = getNonEmptyLine("ENTER TITLE: ");
            vector<Goal *> result = user.getGoalManager().searchByTitle(title);
            user.getGoalManager().displaySearchResults(result);
            break;
        }
        case 3:
        {
            string category = getNonEmptyLine("ENTER CATEGORY: ");
            vector<Goal *> result = user.getGoalManager().searchByCategory(category);
            user.getGoalManager().displaySearchResults(result);
            break;
        }
        case 4:
        {
            Date date;
            cout << "ENTER DEADLINE:\n";
            date.inputDate();
            vector<Goal *> result = user.getGoalManager().searchByDeadline(date);
            user.getGoalManager().displaySearchResults(result);
            break;
        }
        case 5:
        {
            string status = getNonEmptyLine("ENTER STATUS (Incomplete/Completed): ");
            vector<Goal *> result = user.getGoalManager().searchByStatus(status);
            user.getGoalManager().displaySearchResults(result);
            break;
        }
        }
    }

    void expenseMenu(User &user)
    {
        int choice;
        do
        {
            cout << "\n================ EXPENSE MANAGER ================\n";
            cout << "1. Add Expense\n";
            cout << "2. Display All Expenses\n";
            cout << "3. Search Expense\n";
            cout << "4. Update Expense\n";
            cout << "5. Delete Expense\n";
            cout << "6. Show Total Expense\n";
            cout << "7. Set Category Budget\n";
            cout << "8. View Budgets\n";
            cout << "9. View Spending Chart\n";
            cout << "10. Back\n";
            choice = getValidIntInRange("ENTER CHOICE: ", 1, 10);

            try
            {
                switch (choice)
                {
                case 1:
                {
                    Expense *e = user.getExpenseManager().addExpense();
                    if (e)
                        saveExpenseToFile(e, user.getUID());
                }
                break;
                case 2:
                    user.getExpenseManager().displayAllExpenses();
                    break;
                case 3:
                    expenseSearchMenu(user);
                    break;
                case 4:
                    user.getExpenseManager().updateExpenseFlow();
                    rewriteExpensesFile();
                    break;
                case 5:
                    user.getExpenseManager().deleteExpenseFlow();
                    rewriteExpensesFile();
                    break;
                case 6:
                    cout << "TOTAL EXPENSE: " << user.getExpenseManager().calculateTotalExpense() << endl;
                    break;
                case 7:
                    user.getExpenseManager().setBudget();
                    break;
                case 8:
                    user.getExpenseManager().viewBudgets();
                    break;
                case 9:
                    user.getExpenseManager().displaySpendingChart();
                    break;
                case 10:
                    cout << "RETURNING TO USER MENU...\n";
                    break;
                }
            }
            catch (NotFoundException &e)
            {
                cout << e.what() << endl;
            }
            catch (ValidationException &e)
            {
                cout << e.what() << endl;
            }
        } while (choice != 10);
    }

    void expenseSearchMenu(User &user)
    {
        if (user.getExpenseManager().getExpenses().empty())
            throw NotFoundException("NO EXPENSES AVAILABLE!");

        cout << "\n========== SEARCH EXPENSE ==========\n";
        cout << "1. Search By ID\n";
        cout << "2. Search By Title\n";
        cout << "3. Search By Category\n";
        cout << "4. Search By Date\n";
        cout << "5. Back\n";
        int searchChoice = getValidIntInRange("ENTER CHOICE: ", 1, 5);

        switch (searchChoice)
        {
        case 5:
            cout << "RETURNING TO EXPENSE MANAGER...\n";
            return;
        case 1:
        {
            int id = getValidInt("ENTER EXPENSE ID: ");
            Expense *expense = user.getExpenseManager().searchByID(id);
            if (expense)
                expense->displayExpense();
            else
                throw NotFoundException("EXPENSE NOT FOUND WITH ID: " + to_string(id));
            break;
        }
        case 2:
        {
            string title = getNonEmptyLine("ENTER TITLE: ");
            vector<Expense *> result = user.getExpenseManager().searchByTitle(title);
            user.getExpenseManager().displaySearchResults(result);
            break;
        }
        case 3:
        {
            string category = getNonEmptyLine("ENTER CATEGORY: ");
            vector<Expense *> result = user.getExpenseManager().searchByCategory(category);
            user.getExpenseManager().displaySearchResults(result);
            break;
        }
        case 4:
        {
            Date date;
            cout << "ENTER DATE:\n";
            date.inputDate();
            vector<Expense *> result = user.getExpenseManager().searchByDate(date);
            user.getExpenseManager().displaySearchResults(result);
            break;
        }
        }
    }

    // ---- FILE I/O ----
    void loadAllData();
    void saveUserToFile(User *u);
    void loadUsersFromFile();
    void rewriteUsersFile();
    void saveTaskToFile(Task *t, int userID);
    void loadTasksFromFile();
    void rewriteTasksFile();
    void saveExpenseToFile(Expense *e, int userID);
    void loadExpensesFromFile();
    void rewriteExpensesFile();
    void saveGoalToFile(Goal *g, int userID);
    void loadGoalsFromFile();
    void rewriteGoalsFile();
    void saveActivityToFile(ActivityLog *a, int userID);
    void loadActivityLogFromFile();
    void rewriteActivityLogFile();

    // ---- GUI helper: records one entry in the user's activity log (both
    // in memory and appended to activity_log.dat) and returns it. Called
    // right alongside the existing save*ToFile() calls whenever the GUI
    // adds/completes a task, completes a goal, or logs an expense. ----
    void logActivity(User *user, const string &type, const string &title, const string &category)
    {
        if (!user)
            return;
        time_t now = time(nullptr);
        tm *ltm = localtime(&now);
        Time nowTime(ltm->tm_hour == 0 ? 12 : (ltm->tm_hour > 12 ? ltm->tm_hour - 12 : ltm->tm_hour),
                     ltm->tm_min,
                     ltm->tm_hour < 12 ? "AM" : "PM");
        ActivityLog *entry = user->getActivityLogManager().addEntry(type, title, category, Date::getToday(), nowTime);
        saveActivityToFile(entry, user->getUID());
    }
    void saveAchievementToFile(Achievement &a);
    void loadAchievementsFromFile();
    void rewriteAchievementsFile();
    void saveAdminToFile();
    void loadAdminFromFile();
    void rewriteAdminFile();
    void savePermanentDeletedUserToFile(const PermanentDeletedUserRecord &pdr);

    void mainMenu()
    {
        int choice;
        do
        {
            cout << "\n================================================\n";
            cout << "                    R E V O L I F\n";
            cout << "              Life , Beautifully  Aligned\n";
            cout << "==================================================\n";
            cout << "1. Login\n";
            cout << "2. Register\n";
            cout << "3. Exit\n";
            choice = getValidIntInRange("ENTER CHOICE: ", 1, 3);

            try
            {
                switch (choice)
                {
                case 1:
                    loginFlow();
                    break;
                case 2:
                {
                    bool isReusedAccount = false;
                    User *newUser = auth.registerUser(isReusedAccount);
                    if (newUser)
                    {
                        if (isReusedAccount)
                        {
                            // The record already exists on disk -- rewrite it
                            // (and every file that could still hold the old
                            // account's data) instead of appending a duplicate.
                            rewriteUsersFile();
                            rewriteTasksFile();
                            rewriteGoalsFile();
                            rewriteExpensesFile();
                        }
                        else
                        {
                            saveUserToFile(newUser);
                        }
                    }
                }
                break;
                case 3:
                    cout << "EXITING REVOLIF...\n";
                    break;
                }
            }
            catch (UserException &e)
            {
                cout << e.what() << endl;
            }
            catch (AuthException &e)
            {
                cout << e.what() << endl;
            }
            catch (ValidationException &e)
            {
                cout << e.what() << endl;
            }
            catch (NotFoundException &e)
            {
                cout << e.what() << endl;
            }
            catch (FileException &e)
            {
                cout << e.what() << endl;
            }
        } while (choice != 3);
    }
};

const vector<string> System::DELETION_REASONS = {
    "No longer using the application",
    "Switching to another app",
    "Privacy concerns",
    "Too many notifications",
    "Account issues/problems",
    "Other"};

// ============================================================
//                  FILE I/O IMPLEMENTATIONS
// ============================================================

void System::saveUserToFile(User *u)
{
    ofstream file("users.dat", ios::binary | ios::app);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN users.dat FOR WRITING!");

    UserRecord ur = {};
    ur.uid = u->getUID();
    strncpy(ur.username, u->getUsername().c_str(), sizeof(ur.username) - 1);
    strncpy(ur.name, u->getName().c_str(), sizeof(ur.name) - 1);
    ur.dob_day = u->getDOB().getDay();
    ur.dob_month = u->getDOB().getMonth();
    ur.dob_year = u->getDOB().getYear();
    strncpy(ur.password_hash, u->getPasswordHash().c_str(), sizeof(ur.password_hash) - 1);
    strncpy(ur.email, u->getEmail().c_str(), sizeof(ur.email) - 1);
    ur.reg_day = u->getRegistrationDate().getDay();
    ur.reg_month = u->getRegistrationDate().getMonth();
    ur.reg_year = u->getRegistrationDate().getYear();
    ur.lastLogin_day = u->getLastLogin().getDay();
    ur.lastLogin_month = u->getLastLogin().getMonth();
    ur.lastLogin_year = u->getLastLogin().getYear();
    ur.isActive = u->getIsActive();
    ur.deactivatedBySelf = u->getDeactivatedBySelf();
    ur.currentStreak = u->getCurrentStreak();
    ur.bestStreak = u->getBestStreak();
    strncpy(ur.title, u->getTitle().c_str(), sizeof(ur.title) - 1);
    ur.displayedAchievementID = u->getDisplayedAchievementID();
    vector<int> unlocked = u->getUnlockedAchievementIDs();
    ur.unlockedCount = (int)unlocked.size();
    for (int i = 0; i < ur.unlockedCount && i < 100; i++)
        ur.unlockedIDs[i] = unlocked[i];
    ur.welcomeEmailSent = u->getWelcomeEmailSent();
    ur.lastSummaryEmail_day = u->getLastSummaryEmailDate().getDay();
    ur.lastSummaryEmail_month = u->getLastSummaryEmailDate().getMonth();
    ur.lastSummaryEmail_year = u->getLastSummaryEmailDate().getYear();

    file.write((char *)&ur, sizeof(ur));
    file.close();
}

void System::loadUsersFromFile()
{
    ifstream file("users.dat", ios::binary);
    if (!file)
        return;

    UserRecord ur;
    int maxUID = 0;

    while (file.read((char *)&ur, sizeof(ur)))
    {
        Date dob(ur.dob_day, ur.dob_month, ur.dob_year);
        User *u = new User(ur.name, ur.username, dob, "", ur.email);
        u->setUID(ur.uid);
        u->setPasswordHash(string(ur.password_hash));
        u->setActive(ur.isActive);
        u->setDeactivatedBySelf(ur.deactivatedBySelf);
        u->setLastLogin(Date(ur.lastLogin_day, ur.lastLogin_month, ur.lastLogin_year));
        u->setCurrentStreak(ur.currentStreak);
        u->setBestStreak(ur.bestStreak);
        u->setTitle(string(ur.title));
        u->setDisplayedAchievementID(ur.displayedAchievementID);
        for (int k = 0; k < ur.unlockedCount && k < 100; k++)
            u->addUnlockedAchievement(ur.unlockedIDs[k]);
        u->setWelcomeEmailSent(ur.welcomeEmailSent);
        u->setLastSummaryEmailDate(Date(ur.lastSummaryEmail_day, ur.lastSummaryEmail_month, ur.lastSummaryEmail_year));
        users.push_back(u);
        if (ur.uid > maxUID)
            maxUID = ur.uid;
    }
    file.close();
    User::setNextID(maxUID + 1);
}

void System::rewriteUsersFile()
{
    ofstream file("users.dat", ios::binary);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN users.dat FOR REWRITING!");

    for (int i = 0; i < (int)users.size(); i++)
    {
        User *u = users[i];
        UserRecord ur = {};
        ur.uid = u->getUID();
        strncpy(ur.username, u->getUsername().c_str(), sizeof(ur.username) - 1);
        strncpy(ur.name, u->getName().c_str(), sizeof(ur.name) - 1);
        ur.dob_day = u->getDOB().getDay();
        ur.dob_month = u->getDOB().getMonth();
        ur.dob_year = u->getDOB().getYear();
        strncpy(ur.password_hash, u->getPasswordHash().c_str(), sizeof(ur.password_hash) - 1);
        strncpy(ur.email, u->getEmail().c_str(), sizeof(ur.email) - 1);
        ur.reg_day = u->getRegistrationDate().getDay();
        ur.reg_month = u->getRegistrationDate().getMonth();
        ur.reg_year = u->getRegistrationDate().getYear();
        ur.lastLogin_day = u->getLastLogin().getDay();
        ur.lastLogin_month = u->getLastLogin().getMonth();
        ur.lastLogin_year = u->getLastLogin().getYear();
        ur.isActive = u->getIsActive();
        ur.deactivatedBySelf = u->getDeactivatedBySelf();
        ur.currentStreak = u->getCurrentStreak();
        ur.bestStreak = u->getBestStreak();
        strncpy(ur.title, u->getTitle().c_str(), sizeof(ur.title) - 1);
        ur.displayedAchievementID = u->getDisplayedAchievementID();
        vector<int> unlocked = u->getUnlockedAchievementIDs();
        ur.unlockedCount = (int)unlocked.size();
        for (int j = 0; j < ur.unlockedCount && j < 100; j++)
            ur.unlockedIDs[j] = unlocked[j];
        ur.welcomeEmailSent = u->getWelcomeEmailSent();
        ur.lastSummaryEmail_day = u->getLastSummaryEmailDate().getDay();
        ur.lastSummaryEmail_month = u->getLastSummaryEmailDate().getMonth();
        ur.lastSummaryEmail_year = u->getLastSummaryEmailDate().getYear();

        file.write((char *)&ur, sizeof(ur));
    }
    file.close();
}

void System::saveTaskToFile(Task *t, int userID)
{
    ofstream file("tasks.dat", ios::binary | ios::app);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN tasks.dat FOR WRITING!");

    TaskRecord tr = {};
    tr.taskID = t->getTaskID();
    tr.userID = userID;
    strncpy(tr.title, t->getTitle().c_str(), sizeof(tr.title) - 1);
    strncpy(tr.description, t->getDescription().c_str(), sizeof(tr.description) - 1);
    tr.deadline_day = t->getDeadline().getDay();
    tr.deadline_month = t->getDeadline().getMonth();
    tr.deadline_year = t->getDeadline().getYear();
    tr.deadline_hour = t->getDeadlineTime().getHour();
    tr.deadline_minute = t->getDeadlineTime().getMinute();
    strncpy(tr.deadline_meridiem, t->getDeadlineTime().getMeridiem().c_str(), sizeof(tr.deadline_meridiem) - 1);
    strncpy(tr.category, t->getCategory().c_str(), sizeof(tr.category) - 1);
    strncpy(tr.status, t->getStatus().c_str(), sizeof(tr.status) - 1);
    strncpy(tr.priority, t->getPriority().c_str(), sizeof(tr.priority) - 1);
    tr.isRecurring = t->getIsRecurring();
    strncpy(tr.recurrenceInterval, t->getRecurrenceInterval().c_str(), sizeof(tr.recurrenceInterval) - 1);
    tr.taskType = t->getTaskType();

    file.write((char *)&tr, sizeof(tr));
    file.close();
}

void System::loadTasksFromFile()
{
    ifstream file("tasks.dat", ios::binary);
    if (!file)
        return;

    TaskRecord tr;
    int maxTaskID = 0;

    while (file.read((char *)&tr, sizeof(tr)))
    {
        Date deadline(tr.deadline_day, tr.deadline_month, tr.deadline_year);
        Time deadlineTime(tr.deadline_hour, tr.deadline_minute, string(tr.deadline_meridiem));
        string title(tr.title);
        string desc(tr.description);

        Task *task;
        if (tr.taskType == 1)
            task = new AcademicTask(title, desc, deadline, deadlineTime);
        else
            task = new DailyTask(title, desc, deadline, deadlineTime);

        task->setTaskID(tr.taskID);
        task->setPriority(string(tr.priority));
        task->setRecurring(tr.isRecurring, string(tr.recurrenceInterval));
        if (string(tr.status) == "Completed")
            task->markCompleted();
        else
            task->markPending();

        for (int i = 0; i < (int)users.size(); i++)
        {
            if (users[i]->getUID() == tr.userID)
            {
                users[i]->getTaskManager().loadTask(task);
                break;
            }
        }

        if (tr.taskID > maxTaskID)
            maxTaskID = tr.taskID;
    }
    file.close();
    Task::setNextTaskID(maxTaskID + 1);
}

void System::rewriteTasksFile()
{
    ofstream file("tasks.dat", ios::binary);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN tasks.dat FOR REWRITING!");

    for (int i = 0; i < (int)users.size(); i++)
    {
        int uid = users[i]->getUID();
        const vector<Task *> &tasks = users[i]->getTaskManager().getTasks();
        for (int j = 0; j < (int)tasks.size(); j++)
        {
            Task *t = tasks[j];
            TaskRecord tr = {};
            tr.taskID = t->getTaskID();
            tr.userID = uid;
            strncpy(tr.title, t->getTitle().c_str(), sizeof(tr.title) - 1);
            strncpy(tr.description, t->getDescription().c_str(), sizeof(tr.description) - 1);
            tr.deadline_day = t->getDeadline().getDay();
            tr.deadline_month = t->getDeadline().getMonth();
            tr.deadline_year = t->getDeadline().getYear();
            tr.deadline_hour = t->getDeadlineTime().getHour();
            tr.deadline_minute = t->getDeadlineTime().getMinute();
            strncpy(tr.deadline_meridiem, t->getDeadlineTime().getMeridiem().c_str(), sizeof(tr.deadline_meridiem) - 1);
            strncpy(tr.category, t->getCategory().c_str(), sizeof(tr.category) - 1);
            strncpy(tr.status, t->getStatus().c_str(), sizeof(tr.status) - 1);
            strncpy(tr.priority, t->getPriority().c_str(), sizeof(tr.priority) - 1);
            tr.isRecurring = t->getIsRecurring();
            strncpy(tr.recurrenceInterval, t->getRecurrenceInterval().c_str(), sizeof(tr.recurrenceInterval) - 1);
            tr.taskType = t->getTaskType();

            file.write((char *)&tr, sizeof(tr));
        }
    }
    file.close();
}

void System::saveExpenseToFile(Expense *e, int userID)
{
    ofstream file("expenses.dat", ios::binary | ios::app);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN expenses.dat FOR WRITING!");

    ExpenseRecord er = {};
    er.expenseID = e->getExpenseID();
    er.userID = userID;
    strncpy(er.title, e->getTitle().c_str(), sizeof(er.title) - 1);
    er.amount = e->getAmount();
    strncpy(er.category, e->getCategory().c_str(), sizeof(er.category) - 1);
    er.date_day = e->getDate().getDay();
    er.date_month = e->getDate().getMonth();
    er.date_year = e->getDate().getYear();
    strncpy(er.description, e->getDescription().c_str(), sizeof(er.description) - 1);

    file.write((char *)&er, sizeof(er));
    file.close();
}

void System::loadExpensesFromFile()
{
    ifstream file("expenses.dat", ios::binary);
    if (!file)
        return;

    ExpenseRecord er;
    int maxExpenseID = 0;

    while (file.read((char *)&er, sizeof(er)))
    {
        Date date(er.date_day, er.date_month, er.date_year);
        Expense *expense = new Expense(string(er.title), er.amount, string(er.category), date, string(er.description));
        expense->setExpenseID(er.expenseID);

        for (int i = 0; i < (int)users.size(); i++)
        {
            if (users[i]->getUID() == er.userID)
            {
                users[i]->getExpenseManager().loadExpense(expense);
                break;
            }
        }

        if (er.expenseID > maxExpenseID)
            maxExpenseID = er.expenseID;
    }
    file.close();
    Expense::setNextExpenseID(maxExpenseID + 1);
}

void System::rewriteExpensesFile()
{
    ofstream file("expenses.dat", ios::binary);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN expenses.dat FOR REWRITING!");

    for (int i = 0; i < (int)users.size(); i++)
    {
        int uid = users[i]->getUID();
        const vector<Expense *> &expenses = users[i]->getExpenseManager().getExpenses();
        for (int j = 0; j < (int)expenses.size(); j++)
        {
            Expense *e = expenses[j];
            ExpenseRecord er = {};
            er.expenseID = e->getExpenseID();
            er.userID = uid;
            strncpy(er.title, e->getTitle().c_str(), sizeof(er.title) - 1);
            er.amount = e->getAmount();
            strncpy(er.category, e->getCategory().c_str(), sizeof(er.category) - 1);
            er.date_day = e->getDate().getDay();
            er.date_month = e->getDate().getMonth();
            er.date_year = e->getDate().getYear();
            strncpy(er.description, e->getDescription().c_str(), sizeof(er.description) - 1);

            file.write((char *)&er, sizeof(er));
        }
    }
    file.close();
}

void System::saveGoalToFile(Goal *g, int userID)
{
    ofstream file("goals.dat", ios::binary | ios::app);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN goals.dat FOR WRITING!");

    GoalRecord gr = {};
    gr.goalID = g->getGoalID();
    gr.userID = userID;
    strncpy(gr.title, g->getTitle().c_str(), sizeof(gr.title) - 1);
    strncpy(gr.description, g->getDescription().c_str(), sizeof(gr.description) - 1);
    strncpy(gr.category, g->getCategory().c_str(), sizeof(gr.category) - 1);
    gr.deadline_day = g->getDeadline().getDay();
    gr.deadline_month = g->getDeadline().getMonth();
    gr.deadline_year = g->getDeadline().getYear();
    strncpy(gr.status, g->getStatus().c_str(), sizeof(gr.status) - 1);

    file.write((char *)&gr, sizeof(gr));
    file.close();
}

void System::loadGoalsFromFile()
{
    ifstream file("goals.dat", ios::binary);
    if (!file)
        return;

    GoalRecord gr;
    int maxGoalID = 0;

    while (file.read((char *)&gr, sizeof(gr)))
    {
        Date deadline(gr.deadline_day, gr.deadline_month, gr.deadline_year);
        Goal *goal = new Goal(string(gr.title), string(gr.description), string(gr.category), deadline);
        goal->setGoalID(gr.goalID);
        if (string(gr.status) == "Completed")
            goal->completeGoal();

        for (int i = 0; i < (int)users.size(); i++)
        {
            if (users[i]->getUID() == gr.userID)
            {
                users[i]->getGoalManager().loadGoal(goal);
                break;
            }
        }

        if (gr.goalID > maxGoalID)
            maxGoalID = gr.goalID;
    }
    file.close();
    Goal::setNextGoalID(maxGoalID + 1);
}

void System::rewriteGoalsFile()
{
    ofstream file("goals.dat", ios::binary);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN goals.dat FOR REWRITING!");

    for (int i = 0; i < (int)users.size(); i++)
    {
        int uid = users[i]->getUID();
        const vector<Goal *> &goals = users[i]->getGoalManager().getGoals();
        for (int j = 0; j < (int)goals.size(); j++)
        {
            Goal *g = goals[j];
            GoalRecord gr = {};
            gr.goalID = g->getGoalID();
            gr.userID = uid;
            strncpy(gr.title, g->getTitle().c_str(), sizeof(gr.title) - 1);
            strncpy(gr.description, g->getDescription().c_str(), sizeof(gr.description) - 1);
            strncpy(gr.category, g->getCategory().c_str(), sizeof(gr.category) - 1);
            gr.deadline_day = g->getDeadline().getDay();
            gr.deadline_month = g->getDeadline().getMonth();
            gr.deadline_year = g->getDeadline().getYear();
            strncpy(gr.status, g->getStatus().c_str(), sizeof(gr.status) - 1);

            file.write((char *)&gr, sizeof(gr));
        }
    }
    file.close();
}

void System::saveActivityToFile(ActivityLog *a, int userID)
{
    ofstream file("activity_log.dat", ios::binary | ios::app);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN activity_log.dat FOR WRITING!");

    ActivityRecord ar = {};
    ar.activityID = a->getActivityID();
    ar.userID = userID;
    strncpy(ar.type, a->getType().c_str(), sizeof(ar.type) - 1);
    strncpy(ar.title, a->getTitle().c_str(), sizeof(ar.title) - 1);
    strncpy(ar.category, a->getCategory().c_str(), sizeof(ar.category) - 1);
    ar.date_day = a->getDate().getDay();
    ar.date_month = a->getDate().getMonth();
    ar.date_year = a->getDate().getYear();
    ar.time_hour = a->getTime().getHour();
    ar.time_minute = a->getTime().getMinute();
    strncpy(ar.time_meridiem, a->getTime().getMeridiem().c_str(), sizeof(ar.time_meridiem) - 1);

    file.write((char *)&ar, sizeof(ar));
    file.close();
}

void System::loadActivityLogFromFile()
{
    ifstream file("activity_log.dat", ios::binary);
    if (!file)
        return;

    ActivityRecord ar;
    int maxActivityID = 0;

    while (file.read((char *)&ar, sizeof(ar)))
    {
        Date date(ar.date_day, ar.date_month, ar.date_year);
        Time time(ar.time_hour, ar.time_minute, string(ar.time_meridiem));

        ActivityLog *entry = new ActivityLog(string(ar.type), string(ar.title), string(ar.category), date, time);
        entry->setActivityID(ar.activityID);

        for (int i = 0; i < (int)users.size(); i++)
        {
            if (users[i]->getUID() == ar.userID)
            {
                users[i]->getActivityLogManager().loadEntry(entry);
                break;
            }
        }

        if (ar.activityID > maxActivityID)
            maxActivityID = ar.activityID;
    }
    file.close();
    ActivityLog::setNextActivityID(maxActivityID + 1);
}

void System::rewriteActivityLogFile()
{
    ofstream file("activity_log.dat", ios::binary);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN activity_log.dat FOR REWRITING!");

    for (int i = 0; i < (int)users.size(); i++)
    {
        int uid = users[i]->getUID();
        const vector<ActivityLog *> &entries = users[i]->getActivityLogManager().getEntries();
        for (int j = 0; j < (int)entries.size(); j++)
        {
            ActivityLog *a = entries[j];
            ActivityRecord ar = {};
            ar.activityID = a->getActivityID();
            ar.userID = uid;
            strncpy(ar.type, a->getType().c_str(), sizeof(ar.type) - 1);
            strncpy(ar.title, a->getTitle().c_str(), sizeof(ar.title) - 1);
            strncpy(ar.category, a->getCategory().c_str(), sizeof(ar.category) - 1);
            ar.date_day = a->getDate().getDay();
            ar.date_month = a->getDate().getMonth();
            ar.date_year = a->getDate().getYear();
            ar.time_hour = a->getTime().getHour();
            ar.time_minute = a->getTime().getMinute();

            file.write((char *)&ar, sizeof(ar));
        }
    }
    file.close();
}

void System::saveAchievementToFile(Achievement &a)
{
    ofstream file("achievements.dat", ios::binary | ios::app);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN achievements.dat FOR WRITING!");

    AchievementRecord ar = {};
    ar.achievementID = a.getAchievementID();
    strncpy(ar.name, a.getName().c_str(), sizeof(ar.name) - 1);
    strncpy(ar.description, a.getDescription().c_str(), sizeof(ar.description) - 1);
    ar.requiredGoals = a.getRequiredGoals();
    ar.isDefault = a.getIsDefault();

    file.write((char *)&ar, sizeof(ar));
    file.close();
}

void System::loadAchievementsFromFile()
{
    ifstream file("achievements.dat", ios::binary);
    if (!file)
    {
        loadDefaultAchievements();
        rewriteAchievementsFile();
        return;
    }

    AchievementRecord ar;
    int maxAchID = 0;
    achievements.clear();

    while (file.read((char *)&ar, sizeof(ar)))
    {
        Achievement ach(string(ar.name), string(ar.description), ar.requiredGoals, ar.isDefault);
        ach.setAchievementID(ar.achievementID);
        achievements.push_back(ach);
        if (ar.achievementID > maxAchID)
            maxAchID = ar.achievementID;
    }
    file.close();

    if (achievements.empty())
    {
        loadDefaultAchievements();
        rewriteAchievementsFile();
    }
    else
        Achievement::setNextAchievementID(maxAchID + 1);
}

void System::rewriteAchievementsFile()
{
    ofstream file("achievements.dat", ios::binary);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN achievements.dat FOR REWRITING!");

    for (int i = 0; i < (int)achievements.size(); i++)
    {
        AchievementRecord ar = {};
        ar.achievementID = achievements[i].getAchievementID();
        strncpy(ar.name, achievements[i].getName().c_str(), sizeof(ar.name) - 1);
        strncpy(ar.description, achievements[i].getDescription().c_str(), sizeof(ar.description) - 1);
        ar.requiredGoals = achievements[i].getRequiredGoals();
        ar.isDefault = achievements[i].getIsDefault();

        file.write((char *)&ar, sizeof(ar));
    }
    file.close();
}

void System::saveAdminToFile()
{
    ofstream file("admin.dat", ios::binary);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN admin.dat FOR WRITING!");

    AdminRecord ar = {};
    strncpy(ar.username, admin.getUsername().c_str(), sizeof(ar.username) - 1);
    strncpy(ar.password_hash, admin.getPasswordHash().c_str(), sizeof(ar.password_hash) - 1);

    file.write((char *)&ar, sizeof(ar));
    file.close();
}

void System::loadAdminFromFile()
{
    ifstream file("admin.dat", ios::binary);
    if (!file)
    {
        saveAdminToFile();
        return;
    }

    AdminRecord ar;
    if (file.read((char *)&ar, sizeof(ar)))
    {
        admin.setPasswordHash(string(ar.password_hash));
    }
    file.close();
}

void System::rewriteAdminFile()
{
    saveAdminToFile();
}

// ---- Appends a single audit-only record for a permanently deleted account.
// This file is append-only and is never rewritten or loaded into memory --
// it exists purely as a historical log and is read directly from disk by
// System::adminViewPermanentlyDeletedUsers(). ----
void System::savePermanentDeletedUserToFile(const PermanentDeletedUserRecord &pdr)
{
    ofstream file("permanently_deleted_users.dat", ios::binary | ios::app);
    if (!file)
        throw FileException("ERROR: COULD NOT OPEN permanently_deleted_users.dat FOR WRITING!");

    file.write((char *)&pdr, sizeof(pdr));
    file.close();
}

// ============================================================
//                           MAIN
// ============================================================



// ============================================================
//         GUI HELPER METHODS (Added for Qt/QML Frontend)
// ============================================================

std::string Date::toString() const {
    std::ostringstream oss;
    oss << *this;
    return oss.str();
}

std::string Time::toString() const {
    std::ostringstream oss;
    oss << *this;
    return oss.str();
}

void TaskManager::deleteTaskById(int id) {
    for (int i = 0; i < (int)tasks.size(); i++) {
        if (tasks[i]->getTaskID() == id) {
            delete tasks[i];
            tasks.erase(tasks.begin() + i);
            return;
        }
    }
}

void TaskManager::completeTaskById(int id) {
    for (int i = 0; i < (int)tasks.size(); i++) {
        if (tasks[i]->getTaskID() == id) {
            tasks[i]->markCompleted();
            return;
        }
    }
}

void GoalManager::deleteGoalById(int id) {
    for (int i = 0; i < (int)goals.size(); i++) {
        if (goals[i]->getGoalID() == id) {
            delete goals[i];
            goals.erase(goals.begin() + i);
            return;
        }
    }
}

void GoalManager::completeGoalById(int id) {
    Goal* g = searchByID(id);
    if (g && g->getStatus() != "Completed") {
        g->completeGoal();
        updateBadges();
    }
}

void ExpenseManager::deleteExpenseById(int id) {
    for (int i = 0; i < (int)expenses.size(); i++) {
        if (expenses[i]->getExpenseID() == id) {
            delete expenses[i];
            expenses.erase(expenses.begin() + i);
            return;
        }
    }
}

