cmake_minimum_required(VERSION 3.20)

project(PluginIfcplusplus)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

set(3rd_dir ${CMAKE_SOURCE_DIR}/etop-3rd)
set(ifcppsrc ${CMAKE_SOURCE_DIR}/etop-3rd/etop_ifcpp/IfcPlusPlus/src)

#file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
#list(REMOVE_ITEM headers ${CMAKE_CURRENT_SOURCE_DIR}/LabGeometryConverter_old.h)
#file(GLOB srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

set(headers framework.h LabGeometryConverter.h
 LabRepresentationConverter.h LabStylesConverter.h md5.h
 PluginIfcpplast.h 
 )
set(srcs md5.cpp PluginIfcpplast.cpp)

find_package(glog REQUIRED)
#find_package(OpenMP)

#add_library(PluginIfcplusplus SHARED ${headers} ${srcs})

target_compile_definitions(PluginIfcplusplus PRIVATE 
    PLUGINIFCPLUSPLUS_EXPORTS
)
target_include_directories(PluginIfcplusplus INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(PluginIfcplusplus PRIVATE
    ${3rd_dir}/json
    ${3rd_dir}/stb/include/

    ${ifcppsrc}/
    ${ifcppsrc}/ifcpp/
    ${ifcppsrc}/ifcpp/model
    ${ifcppsrc}/ifcpp/reader
    ${ifcppsrc}/ifcpp/writer
    ${ifcppsrc}/ifcpp/geometry/
    ${ifcppsrc}/ifcpp/IFC4X3/include
    ${ifcppsrc}/external
    ${ifcppsrc}/external/glm
    ${ifcppsrc}/external/Carve/include
    ${ifcppsrc}/external/Carve/src
    ${ifcppsrc}/external/Carve/src/include
    ${ifcppsrc}/external/Carve/src/common
    ${ifcppsrc}/external/Carve/build/src
    ${ifcppsrc}/external/glm
    ${ifcppsrc}/external/manifold/src/utilities/include
    ${ifcppsrc}/external/manifold/src/utilities
    ${ifcppsrc}/external/manifold/src/polygon/include
    ${ifcppsrc}/external/manifold/src/manifold/include
    ${ifcppsrc}/external/manifold/src
    ${ifcppsrc}/external/manifold/src/collider/include
    ${ifcppsrc}/external/manifold/src/third_party
    ${ifcppsrc}/external/manifold/src/third_party/thrust
    ${ifcppsrc}/external/manifold/src/third_party/graphlite/include

    /usr/include/
    /usr/local/include

    ${3rd_dir}/crunch-master/inc
    ${3rd_dir}/gflags-master/build/include

)

target_link_directories(PluginIfcplusplus PRIVATE
    ${3rd_dir}/gflags-master/build/lib
    ${3rd_dir}/crunch-master/build/crnlib/
    #${3rd_dir}/etop_ifcpp/build/Release
    ${3rd_dir}/etop_ifcpp/build_debug/Debug
    /usr/local/bin/
    /usr/lib/x86_64-linux-gnu/
)

target_link_libraries(PluginIfcplusplus PRIVATE 
    BimModelInput
    libosg.so
    libosgDB.so
    libosgUtil.so
    libosgText.so
    libOpenThreads.so
    freeimage.so
    #libzmq.so
    #libcrnlib.so
    libgflags.a
    libIfcPlusPlusd.a
    glog::glog
    #OpenMP::OpenMP_CXX
)