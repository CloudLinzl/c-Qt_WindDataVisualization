cmake_minimum_required(VERSION 3.16)
project(WindDataVisualization VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理Qt的MOC、UIC和RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找Qt6包，如果找不到则尝试Qt5
find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/inc)

# 源文件
set(SOURCES
    src/main.cpp
    src/loader.cpp
    src/MainWindow.cpp
    src/PPIWidget.cpp
    src/ProfileWidget.cpp
)

# 头文件
set(HEADERS
    inc/loader.hpp
    inc/MainWindow.h
    inc/PPIWidget.h
    inc/ProfileWidget.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 链接Qt库
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(${PROJECT_NAME}
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
    )
else()
    target_link_libraries(${PROJECT_NAME}
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
    )
endif()

# 在macOS上设置应用程序包属性
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()

# 在Windows上设置应用程序属性
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# 打印配置信息
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Qt version: ${QT_VERSION_MAJOR}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
