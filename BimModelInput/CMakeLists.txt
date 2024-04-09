project(BimModelInput)

file(GLOB headers ${CMAKE_CURRENT_LIST_DIR}/*.h)
#set(headers ${CMAKE_CURRENT_LIST_DIR}/../thirdparty/json/include/nlohmann/json.hpp)
file(GLOB srcs ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

set(3rd_dir ${CMAKE_SOURCE_DIR}/etop-3rd)

add_library(BimModelInput SHARED
  ${headers}
  ${srcs}
)

configure_dt_library(BimModelInput)

target_include_directories(
    BimModelInput
        SYSTEM PUBLIC
        ${CMAKE_INSTALL_INCLUDEDIR}
        ${CMAKE_CURRENT_LIST_DIR}/../thirdparty/json/include/nlohmann
)

target_compile_definitions(BimModelInput PRIVATE BIMMODELINPUT_EXPORTS)

target_include_directories(BimModelInput INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(BimModelInput PROPERTIES
PUBLIC_HEADER "BimModelInput.h")

target_link_libraries(BimModelInput 
    PUBLIC
    OpenSceneGraph
    glog::glog
    ZeroMQ
    crnlib
)
