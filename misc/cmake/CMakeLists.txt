include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/TreelandConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/TreelandConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/treeland
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/TreelandCommonConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/TreelandCommonConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/treeland
    PATH_VARS CMAKE_INSTALL_PREFIX
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/TreelandConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/TreelandCommonConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/treeland
)
