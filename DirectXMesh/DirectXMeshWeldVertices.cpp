//-------------------------------------------------------------------------------------
// DirectXMeshWeldVertices.cpp
//  
// DirectX Mesh Geometry Library - Vertex welding
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

#include "DirectXMeshP.h"

using namespace DirectX;

namespace
{
    template<class index_t>
    HRESULT WeldVerticesImpl(
        _In_reads_(nFaces * 3) const index_t* indices, size_t nFaces,
        size_t nVerts, _In_reads_(nVerts) const uint32_t* pointRep,
        _Out_writes_(nVerts) uint32_t* vertexRemap, _Out_writes_(nFaces) uint32_t* faceRemap,
        std::function<bool __cdecl(index_t v0, index_t v1)>& weldTest)
    {
        if (!indices || !nFaces || !nVerts || !pointRep || !vertexRemap || !faceRemap)
            return E_INVALIDARG;

        if (nVerts >= index_t(-1))
            return E_INVALIDARG;

        // TODO -
        weldTest;

        return E_NOTIMPL;
    }
}

//=====================================================================================
// Entry-points
//=====================================================================================

_Use_decl_annotations_
HRESULT DirectX::WeldVertices(
    const uint16_t* indices, size_t nFaces,
    size_t nVerts, const uint32_t* pointRep,
    uint32_t* vertexRemap, uint32_t* faceRemap,
    std::function<bool __cdecl(uint16_t v0, uint16_t v1)>& weldTest)
{
    return WeldVerticesImpl<uint16_t>(indices, nFaces, nVerts, pointRep, vertexRemap, faceRemap, weldTest);
}

_Use_decl_annotations_
HRESULT DirectX::WeldVertices(
    const uint32_t* indices, size_t nFaces,
    size_t nVerts, const uint32_t* pointRep,
    uint32_t* vertexRemap, uint32_t* faceRemap,
    std::function<bool __cdecl(uint32_t v0, uint32_t v1)>& weldTest)
{
    return WeldVerticesImpl<uint32_t>(indices, nFaces, nVerts, pointRep, vertexRemap, faceRemap, weldTest);
}
