cmake_minimum_required(VERSION 3.16)

project(qt6_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)
find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2)

qt_policy(SET QTP0001 NEW)
qt_standard_project_setup(REQUIRES 6.8)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(cmake/utils_cmake.cmake)
include(cmake/utils_git.cmake)

# 自动生成版本文件
make_version_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/version.h
)

recurse_auto_sources(user_files_c   "*.c"   "src/" RELATIVE)
recurse_auto_sources(user_files_cpp "*.cpp" "src/" RELATIVE)
recurse_auto_sources(user_files_h   "*.h"   "src/" RELATIVE)
recurse_auto_sources(user_files_hpp "*.hpp" "src/" RELATIVE)
recurse_auto_sources(user_files_qrc "*.qrc" "res/" RELATIVE)
recurse_auto_sources(user_files_qml "*.qml" "res/" RELATIVE)

if (MSVC)
    recurse_auto_sources(user_files_lib "*.lib" "src/" RELATIVE)
else ()
    recurse_auto_sources(user_files_lib "*.so" "src/" RELATIVE)
    recurse_auto_sources(user_files_lib "*.a"  "src/" RELATIVE)
endif (MSVC)

list(APPEND user_files_lib
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
)

find_files_dirs(user_dirs_lib "${user_files_lib}" RELATIVE)
find_files_dirs(user_dirs_inc "${user_files_h}; ${user_files_hpp}" RELATIVE)
list(APPEND user_dirs_inc "src/third_party")

qt_add_executable(${PROJECT_NAME}
    ${user_files_c}
    ${user_files_cpp}
    ${user_files_h}
    ${user_files_hpp}
)

qt_add_qml_module(${PROJECT_NAME}
    URI UserCom
    QML_FILES
        ${user_files_qml}
    SOURCES
    RESOURCES
        ${user_files_qrc}
)

# 添加链接库, 库文件目录, 头文件目录
target_link_directories(${PROJECT_NAME} PRIVATE ${user_dirs_lib})
target_link_libraries(${PROJECT_NAME} PRIVATE ${user_files_lib})
target_include_directories(${PROJECT_NAME} PRIVATE ${user_dirs_inc})


include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
