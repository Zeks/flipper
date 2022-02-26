cmake_minimum_required(VERSION 3.22.2)
project(Flipper_Project)

# SETTING UP COMPILE FEATURE BY ENVIRONMENT
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(USE_POSTGRES "whether postregs is enabled" ON)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)

add_subdirectory(../../libs/Logger build/logger)
add_subdirectory(../../libs/ui-models build/uimodels)
add_subdirectory(../../libs/sql build/sql)
