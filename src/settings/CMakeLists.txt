qt_add_library(treelandconfig SHARED
    dconfig-autogen.hpp
    treelandconfig.h
    treelandconfig.cpp
)

target_include_directories(treelandconfig INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(treelandconfig
    PUBLIC
        Dtk6::Core
        Qt6::Quick
)