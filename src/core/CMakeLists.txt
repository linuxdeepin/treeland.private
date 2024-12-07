find_package(QT NAMES Qt6 COMPONENTS Core REQUIRED)

qt_add_dbus_adaptor(DBUS_ADAPTOR "${CMAKE_SOURCE_DIR}/misc/dbus/org.deepin.compositor1.xml" "treeland.h" Treeland::Treeland)

set_source_files_properties("${CMAKE_SOURCE_DIR}/misc/interfaces/org.freedesktop.login1.Manager.xml" PROPERTIES
   INCLUDE "loginddbustypes.h"
)
set_source_files_properties("${CMAKE_SOURCE_DIR}/misc/interfaces/org.freedesktop.login1.Seat.xml" PROPERTIES
   INCLUDE "loginddbustypes.h"
)

set_source_files_properties("${CMAKE_SOURCE_DIR}/misc/interfaces/org.freedesktop.login1.Session.xml" PROPERTIES
   INCLUDE "loginddbustypes.h"
)

set_source_files_properties("${CMAKE_SOURCE_DIR}/misc/interfaces/org.freedesktop.login1.User.xml" PROPERTIES
   INCLUDE "loginddbustypes.h"
)

qt_add_dbus_interface(DBUS_INTERFACE "${CMAKE_SOURCE_DIR}/misc/interfaces/org.freedesktop.login1.Manager.xml"  "Login1Manager")
qt_add_dbus_interface(DBUS_INTERFACE "${CMAKE_SOURCE_DIR}/misc/interfaces/org.freedesktop.login1.Seat.xml"  "Login1Seat")
qt_add_dbus_interface(DBUS_INTERFACE "${CMAKE_SOURCE_DIR}/misc/interfaces/org.freedesktop.login1.Session.xml"  "Login1Session")
qt_add_dbus_interface(DBUS_INTERFACE "${CMAKE_SOURCE_DIR}/misc/interfaces/org.freedesktop.login1.User.xml"  "Login1User")

qt_add_library(treeland_config SHARED
    treelandconfig.h
    treelandconfig.cpp
)

set_target_properties(treeland_config PROPERTIES
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR}
    OUTPUT_NAME treeland-config
    EXPORT_NAME Config
)

target_include_directories(treeland_config INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/treeland/config>
)

target_link_directories(treeland_config INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>
)

target_link_libraries(treeland_config
    PUBLIC
        Dtk6::Core
        Qt6::Quick
)

add_library(treeland_core SHARED)

qt_add_resources(treeland_core "treeland_assets"
set_target_properties(treeland-core PROPERTIES
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR}
    OUTPUT_NAME treeland-core
    EXPORT_NAME Core
)

target_include_directories(treeland-core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/core>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src/core>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/treeland/core>
)

target_link_directories(treeland-core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>
)

    PREFIX "/dsg/icons"
    BASE ${PROJECT_RESOURCES_DIR}/icons
    FILES
        ${PROJECT_RESOURCES_DIR}/icons/close.dci
        ${PROJECT_RESOURCES_DIR}/icons/add.dci
        ${PROJECT_RESOURCES_DIR}/icons/select_output.dci
        ${PROJECT_RESOURCES_DIR}/icons/select_window.dci
        ${PROJECT_RESOURCES_DIR}/icons/select_region.dci
)

target_include_directories(treeland_core
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/treeland>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

set(CORE_HEADERS
    cmdline.h
    helper.h
    surfacewrapper.h
    workspace.h
    shellhandler.h
    output.h
    qmlengine.h
    surfacecontainer.h
    layersurfacecontainer.h
    rootsurfacecontainer.h
    surfaceproxy.h
    treeland.h
    treeland.cpp
    wallpaperimage.h
    wallpapercontroller.h
    wallpapermanager.h
    workspacemodel.h
    propertymonitor.h
    tsgradiusimagenode.h
    tquickradiuseffect_p.h
    tquickradiuseffect.h
    gestures.h
    togglablegesture.h
    inputdevice.h
    workspaceanimationcontroller.h
    surfacefilterproxymodel.h
    windowpicker.h
    wrappointer.h
)

set(CORE_SRCS
    cmdline.cpp
    helper.cpp
    surfacewrapper.cpp
    workspace.cpp
    shellhandler.cpp
    output.cpp
    qmlengine.cpp
    surfacecontainer.cpp
    layersurfacecontainer.cpp
    rootsurfacecontainer.cpp
    surfaceproxy.cpp
    treeland.cpp
    wallpaperimage.cpp
    wallpapercontroller.cpp
    wallpapermanager.cpp
    workspacemodel.cpp
    propertymonitor.cpp
    tsgradiusimagenode.cpp
    tquickradiuseffect.cpp
    gestures.cpp
    togglablegesture.cpp
    inputdevice.cpp
    workspaceanimationcontroller.cpp
    surfacefilterproxymodel.cpp
    windowpicker.cpp
)

if (NOT DISABLE_DDM)
    list(APPEND CORE_HEADERS lockscreen.h)
    list(APPEND CORE_SRCS lockscreen.cpp)
endif()

qt_add_qml_module(treeland_core
    URI Treeland
    VERSION "2.0"

    SOURCES
        ${CORE_HEADERS}
        ${CORE_SRCS}
        ${DBUS_ADAPTOR}
    QML_FILES
        qml/PrimaryOutput.qml
        qml/CopyOutput.qml
        qml/TitleBar.qml
        qml/Decoration.qml
        qml/WindowMenu.qml
        qml/TaskBar.qml
        qml/SurfaceContent.qml
        qml/XdgShadow.qml
        qml/Border.qml
        qml/OutputMenuBar.qml
        qml/WorkspaceSwitcher.qml
        qml/WorkspaceProxy.qml
        qml/Animations/GeometryAnimation.qml
        qml/Animations/NewAnimation.qml
        qml/Animations/MinimizeAnimation.qml
        qml/Animations/ShowDesktopAnimation.qml
        qml/Animations/LaunchpadAnimation.qml
        qml/Animations/LayerShellAnimation.qml
        qml/Effects/Blur.qml
        qml/Effects/LaunchpadCover.qml
        qml/TaskSwitcher.qml
        qml/TaskWindowPreview.qml
        qml/SwitchViewDelegate.qml
        qml/SwitchViewHighlightDelegate.qml
        qml/DockPreview.qml
        qml/FadeBehavior.qml
        qml/CaptureSelectorLayer.qml
        qml/WindowPickerLayer.qml
    RESOURCE_PREFIX
        /qt/qml
    OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/qt/qml/Treeland
)

target_link_libraries(treeland_core
    PUBLIC
        Dtk6::Core
        Dtk6::Declarative
        Waylib::WaylibServer
        treeland_config
        foreign-toplevel
        primary-output
        personalization
        shortcut
        wallpaper-color
        window-management
        virtual-output
        dde-shell
        capture
        treeland_interface
        treeland_common
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickPrivate
        Qt6::DBus
        Qt6::Concurrent
        PkgConfig::PIXMAN
        PkgConfig::WAYLAND
        PkgConfig::LIBINPUT
)

if (NOT DISABLE_DDM)
    target_link_libraries(treeland_core
        PUBLIC
            greeter
        PRIVATE
            DDM::Common)
endif()

set(TRANSLATED_FILES)
if (${QT_VERSION_MINOR} LESS 7)
    qt_add_lupdate(treeland_core
        TS_FILES
            ${CMAKE_SOURCE_DIR}/translations/treeland.zh_CN.ts
            ${CMAKE_SOURCE_DIR}/translations/treeland.en_US.ts
        NO_GLOBAL_TARGET
    )

    qt_add_lrelease(treeland_core
        TS_FILES
            ${CMAKE_SOURCE_DIR}/translations/treeland.zh_CN.ts
            ${CMAKE_SOURCE_DIR}/translations/treeland.en_US.ts
        QM_FILES_OUTPUT_VARIABLE TRANSLATED_FILES
    )
elseif (${QT_VERSION_MINOR} GREATER_EQUAL 7)
    qt_add_lupdate(
        SOURCE_TARGETS treeland_core
        TS_FILES
            ${CMAKE_SOURCE_DIR}/translations/treeland.zh_CN.ts
            ${CMAKE_SOURCE_DIR}/translations/treeland.en_US.ts
        NO_GLOBAL_TARGET
    )

    qt_add_lrelease(
        TS_FILES
            ${CMAKE_SOURCE_DIR}/translations/treeland.zh_CN.ts
            ${CMAKE_SOURCE_DIR}/translations/treeland.en_US.ts
        QM_FILES_OUTPUT_VARIABLE TRANSLATED_FILES
    )
endif()

target_compile_definitions(treeland_core
    PRIVATE
    WLR_USE_UNSTABLE
)

qt_add_shaders(treeland_core "treeland_shaders_ng"
    BATCHABLE
    PRECOMPILE
    PREFIX
        "/shaders"
    BASE
        ${PROJECT_RESOURCES_DIR}/shaders
    FILES
        ${PROJECT_RESOURCES_DIR}/shaders/radiussmoothtexture.vert
        ${PROJECT_RESOURCES_DIR}/shaders/radiussmoothtexture.frag
)

install(FILES ${TRANSLATED_FILES} DESTINATION ${TREELAND_COMPONENTS_TRANSLATION_DIR})

install(FILES
    ${CORE_HEADERS}
    DESTINATION include/treeland
)

install(
    TARGETS
        treeland_core
        treeland_config
    # EXPORT TreelandTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# install(EXPORT TreelandTargets
#     FILE TreelandTargets.cmake
#     NAMESPACE Treeland::
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/treeland
# )

install(FILES treelandconfig.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/treeland/config")
