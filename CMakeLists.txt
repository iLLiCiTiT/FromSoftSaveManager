cmake_minimum_required(VERSION 3.11)
project(FromSoftSaveManager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

# Add the vendor tiny-AES-c as a subdirectory (uses its own CMakeLists)
add_subdirectory(vendor/tiny-AES-c)

add_executable(FromSoftSaveManager
        src/main.cpp
        src/parse/Parse.cpp
        src/parse/DSRSaveFile.cpp
        src/parse/Utils.cpp

        src/ui/MainWindow.cpp
        src/ui/SideBarWidget.cpp
        src/ui/SquareButton.h
        src/ui/Controller.h
        src/ui/SettingsWidget.cpp
        src/ui/SettingsWidget.h
        src/ui/resources.qrc
        src/ui/Utils.cpp
        src/ui/Utils.h
        src/ui/KeysWindows.cpp
        src/ui/KeysWindows.h
        src/ui/DSRWidget/DSRWidget.cpp
        src/ui/DSRWidget/DSRWidget.h
)

target_link_libraries(FromSoftSaveManager
        Qt::Core
        Qt::Gui
        Qt::Widgets
        tiny-aes
)

if ((MSVC) AND (MSVC_VERSION GREATER_EQUAL 1914))
    target_compile_options(FromSoftSaveManager PUBLIC "-Zc:__cplusplus" "-permissive-")
endif()

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (
        QT_LIB
        Qt6Core
        Qt6Gui
        Qt6Widgets
        libgcc_s_seh-1
        libstdc++-6
        libwinpthread-1
    )
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()
