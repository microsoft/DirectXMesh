//-------------------------------------------------------------------------------------
// ACMR.h
//
// Function for computing the average cache miss ratio for vertex caches
//
// http://www.realtimerendering.com/blog/acmr-and-atvr/
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

#include <sal.h>
#include <stdint.h>

template<class index_t>
void CalculateMissRate( _In_reads_(nFaces*3) const index_t* indices, size_t nFaces, size_t nVerts, size_t cacheSize,
                        float& acmr, float& atvr )
{
    acmr = -1.f;
    atvr = -1.f;

    if ( !indices || !nFaces || !nVerts || !cacheSize )
        return;

    if ( ( uint64_t(nFaces) * 3 ) > 0xFFFFFFFF )
        return;

    size_t misses = 0;

    std::unique_ptr<uint32_t> fifo( new uint32_t[ cacheSize ] );
    size_t tail = 0;
    
    memset( fifo.get(), 0xff, sizeof(uint32_t) * cacheSize );

    for( size_t j = 0; j < (nFaces * 3); ++j )
    {
        if ( indices[ j ] == index_t(-1) )
            continue;

        bool found = false;

        for( size_t ptr = 0; ptr < cacheSize; ++ptr )
        {
            if ( fifo.get()[ ptr ] == indices[ j ] )
            {
                found = true;
                break;
            }
        }

        if ( !found )
        {
            ++misses;
            fifo.get()[ tail ] = indices[ j ];
            ++tail;
            if ( tail == cacheSize ) tail = 0;
        }
    }

    // ideal is 0.5, individual tris have 3.0
    acmr = float( misses ) / float( nFaces );

    // ideal is 1.0, worst case is 6.0
    atvr = float( misses ) / float( nVerts );
}
