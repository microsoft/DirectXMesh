//--------------------------------------------------------------------------------------
// File: FlexibleVertexFormat.h
//
// Helpers for legacy Direct3D 9 era FVF codes and vertex decls
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
//--------------------------------------------------------------------------------------

#pragma once

#include <d3d9.h>

#include <cstdint>

namespace FVF
{
    inline size_t ComputeFVFVertexSize(uint32_t fvfCode)
    {
        size_t vertexSize = 0;

        switch (fvfCode & D3DFVF_POSITION_MASK)
        {
        case D3DFVF_XYZ:    vertexSize = 3 * sizeof(float); break;
        case D3DFVF_XYZRHW: vertexSize = 4 * sizeof(float); break;
        case D3DFVF_XYZB1:  vertexSize = 4 * sizeof(float); break;
        case D3DFVF_XYZB2:  vertexSize = 5 * sizeof(float); break;
        case D3DFVF_XYZB3:  vertexSize = 6 * sizeof(float); break;
        case D3DFVF_XYZB4:  vertexSize = 7 * sizeof(float); break;
        case D3DFVF_XYZB5:  vertexSize = 8 * sizeof(float); break;
        default:
            return 0;
        }

        if (fvfCode & D3DFVF_NORMAL)
            vertexSize += 3 * sizeof(float);

        if (fvfCode & D3DFVF_PSIZE)
            vertexSize += sizeof(uint32_t);

        if (fvfCode & D3DFVF_DIFFUSE)
            vertexSize += sizeof(uint32_t);

        if (fvfCode & D3DFVF_SPECULAR)
            vertexSize += sizeof(uint32_t);

        // Texture coordinates
        size_t numCoords = (((fvfCode)&D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT);
        uint32_t textureFormats = fvfCode >> 16u;

        if (textureFormats)
        {
            for (size_t i = 0; i < numCoords; i++)
            {
                switch (textureFormats & 3)
                {
                case 0: vertexSize += 2 * sizeof(float); break;
                case 1: vertexSize += 3 * sizeof(float); break;
                case 2: vertexSize += 4 * sizeof(float); break;
                case 3: vertexSize += 1 * sizeof(float); break;
                }

                textureFormats >>= 2;
            }
        }
        else
        {
            vertexSize += numCoords * (2 * sizeof(float));
        }

        return vertexSize;
    }
}
