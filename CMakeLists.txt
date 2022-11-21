# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

set(DIRECTXMESH_VERSION 1.6.3)

project(DirectXMesh
  VERSION ${DIRECTXMESH_VERSION}
  DESCRIPTION "DirectXMesh geometry Library"
  HOMEPAGE_URL "https://go.microsoft.com/fwlink/?LinkID=324981"
  LANGUAGES CXX)

option(BUILD_TOOLS "Build meshconvert" ON)

# Includes the support for DirectX 12 input layouts
option(BUILD_DX12 "Build with DirectX12 Runtime support" ON)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")

if (DEFINED VCPKG_TARGET_ARCHITECTURE)
    set(DIRECTX_ARCH ${VCPKG_TARGET_ARCHITECTURE})
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Ww][Ii][Nn]32$")
    set(DIRECTX_ARCH x86)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Xx]64$")
    set(DIRECTX_ARCH x64)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]$")
    set(DIRECTX_ARCH arm)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$")
    set(DIRECTX_ARCH arm64)
endif()

include(GNUInstallDirs)

#--- Library
set(LIBRARY_HEADERS
    DirectXMesh/DirectXMesh.h
    DirectXMesh/DirectXMesh.inl)

set(LIBRARY_SOURCES
    DirectXMesh/DirectXMeshP.h
    DirectXMesh/scoped.h
    DirectXMesh/DirectXMeshAdjacency.cpp
    DirectXMesh/DirectXMeshClean.cpp
    DirectXMesh/DirectXMeshConcat.cpp
    DirectXMesh/DirectXMeshGSAdjacency.cpp
    DirectXMesh/DirectXMeshletGenerator.cpp
    DirectXMesh/DirectXMeshNormals.cpp
    DirectXMesh/DirectXMeshOptimize.cpp
    DirectXMesh/DirectXMeshOptimizeLRU.cpp
    DirectXMesh/DirectXMeshOptimizeTVC.cpp
    DirectXMesh/DirectXMeshRemap.cpp
    DirectXMesh/DirectXMeshTangentFrame.cpp
    DirectXMesh/DirectXMeshUtil.cpp
    DirectXMesh/DirectXMeshValidate.cpp
    DirectXMesh/DirectXMeshVBReader.cpp
    DirectXMesh/DirectXMeshVBWriter.cpp
    DirectXMesh/DirectXMeshWeldVertices.cpp)

add_library(${PROJECT_NAME} STATIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})

source_group(${PROJECT_NAME} REGULAR_EXPRESSION DirectXMesh/*.*)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/DirectXMesh>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

if (NOT MINGW)
    target_precompile_headers(${PROJECT_NAME} PRIVATE DirectXMesh/DirectXMeshP.h)
endif()

if (MINGW OR (NOT WIN32) OR VCPKG_TOOLCHAIN)
    message("INFO: Using VCPKG for DirectX-Headers and DirectXMath.")
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directxmath CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectX-Headers Microsoft::DirectXMath)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USING_DIRECTX_HEADERS)
endif()

#--- Utilities
set(UTILS_HEADERS
    Utilities/FlexibleVertexFormat.h
    Utilities/WaveFrontReader.h)

add_library(Utilities INTERFACE)

source_group(Utilities REGULAR_EXPRESSION Utilities/*.*)

target_include_directories(Utilities INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Utilities>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/utils>)

#--- Package
include(CMakePackageConfigHelpers)

string(TOLOWER ${PROJECT_NAME} PACKAGE_NAME)

write_basic_package_version_file(
  ${PACKAGE_NAME}-config-version.cmake
  VERSION ${DIRECTXMESH_VERSION}
  COMPATIBILITY AnyNewerVersion)

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT library)

install(TARGETS Utilities
  EXPORT Utilities-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT utils)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/.nuget/${PROJECT_NAME}-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME})

install(EXPORT ${PROJECT_NAME}-targets
  FILE ${PROJECT_NAME}-targets.cmake
  NAMESPACE Microsoft::
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME})

install(EXPORT Utilities-targets
  FILE Utilities-targets.cmake
  NAMESPACE Microsoft::DirectXMesh::
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME})

install(FILES ${LIBRARY_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES ${UTILS_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/utils)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME})

#--- Command-line tool
if(BUILD_TOOLS AND WIN32 AND (NOT WINDOWS_STORE))
  set(TOOL_EXES meshconvert)

  add_executable(meshconvert
    Meshconvert/Meshconvert.cpp
    Meshconvert/Meshconvert.rc
    Meshconvert/MeshOBJ.cpp
    Meshconvert/Mesh.h
    Meshconvert/Mesh.cpp
    Meshconvert/SDKMesh.h)
  target_include_directories(meshconvert PUBLIC MeshConvert Utilities)
  target_link_libraries(meshconvert ${PROJECT_NAME} version.lib)
  source_group(meshconvert REGULAR_EXPRESSION meshconvert/*.*)

  if (MINGW OR VCPKG_TOOLCHAIN)
    target_link_libraries(meshconvert Microsoft::DirectXMath)
  endif()
endif()

if(MSVC)
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE /Wall /GR- /fp:fast "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
      target_link_options(${t} PRIVATE /DYNAMICBASE /NXCOMPAT)
    endforeach()

    if((${CMAKE_SIZEOF_VOID_P} EQUAL 4) AND (NOT ${DIRECTX_ARCH} MATCHES "^arm"))
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_link_options(${t} PRIVATE /SAFESEH)
        endforeach()
    endif()

    if((MSVC_VERSION GREATER_EQUAL 1928) AND (CMAKE_SIZEOF_VOID_P EQUAL 8)
       AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)))
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_options(${t} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
        target_link_options(${t} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
      endforeach()
    endif()
endif()

if(NOT ${DIRECTX_ARCH} MATCHES "^arm")
    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    endif()

    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE ${ARCH_SSE2})
    endforeach()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(WarningsLib -Wall -Wpedantic -Wextra)
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsLib})

    set(WarningsEXE ${WarningsLib} "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-switch" "-Wno-switch-enum" "-Wno-double-promotion" "-Wno-exit-time-destructors" "-Wno-missing-prototypes")
    foreach(t IN LISTS TOOL_EXES)
      target_compile_options(${t} PRIVATE ${WarningsEXE})
    endforeach()
endif()
if(MINGW)
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE -Wno-ignored-attributes)
      target_link_options(${t} PRIVATE -municode)
    endforeach()
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE /sdl /permissive- /JMC- /Zc:__cplusplus)
    endforeach()

    if(ENABLE_CODE_ANALYSIS)
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_options(${t} PRIVATE /analyze)
      endforeach()
    endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.24)
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE /ZH:SHA_256)
        endforeach()
    endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE /Zc:preprocessor /wd5105)
        endforeach()
    endif()
    
    if ((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_link_options(${t} PRIVATE /CETCOMPAT)
        endforeach()
    endif()

    set(WarningsEXE "/wd4061" "/wd4365" "/wd4514" "/wd4625" "/wd4626" "/wd4627" "/wd4668" "/wd4710" "/wd4751" "/wd4820" "/wd5026" "/wd5027" "/wd5039" "/wd5045" "/wd5219")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
        list(APPEND WarningsEXE "/wd5262" "/wd5264")
    endif()
    foreach(t IN LISTS TOOL_EXES)
      target_compile_options(${t} PRIVATE ${WarningsEXE})
    endforeach()
endif()

if(WIN32)
    if(WINDOWS_STORE)
      target_compile_definitions(${PROJECT_NAME} PRIVATE WINAPI_FAMILY=WINAPI_FAMILY_APP)
    endif()

    if(BUILD_DX12 OR WINDOWS_STORE OR (${DIRECTX_ARCH} MATCHES "^arm64"))
        message("INFO: Building with DirectX 12 Runtime support")
        set(WINVER 0x0A00)
    elseif(${DIRECTX_ARCH} MATCHES "^arm")
        set(WINVER 0x0602)
    else()
        message("INFO: Building with Windows 7 compatibility")
        set(WINVER 0x0601)
    endif()

    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_definitions(${t} PRIVATE _UNICODE UNICODE _WIN32_WINNT=${WINVER})
    endforeach()
endif()

if(BUILD_TOOLS AND WIN32 AND (NOT WINDOWS_STORE))
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT meshconvert)
endif()

include(CTest)
if(BUILD_TESTING AND WIN32 AND (NOT WINDOWS_STORE) AND (EXISTS "${CMAKE_CURRENT_LIST_DIR}/Tests/CMakeLists.txt"))
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Tests)
endif()
