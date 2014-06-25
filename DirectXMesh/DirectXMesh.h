//-------------------------------------------------------------------------------------
// DirectXMesh.h
//  
// DirectX Mesh Geometry Library
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
//-------------------------------------------------------------------------------------

#if defined(_MSC_VER) && (_MSC_VER > 1000)
#pragma once
#endif

// VS 2010's stdint.h conflicts with intsafe.h
#pragma warning(push)
#pragma warning(disable : 4005)
#include <stdint.h>
#pragma warning(pop)

#include <memory>
#include <string>
#include <vector>

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#define DCOMMON_H_INCLUDED
#else
#include <d3d11_1.h>
#endif

#include <directxmath.h>

#define DIRECTX_MESH_VERSION 100

namespace DirectX
{
    //---------------------------------------------------------------------------------
    // DXGI Format Utilities
    bool IsValidVB( _In_ DXGI_FORMAT fmt );
    bool IsValidIB( _In_ DXGI_FORMAT fmt );
    size_t BytesPerElement( _In_ DXGI_FORMAT fmt );


    //---------------------------------------------------------------------------------
    // Input Layout Descriptor Utilities
    bool IsValid( _In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl );
    void ComputeInputLayout( _In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl,
                             _Out_writes_opt_(nDecl) uint32_t* offsets,
                             _Out_writes_opt_(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT) uint32_t* strides );

    //---------------------------------------------------------------------------------
    // Attribute Utilities
    std::vector<std::pair<size_t,size_t>> ComputeSubsets( _In_reads_opt_(nFaces) const uint32_t* attributes, _In_ size_t nFaces );
        // Returns a list of face offset,counts for attribute groups

    //---------------------------------------------------------------------------------
    // Mesh Optimization Utilities
    void ComputeVertexCacheMissRate( _In_reads_(nFaces*3) const uint16_t* indices, _In_ size_t nFaces, _In_ size_t nVerts,
                                     _In_ size_t cacheSize, _Out_ float& acmr, _Out_ float& atvr );
    void ComputeVertexCacheMissRate( _In_reads_(nFaces*3) const uint32_t* indices, _In_ size_t nFaces, _In_ size_t nVerts,
                                     _In_ size_t cacheSize, _Out_ float& acmr, _Out_ float& atvr );
        // Compute the average cache miss ratio and average triangle vertex reuse for the post-transform vertex cache

    //---------------------------------------------------------------------------------
    // Vertex Buffer Reader/Writer

    class VBReader
    {
    public:
        VBReader();
        VBReader(VBReader&& moveFrom);
        VBReader& operator= (VBReader&& moveFrom);
        ~VBReader();

        HRESULT Initialize( _In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl );
            // Does not support VB decls with D3D11_INPUT_PER_INSTANCE_DATA

        HRESULT AddStream( _In_reads_bytes_(stride*nVerts) const void* vb, _In_ size_t nVerts, _In_ size_t inputSlot, _In_ size_t stride = 0 );
            // Add vertex buffer to reader

        HRESULT Read( _Out_writes_(count) XMVECTOR* buffer, _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex, _In_ size_t count ) const;
            // Extracts data elements from vertex buffer

        HRESULT Read( _Out_writes_(count) float* buffer, _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex, _In_ size_t count ) const;
        HRESULT Read( _Out_writes_(count) XMFLOAT2* buffer, _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex, _In_ size_t count ) const;
        HRESULT Read( _Out_writes_(count) XMFLOAT3* buffer, _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex, _In_ size_t count ) const;
        HRESULT Read( _Out_writes_(count) XMFLOAT4* buffer, _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex, _In_ size_t count ) const;
            // Helpers for data extraction

        void Release();

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Prevent copying.
        VBReader(VBReader const&);
        VBReader& operator= (VBReader const&);
    };

    class VBWriter
    {
    public:
        VBWriter();
        VBWriter(VBWriter&& moveFrom);
        VBWriter& operator= (VBWriter&& moveFrom);
        ~VBWriter();

        HRESULT Initialize( _In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl );
            // Does not support VB decls with D3D11_INPUT_PER_INSTANCE_DATA

        HRESULT AddStream( _Out_writes_bytes_(stride*nVerts) void* vb, _In_ size_t nVerts, _In_ size_t inputSlot, _In_ size_t stride = 0 );
            // Add vertex buffer to writer

        HRESULT Write( _In_reads_(count) const XMVECTOR* buffer, _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex, _In_ size_t count ) const;
            // Inserts data elements into vertex buffer

        HRESULT Write( _In_reads_(count) const float* buffer, _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex, _In_ size_t count ) const;
        HRESULT Write( _In_reads_(count) const XMFLOAT2* buffer, _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex, _In_ size_t count ) const;
        HRESULT Write( _In_reads_(count) const XMFLOAT3* buffer, _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex, _In_ size_t count ) const;
        HRESULT Write( _In_reads_(count) const XMFLOAT4* buffer, _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex, _In_ size_t count ) const;
            // Helpers for data insertion

        void Release();

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Prevent copying.
        VBWriter(VBWriter const&);
        VBWriter& operator= (VBWriter const&);
    };

    //---------------------------------------------------------------------------------
    // Adjacency Computation

    HRESULT GenerateAdjacencyAndPointReps( _In_reads_(nFaces*3) const uint16_t* indices, _In_ size_t nFaces,
                                           _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts, 
                                           _In_ float epsilon,
                                           _Out_writes_opt_(nVerts) uint32_t* pointRep,
                                           _Out_writes_opt_(nFaces*3) uint32_t* adjacency );
    HRESULT GenerateAdjacencyAndPointReps( _In_reads_(nFaces*3) const uint32_t* indices, _In_ size_t nFaces,
                                           _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts, 
                                           _In_ float epsilon,
                                           _Out_writes_opt_(nVerts) uint32_t* pointRep,
                                           _Out_writes_opt_(nFaces*3) uint32_t* adjacency );
        // If pointRep is null, it still generates them internally as they are needed for the final adjacency computation

    HRESULT ConvertPointRepsToAdjacency( _In_reads_(nFaces*3) const uint16_t* indices, _In_ size_t nFaces,
                                         _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts, 
                                         _In_reads_opt_(nVerts) const uint32_t* pointRep,
                                         _Out_writes_(nFaces*3) uint32_t* adjacency );
    HRESULT ConvertPointRepsToAdjacency( _In_reads_(nFaces*3) const uint32_t* indices, _In_ size_t nFaces,
                                         _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts, 
                                         _In_reads_opt_(nVerts) const uint32_t* pointRep,
                                         _Out_writes_(nFaces*3) uint32_t* adjacency );
        // If pointRep is null, assumes an identity

    HRESULT GenerateGSAdjacency( _In_reads_(nFaces*3) const uint16_t* indices, _In_ size_t nFaces,
                                 _In_reads_(nVerts) const uint32_t* pointRep,
                                 _In_reads_(nFaces*3) const uint32_t* adjacency, _In_ size_t nVerts, 
                                 _Out_writes_(nFaces*6) uint16_t* indicesAdj );
    HRESULT GenerateGSAdjacency( _In_reads_(nFaces*3) const uint32_t* indices, _In_ size_t nFaces,
                                 _In_reads_(nVerts) const uint32_t* pointRep,
                                 _In_reads_(nFaces*3) const uint32_t* adjacency, _In_ size_t nVerts,
                                 _Out_writes_(nFaces*6) uint32_t* indicesAdj );
        // Generates an IB suitable for Geometry Shader using D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ

    //---------------------------------------------------------------------------------
    // Normals, Tangents, and Bi-Tangents Computation

    enum CNORM_FLAGS
    {
        CNORM_DEFAULT                   = 0x0,
            // Default is to compute normals using weight-by-angle

        CNORM_WEIGHT_BY_AREA            = 0x1,
            // Computes normals using weight-by-area

        CNORM_WEIGHT_EQUAL              = 0x2,
            // Compute normals with equal weights

        CNORM_WIND_CW                   = 0x4,
            // Vertices are clock-wise (defaults to CCW)
    };

    HRESULT ComputeNormals( _In_reads_(nFaces*3) const uint16_t* indices, _In_ size_t nFaces,
                            _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts, 
                            _In_ DWORD flags,
                            _Out_writes_(nVerts) XMFLOAT3* normals );
    HRESULT ComputeNormals( _In_reads_(nFaces*3) const uint32_t* indices, _In_ size_t nFaces,
                            _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts, 
                            _In_ DWORD flags,
                            _Out_writes_(nVerts) XMFLOAT3* normals );
        // Computes vertex normals

    HRESULT ComputeTangentFrame( _In_reads_(nFaces*3) const uint16_t* indices, _In_ size_t nFaces,
                                 _In_reads_(nVerts) const XMFLOAT3* positions,
                                 _In_reads_(nVerts) const XMFLOAT3* normals,
                                 _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts, 
                                 _Out_writes_opt_(nVerts) XMFLOAT3* tangents,
                                 _Out_writes_opt_(nVerts) XMFLOAT3* bitangents );
    HRESULT ComputeTangentFrame( _In_reads_(nFaces*3) const uint32_t* indices, _In_ size_t nFaces,
                                 _In_reads_(nVerts) const XMFLOAT3* positions,
                                 _In_reads_(nVerts) const XMFLOAT3* normals,
                                 _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts, 
                                 _Out_writes_opt_(nVerts) XMFLOAT3* tangents,
                                 _Out_writes_opt_(nVerts) XMFLOAT3* bitangents );
    HRESULT ComputeTangentFrame( _In_reads_(nFaces*3) const uint16_t* indices, _In_ size_t nFaces,
                                 _In_reads_(nVerts) const XMFLOAT3* positions,
                                 _In_reads_(nVerts) const XMFLOAT3* normals,
                                 _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts, 
                                 _Out_writes_(nVerts) XMFLOAT4* tangents );
    HRESULT ComputeTangentFrame( _In_reads_(nFaces*3) const uint32_t* indices, _In_ size_t nFaces,
                                 _In_reads_(nVerts) const XMFLOAT3* positions,
                                 _In_reads_(nVerts) const XMFLOAT3* normals,
                                 _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts, 
                                 _Out_writes_(nVerts) XMFLOAT4* tangents );
        // Computes tangents and/or bi-tangents (optionally with handedness stored in .w)

    //---------------------------------------------------------------------------------
    // Mesh clean-up and validation

    enum VALIDATE_FLAGS
    {
        VALIDATE_DEFAULT                = 0x0,

        VALIDATE_BACKFACING             = 0x1,
            // Check for duplicate neighbor from triangle (requires adjacency)

        VALIDATE_BOWTIES                = 0x2,
            // Check for two fans of triangles using the same vertex (requires adjacency)

        VALIDATE_DEGENERATE             = 0x4,
            // Check for degenerate triangles

        VALIDATE_UNUSED                 = 0x8,
            // Check for issues with 'unused' triangles

        VALIDATE_ASYMMETRIC_ADJ         = 0x10,
            // Checks that neighbors are symmetric (requires adjacency)
    };

    HRESULT Validate( _In_reads_(nFaces*3) const uint16_t* indices, _In_ size_t nFaces,
                      _In_ size_t nVerts, _In_reads_opt_(nFaces*3) const uint32_t* adjacency,
                      _In_ DWORD flags, _In_opt_ std::wstring* msgs = nullptr );
    HRESULT Validate( _In_reads_(nFaces*3) const uint32_t* indices, _In_ size_t nFaces,
                      _In_ size_t nVerts, _In_reads_opt_(nFaces*3) const uint32_t* adjacency,
                      _In_ DWORD flags, _In_opt_ std::wstring* msgs = nullptr );
        // Checks the mesh for common problems, return 'S_OK' if no problems were found

    HRESULT Clean( _Inout_updates_all_(nFaces*3) uint16_t* indices, _In_ size_t nFaces,
                   _In_ size_t nVerts, _Inout_updates_all_opt_(nFaces*3) uint32_t* adjacency,
                   _In_reads_opt_(nFaces) const uint32_t* attributes,
                   _Inout_ std::vector<uint32_t>& dupVerts, _In_ bool breakBowties=false );
    HRESULT Clean( _Inout_updates_all_(nFaces*3) uint32_t* indices, _In_ size_t nFaces,
                   _In_ size_t nVerts, _Inout_updates_all_opt_(nFaces*3) uint32_t* adjacency,
                   _In_reads_opt_(nFaces) const uint32_t* attributes,
                   _Inout_ std::vector<uint32_t>& dupVerts, _In_ bool breakBowties=false );
        // Cleans the mesh, splitting vertices if needed

    //---------------------------------------------------------------------------------
    // Mesh Optimization

    HRESULT AttributeSort( _In_ size_t nFaces, _Inout_updates_all_opt_(nFaces) uint32_t* attributes,
                           _Out_writes_(nFaces) uint32_t* faceRemap );
        // Reorders faces by attribute id

    enum OPTFACES
    {
        OPTFACES_V_DEFAULT      = 12,
        OPTFACES_R_DEFAULT      = 7,
            // Default vertex cache size and restart threshold which is considered 'device independent'

        OPTFACES_V_STRIPORDER   = 0,
            // Indicates no vertex cache optimization, only reordering into strips
    };

    HRESULT OptimizeFaces( _In_reads_(nFaces*3) const uint16_t* indices, _In_ size_t nFaces,
                           _In_reads_(nFaces*3) const uint32_t* adjacency,
                           _Out_writes_(nFaces) uint32_t* faceRemap,
                           _In_ uint32_t vertexCache = OPTFACES_V_DEFAULT,
                           _In_ uint32_t restart = OPTFACES_R_DEFAULT );
    HRESULT OptimizeFaces( _In_reads_(nFaces*3) const uint32_t* indices, _In_ size_t nFaces,
                           _In_reads_(nFaces*3) const uint32_t* adjacency,
                           _Out_writes_(nFaces) uint32_t* faceRemap,
                           _In_ uint32_t vertexCache = OPTFACES_V_DEFAULT,
                           _In_ uint32_t restart = OPTFACES_R_DEFAULT );
        // Reorders faces to increase hit rate of vertex caches

    HRESULT OptimizeFacesEx( _In_reads_(nFaces*3) const uint16_t* indices, _In_ size_t nFaces,
                             _In_reads_(nFaces*3) const uint32_t* adjacency,
                             _In_reads_(nFaces) const uint32_t* attributes,
                             _Out_writes_(nFaces) uint32_t* faceRemap,
                             _In_ uint32_t vertexCache = OPTFACES_V_DEFAULT,
                             _In_ uint32_t restart = OPTFACES_R_DEFAULT );
    HRESULT OptimizeFacesEx( _In_reads_(nFaces*3) const uint32_t* indices, _In_ size_t nFaces,
                             _In_reads_(nFaces*3) const uint32_t* adjacency,
                             _In_reads_(nFaces) const uint32_t* attributes,
                             _Out_writes_(nFaces) uint32_t* faceRemap,
                             _In_ uint32_t vertexCache = OPTFACES_V_DEFAULT,
                             _In_ uint32_t restart = OPTFACES_R_DEFAULT );
        // Attribute group version of OptimizeFaces

    HRESULT OptimizeVertices( _In_reads_(nFaces*3) const uint16_t* indices, _In_ size_t nFaces, _In_ size_t nVerts,
                              _Out_writes_(nVerts) uint32_t* vertexRemap );
    HRESULT OptimizeVertices( _In_reads_(nFaces*3) const uint32_t* indices, _In_ size_t nFaces, _In_ size_t nVerts,
                              _Out_writes_(nVerts) uint32_t* vertexRemap );
        // Reorders vertices in order of use

    //---------------------------------------------------------------------------------
    // Remap functions

    HRESULT ReorderIB( _In_reads_(nFaces*3) const uint16_t* ibin, _In_ size_t nFaces,
                       _In_reads_(nFaces) const uint32_t* faceRemap,
                       _Out_writes_(nFaces*3) uint16_t* ibout );
    HRESULT ReorderIB( _Inout_updates_all_(nFaces*3) uint16_t* ib, _In_ size_t nFaces,
                       _In_reads_(nFaces) const uint32_t* faceRemap );
    HRESULT ReorderIB( _In_reads_(nFaces*3) const uint32_t* ibin, _In_ size_t nFaces,
                       _In_reads_(nFaces) const uint32_t* faceRemap,
                       _Out_writes_(nFaces*3) uint32_t* ibout );
    HRESULT ReorderIB( _Inout_updates_all_(nFaces*3) uint32_t* ib, _In_ size_t nFaces,
                       _In_reads_(nFaces) const uint32_t* faceRemap );
        // Applies a face remap reordering to an index buffer

    HRESULT ReorderIBAndAdjacency( _In_reads_(nFaces*3) const uint16_t* ibin, _In_ size_t nFaces, _In_reads_(nFaces*3) const uint32_t* adjin,
                                   _In_reads_(nFaces) const uint32_t* faceRemap,
                                   _Out_writes_(nFaces*3) uint16_t* ibout, _Out_writes_(nFaces*3) uint32_t* adjout );
    HRESULT ReorderIBAndAdjacency( _Inout_updates_all_(nFaces*3) uint16_t* ib, _In_ size_t nFaces, _Inout_updates_all_(nFaces*3) uint32_t* adj,
                                   _In_reads_(nFaces) const uint32_t* faceRemap );
    HRESULT ReorderIBAndAdjacency( _In_reads_(nFaces*3) const uint32_t* ibin, _In_ size_t nFaces, _In_reads_(nFaces*3) const uint32_t* adjin,
                                   _In_reads_(nFaces) const uint32_t* faceRemap,
                                   _Out_writes_(nFaces*3) uint32_t* ibout, _Out_writes_(nFaces*3) uint32_t* adjout );
    HRESULT ReorderIBAndAdjacency( _Inout_updates_all_(nFaces*3) uint32_t* ib, _In_ size_t nFaces, _Inout_updates_all_(nFaces*3) uint32_t* adj,
                                   _In_reads_(nFaces) const uint32_t* faceRemap );
    // Applies a face remap reordering to an index buffer and adjacency

    HRESULT FinalizeIB( _In_reads_(nFaces*3) const uint16_t* ibin, _In_ size_t nFaces,
                        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts,
                        _Out_writes_(nFaces*3) uint16_t* ibout );
    HRESULT FinalizeIB( _Inout_updates_all_(nFaces*3) uint16_t* ib, _In_ size_t nFaces,
                        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts );
    HRESULT FinalizeIB( _In_reads_(nFaces*3) const uint32_t* ibin, _In_ size_t nFaces,
                        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts,
                        _Out_writes_(nFaces*3) uint32_t* ibout );
    HRESULT FinalizeIB( _Inout_updates_all_(nFaces*3) uint32_t* ib, _In_ size_t nFaces,
                        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts );
        // Applies a vertex remap reordering to an index buffer

    HRESULT FinalizeVB( _In_reads_bytes_(nVerts*stride) const void* vbin, _In_ size_t stride, _In_ size_t nVerts,
                        _In_reads_opt_(nDupVerts) const uint32_t* dupVerts, _In_ size_t nDupVerts,
                        _In_reads_opt_(nVerts+nDupVerts) const uint32_t* vertexRemap, 
                        _Out_writes_bytes_((nVerts+nDupVerts)*stride) void* vbout );
    HRESULT FinalizeVB( _Inout_updates_bytes_all_(nVerts*stride) void* vb, _In_ size_t stride, _In_ size_t nVerts,
                        _In_reads_(nVerts) const uint32_t* vertexRemap );
        // Applies a vertex remap and/or a vertex duplication set to a vertex buffer

    HRESULT FinalizeVBAndPointReps( _In_reads_bytes_(nVerts*stride) const void* vbin, _In_ size_t stride, _In_ size_t nVerts,
                                    _In_reads_(nVerts) const uint32_t* prin, 
                                    _In_reads_opt_(nDupVerts) const uint32_t* dupVerts, _In_ size_t nDupVerts,
                                    _In_reads_opt_(nVerts+nDupVerts) const uint32_t* vertexRemap, 
                                    _Out_writes_bytes_((nVerts+nDupVerts)*stride) void* vbout,
                                    _Out_writes_(nVerts+nDupVerts) uint32_t* prout );
    HRESULT FinalizeVBAndPointReps( _Inout_updates_bytes_all_(nVerts*stride) void* vb, _In_ size_t stride, _In_ size_t nVerts,
                                    _Inout_updates_all_(nVerts) uint32_t* pointRep,
                                    _In_reads_(nVerts) const uint32_t* vertexRemap );
        // Applies a vertex remap and/or a vertex duplication set to a vertex buffer and point representatives

#include "DirectXMesh.inl"

}; // namespace
