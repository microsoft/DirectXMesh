//-------------------------------------------------------------------------------------
// DirectXMeshOptimizeLRU.cpp
//  
// DirectX Mesh Geometry Library - Mesh optimization
//
// Forsyth "Linear-Speed Vertex Cache Optimisation"
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
}

//=====================================================================================
// Entry-points
//=====================================================================================

_Use_decl_annotations_
HRESULT DirectX::OptimizeFacesLRU(
    const uint16_t* indices, size_t nFaces,
    uint32_t* faceRemap)
{
    if (!indices || !nFaces || !faceRemap)
        return E_INVALIDARG;

    if ((uint64_t(nFaces) * 3) >= UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    // TODO -
    return E_NOTIMPL;
}

_Use_decl_annotations_
HRESULT DirectX::OptimizeFacesLRU(
    const uint32_t* indices, size_t nFaces,
    uint32_t* faceRemap)
{
    if (!indices || !nFaces || !faceRemap)
        return E_INVALIDARG;

    if ((uint64_t(nFaces) * 3) >= UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    // TODO -
    return E_NOTIMPL;
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::OptimizeFacesLRUEx(
    const uint16_t* indices, size_t nFaces, const uint32_t* attributes,
    uint32_t* faceRemap)
{
    if (!indices || !nFaces || !attributes || !faceRemap)
        return E_INVALIDARG;

    if ((uint64_t(nFaces) * 3) >= UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    // TODO -
    return E_NOTIMPL;
}

_Use_decl_annotations_
HRESULT DirectX::OptimizeFacesLRUEx(
    const uint32_t* indices, size_t nFaces, const uint32_t* attributes,
    uint32_t* faceRemap)
{
    if (!indices || !nFaces || !attributes || !faceRemap)
        return E_INVALIDARG;

    if ((uint64_t(nFaces) * 3) >= UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    // TODO -
    return E_NOTIMPL;
}
