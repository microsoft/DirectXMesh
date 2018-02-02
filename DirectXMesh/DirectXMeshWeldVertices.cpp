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

//=====================================================================================
// Entry-points
//=====================================================================================

_Use_decl_annotations_
HRESULT DirectX::WeldVertices(
    size_t nVerts, const uint32_t* pointRep,
    uint32_t* vertexRemap,
    std::function<bool __cdecl(uint32_t v0, uint32_t v1)> weldTest)
{
    if (!nVerts || !pointRep || !vertexRemap)
        return E_INVALIDARG;

    if (nVerts >= UINT32_MAX)
        return E_INVALIDARG;

    std::unique_ptr<uint32_t[]> wedgeList(new (std::nothrow) uint32_t[nVerts]);
    if (!wedgeList)
        return E_OUTOFMEMORY;

    for (uint32_t j = 0; j < nVerts; ++j)
    {
        vertexRemap[j] = j;
        wedgeList[j] = j;
    }

    // Generate wedge list
    bool identity = true;

    for (uint32_t j = 0; j < nVerts; ++j)
    {
        uint32_t pr = pointRep[j];
        if (pr == UNUSED32)
            continue;

        if (pr >= nVerts)
            return E_UNEXPECTED;

        if (pr != j)
        {
            identity = false;

            wedgeList[j] = wedgeList[pr];
            wedgeList[pr] = j;
        }
    }

    if (identity)
    {
        // No candidates for welding, so return now
        return S_FALSE;
    }

    bool weld = false;

    for (uint32_t vert = 0; vert < nVerts; ++vert)
    {
        if (pointRep[vert] == vert && wedgeList[vert] != vert)
        {
            uint32_t curOuter = vert;
            do
            {
                // if a remapping for the vertex hasn't been found, check to see if it matches any other vertices
                assert(curOuter < nVerts);
                _Analysis_assume_(curOuter < nVerts);
                if (vertexRemap[curOuter] == curOuter)
                {
                    uint32_t curInner = wedgeList[vert];
                    assert(curInner < nVerts);
                    _Analysis_assume_(curInner < nVerts);
                    do
                    {
                        // don't check for equalivalence if indices the same (had better be equal then)
                        // and/or if the one being checked is already being remapped
                        if ((curInner != curOuter) && (vertexRemap[curInner] == curInner))
                        {
                            // if the two vertices are equal, then remap one to the other
                            if (weldTest(curOuter, curInner))
                            {
                                // remap the inner vertices to the outer...
                                vertexRemap[curInner] = curOuter;

                                weld = true;
                            }
                        }

                        curInner = wedgeList[curInner];
                    } while (curInner != vert);
                }

                curOuter = wedgeList[curOuter];
            } while (curOuter != vert);
        }
    }

    return (weld) ? S_OK : S_FALSE;
}
