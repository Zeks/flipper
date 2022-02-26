#set(REPOSITORY_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(REPOSITORY_ROOT /home/zeks/flipper)
set(HOME /home/zeks)

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


set(GRPC_FILES ${REPOSITORY_ROOT}/proto/feeder_service)
set(PROTOBUF_OUTPUT)

find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

if(MSVC AND protobuf_MSVC_STATIC_RUNTIME)
  foreach(flag_var
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
  endforeach()
endif()

#include_directories(${Protobuf_INCLUDE_DIRS})
#include_directories(/home/zeks/flipper/proto/search)

foreach(processed_file ${PROTOBUF_FILES})
    set(${fiddle}_PROTOS ${processed_file}.proto)
    set(_protobuf_include_path -I ${REPOSITORY_ROOT}/proto )
    #set(protobuf_generate_PROTOC_OUT_DIR ${REPOSITORY_ROOT})

    protobuf_generate(TARGET flipper
        PROTOS  ${${fiddle}_PROTOS}
        PROTOC_OUT_DIR ${REPOSITORY_ROOT}/proto
        )
endforeach()

foreach(processed_file ${GRPC_FILES})
    set(${fiddle}_PROTOS ${processed_file}.proto)
    set(_protobuf_include_path -I ${REPOSITORY_ROOT}/proto )

    protobuf_generate(TARGET flipper
        LANGUAGE GRPC
        PLUGIN "protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin"
        PROTOS  ${${fiddle}_PROTOS}
        PROTOC_OUT_DIR ${REPOSITORY_ROOT}/proto
        )
endforeach()

target_include_directories(flipper PUBLIC ${REPOSITORY_ROOT})
