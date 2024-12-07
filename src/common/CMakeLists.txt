set(PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/src/common/loginddbustypes.h
)

set(SRCS
    ${CMAKE_SOURCE_DIR}/src/common/loginddbustypes.cpp
)

qt_add_library(treeland_common SHARED
    ${PUBLIC_HEADERS}
    ${SRCS}
)

set_target_properties(treeland_common PROPERTIES
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR}
    OUTPUT_NAME treeland-common
    EXPORT_NAME Common
)

target_link_libraries(treeland_common
    PRIVATE
        Qt${QT_MAJOR_VERSION}::DBus
        Qt${QT_MAJOR_VERSION}::Network
        Qt${QT_MAJOR_VERSION}::Qml
)

target_include_directories(treeland_common INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/common>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src/common>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/treeland/common>
)

target_link_directories(treeland_common INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>
)

install(
    TARGETS treeland_common
    EXPORT CommonTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include/treeland/common
)

install(FILES ${PUBLIC_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/treeland/common")

install(EXPORT CommonTargets
        FILE CommonTargets.cmake
        NAMESPACE Treeland::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Treeland
)
