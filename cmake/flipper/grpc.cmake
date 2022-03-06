set(REPOSITORY_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
get_filename_component(REPOSITORY_ROOT_ABSOLUTE ${REPOSITORY_ROOT} ABSOLUTE DIRECTORY)

set(PROTOBUF_FILES
${REPOSITORY_ROOT}/proto/search/filter
${REPOSITORY_ROOT}/proto/search/fanfic
${REPOSITORY_ROOT}/proto/search/fandom
${REPOSITORY_ROOT}/proto/statistics/favlist
${REPOSITORY_ROOT}/proto/recommendations/diagnostic_recommendations
${REPOSITORY_ROOT}/proto/recommendations/recommendations
${REPOSITORY_ROOT}/proto/server_base_structs
${REPOSITORY_ROOT}/proto/feeder_service
)

set(GRPC_FILES ${REPOSITORY_ROOT_ABSOLUTE}/proto/feeder_service)

find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

if(MSVC AND protobuf_MSVC_STATIC_RUNTIME)
  foreach(flag_var
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
  endforeach()
endif()

function(generate_cpp)
    include(CMakeParseArguments)
    set(_options)
    set(_singleargs MODE PLUGIN)
    set(_multiargs FILES)
    cmake_parse_arguments(GENERATE_CPP "${_options}" "${_singleargs}" "${_multiargs}" "${ARGN}")

    #message("passed files " ${GENERATE_CPP_FILES})
    #message("passed language " ${GENERATE_CPP_MODE})
    #message("passed plugin " ${GENERATE_CPP_PLUGIN})
    foreach(processed_file ${GENERATE_CPP_FILES})
        get_filename_component(_abs_file ${processed_file}.proto ABSOLUTE)
        set(${actual}_PROTOS ${_abs_file})
        set(_protobuf_include_path -I ${REPOSITORY_ROOT_ABSOLUTE}/proto )

        protobuf_generate(TARGET flipper
            LANGUAGE ${GENERATE_CPP_MODE}
            PLUGIN ${GENERATE_CPP_PLUGIN}
            PROTOS  ${${actual}_PROTOS}
            PROTOC_OUT_DIR ${REPOSITORY_ROOT_ABSOLUTE}/proto
            )
    endforeach()
endfunction()

generate_cpp(FILES ${PROTOBUF_FILES}
             MODE CPP
             PLUGIN "")

generate_cpp(FILES ${GRPC_FILES}
          MODE GRPC
          PLUGIN "protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin")


