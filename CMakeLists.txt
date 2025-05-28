# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

set(DIRECTXMESH_VERSION 1.6.8)

project(DirectXMesh
  VERSION ${DIRECTXMESH_VERSION}
  DESCRIPTION "DirectXMesh geometry Library"
  HOMEPAGE_URL "https://go.microsoft.com/fwlink/?LinkID=324981"
  LANGUAGES CXX)

if(DEFINED XBOX_CONSOLE_TARGET)
  set(CMAKE_CXX_STANDARD_LIBRARIES "")
endif()

option(BUILD_TOOLS "Build meshconvert" ON)

option(BUILD_SHARED_LIBS "Build DirectXMesh as a shared library" OFF)

# Includes the support for DirectX 12 input layouts
option(BUILD_DX12 "Build with DirectX12 Runtime support" ON)

# https://devblogs.microsoft.com/cppblog/spectre-mitigations-in-msvc/
option(ENABLE_SPECTRE_MITIGATION "Build using /Qspectre for MSVC" OFF)

option(DISABLE_MSVC_ITERATOR_DEBUGGING "Disable iterator debugging in Debug configurations with the MSVC CRT" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

option(ENABLE_CODE_COVERAGE "Build with code-coverage" OFF)

option(BUILD_FUZZING "Build for fuzz testing" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(WINDOWS_STORE OR (DEFINED XBOX_CONSOLE_TARGET))
  set(BUILD_DX12 ON)
  set(BUILD_TOOLS OFF)
endif()

include(GNUInstallDirs)
include(build/CompilerAndLinker.cmake)

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

if(WIN32 AND BUILD_SHARED_LIBS)
  message(STATUS "Build library as a DLL")

  configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/build/DirectXMesh.rc.in"
      "${CMAKE_CURRENT_BINARY_DIR}/DirectXMesh.rc" @ONLY)

  add_library(${PROJECT_NAME} SHARED ${LIBRARY_SOURCES} ${LIBRARY_HEADERS} "${CMAKE_CURRENT_BINARY_DIR}/DirectXMesh.rc")

  target_compile_definitions(${PROJECT_NAME} PRIVATE DIRECTX_MESH_EXPORT)
  target_compile_definitions(${PROJECT_NAME} INTERFACE DIRECTX_MESH_IMPORT)

  if(XBOX_CONSOLE_TARGET MATCHES "scarlett|xboxone")
    target_link_libraries(${PROJECT_NAME} PRIVATE xgameplatform.lib)
  endif()
else()
  add_library(${PROJECT_NAME} ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})
endif()

source_group(${PROJECT_NAME} REGULAR_EXPRESSION DirectXMesh/*.*)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/DirectXMesh>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

if(NOT MINGW)
    target_precompile_headers(${PROJECT_NAME} PRIVATE DirectXMesh/DirectXMeshP.h)
endif()

if(MINGW OR (NOT WIN32))
    find_package(directxmath CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectXMath)

    find_package(directx-headers CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectX-Headers)
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_DIRECTX_HEADERS)
else()
    find_package(directxmath CONFIG QUIET)
    find_package(directx-headers CONFIG QUIET)
endif()

if(directxmath_FOUND)
    message(STATUS "Using DirectXMath package")
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectXMath)
endif()

if(directx-headers_FOUND)
    message(STATUS "Using DirectX-Headers package")
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectX-Headers)
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

# Create pkg-config file
include(build/JoinPaths.cmake)
# from: https://github.com/jtojnar/cmake-snips#concatenating-paths-when-building-pkg-config-files
join_paths(DIRECTXMESH_INCLUDEDIR_FOR_PKG_CONFIG "\${prefix}" "${CMAKE_INSTALL_INCLUDEDIR}")
join_paths(DIRECTXMESH_LIBDIR_FOR_PKG_CONFIG "\${prefix}"     "${CMAKE_INSTALL_LIBDIR}")

set(DIRECTXMESH_DEP_L "")
if(directxmath_FOUND)
  list(APPEND DIRECTXMESH_DEP_L "DirectXMath")
endif()
if(directx-headers_FOUND)
  list(APPEND DIRECTXMESH_DEP_L "DirectX-Headers")
endif()
list(LENGTH DIRECTXMESH_DEP_L DEP_L)
if(DEP_L)
  string(REPLACE ";" ", " DIRECTXMESH_DEP " ${DIRECTXMESH_DEP_L}")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/build/DirectXMesh.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DirectXMesh.pc" @ONLY)

# Install the pkg-config file
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DirectXMesh.pc"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(TARGETS Utilities
  EXPORT Utilities-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT utils)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/build/${PROJECT_NAME}-config.cmake.in
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
if(BUILD_TOOLS AND WIN32)
  set(TOOL_EXES meshconvert)

  add_executable(meshconvert
    Meshconvert/Meshconvert.cpp
    Meshconvert/Meshconvert.rc
    Meshconvert/settings.manifest
    Meshconvert/CmdLineHelpers.h
    Meshconvert/MeshOBJ.cpp
    Meshconvert/Mesh.h
    Meshconvert/Mesh.cpp
    Meshconvert/CMO.h
    Meshconvert/SDKMesh.h
    Meshconvert/vbo.h)
  target_compile_features(meshconvert PRIVATE cxx_std_17)
  target_include_directories(meshconvert PRIVATE MeshConvert Utilities)
  target_link_libraries(meshconvert PRIVATE ${PROJECT_NAME} version.lib)
  source_group(meshconvert REGULAR_EXPRESSION meshconvert/*.*)

  if(directxmath_FOUND)
    target_link_libraries(meshconvert PRIVATE Microsoft::DirectXMath)
  endif()
endif()

if(TOOL_EXES)
  message(STATUS "Building tools: ${TOOL_EXES}")
endif()

if(MSVC)
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE /Wall /GR-)
    endforeach()
endif()

foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
  target_compile_definitions(${t} PRIVATE ${COMPILER_DEFINES})
  target_compile_options(${t} PRIVATE ${COMPILER_SWITCHES})
  target_link_options(${t} PRIVATE ${LINKER_SWITCHES})
endforeach()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")
    set(WarningsLib -Wall -Wpedantic -Wextra)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        list(APPEND WarningsLib "-Wno-unsafe-buffer-usage")
    endif()
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsLib})

    set(WarningsEXE ${WarningsLib} "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-switch" "-Wno-switch-enum" "-Wno-switch-default" "-Wno-double-promotion" "-Wno-exit-time-destructors" "-Wno-missing-prototypes")
    foreach(t IN LISTS TOOL_EXES)
      target_compile_options(${t} PRIVATE ${WarningsEXE})
    endforeach()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE "-Wno-ignored-attributes" "-Walloc-size-larger-than=4GB")

      if(BUILD_SHARED_LIBS)
        target_compile_options(${t} PRIVATE "-Wno-attributes")
      endif()
    endforeach()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if(ENABLE_CODE_ANALYSIS)
      message(STATUS "Building with Code Analysis (PREFIX)")
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_options(${t} PRIVATE /analyze /WX)
      endforeach()
    endif()

    if(ENABLE_SPECTRE_MITIGATION
       AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.13)
       AND (NOT WINDOWS_STORE))
        message(STATUS "Building Spectre-mitigated libraries")
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE "/Qspectre")
        endforeach()
    endif()

    set(WarningsEXE "/wd4061" "/wd4365" "/wd4514" "/wd4571" "/wd4625" "/wd4626" "/wd4627" "/wd4668" "/wd4710" "/wd4711" "/wd4751" "/wd4774" "/wd4820" "/wd5026" "/wd5027" "/wd5039" "/wd5045" "/wd5219")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
      list(APPEND WarningsEXE "/wd5262" "/wd5264")
    endif()
    foreach(t IN LISTS TOOL_EXES)
      target_compile_options(${t} PRIVATE ${WarningsEXE})
    endforeach()

    if(BUILD_FUZZING AND (NOT WINDOWS_STORE))
      string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
      string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})

      if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.32)
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE ${ASAN_SWITCHES})
          target_link_libraries(${t} PRIVATE ${ASAN_LIBS})
        endforeach()
      endif()
    endif()
endif()

if(WIN32)
    if(BUILD_DX12 OR (${DIRECTX_ARCH} MATCHES "^arm64"))
        message(STATUS "Building with DirectX 12 Runtime support")
        set(WINVER 0x0A00)
    elseif(${DIRECTX_ARCH} MATCHES "^arm")
        set(WINVER 0x0602)
    else()
        message(STATUS "Building with Windows 8.1 compatibility")
        set(WINVER 0x0603)
    endif()

    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_definitions(${t} PRIVATE _WIN32_WINNT=${WINVER})
    endforeach()

    if(DISABLE_MSVC_ITERATOR_DEBUGGING)
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_definitions(${t} PRIVATE _ITERATOR_DEBUG_LEVEL=0)
      endforeach()
    endif()
endif()

if(BUILD_TOOLS AND WIN32)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT meshconvert)
endif()

if(BUILD_TOOLS AND (NOT VCPKG_TOOLCHAIN))
    foreach(t IN LISTS TOOL_EXES)
      install(TARGETS ${t} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endforeach()
endif()

#--- Test suite
if(WIN32 AND (NOT WINDOWS_STORE) AND (NOT (DEFINED XBOX_CONSOLE_TARGET)))
    include(CTest)
    if(BUILD_TESTING AND (EXISTS "${CMAKE_CURRENT_LIST_DIR}/Tests/CMakeLists.txt"))
        enable_testing()
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Tests)

        if(ENABLE_CODE_COVERAGE AND (DEFINED COV_COMPILER_SWITCHES))
          target_compile_options(${PROJECT_NAME} PRIVATE ${COV_COMPILER_SWITCHES})
        endif()
    elseif(BUILD_FUZZING AND (EXISTS "${CMAKE_CURRENT_LIST_DIR}/Tests/fuzzloaders/CMakeLists.txt"))
        message(STATUS "Building for fuzzing")
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Tests/fuzzloaders)
    endif()
endif()
