DIRECTX MESH LIBRARY (DirectXMesh)
------------------------------------

Copyright (c) Microsoft Corporation. All rights reserved.

August 21, 2019

This package contains DirectXMesh, a shared source library for performing various geometry
content processing operations including generating normals and tangent frames, triangle
adjacency computations, and vertex cache optimization.

This code is designed to build with Visual Studio 2015 Update 3, Visual Studio 2017,
or Visual Studio 2019. It is recommended that you make use of VS 2015 Update 3,
Windows Tools 1.4.1, and the Windows 10 Anniversary Update SDK (14393) -or-
VS 2017 (15.9 update) / VS 2019 with the Windows 10 October 2018 Update SDK (17763).

These components are designed to work without requiring any content from the
legacy DirectX SDK. For details, see "Where is the DirectX SDK?"
<https://aka.ms/dxsdk>.

DirectXMesh\
    This contains the DirectXMesh library.

    Note that the majority of the header files here are intended for internal implementation
    of the library only (DirectXMeshP.h and scoped.h). Only DirectXMesh.h is meant as a
    'public' header for the library.

Utilities\
    This contains helper code related to mesh processing that is not general enough to be
    part of the DirectXMesh library.

         WaveFrontReader.h - Contains a simple C++ class for reading mesh data from a WaveFront OBJ file.

Meshconvert\
    This DirectXMesh sample is an implementation of the "meshconvert" command-line texture utility
    from the DirectX SDK utilizing DirectXMesh rather than D3DX.

    Note this tool does not support legacy .X files, but can export CMO, SDKMESH, and VBO files.

All content and source code for this package are subject to the terms of the MIT License.
<http://opensource.org/licenses/MIT>.

Documentation is available at <https://github.com/Microsoft/DirectXMesh/wiki>.

For the latest version of DirectXMesh, bug reports, etc. please visit the project site.

http://go.microsoft.com/fwlink/?LinkID=324981

This project has adopted the Microsoft Open Source Code of Conduct. For more information see the
Code of Conduct FAQ or contact opencode@microsoft.com with any additional questions or comments.

https://opensource.microsoft.com/codeofconduct/


-------------
RELEASE NOTES
-------------

* The VS 2017/2019 projects make use of /permissive- for improved C++ standard
  conformance. Use of a Windows 10 SDK prior to the Fall Creators Update (16299)
  or an Xbox One XDK prior to June 2017 QFE 4 may result in failures due to
  problems with the system headers. You can work around these by disabling this
  switch in the project files which is found in the <ConformanceMode> elements.

* The VS 2017 projects require the 15.5 update or later. For UWP and Win32
  classic desktop projects with the 15.5 - 15.7 updates, you need to install the
  standalone Windows 10 SDK (17763) which is otherwise included in the 15.8.6 or
  later update. Older VS 2017 updates will fail to load the projects due to use
  of the <ConformanceMode> element. If using the 15.5 or 15.6 updates, you will
  see "warning D9002: ignoring unknown option '/Zc:__cplusplus'" because this
  switch isn't supported until 15.7. It is safe to ignore this warning, or you
  can edit the project files <AdditionalOptions> elements.

* The UWP projects include configurations for the ARM64 platform. These require
  VS 2017 (15.9 update) or VS 2019 to build.


---------------
RELEASE HISTORY
---------------

August 21, 2019
    Added meshconvert to CMake project
    Code cleanup

June 30, 2019
    Clang/LLVM warning cleanup
    Renamed DirectXMesh_Windows10.vcxproj to _Windows10_2017.vcxproj
    Added VS 2019 UWP project

May 30, 2019
    Added CMake project files
    Code cleanup

April 26, 2019
    Added VS 2019 desktop projects
    Officially dropped Windows Vista support
    WaveFrontReader utility header updated for RMA texture in MTL

February 7, 2019
    meshconvert: added -sdkmesh2 switch for PBR materials
    WaveFrontReader utility header updated for some extended material textures

November 16, 2018
    VS 2017 updated for Windows 10 October 2018 Update SDK (17763)
    ARM64 platform configurations added to UWP projects

October 25, 2018
    Use UTF-8 instead of ANSI for narrow strings
    Minor project fix

August 17, 2018
    Updated for VS 2017 15.8
    Code cleanup

July 3, 2018
    Code and project cleanup
    meshconvert: added -ib32 switch

May 31, 2018
    VS 2017 updated for Windows 10 April 2018 Update SDK (17134)

May 11, 2018
    Updated for VS 2017 15.7 update warnings
    Code and project cleanup
    Retired VS 2013 projects

April 23, 2018
    Code and project cleanup

February 7, 2018
    New functions: WeldVertices, CompactVB
    Added new optional parameter to OptimizeVertices
    Fixed bug in remap functions which were applying vertex remaps backwards
    - FinalizeIB, FinalizeVB, and FinalizeVBAndPointReps
    meshconvert: added -oplru switch
    WaveFrontReader utility header made more robust
    Minor code cleanup

December 13, 2017
    Updated for VS 2017 15.5 update warnings
    Support building library with _XM_NO_XMVECTOR_OVERLOADS_
    Added support for relative face indices to WaveFrontReader.h utility header

November 1, 2017
    VS 2017 updated for Windows 10 Fall Creators Update SDK (16299)
    Code reformat and cleanup

September 22, 2017
    Updated for VS 2017 15.3 update /permissive- changes
    meshconvert: added -flist option
    Minor code cleanup

July 28, 2017
    Code cleanup

April 24, 2017
    VS 2017 project updates

April 7, 2017
    VS 2017 updated for Windows Creators Update SDK (15063)
    VBReader/VBWriter GetElement adapter for GetElement11

January 31, 2017
    DirectX 12 support for VBReader, VBWriter, IsValid, and ComputeInputLayout
    *breaking change*: VBReader/VBWriter method GetElement method now named GetElement11 for DirectX 11

October 27, 2016
    x2bias optional parameter for VB reader/writer
    Code cleanup

September 14, 2016
    meshconvert: added wildcard support for input filename and optional -r switch for recursive search
    Code cleanup

August 2, 2016
    Updated for VS 2015 Update 3 and Windows 10 SDK (14393)

July 19, 2016
    meshconvert command-line tool updated with -flipu switch

June 27, 2016
    Code cleanup

April 26, 2016
    Retired VS 2012 projects and obsolete adapter code
    Minor code and project file cleanup

November 30, 2015
    meshconvert command-line tool updated with -flipv and -flipz switches; removed -fliptc
    Updated for VS 2015 Update 1 and Windows 10 SDK (10586)

October 30, 2015
    Minor code cleanup

August 18, 2015
    Xbox One platform updates

July 29, 2015
    Updated for VS 2015 and Windows 10 SDK RTM
    Retired VS 2010 projects
    WaveFrontReader: updated utility to minimize debug output

July 8, 2015
    Minor SAL fix and project cleanup

March 27, 2015
    Added projects for Windows apps Technical Preview
    Fixed attributes usage for OptimizeFacesEx
    meshconvert: fix when importing from .vbo
    Minor code cleanup

November 14, 2014
    meshconvert: sample improvements and fixes
    Added workarounds for potential compiler bug when using VB reader/writer

October 28, 2014
    meshconvert command-line sample
    Added VBReader/VBWriter::GetElement method
    Added more ComputeTangentFrame overloads
    Explicit calling-convention annotation for public headers
    Windows phone 8.1 platform support
    Minor code and project cleanup

June 27, 2014
    Original release
