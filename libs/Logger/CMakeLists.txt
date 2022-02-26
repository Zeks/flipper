cmake_minimum_required(VERSION 3.22.2)
project(Logger)

find_package(Qt5 REQUIRED COMPONENTS Core Sql Concurrent)

add_library(Logger STATIC
    "src/QsLog.cpp"
    "src/QsLogDest.cpp"
    "src/QsLogDestConsole.cpp"
    "src/QsLogDestFile.cpp"
    #"include/logger/l_logger_global.h"
    "include/logger/QsLog.h"
    "include/logger/QsLogDest.h"
    "include/logger/QsLogDestConsole.h"
    "include/logger/QsLogDestFile.h"
    "include/logger/QsLogDisableForThisFile.h"
    "include/logger/QsLogLevel.h"
    "include/logger/Tracer.h"
    "include/logger/QsLogger.h"
    )

target_link_libraries(Logger PUBLIC  Qt5::Core Qt5::Sql Qt5::Concurrent)

target_include_directories(Logger PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/logger
    ${CMAKE_CURRENT_LIST_DIR}/../)

include(GenerateExportHeader)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

generate_export_header(Logger
    EXPORT_FILE_NAME "${CMAKE_CURRENT_LIST_DIR}/include/logger/l_logger_global.h"
    EXPORT_MACRO_NAME "L_LOGGERSHARED_EXPORT")

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(Logger PUBLIC LOGGER_STATIC_DEFINE)
endif()

install(TARGETS Logger DESTINATION ${CMAKE_CURRENT_LIST_DIR}/../../debug)
