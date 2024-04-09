cmake_minimum_required(VERSION 3.20)

project(TilesStorage)

#option(debug "debug build type or release" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

set(3rd_dir ${CMAKE_SOURCE_DIR}/etop-3rd)

file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(TilesStorage SHARED ${headers} ${srcs})

target_compile_definitions(TilesStorage PRIVATE 
    TILESSTORAGE_EXPORTS
    MONGOC_STATIC
    BSON_STATIC
)

target_include_directories(TilesStorage INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(TilesStorage PRIVATE
    ${3rd_dir}/json

    /usr/local/include
    /usr/local/include/libbson-1.0
    /usr/local/include/libmongoc-1.0
    /usr/include/
    ${3rd_dir}/crunch-master/inc
    ${3rd_dir}/gflags-master/build/include
)

target_link_directories(TilesStorage PRIVATE
    /usr/lib/x86_64-linux-gnu/
    ${3rd_dir}/crunch-master/build/crnlib/
    ${3rd_dir}/gflags-master/build/lib
)

target_link_libraries(TilesStorage PRIVATE 
    BimModelInput
    libmongoc-static-1.0.a
    libbson-static-1.0.a
)