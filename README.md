![DirectX Logo](https://github.com/Microsoft/DirectXMesh/wiki/X_jpg.jpg)

# DirectXMesh geometry processing library

http://go.microsoft.com/fwlink/?LinkID=324981

Copyright (c) Microsoft Corporation. All rights reserved.

**June 1, 2020**

This package contains DirectXMesh, a shared source library for performing various geometry content processing operations including generating normals and tangent frames, triangle adjacency computations, vertex cache optimization, and meshlet generation.

This code is designed to build with Visual Studio 2017 ([15.9](https://walbourn.github.io/vs-2017-15-9-update/)), Visual Studio 2019, or clang for Windows v9 or later. It is recommended that you make use of the Windows 10 May 2020 Update SDK ([19041](https://walbourn.github.io/windows-10-may-2020-update-sdk/)).

These components are designed to work without requiring any content from the legacy DirectX SDK. For details, see [Where is the DirectX SDK?](https://aka.ms/dxsdk).

## Directory Layout

* ``DirectXMesh\``

   + This contains the DirectXMesh library.

> The majority of the header files here are intended for  implementation the library only (``DirectXMeshP.h``, ``scoped.h``, etc.). Only ``DirectXMesh.h`` and ``DirectXMesh.inl`` are meant as a 'public' headers for the library.

* ``Utilities\``

  + This contains helper code related to mesh processing that is not general enough to be part of the DirectXMesh library.
    * ``WaveFrontReader.h``: Contains a simple C++ class for reading mesh data from a WaveFront OBJ file.

* ``Meshconvert\``

  + This DirectXMesh sample is an implementation of the ``meshconvert`` command-line texture utility from the legacy DirectX SDK utilizing DirectXMesh rather than D3DX.

> This tool does not support legacy ``.X`` files, but can export ``CMO``, ``SDKMESH``, and ``VBO`` files.

## Documentation

Documentation is available on the [GitHub wiki](https://github.com/Microsoft/DirectXMesh/wiki).

## Notices

All content and source code for this package are subject to the terms of the [MIT License](http://opensource.org/licenses/MIT).

For the latest version of DirectXMesh, bug reports, etc. please visit the project site on [GitHub](https://github.com/microsoft/DirectXMesh).

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Release Notes

* Starting with the June 2020 release, this library makes use of typed enum bitmask flags per the recommendation of the _C++ Standard_ section *17.5.2.1.3 Bitmask types*. This is consistent with Direct3D 12's use of the ``DEFINE_ENUM_FLAG_OPERATORS`` macro. This may have *breaking change* impacts to client code:

  * You cannot pass the ``0`` literal as your flags value. Instead you must make use of the appropriate default enum value: ``CNORM_DEFAULT``, ``VALIDATE_DEFAULT``, or ``MESHLET_DEFAULT``.

  * Use the enum type instead of ``DWORD`` if building up flags values locally with bitmask operations. For example, ```CNORM_FLAGS flags = CNORM_DEFAULT; if (...) flags |= CNORM_WIND_CW;```

* The UWP projects and the VS 2019 Win10 classic desktop project include configurations for the ARM64 platform. These require VS 2017 (15.9 update) or VS 2019 to build, with the ARM64 toolset installed.
