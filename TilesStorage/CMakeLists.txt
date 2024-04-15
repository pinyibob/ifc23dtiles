project(TilesStorage)

file(GLOB headers ${CMAKE_CURRENT_LIST_DIR}/*.h)
file(GLOB srcs ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

set(3rd_dir ${CMAKE_SOURCE_DIR}/dt-3rd)

add_library(TilesStorage SHARED
  ${headers}
  ${srcs}
)

configure_dt_library(TilesStorage)

#find_package(mongo-c-driver)

target_compile_definitions(TilesStorage PRIVATE 
    TILESSTORAGE_EXPORTS
    MONGOC_STATIC
    BSON_STATIC
)

target_include_directories(TilesStorage INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(TilesStorage PRIVATE
    #直接用linux包管理器下载 cmake暂时不好配置
    /usr/local/include/libbson-1.0
    /usr/local/include/libmongoc-1.0
    #/usr/include/
    ${3rd_dir}/json/include/
    #${3rd_dir}/mongo-c-driver/src/libmongoc/src/
    #${3rd_dir}/mongo-c-driver/src/libbson/src/

)

target_link_directories(TilesStorage PRIVATE
    #/usr/local/bin/
    #${3rd_dir}/gflags/build/lib
)

target_link_libraries(TilesStorage PRIVATE 
    BimModelInput
    #libbson
    #libmongoc
    libmongoc-static-1.0.a
    libbson-static-1.0.a
)