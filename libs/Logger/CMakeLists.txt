cmake_minimum_required(VERSION 3.22.2)
# SETTING UP COMPILE FEATURE BY ENVIRONMENT
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Sql Concurrent)

project(Logger)

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

#fetching path to the output, testing if the outside variable is set, if it's not do it here
if (NOT ${PROJECT_OUTPUT_PATH})
    set(PROJECT_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_OUTPUT_PATH}+"/debug")
set(CMAKE_PDB_OUTPUT_DIRECTORY_DEBUG ${PROJECT_OUTPUT_PATH}+"/debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_PATH}+"/debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_PATH}+"/debug")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_OUTPUT_PATH}+"/release")
set(CMAKE_PDB_OUTPUT_DIRECTORY_RELEASE ${PROJECT_OUTPUT_PATH}+"/release")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_PATH}+"/release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_PATH}+"/release")

if(CMAKE_COMPILER_IS_GNUCXX)
    find_program(CCACHE ccache)
    if(CCACHE)
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
    else()
        message(WARNING "ccache nto found")
    endif()
endif()


target_include_directories(Logger PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/logger
    ${CMAKE_CURRENT_SOURCE_DIR}/../    )

include(GenerateExportHeader)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

generate_export_header(Logger
    EXPORT_FILE_NAME "${CMAKE_CURRENT_LIST_DIR}/include/logger/l_logger_global.h"
    EXPORT_MACRO_NAME "L_LOGGERSHARED_EXPORT")

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(Logger PUBLIC LOGGER_STATIC_DEFINE)
endif()

