cmake_minimum_required(VERSION 3.11)
project(FromSoftSaveManager VERSION 0.6.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Multimedia
        Network
        Concurrent
        REQUIRED)


include_directories(vendor/nlohmann_json)
# Add the vendor tiny-AES-c as a subdirectory (uses its own CMakeLists)
add_subdirectory(vendor/tiny-AES-c)

add_executable(FromSoftSaveManager
        src/main.cpp

        src/parse/Utils.cpp
        src/parse/SL2File.cpp
        src/parse/DSR/Items.cpp
        src/parse/DSR/SaveFile.cpp
        src/parse/DS3/Items.cpp
        src/parse/DS3/SaveFile.cpp
        src/parse/EldenRing/SaveFile.cpp

        src/resources/resources.qrc
        src/resources/dsr_images.qrc
        src/resources/ds2_images.qrc
        src/resources/ds3_images.qrc
        src/resources/sekiro_images.qrc
        src/resources/er_images.qrc

        src/ui/SquareButton.h
        src/ui/NiceCheckbox.cpp
        src/ui/Utils.cpp
        src/ui/ConfigModel.cpp
        src/ui/BackupsModel.cpp
        src/ui/Controller.cpp
        src/ui/KeysWindows.cpp
        src/ui/KeysWindows.h
        src/ui/MainWindow.cpp
        src/ui/SideBarWidget.cpp
        src/ui/SettingsWidget.cpp
        src/ui/BaseGameWidget.h
        src/ui/ManageBackupsWidget.cpp
        src/ui/DSRWidget/DSRWidget.cpp
        src/ui/DSRWidget/Inventory.cpp
        src/ui/DSRWidget/Covenants.cpp
        src/ui/DSRWidget/CharInfo.cpp
        src/ui/DS2Widget/DS2Widget.cpp
        src/ui/DS3Widget/DS3Widget.cpp
        src/ui/DS3Widget/Inventory.cpp
        src/ui/DS3Widget/CharInfo.cpp
        src/ui/SekiroWidget/SekiroWidget.cpp
        src/ui/ERWidget/ERWidget.cpp
        src/ui/ERWidget/CharInfo.cpp
)

target_compile_definitions(FromSoftSaveManager PRIVATE
        FSSM_VERSION="${PROJECT_VERSION}"
        FSSM_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        FSSM_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        FSSM_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

target_link_libraries(FromSoftSaveManager
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Concurrent
        Qt::Multimedia
        Qt::Network
        tiny-aes
)
target_link_options(FromSoftSaveManager PRIVATE -static-libgcc -static-libstdc++)

# --- DSR inventory resources ---
add_library(inv_resources_dsr SHARED)
set_source_files_properties(src/resources/dsr_inv_images.qrc PROPERTIES
    QT_RESOURCE_ALIAS "dsr_inv_images"
)
set_target_properties(inv_resources_dsr PROPERTIES PREFIX "")
target_sources(inv_resources_dsr PRIVATE
    src/resources/dsr_inv_images.qrc
    src/resources/dsr_inv_images_init.h
)
target_link_libraries(inv_resources_dsr PUBLIC Qt::Core)
# -------------------------------

# --- DS3 inventory resources ---
add_library(inv_resources_ds3 SHARED)
set_source_files_properties(src/resources/ds3_inv_images.qrc PROPERTIES
    QT_RESOURCE_ALIAS "ds3_inv_images"
)
set_target_properties(inv_resources_ds3 PROPERTIES PREFIX "")
target_sources(inv_resources_ds3 PRIVATE
    src/resources/ds3_inv_images.qrc
    src/resources/ds3_inv_images_init.h
)
target_link_libraries(inv_resources_ds3 PUBLIC Qt::Core)
# -------------------------------

if (WIN32)
    configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/app.rc.in
            ${CMAKE_CURRENT_BINARY_DIR}/generated/app.rc
            @ONLY
    )
    target_sources(FromSoftSaveManager PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/generated/app.rc)
endif()
if ((WIN32) AND (CMAKE_BUILD_TYPE MATCHES "Release"))
    set_target_properties(FromSoftSaveManager PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if ((MSVC) AND (MSVC_VERSION GREATER_EQUAL 1914))
    target_compile_options(FromSoftSaveManager PUBLIC "-Zc:__cplusplus" "-permissive-")
endif()

add_custom_command(TARGET FromSoftSaveManager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:inv_resources_dsr>
    $<TARGET_FILE_DIR:FromSoftSaveManager>
)

add_custom_command(TARGET FromSoftSaveManager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:inv_resources_ds3>
    $<TARGET_FILE_DIR:FromSoftSaveManager>
)

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
        Qt6Multimedia
        Qt6Concurrent
        Qt6Network
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
