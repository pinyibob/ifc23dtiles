project(bim23dtiles)

set(3rd_dir ${CMAKE_SOURCE_DIR}/dt-3rd)

# Register.h Register.cpp 
set(headers stdafx.h B3dm.h binpack.hpp BintreeSplitor.h Cesium.h custom_material.h 
	ModelInput.h OsgUtil.h PostProcess.h  resource.h Scene2Gltf.h
	SceneOutput.h SceneOutputConfig.h SubScene.h SubScenePacker.h
	targetver.h TexturePacker.h tiny_gltf.h VcgUtil.h
)	
set(srcs stdafx.cpp B3dm.cpp bim23dtiles.cpp BintreeSplitor.cpp Cesium.cpp
	custom_json.cpp custom_material.cpp draco_gltf.cpp flat_shaders.cpp
	ModelInput.cpp ModelInputGdal.cpp normal_shaders.cpp OsgUtil.cpp
	PostProcess.cpp Scene2Gltf.cpp SceneOutput.cpp
	SceneOutputConfig.cpp SubScene.cpp SubScenePacker.cpp 
	TexturePacker.cpp tinygltf_impl.cpp VcgUtil.cpp
)

find_package(glog REQUIRED)

add_executable(bim23dtiles ${headers} ${srcs})

configure_dt_library(bim23dtiles)

#target_compile_definitions(bim23dtiles PRIVATE 
#    bim23dtiles_EXPORTS
#)

target_include_directories(bim23dtiles PRIVATE
    ${3rd_dir}/stb/include/
    ${3rd_dir}/json/

    #/usr/include/
    #/usr/local/include
    #/usr/local/include/ifcpp/IFC4X3/include

    ${3rd_dir}/crunch/inc/
    ${3rd_dir}/OpenSceneGraph/src/
    ${3rd_dir}/freeimage/source/
    ${3rd_dir}/draco/src/

    #${3rd_dir}/gflags-master/build/include

    ${3rd_dir}/ifcplusplus/IfcPlusPlus/src/ifcpp/geometry
    ${3rd_dir}/ifcplusplus/IfcPlusPlus/src/external
    ${3rd_dir}/ifcplusplus/IfcPlusPlus/src/external/Carve/src/include

    ${3rd_dir}/vcglib/
    ${3rd_dir}/vcglib/eigenlib/

)

target_link_directories(bim23dtiles PRIVATE
    #${3rd_dir}/crunch-master/build/crnlib/
    #${3rd_dir}/gflags-master/build/lib/
    
    #/usr/lib/x86_64-linux-gnu/
    /usr/local/bin/
)

# target_link_libraries(bim23dtiles PRIVATE
#     BimModelInput
#     TilesStorage
#     PluginIfcplusplus
#     #shlwapi.lib
#     #ws2_32.lib
#     osgd.lib
#     #osgdbd.lib
#     #osgutild.lib
#     #osgtextd.lib
#     #OpenThreadsd.lib
#     #freeimaged.lib
#     gflags
#     glog
#     IfcPlusPlus
#     vcglib
#     gdal
#     draco
#     freeimage
    
#     #Carve
#     #register_engine.lib
#     #botand.lib
# )
target_link_libraries(bim23dtiles PRIVATE
    BimModelInput
    TilesStorage

    libosg.so
    libosgDB.so
    libosgUtil.so
    libosgText.so
    libOpenThreads.so
    
    freeimage.so
    glog
    gflags

    libdraco.so
    libbson-1.0.so
    libgdal.so
)