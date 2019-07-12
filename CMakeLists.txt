# DirectXMesh geometry Library
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#
# http://go.microsoft.com/fwlink/?LinkID=324981

cmake_minimum_required (VERSION 3.8)
project (DirectXMesh_CMake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")

set(WarningsLib "-Wall" "-Wpedantic" "-Wextra")
set(WarningsEXE ${WarningsLib} "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-double-promotion" "-Wno-exit-time-destructors" "-Wno-missing-prototypes")

add_library (directxmesh STATIC
    DirectXMesh/DirectXMesh.h
    DirectXMesh/DirectXMeshP.h
    DirectXMesh/scoped.h
    DirectXMesh/DirectXMeshAdjacency.cpp
    DirectXMesh/DirectXMeshClean.cpp
    DirectXMesh/DirectXMeshGSAdjacency.cpp
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
    DirectXMesh/DirectXMeshWeldVertices.cpp
)

target_include_directories( directxmesh PUBLIC DirectXMesh )

target_compile_options( directxmesh PRIVATE /fp:fast )

add_executable(meshconvert
    meshconvert/meshconvert.cpp
    meshconvert/MeshOBJ.cpp
    meshconvert/Mesh.h
    meshconvert/Mesh.cpp
    meshconvert/SDKMesh.h)
target_include_directories(meshconvert PUBLIC MeshConvert Utilities)
target_link_libraries(meshconvert directxmesh)

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    target_compile_options( directxmesh PRIVATE ${WarningsLib} )
    target_compile_options( meshconvert PRIVATE ${WarningsEXE} )
    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
        target_compile_options( directxmesh PRIVATE /arch:SSE2 )
        target_compile_options( meshconvert PRIVATE /arch:SSE2 )
    endif()
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options( directxmesh PRIVATE /Wall /permissive- /Zc:__cplusplus )
    target_compile_options( meshconvert PRIVATE /Wall /permissive- /Zc:__cplusplus )
endif()

# Windows 10 is used here to build the DirectX 12 code paths as well as 11
add_compile_definitions(_UNICODE UNICODE _WIN32_WINNT=0x0A00)
