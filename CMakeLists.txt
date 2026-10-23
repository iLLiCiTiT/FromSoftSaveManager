cmake_minimum_required(VERSION 3.11)
project(FromSoftSaveManager VERSION 0.6.2.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
option(FSSM_USE_INV_IMAGES "Use inventory item images" ON)

set(FSSM_VERSION "${PROJECT_VERSION}")
set(FSSM_VERSION_TWEAK "0")
if (PROJECT_VERSION_TWEAK)
    set(FSSM_VERSION_TWEAK "${PROJECT_VERSION_TWEAK}")
endif()
set(FSSM_APP_RC_VERSION "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},${FSSM_VERSION_TWEAK}")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Concurrent
        Sql
        REQUIRED)


include_directories(vendor/nlohmann_json)
include_directories(vendor/miniaudio)
# Add the vendor tiny-AES-c as a subdirectory (uses its own CMakeLists)
add_subdirectory(vendor/tiny-AES-c)

add_executable(FromSoftSaveManager
        src/main.cpp

        src/parse/Utils.cpp
        src/parse/SL2File.cpp
        src/parse/DSR/Items.cpp
        src/parse/DSR/SaveFile.cpp
        src/parse/DSR/SaveData.cpp
        src/parse/DSR/EventFlags.cpp
        src/parse/DSR/BonfireDb.cpp
        src/parse/DSR/WorldProgress.cpp
        src/parse/DS2/SaveData.cpp
        src/parse/DS2/Items.cpp
        src/parse/DS2/ItemLookup.cpp
        src/parse/DS2/SaveFile.cpp
        src/parse/DS2/Areas.cpp
        src/parse/DS2/StatTables.cpp
        src/parse/DS2/WorldProgress.cpp
        src/parse/Sekiro/SaveData.cpp
        src/parse/Sekiro/Items.cpp
        src/parse/Sekiro/ItemLookup.cpp
        src/parse/Sekiro/SaveFile.cpp
        src/parse/Sekiro/SkillTree.cpp
        src/parse/Sekiro/EventFlags.cpp
        src/parse/Sekiro/WorldProgress.cpp
        src/parse/DS3/Items.cpp
        src/parse/DS3/SaveFile.cpp
        src/parse/DS3/SaveData.cpp
        src/parse/DS3/Areas.cpp
        src/parse/DS3/EventFlags.cpp
        src/parse/DS3/WorldProgress.cpp
        src/parse/EldenRing/SaveFile.cpp
        src/parse/EldenRing/Items.cpp
        src/parse/EldenRing/ItemLookup.cpp
        src/parse/EldenRing/EventFlags.cpp
        src/parse/EldenRing/WorldProgress.cpp
        src/parse/EldenRing/NpcProgress.cpp
        src/parse/EldenRing/PlaceNames.cpp

        src/resources/resources.qrc
        src/resources/dsr_images.qrc
        src/resources/ds2_images.qrc
        src/resources/ds3_images.qrc
        src/resources/sekiro_images.qrc
        src/resources/er_images.qrc

        src/ui/SquareButton.h
        src/ui/NiceCheckbox.cpp
        src/ui/Utils.cpp
        src/ui/ScrollHelper.cpp
        src/ui/AudioNotifier.cpp
        src/ui/ConfigModel.cpp
        src/ui/BackupsDb.cpp
        src/ui/MigrationHelper.cpp
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
        src/ui/DSRWidget/CharacterList.cpp
        src/ui/DSRWidget/Inventory.cpp
        src/ui/DSRWidget/Covenants.cpp
        src/ui/DSRWidget/CharInfo.cpp
        src/ui/DSRWidget/WorldProgress.cpp
        src/ui/DS2Widget/DS2Widget.cpp
        src/ui/DS2Widget/Inventory.cpp
        src/ui/DS2Widget/PlayerStatus.cpp
        src/ui/DS2Widget/CharacterList.cpp
        src/ui/DS2Widget/WorldProgress.cpp
        src/ui/DS2Widget/HeaderTabs.cpp
        src/ui/DS3Widget/DS3Widget.cpp
        src/ui/DS3Widget/Inventory.cpp
        src/ui/DS3Widget/CharInfo.cpp
        src/ui/DS3Widget/CharacterList.cpp
        src/ui/DS3Widget/HeaderTabs.cpp
        src/ui/DS3Widget/WorldProgress.cpp
        src/ui/SekiroWidget/SekiroWidget.cpp
        src/ui/SekiroWidget/CharacterList.cpp
        src/ui/SekiroWidget/Inventory.cpp
        src/ui/SekiroWidget/WorldProgress.cpp
        src/ui/ERWidget/ERWidget.cpp
        src/ui/ERWidget/CharacterList.cpp
        src/ui/ERWidget/CharInfo.cpp
        src/ui/ERWidget/Inventory.cpp
        src/ui/ERWidget/WorldProgress.cpp
        src/ui/ERWidget/HeaderTabs.cpp
)

if (FSSM_USE_INV_IMAGES)
    target_sources(FromSoftSaveManager PRIVATE
            src/resources/dsr_inv_images.qrc
            src/resources/ds2_inv_images.qrc
            src/resources/sekiro_inv_images.qrc
            src/resources/sekiro_skill_images.qrc
            src/resources/er_inv_images.qrc
            src/resources/ds3_inv_images.qrc)
    target_compile_definitions(FromSoftSaveManager PRIVATE
            FSSM_USE_INV_IMAGES=${FSSM_USE_INV_IMAGES}
    )
endif()

target_compile_definitions(FromSoftSaveManager PRIVATE
        FSSM_VERSION="${PROJECT_VERSION}"
        FSSM_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        FSSM_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        FSSM_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        FSSM_VERSION_TWEAK=${FSSM_VERSION_TWEAK}
)

target_link_libraries(FromSoftSaveManager
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Concurrent
        Qt::Sql
        tiny-aes
)
target_link_options(FromSoftSaveManager PRIVATE -static-libgcc -static-libstdc++)

# Console tool: checks that the DSR slot parser round-trips save files exactly.
add_executable(DSRRoundTrip
        tools/dsr_roundtrip.cpp
        src/parse/Utils.cpp
        src/parse/SL2File.cpp
        src/parse/DSR/SaveData.cpp
        src/parse/DSR/EventFlags.cpp
        src/parse/DSR/BonfireDb.cpp
        src/parse/DSR/WorldProgress.cpp
)
target_link_libraries(DSRRoundTrip tiny-aes)
target_link_options(DSRRoundTrip PRIVATE -static-libgcc -static-libstdc++)

add_executable(DS2RoundTrip
        tools/ds2_roundtrip.cpp
        src/parse/Utils.cpp
        src/parse/SL2File.cpp
        src/parse/DS2/SaveData.cpp
        src/parse/DS2/Items.cpp
        src/parse/DS2/ItemLookup.cpp
        src/parse/DS2/WorldProgress.cpp
)
target_link_libraries(DS2RoundTrip tiny-aes)
target_link_options(DS2RoundTrip PRIVATE -static-libgcc -static-libstdc++)

add_executable(SekiroRoundTrip
        tools/sekiro_roundtrip.cpp
        src/parse/Utils.cpp
        src/parse/SL2File.cpp
        src/parse/Sekiro/SaveData.cpp
        src/parse/Sekiro/Items.cpp
        src/parse/Sekiro/ItemLookup.cpp
        src/parse/Sekiro/SaveFile.cpp
        src/parse/Sekiro/EventFlags.cpp
        src/parse/Sekiro/WorldProgress.cpp
)
target_link_libraries(SekiroRoundTrip tiny-aes)
target_link_options(SekiroRoundTrip PRIVATE -static-libgcc -static-libstdc++)

add_executable(DS3RoundTrip
        tools/ds3_roundtrip.cpp
        src/parse/Utils.cpp
        src/parse/SL2File.cpp
        src/parse/DS3/SaveData.cpp
)
target_link_libraries(DS3RoundTrip tiny-aes)
target_link_options(DS3RoundTrip PRIVATE -static-libgcc -static-libstdc++)

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
    if (EXISTS "${QT_INSTALL_PATH}/plugins/sqldrivers/qsqlite${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/sqldrivers/qsqlite${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")
    endif ()
    foreach (
        QT_LIB
        Qt6Core
        Qt6Gui
        Qt6Widgets
        Qt6Sql
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
