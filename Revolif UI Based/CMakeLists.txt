cmake_minimum_required(VERSION 3.16)

project(Revolif VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Multimedia)


# ================================
# libcurl + OpenSSL + ZLIB
# ================================

set(CMAKE_PREFIX_PATH "C:/msys64/ucrt64")

set(OPENSSL_ROOT_DIR "C:/msys64/ucrt64")
set(ZLIB_ROOT "C:/msys64/ucrt64")

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)


add_compile_definitions(HAS_CURL)


# ================================
# Source Files
# ================================

set(SOURCES
    main.cpp

    src/revolifcontroller.cpp

    cpp/revuser.cpp
    cpp/revtask.cpp
    cpp/revgoal.cpp
    cpp/revexpense.cpp
    cpp/revachievement.cpp

    cpp/models/tasklistmodel.cpp
    cpp/models/goallistmodel.cpp
    cpp/models/expenselistmodel.cpp
)


set(HEADERS

    src/revolifcontroller.h

    cpp/revuser.h
    cpp/revtask.h
    cpp/revgoal.h
    cpp/revexpense.h
    cpp/revachievement.h

    cpp/models/tasklistmodel.h
    cpp/models/goallistmodel.h
    cpp/models/expenselistmodel.h
)


# ================================
# Qt Executable
# ================================

qt_add_executable(Revolif
    ${SOURCES}
    ${HEADERS}
    qml.qrc
)


# ================================
# Libraries
# ================================

target_link_libraries(Revolif PRIVATE

    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia

    CURL::libcurl

    OpenSSL::SSL
    OpenSSL::Crypto

    ZLIB::ZLIB
)


# ================================
# Properties
# ================================

set_target_properties(Revolif PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)