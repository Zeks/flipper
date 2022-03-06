cmake_minimum_required(VERSION 3.22.2)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

project(Flipper_Project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_LIST_DIR}/../../UI)


option(USE_POSTGRES "whether postregs is enabled" ON)
option(BUILD_SHARED_LIBS "build libs" ON)



add_executable(flipper)
target_include_directories(flipper PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../)

find_package(Qt5 REQUIRED COMPONENTS Core Sql Concurrent Widgets Gui Network Quick QuickWidgets Qml WebEngine WebEngineWidgets WebView)
target_link_libraries(flipper PRIVATE  Qt5::Core Qt5::Sql Qt5::Concurrent Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Quick
    Qt5::Qml Qt5::QuickWidgets Qt5::WebView Qt5::WebEngineWidgets )
target_link_libraries(flipper PRIVATE protobuf dl grpc grpc++ gpr cares crypto ssl curl cpr pthread)
target_link_libraries(flipper PRIVATE Logger UniversalModels Sql)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable -Wno-unused-parameter -Wno-cast-function-type -Wno-implicit-fallthrough")

target_compile_definitions(flipper PUBLIC USE_WEBVIEW "CLIENT_VERSION=1.4.5" "MAJOR_PROTOCOL_VERSION=2" "MINOR_PROTOCOL_VERSION=0")

add_subdirectory(../../libs/Logger build/logger)
add_subdirectory(../../libs/ui-models build/uimodels)
add_subdirectory(../../libs/sql build/sql)

include(debug.cmake)
include(grpc.cmake)
include(app.cmake)
