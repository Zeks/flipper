cmake_minimum_required(VERSION 3.22.2)
project(Flipper_Project)

# SETTING UP COMPILE FEATURE BY ENVIRONMENT
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_LIST_DIR}/../../UI)


option(USE_POSTGRES "whether postregs is enabled" ON)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)

add_executable(flipper)
find_package(Qt5 REQUIRED COMPONENTS Core Sql Concurrent Widgets Gui Network Quick QuickWidgets Qml WebEngine WebEngineWidgets WebView)
target_link_libraries(flipper PUBLIC  Qt5::Core Qt5::Sql Qt5::Concurrent Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Quick
    Qt5::Qml Qt5::QuickWidgets Qt5::WebView Qt5::WebEngineWidgets Logger UniversalModels Sql)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable -Wno-unused-parameter -Wno-cast-function-type -Wno-implicit-fallthrough")

set_property(TARGET flipper PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(flipper PRIVATE protobuf dl grpc grpc++ gpr cares crypto ssl curl cpr pthread)
#["dl", "protobuf", "grpc", "grpc++", "gpr","cpr", "curl", "crypto", "ssl", "pthread"

target_compile_definitions(flipper PUBLIC USE_WEBVIEW "CLIENT_VERSION=1.4.5" "MAJOR_PROTOCOL_VERSION=2" "MINOR_PROTOCOL_VERSION=0")

add_subdirectory(../../libs/Logger build/logger)
add_subdirectory(../../libs/ui-models build/uimodels)
add_subdirectory(../../libs/sql build/sql)

include(debug.cmake)
include(grpc.cmake)
include(app.cmake)


set(_deps)
get_link_dependencies(flipper _deps)
foreach(_dep IN LISTS _deps)
    get_target_property(_srcs ${_dep} SOURCES)
    get_target_property(_src_dir ${_dep} SOURCE_DIR)
    foreach(_src IN LISTS _srcs)
        message("${_src_dir}/${_src}")
    endforeach()
endforeach()
