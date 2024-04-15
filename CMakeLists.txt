cmake_minimum_required(VERSION 3.15)

project(szifc23dtiles)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(iproject_ARCHITECTURE "aarch64")
    set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
    set(ANDROID_ABI ${CMAKE_ANDROID_ARCH_ABI})
    set(ANDROID_NDK ${CMAKE_ANDROID_NDK})
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    set(iproject_ARCHITECTURE "arm64")
else()
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(iproject_ARCHITECTURE "64")
    else()
        set(iproject_ARCHITECTURE "32")
    endif()
endif()

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/binary)
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SYSTEM_NAME}-x${iproject_ARCHITECTURE})

set(iproject_DEBUG_POSTFIX "d")
set(iproject_RELEASE_POSTFIX "")

set(CMAKE_DEBUG_POSTFIX ${iproject_DEBUG_POSTFIX})
set(CMAKE_RELEASE_POSTFIX ${iproject_RELEASE_POSTFIX})
set(CMAKE_MINSIZEREL_POSTFIX ${iproject_RELEASE_POSTFIX})
set(CMAKE_RELWITHDEBINFO_POSTFIX ${iproject_RELEASE_POSTFIX})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

function(configure_dt_library targetName)
    # if (MSVC)
    #     target_compile_options(${targetName} PRIVATE /W4 /WX /wd4201 /bigobj)
    # else()
    #     target_compile_options(${targetName} PRIVATE -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow -Wsign-conversion)
    # endif()

    set_target_properties(${targetName} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
endfunction()

add_subdirectory(dt-3rd EXCLUDE_FROM_ALL)

add_subdirectory(BimModelInput)
add_subdirectory(PluginIfcpplast)
add_subdirectory(TilesStorage)
add_subdirectory(bim23dtiles)

#install(TARGETS draco)

#### install steps todo!

# set_target_properties(BimModelInput PROPERTIES INSTALL_RPATH "$ORIGIN/")
# set_target_properties(PluginIfcplusplus PROPERTIES INSTALL_RPATH "$ORIGIN/")
# set_target_properties(TilesStorage PROPERTIES INSTALL_RPATH "$ORIGIN/")
# set_target_properties(bim23dtiles PROPERTIES INSTALL_RPATH "$ORIGIN/")