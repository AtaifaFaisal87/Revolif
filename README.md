<div align="center">

   <img src="revolif_logo_circle.png" alt="Revolif Logo" width="160"/>


  # Revolif

  ### Life, Beautifully Aligned.

  A desktop life-management application built with **Qt 6 / QML** and a **C++17** backend — unifying tasks, goals, expenses, achievements, and focus tracking into a single, elegant dashboard.

  ![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus)
  ![Qt](https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt)
  ![QML](https://img.shields.io/badge/QML-Declarative%20UI-41CD52?style=for-the-badge)
  ![CMake](https://img.shields.io/badge/CMake-3.16%2B-064F8C?style=for-the-badge&logo=cmake)
  ![License](https://img.shields.io/badge/License-MIT-blue?style=for-the-badge)

</div>

---

## 📖 Overview

**Revolif** is a full-featured personal productivity and life-management system. It combines task scheduling, goal tracking, expense/budget management, an achievements system, a focus/Pomodoro-style timer, and an admin control panel — all wrapped in a modern, animated QML interface backed by a robust object-oriented C++ engine.

The project started as a console-based OOP system in C++ and evolved into a complete **Qt Quick desktop application**, with the original console logic re-engineered into a reusable backend that now drives a fully graphical UI.

---

## ✨ Features

### 🔐 Authentication & Accounts
- Secure login/registration system with password hashing
- Account suspension, restoration, and permanent deletion workflows
- Self-deactivation and admin-driven account management
- Automated **welcome emails** and **login/summary emails** via SMTP (libcurl)

### ✅ Task Management
- Academic & Daily task types with categories and priorities
- Recurring tasks with configurable intervals
- Deadline & due-time tracking with automatic **overdue** / **due-soon** detection
- One-click completion tracking

### 🎯 Goal Tracking
- Category-based goal creation with deadlines
- Status tracking (Incomplete → Completed) with overdue detection
- Progress feeds directly into the Life Score engine

### 💰 Expense & Budget Manager
- Category-wise expense logging with descriptions and dates
- Custom budget limits per category
- Visual spending breakdown via donut charts
- Monthly financial report generation

### 🏆 Achievements System
- Unlockable achievements based on goals completed
- Custom achievement creation (admin-configurable)
- Profile badge/achievement showcase

### ⏱️ Focus Mode
- Start / Pause / Resume / Stop / Reset session controls
- Session history and focus statistics tracking

### 📊 Dashboard & Analytics
- **Life Score** — a computed wellness/productivity metric with descriptive labels
- Activity heatmap (contribution-graph style, configurable week range)
- Real-time stats: pending/completed/overdue tasks & goals, total expenses

### 🛠️ Admin Panel
- Dedicated admin dashboard, sidebar, and settings pages
- User management: suspend, unsuspend, permanently delete (with reason tracking)
- System-wide statistics and recent activity feed
- System-level report generation
- Deletion-reason analytics

### 🎨 Polished UI/UX
- Custom-built component library (glassmorphism panels, animated buttons, toasts, progress rings, mini calendar, wave backgrounds)
- Light/Dark mode toggle
- Multi-currency support
- Login sound chime (Qt Multimedia)
- Fully responsive custom sidebar/topbar navigation for both user and admin views

---

## 🏗️ Architecture

Revolif follows a clean separation between UI and logic:

```
QML Frontend  ⇄  RevolifController (C++/Qt bridge)  ⇄  System Backend (Core Engine)
```

- **QML Frontend** — Declarative UI (pages, dialogs, reusable components)
- **RevolifController** — `Q_INVOKABLE` bridge exposing backend functionality to QML
- **Core Backend (`revolif_backend.cpp`)** — Pure C++ engine containing the domain model, file persistence, business rules, and email service
- **Qt Data Models** — `QAbstractListModel` implementations (`TaskListModel`, `GoalListModel`, `ExpenseListModel`) for efficient list binding in QML

### Core Domain Classes

| Class | Responsibility |
|---|---|
| `User` | User profile, stats, streaks, achievements |
| `Task`, `AcademicTask`, `DailyTask` | Task hierarchy with polymorphic behavior |
| `TaskManager` | Task CRUD, filtering, overdue detection |
| `Goal`, `GoalManager` | Goal lifecycle management |
| `Expense`, `ExpenseManager` | Expense tracking & budgets |
| `Achievement` | Unlockable milestone system |
| `ActivityLog`, `ActivityLogManager` | Streaks & heatmap data |
| `Admin` | Administrative privileges & actions |
| `Authentication` | Login, registration, password verification |
| `EmailManager` | SMTP email dispatch (welcome/summary emails) via libcurl |
| `System` | Central orchestrator — file I/O, session state, report generation |

Custom exception classes (`FileException`, `NotFoundException`, `UserException`, `AuthException`, `ValidationException`) provide structured error handling throughout the engine.

---

## 📁 Project Structure

```
Revolif/
├── main.cpp                       # Application entry point
├── CMakeLists.txt                 # Primary build configuration (Qt6 + CURL + OpenSSL + ZLIB)
├── Revolif.pro                    # Alternate qmake build configuration
├── qml.qrc                        # Qt resource file (bundles QML + assets)
│
├── src/
│   ├── revolifcontroller.h/.cpp   # C++ ⇄ QML bridge (exposed API)
│
├── core/
│   └── revolif_backend.cpp        # Core domain engine & business logic
│
├── cpp/
│   ├── revuser.h/.cpp             # User model
│   ├── revtask.h/.cpp             # Task model
│   ├── revgoal.h/.cpp             # Goal model
│   ├── revexpense.h/.cpp          # Expense model
│   ├── revachievement.h/.cpp      # Achievement model
│   └── models/                    # QAbstractListModel implementations
│       ├── tasklistmodel.h/.cpp
│       ├── goallistmodel.h/.cpp
│       └── expenselistmodel.h/.cpp
│
├── qml/
│   ├── main.qml                   # App window entry
│   ├── MainShell.qml              # User shell/layout
│   ├── AdminShell.qml             # Admin shell/layout
│   ├── AuthScreen.qml             # Login / registration screen
│   ├── Theme.qml / AdminTheme.qml # Design tokens
│   ├── JsHelpers.js               # Shared JS utilities
│   ├── pages/                     # Dashboard, Tasks, Goals, Expenses,
│   │                               Achievements, Focus, Profile, Settings
│   │                               (+ Admin variants)
│   ├── dialogs/                   # Add Task/Goal/Expense/Achievement dialogs
│   └── components/                # Reusable UI kit (16+ custom components)
│
└── assets/                        # Logos, icons, login chime audio
```

---

## 🚀 Getting Started

### Prerequisites

| Dependency | Notes |
|---|---|
| **Qt 6** (Quick, QuickControls2, Multimedia) | UI framework |
| **CMake ≥ 3.16** | Build system |
| **C++17-compatible compiler** | MinGW-w64 / MSVC / GCC / Clang |
| **libcurl** | Email (SMTP) functionality |
| **OpenSSL** | TLS for SMTP connection |
| **zlib** | Required by libcurl |

> Developed and tested on **Windows** using **MSYS2 (UCRT64)** toolchain.

### Installing dependencies (MSYS2 example)

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6 \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-curl \
          mingw-w64-ucrt-x86_64-openssl \
          mingw-w64-ucrt-x86_64-zlib
```

### Build (CMake)

```bash
git clone https://github.com/<your-username>/Revolif.git
cd Revolif
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64"
cmake --build .
```

### Build (qmake, alternative)

```bash
qmake Revolif.pro
mingw32-make
```

### Run

```bash
./Revolif
```

---

## 🖥️ Tech Stack

- **Language:** C++17
- **UI Framework:** Qt 6 (Quick, QuickControls2, Multimedia)
- **UI Language:** QML + JavaScript
- **Build Systems:** CMake, qmake
- **Networking:** libcurl (SMTP over SSL) + OpenSSL
- **Data Persistence:** Flat-file storage (per-entity file I/O layer in `System`)
- **Design Pattern:** MVC-inspired — Domain model (C++) / Controller bridge (`RevolifController`) / View (QML)

---

## 📊 Project Stats

| Metric | Count |
|---|---|
| Total Lines of Code | **~17,000** |
| C++ Source Files (.cpp) | 11 |
| C++ Header Files (.h) | 9 |
| QML Files | 40 |
| Domain/Manager Classes | 20+ |
| Exposed Controller API Methods | 50+ |
| Custom Reusable QML Components | 16 |
| Application Pages (User + Admin) | 14 |

---

## 👥 Authors

Built by:

- **Ataifa Faisal       @AtaifaFaisal87** 
- **Yazdaan Ali Mirza   @Yazdaan-Ali2006** 

### 🧪 Tested By

- **Summaiya Abubakker  @summaiyaabubakker**
- **Aoun Ali            @AounAli2k06**

---
🙏 Acknowledgment

Revolif was built through genuine hands-on effort  architecture, logic, and design decisions were ours — with AI tools (Claude, ChatGPT, etc.) used along the way for debugging, boilerplate, and speeding up development. We believe in being transparent about that process.

## 📄 License

This project is licensed under the MIT License — feel free to use, modify, and distribute with attribution.

---

<div align="center">
  <sub>Revolif — organize your tasks, track your goals, manage your money, and level up your life.</sub>
</div>
