project(BimModelInput)

file(GLOB headers ${CMAKE_CURRENT_LIST_DIR}/*.h)
file(GLOB srcs ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

add_library(BimModelInput SHARED
  ${headers}
  ${srcs}
)

configure_dt_library(BimModelInput)

target_include_directories(
    BimModelInput
        SYSTEM PUBLIC
        ${CMAKE_INSTALL_INCLUDEDIR}
        ${CMAKE_CURRENT_LIST_DIR}/../dt-3rd/json/include/
        ${CMAKE_CURRENT_LIST_DIR}/../dt-3rd/stb/
        ${CMAKE_CURRENT_LIST_DIR}/../dt-3rd/OpenSceneGraph/include/
)

target_compile_definitions(BimModelInput PRIVATE BIMMODELINPUT_EXPORTS)

target_include_directories(BimModelInput INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(BimModelInput PROPERTIES
PUBLIC_HEADER "BimModelInput.h")

target_link_libraries(BimModelInput 
    PUBLIC

    glog
    crnlib
)
