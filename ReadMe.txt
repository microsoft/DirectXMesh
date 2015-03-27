DIRECTX MESH LIBRARY (DirectXMesh)
------------------------------------

Copyright (c) Microsoft Corporation. All rights reserved.

March 27, 2015

This package contains DirectXMesh, a shared source library for performing various geometry
content processing operations including generating normals and tangent frames, triangle
adjacency computations, and vertex cache optimization.

The source is written for Visual Studio 2010, 2012, or 2013. It is recommended that you
make use of the Windows 8.1 SDK and Windows 7 Service Pack 1 or later.

Details on using the Windows 8.1 SDK with VS 2010 are described on the Visual C++ Team Blog:
<http://blogs.msdn.com/b/vcblog/archive/2012/11/23/using-the-windows-8-sdk-with-visual-studio-2010-configuring-multiple-projects.aspx>

These components are designed to work without requiring any content from the DirectX SDK. For details,
see "Where is the DirectX SDK?" <http://msdn.microsoft.com/en-us/library/ee663275.aspx>.

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

For the latest version of DirectXMesh, more detailed documentation, discussion forums, bug
reports and feature requests, please visit the Codeplex site.

http://go.microsoft.com/fwlink/?LinkID=324981


----------------------------------
Normals, tangents, and bi-tangents
----------------------------------

Geometric meshes often must include surface information for lighting computations. DirectXMesh implements
functions for computing this from mesh triangles.

   std::unique_ptr<WaveFrontReader<uint16_t>> mesh( new WaveFrontReader<uint16_t>() );

   if ( FAILED( mesh->Load( L"test.obj" ) ) )
      // Error

   if ( mesh->hasNormals )
       // Skip next computation

   size_t nFaces = mesh->indices.size() / 3;
   size_t nVerts = mesh->vertices.size();

   std::unique_ptr<XMFLOAT3[]> pos( new XMFLOAT3[ nVerts ] );
   for( size_t j = 0; j < nVerts; ++j )
      pos[ j ] = mesh->vertices[ j ].position;

   std::unique_ptr<XMFLOAT3[]> normals( new XMFLOAT3[ nVerts ] );
   if ( FAILED( ComputeNormals( &mesh->indices.front(), nFaces, pos.get(), nVerts, CNORM_DEFAULT, normals.get() ) ) )
      // Error

   if ( !mesh->hasTexcoords )
      // Skip next computation

   std::unique_ptr<XMFLOAT2[]> texcoords( new XMFLOAT2[ nVerts ] );
   for( size_t j = 0; j < nVerts; ++j )
      texcoords[ j ] = mesh->vertices[ j ].textureCoordinate;

   std::unique_ptr<XMFLOAT3[]> tangents( new XMFLOAT3[ nVerts ] );
   std::unique_ptr<XMFLOAT3[]> bitangents( new XMFLOAT3[ nVerts ] );

   if ( FAILED( ComputeTangentFrame( &mesh->indices.front(), nFaces,
                                     pos.get(), normals.get(), texcoords.get(), nVerts,
                                     tangents.get(), bitangents.get() ) ) )
      // Error


--------------
Mesh adjacency
--------------

A useful property of geometric meshes is the adjacency relationship between faces of the mesh as
defined by sharing triangle edges. DirectXMesh implements a number of functions that require
adjacency information to function.

   std::unique_ptr<WaveFrontReader<uint16_t>> mesh( new WaveFrontReader<uint16_t>() );

   if ( FAILED( mesh->Load( L"test.obj" ) ) )
      // Error

   size_t nFaces = mesh->indices.size() / 3;
   size_t nVerts = mesh->vertices.size();

   std::unique_ptr<XMFLOAT3[]> pos( new XMFLOAT3[ nVerts ] );
   for( size_t j = 0; j < nVerts; ++j )
      pos[ j ] = mesh->vertices[ j ].position;

   std::unique_ptr<uint32_t[]> adj( new uint32_t[ mesh->indices.size() ] );
   if ( FAILED( GenerateAdjacencyAndPointReps( &mesh->indices.front(), nFaces, pos.get(), nVerts,
                                               0.f, nullptr, adj.get() ) ) )
      // Error


---------------------------
Mesh cleanup and validation
---------------------------

Triangular mesh descriptions can contain a number of errors which result in invalid or failed
geometric operations. There are also a number of cases where a triangular mesh description can
cause mesh algorithms to fail. DirectXMesh implements a number of functions to detect and
resolve such issues.

   std::unique_ptr<WaveFrontReader<uint16_t>> mesh( new WaveFrontReader<uint16_t>() );

   if ( FAILED( mesh->Load( L"test.obj" ) ) )
      // Error

   size_t nFaces = mesh->indices.size() / 3;
   size_t nVerts = mesh->vertices.size();

   hr = Validate( &mesh->indices.front(), nFaces, nVerts, nullptr, VALIDATE_DEFAULT );
   if ( FAILED(hr) )
      // E_FAIL indicates that mesh failed validation

   std::unique_ptr<XMFLOAT3[]> pos( new XMFLOAT3[ nVerts ] );
   for( size_t j = 0; j < nVerts; ++j )
      pos[ j ] = mesh->vertices[ j ].position;

   std::unique_ptr<uint32_t[]> adj( new uint32_t[ mesh->indices.size() ] );
   if ( FAILED( GenerateAdjacencyAndPointReps( &mesh->indices.front(), nFaces, pos.get(), nVerts,
                                               0.f, nullptr, adj.get() ) ) )
      // Error

   std::unique_ptr<uint16_t[]> indices( new uint16_t[ nFaces * 3 ] );
   memcpy( indices.get(), &mesh->indices.front(), sizeof(uint16_t) * nFaces * 3 ) );

   std::vector<uint32_t> dupVerts;
   hr = Clean( indices.get(), nFaces, nVerts, adj.get(), nullptr, dupVerts, true );
   if ( FAILED(hr) )
      // Error


-----------------
Mesh optimization
-----------------

Direct3D can render valid meshes with the same visual results no matter how the data is ordered, but the
efficiency of the rendering performance can be impacted by ordering that is well-matched to modern GPUs.
Mesh optimization is a process for reordering faces and vertices to provide the same visual result, with
improved utilization of hardware resources.

   std::unique_ptr<WaveFrontReader<uint16_t>> mesh( new WaveFrontReader<uint16_t>() );

   if ( FAILED( mesh->Load( L"test.obj" ) ) )
      // Error

   size_t nFaces = mesh->indices.size() / 3;
   size_t nVerts = mesh->vertices.size();

   std::unique_ptr<XMFLOAT3[]> pos( new XMFLOAT3[ nVerts ] );
   for( size_t j = 0; j < nVerts; ++j )
      pos[ j ] = mesh->vertices[ j ].position;

   std::unique_ptr<uint32_t[]> adj( new uint32_t[ mesh->indices.size() ] );
   if ( FAILED( GenerateAdjacencyAndPointReps( &mesh->indices.front(), nFaces, pos.get(), nVerts,
                                               0.f, nullptr, adj.get() ) ) )
      // Error

   std::unique_ptr<uint32_t[]> faceRemap( new uint32_t[ nFaces ] );
   if ( FAILED( OptimizeFaces( &mesh->indices.front(), nFaces, adj.get(), faceRemap.get() ) ) )
      // Error

   std::unique_ptr<uint16_t[]> newIndices( new uint16_t[ nFaces * 3 ] );
   if ( FAILED( ReorderIB( &mesh->indices.front(), nFaces, faceRemap.get(), newIndices.get() ) ) )
      // Error

   std::unique_ptr<uint32_t[]> vertRemap( new uint32_t[ nVerts ] );
   if ( FAILED( OptimizeVertices( newIndices.get(), nFaces, nVerts, vertRemap.get() ) ) )
      // Error

   if ( FAILED( FinalizeIB( newIndices.get(), nFaces, vertRemap.get(), nVerts ) ) )
      // Error

   std::unique_ptr<WaveFrontReader<uint16_t>::Vertex> vb( new WaveFrontReader<uint16_t>::Vertex[ nVerts ] );
   if ( FAILED( FinalizeVB( &mesh->vertices.front(), sizeof(WaveFrontReader<uint16_t>::Vertex),
                            nVerts, nullptr, 0, vertRemap.get(), vb.get() ) ) )
      // Error

Further reading:

   Hoppe, H.; "Optimization of mesh locality for transparent vertex caching", ACM SIGGRAPH 1999 Proceedings
   http://research.microsoft.com/en-us/um/people/hoppe/proj/tvc/


---------------------------
Vertex buffer reader/writer
---------------------------

Mesh processing can require inserting or extracting data from a vertex buffer. The VBReader and VBWriter
classes provide a solution for this based on Direct3D 11 input layouts.

During initialization:

   std::unique_ptr<VBReader> reader( new VBReader() );

   D3D11_INPUT_ELEMENT_DESC layout[] =
   {
       { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
       { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
   };

   struct Vertex
   {
       XMFLOAT3 Pos;
       XMFLOAT2 Tex;
   };

   HRESULT hr = reader->Initialize( layout, 2 );
   if ( FAILED(hr) )
      // Error

   hr = reader->AddStream( vbdata, nVerts, 0, sizeof(Vertex) );
   if ( FAILED(hr) )
      // Error

   Note: The VBWriter is set up the same way.

Extracting vertex data:

   struct aligned_deleter { void operator()(void* p) { _aligned_free(p); } };

   std::unique_ptr<XMVECTOR[], aligned_deleter> buff(
    reinterpret_cast<XMVECTOR*>( _aligned_malloc( sizeof(XMVECTOR) * nVerts, 16 ) ) );

   if ( FAILED( reader->Read( buff.get(), "POSITION", 0, nVerts ) ) )
      // Error

Inserting vertex data:

   struct aligned_deleter { void operator()(void* p) { _aligned_free(p); } };

   std::unique_ptr<XMVECTOR[], aligned_deleter> buff(
       reinterpret_cast<XMVECTOR*>( _aligned_malloc( sizeof(XMVECTOR) * nVerts, 16 ) ) );

   // Store position data into XMVECTOR buff array

   if ( FAILED( writer->Write( buff.get(), "POSITION", 0, nVerts ) ) )
      // Error


---------------
RELEASE HISTORY
---------------

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