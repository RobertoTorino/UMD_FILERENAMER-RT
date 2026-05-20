cmake_minimum_required(VERSION 3.16)
project(UMD_FileRenamer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS Core Gui Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

add_executable(UMD_FileRenamer
    src/main.cpp
    src/mainwindow.cpp
    src/isoreader.cpp
    src/psfparser.cpp
    src/renamerlogic.cpp
    resources/resources.qrc
)

if (QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(UMD_FileRenamer PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
else()
    target_link_libraries(UMD_FileRenamer PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)
endif()

if (WIN32)
    set_target_properties(UMD_FileRenamer PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
