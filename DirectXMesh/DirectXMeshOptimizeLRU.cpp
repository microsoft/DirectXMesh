//-------------------------------------------------------------------------------------
// DirectXMeshOptimizeLRU.cpp
//  
// DirectX Mesh Geometry Library - Mesh optimization
//
// Forsyth "Linear-Speed Vertex Cache Optimisation"
// https://tomforsyth1000.github.io/papers/fast_vert_cache_opt.html
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
    // This code was authored and released into the public domain by Adrian Stone (stone@gameangst.com).

    // code for computing vertex score was taken, as much as possible
    // directly from the original publication.
    float ComputeVertexCacheScore(int cachePosition, uint32_t vertexCacheSize)
    {
        const float FindVertexScore_CacheDecayPower = 1.5f;
        const float FindVertexScore_LastTriScore = 0.75f;

        float score = 0.0f;
        if (cachePosition < 0)
        {
            // Vertex is not in FIFO cache - no score.
        }
        else
        {
            if (cachePosition < 3)
            {
                // This vertex was used in the last triangle,
                // so it has a fixed score, whichever of the three
                // it's in. Otherwise, you can get very different
                // answers depending on whether you add
                // the triangle 1,2,3 or 3,1,2 - which is silly.

                score = FindVertexScore_LastTriScore;
            }
            else
            {
                assert(cachePosition < int(vertexCacheSize));

                // Points for being high in the cache.
                const float scaler = 1.0f / (vertexCacheSize - 3);
                score = 1.0f - (cachePosition - 3) * scaler;
                score = powf(score, FindVertexScore_CacheDecayPower);
            }
        }

        return score;
    }

    float ComputeVertexValenceScore(uint32_t numActiveFaces)
    {
        const float FindVertexScore_ValenceBoostScale = 2.0f;
        const float FindVertexScore_ValenceBoostPower = 0.5f;

        float score = 0.f;

        // Bonus points for having a low number of tris still to
        // use the vert, so we get rid of lone verts quickly.
        float valenceBoost = powf(static_cast<float>(numActiveFaces),
            -FindVertexScore_ValenceBoostPower);

        score += FindVertexScore_ValenceBoostScale * valenceBoost;
        return score;
    }

    enum { kMaxVertexCacheSize = 64 };
    enum { kMaxPrecomputedVertexValenceScores = 64 };

    float s_vertexCacheScores[kMaxVertexCacheSize + 1][kMaxVertexCacheSize];
    float s_vertexValenceScores[kMaxPrecomputedVertexValenceScores];

    static INIT_ONCE s_initOnce = INIT_ONCE_STATIC_INIT;

    BOOL WINAPI ComputeVertexScores(PINIT_ONCE, PVOID, PVOID*)
    {
        for (uint32_t cacheSize = 0; cacheSize <= kMaxVertexCacheSize; ++cacheSize)
        {
            for (uint32_t cachePos = 0; cachePos < cacheSize; ++cachePos)
            {
                s_vertexCacheScores[cacheSize][cachePos] = ComputeVertexCacheScore(cachePos, cacheSize);
            }
        }

        for (uint32_t valence = 0; valence < kMaxPrecomputedVertexValenceScores; ++valence)
        {
            s_vertexValenceScores[valence] = ComputeVertexValenceScore(valence);
        }

        return TRUE;
    }

    inline float FindVertexCacheScore(uint32_t cachePosition, uint32_t maxSizeVertexCache)
    {
        return s_vertexCacheScores[maxSizeVertexCache][cachePosition];
    }

    inline float FindVertexValenceScore(uint32_t numActiveTris)
    {
        return s_vertexValenceScores[numActiveTris];
    }

    float FindVertexScore(uint32_t numActiveFaces, uint32_t cachePosition, uint32_t vertexCacheSize)
    {
        if (numActiveFaces == 0)
        {
            // No tri needs this vertex!

            return -1.0f;
        }

        float score = 0.f;

        if (cachePosition < vertexCacheSize)
        {
            score += s_vertexCacheScores[vertexCacheSize][cachePosition];
        }

        if (numActiveFaces < kMaxPrecomputedVertexValenceScores)
        {
            score += s_vertexValenceScores[numActiveFaces];
        }
        else
        {
            score += ComputeVertexValenceScore(numActiveFaces);
        }

        return score;
    }

    template <typename IndexType>
    struct OptimizeVertexData
    {
        float   score;
        uint32_t    activeFaceListStart;
        uint32_t    activeFaceListSize;
        IndexType  cachePos0;
        IndexType  cachePos1;

        OptimizeVertexData() : score(0.f), activeFaceListStart(0), activeFaceListSize(0), cachePos0(0), cachePos1(0) { }
    };

    template <typename T, typename IndexType>

    struct IndexSortCompareIndexed
    {
        const IndexType *_indexData;

        IndexSortCompareIndexed(const IndexType *indexData) : _indexData(indexData) { }

        bool operator()(T a, T b) const
        {
            IndexType indexA = _indexData[a];
            IndexType indexB = _indexData[b];

            if (indexA < indexB)
                return true;

            return false;
        }
    };

    template <typename T, typename IndexType>
    struct FaceValenceSort
    {
        const OptimizeVertexData<IndexType> *_vertexData;

        FaceValenceSort(const OptimizeVertexData<IndexType> *vertexData) : _vertexData(vertexData) { }

        bool operator()(T a, T b) const
        {
            const OptimizeVertexData<IndexType> *vA0 = _vertexData + a * 3 + 0;
            const OptimizeVertexData<IndexType> *vA1 = _vertexData + a * 3 + 1;
            const OptimizeVertexData<IndexType> *vA2 = _vertexData + a * 3 + 2;
            const OptimizeVertexData<IndexType> *vB0 = _vertexData + b * 3 + 0;
            const OptimizeVertexData<IndexType> *vB1 = _vertexData + b * 3 + 1;
            const OptimizeVertexData<IndexType> *vB2 = _vertexData + b * 3 + 2;

            int aValence = vA0->activeFaceListSize + vA1->activeFaceListSize + vA2->activeFaceListSize;
            int bValence = vB0->activeFaceListSize + vB1->activeFaceListSize + vB2->activeFaceListSize;

            // higher scoring faces are those with lower valence totals

            // reverse sort (reverse of reverse)

            if (aValence < bValence)
                return true;

            return false;
        }
    };

    template <typename IndexType>
    HRESULT OptimizeFacesImpl(_In_reads_(indexCount) const IndexType* indexList, uint32_t indexCount, _Out_writes_(indexCount / 3) uint32_t* faceRemap, uint32_t lruCacheSize)
    {
        std::unique_ptr<OptimizeVertexData<IndexType>[]> vertexDataList(new (std::nothrow) OptimizeVertexData<IndexType>[indexCount]);
        if (!vertexDataList)
            return E_OUTOFMEMORY;

        std::unique_ptr<unsigned int[]> vertexRemap(new (std::nothrow) unsigned int[indexCount]);
        std::unique_ptr<uint32_t[]> activeFaceList(new (std::nothrow) uint32_t[indexCount]);
        if (!vertexRemap || !activeFaceList)
            return E_OUTOFMEMORY;

        const uint32_t faceCount = indexCount / 3;

        std::unique_ptr<uint8_t[]> processedFaceList(new (std::nothrow) uint8_t[faceCount]);
        std::unique_ptr<unsigned int[]> faceSorted(new (std::nothrow) unsigned int[faceCount]);
        std::unique_ptr<unsigned int[]> faceReverseLookup(new (std::nothrow) unsigned int[faceCount]);
        if (!processedFaceList || !faceSorted || !faceReverseLookup)
            return E_OUTOFMEMORY;

        memset(processedFaceList.get(), 0, sizeof(uint8_t) * faceCount);

        // build the vertex remap table
        unsigned int uniqueVertexCount = 0;
        uint32_t unused = 0;
        {
            typedef IndexSortCompareIndexed<unsigned int, IndexType> indexSorter;

            std::unique_ptr<unsigned int[]> indexSorted(new (std::nothrow) unsigned int[indexCount]);
            if (!indexSorted)
                return E_OUTOFMEMORY;

            for (unsigned int i = 0; i < indexCount; i++)
            {
                indexSorted[i] = i;
            }

            indexSorter sortFunc(indexList);
            std::sort(indexSorted.get(), indexSorted.get() + indexCount, sortFunc);

            bool first = false;
            for (unsigned int i = 0; i < indexCount; i++)
            {
                unsigned int idx = indexSorted[i];
                if (indexList[idx] == IndexType(-1))
                {
                    unused++;
                    vertexRemap[idx] = unsigned(-1);
                    continue;
                }

                if (!i || first || sortFunc(indexSorted[i - 1], idx))
                {
                    // it's not a duplicate
                    vertexRemap[idx] = uniqueVertexCount;
                    uniqueVertexCount++;
                    first = true;
                }
                else
                {
                    vertexRemap[indexSorted[i]] = vertexRemap[indexSorted[i - 1]];
                }
            }
        }

        // compute face count per vertex
        for (uint32_t i = 0; i < indexCount; ++i)
        {
            if (vertexRemap[i] == unsigned(-1))
                continue;

            OptimizeVertexData<IndexType>& vertexData = vertexDataList[vertexRemap[i]];
            vertexData.activeFaceListSize++;
        }

        const IndexType kEvictedCacheIndex = std::numeric_limits<IndexType>::max();
        {
            // allocate face list per vertex
            uint32_t curActiveFaceListPos = 0;
            for (uint32_t i = 0; i < uniqueVertexCount; ++i)
            {
                OptimizeVertexData<IndexType>& vertexData = vertexDataList[i];
                vertexData.cachePos0 = kEvictedCacheIndex;
                vertexData.cachePos1 = kEvictedCacheIndex;
                vertexData.activeFaceListStart = curActiveFaceListPos;
                curActiveFaceListPos += vertexData.activeFaceListSize;
                vertexData.score = FindVertexScore(vertexData.activeFaceListSize, vertexData.cachePos0, lruCacheSize);

                vertexData.activeFaceListSize = 0;
            }

            assert(curActiveFaceListPos == (indexCount - unused));
        }

        // sort unprocessed faces by highest score
        for (uint32_t f = 0; f < faceCount; f++)
        {
            faceSorted[f] = f;
        }

        FaceValenceSort<unsigned int, IndexType> faceValenceSort(vertexDataList.get());
        std::sort(faceSorted.get(), faceSorted.get() + faceCount, faceValenceSort);

        for (uint32_t f = 0; f < faceCount; f++)
        {
            faceReverseLookup[faceSorted[f]] = f;
        }

        // fill out face list per vertex
        for (uint32_t i = 0; i < indexCount; i += 3)
        {
            for (uint32_t j = 0; j < 3; ++j)
            {
                unsigned int v = vertexRemap[i + j];
                if (v == unsigned(-1))
                    continue;

                OptimizeVertexData<IndexType>& vertexData = vertexDataList[v];
                activeFaceList[vertexData.activeFaceListStart + vertexData.activeFaceListSize] = i;
                vertexData.activeFaceListSize++;
            }
        }

        unsigned int vertexCacheBuffer[(kMaxVertexCacheSize + 3) * 2];
        unsigned int* cache0 = vertexCacheBuffer;
        unsigned int* cache1 = vertexCacheBuffer + (kMaxVertexCacheSize + 3);
        unsigned int entriesInCache0 = 0;

        uint32_t bestFace = 0;
        for (uint32_t i = 0; i < indexCount; i += 3)
        {
            if (vertexRemap[i] == unsigned(-1)
                || vertexRemap[i + 1] == unsigned(-1)
                || vertexRemap[i + 2] == unsigned(-1))
            {
                ++bestFace;
                continue;
            }
            else
                break;
        }

        float bestScore = -1.f;

        const float maxValenceScore = FindVertexScore(1, kEvictedCacheIndex, lruCacheSize) * 3.f;
        unsigned int nextBestFace = 0;

        uint32_t curFace = 0;
        for (uint32_t i = 0; i < indexCount; i += 3)
        {
            if (vertexRemap[i] == unsigned(-1)
                || vertexRemap[i + 1] == unsigned(-1)
                || vertexRemap[i + 2] == unsigned(-1))
            {
                continue;
            }

            if (bestScore < 0.f)
            {
                // no verts in the cache are used by any unprocessed faces so
                // search all unprocessed faces for a new starting point
                for (; nextBestFace < faceCount; nextBestFace++)
                {
                    unsigned int faceIndex = faceSorted[nextBestFace];
                    if (processedFaceList[faceIndex] == 0)
                    {
                        uint32_t face = faceIndex * 3;
                        float faceScore = 0.f;
                        bool skip = false;
                        for (uint32_t k = 0; k < 3; ++k)
                        {
                            if (vertexRemap[face + k] == unsigned(-1))
                            {
                                skip = true;
                                break;
                            }

                            float vertexScore = vertexDataList[vertexRemap[face + k]].score;
                            faceScore += vertexScore;
                        }

                        if (skip)
                            continue;

                        bestScore = faceScore;
                        bestFace = face;
                        nextBestFace++;
                        break; // we're searching a pre-sorted list, first one we find will be the best
                    }
                }
                assert(bestScore >= 0.f);
            }

            processedFaceList[bestFace / 3] = 1;
            uint16_t entriesInCache1 = 0;

            faceRemap[curFace] = bestFace / 3;
            curFace++;

            // add bestFace to LRU cache
            assert(vertexRemap[bestFace] != unsigned(-1));
            assert(vertexRemap[bestFace + 1] != unsigned(-1));
            assert(vertexRemap[bestFace + 2] != unsigned(-1));

            for (uint32_t v = 0; v < 3; ++v)
            {
                OptimizeVertexData<IndexType>& vertexData = vertexDataList[vertexRemap[bestFace + v]];

                if (vertexData.cachePos1 >= entriesInCache1)
                {
                    vertexData.cachePos1 = entriesInCache1;
                    cache1[entriesInCache1++] = vertexRemap[bestFace + v];

                    if (vertexData.activeFaceListSize == 1)
                    {
                        --vertexData.activeFaceListSize;
                        continue;
                    }
                }

                assert(vertexData.activeFaceListSize > 0);
                uint32_t* begin = activeFaceList.get() + vertexData.activeFaceListStart;
                uint32_t* end = activeFaceList.get() + (vertexData.activeFaceListStart + vertexData.activeFaceListSize);
                uint32_t* it = std::find(begin, end, bestFace);

                assert(it != end);

                std::swap(*it, *(end - 1));

                --vertexData.activeFaceListSize;
                vertexData.score = FindVertexScore(vertexData.activeFaceListSize, vertexData.cachePos1, lruCacheSize);

                // need to re-sort the faces that use this vertex, as their score will change due to activeFaceListSize shrinking
                for (const uint32_t *fi = begin; fi != end - 1; ++fi)
                {
                    unsigned int faceIndex = *fi / 3;
                    unsigned int n = faceReverseLookup[faceIndex];
                    assert(faceSorted[n] == faceIndex);

                    // found it, now move it up
                    while (n > 0)
                    {
                        if (faceValenceSort(n, n - 1))
                        {
                            faceReverseLookup[faceSorted[n]] = n - 1;
                            faceReverseLookup[faceSorted[n - 1]] = n;
                            std::swap(faceSorted[n], faceSorted[n - 1]);
                            n--;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }

            // move the rest of the old verts in the cache down and compute their new scores
            for (uint32_t c0 = 0; c0 < entriesInCache0; ++c0)
            {
                OptimizeVertexData<IndexType>& vertexData = vertexDataList[cache0[c0]];

                if (vertexData.cachePos1 >= entriesInCache1)
                {
                    vertexData.cachePos1 = entriesInCache1;
                    cache1[entriesInCache1++] = cache0[c0];
                    vertexData.score = FindVertexScore(vertexData.activeFaceListSize, vertexData.cachePos1, lruCacheSize);

                    // don't need to re-sort this vertex... once it gets out of the cache, it'll have its original score
                }
            }

            // find the best scoring triangle in the current cache (including up to 3 that were just evicted)
            bestScore = -1.f;

            for (uint32_t c1 = 0; c1 < entriesInCache1; ++c1)
            {
                OptimizeVertexData<IndexType>& vertexData = vertexDataList[cache1[c1]];
                vertexData.cachePos0 = vertexData.cachePos1;
                vertexData.cachePos1 = kEvictedCacheIndex;

                for (uint32_t j = 0; j < vertexData.activeFaceListSize; ++j)
                {
                    uint32_t face = activeFaceList[vertexData.activeFaceListStart + j];
                    float faceScore = 0.f;

                    for (uint32_t v = 0; v < 3; v++)
                    {
                        OptimizeVertexData<IndexType>& faceVertexData = vertexDataList[vertexRemap[face + v]];
                        faceScore += faceVertexData.score;
                    }

                    if (faceScore > bestScore)
                    {
                        bestScore = faceScore;
                        bestFace = face;
                    }
                }
            }

            std::swap(cache0, cache1);

            entriesInCache0 = std::min<unsigned int>(entriesInCache1, lruCacheSize);
        }

        for (; curFace < faceCount; ++curFace)
        {
            faceRemap[curFace] = uint32_t(-1);
        }

        return S_OK;
    }
}

//=====================================================================================
// Entry-points
//=====================================================================================

_Use_decl_annotations_
HRESULT DirectX::OptimizeFacesLRU(
    const uint16_t* indices, size_t nFaces,
    uint32_t* faceRemap, uint32_t lruCacheSize)
{
    if (!indices || !nFaces || !faceRemap)
        return E_INVALIDARG;

    if (!lruCacheSize || lruCacheSize > kMaxVertexCacheSize)
        return E_INVALIDARG;

    if ((uint64_t(nFaces) * 3) >= UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    InitOnceExecuteOnce(&s_initOnce, ComputeVertexScores, nullptr, nullptr);

    return OptimizeFacesImpl<uint16_t>(indices, static_cast<uint32_t>(nFaces * 3), faceRemap, lruCacheSize);
}

_Use_decl_annotations_
HRESULT DirectX::OptimizeFacesLRU(
    const uint32_t* indices, size_t nFaces,
    uint32_t* faceRemap, uint32_t lruCacheSize)
{
    if (!indices || !nFaces || !faceRemap)
        return E_INVALIDARG;

    if (!lruCacheSize || lruCacheSize > kMaxVertexCacheSize)
        return E_INVALIDARG;

    if ((uint64_t(nFaces) * 3) >= UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    InitOnceExecuteOnce(&s_initOnce, ComputeVertexScores, nullptr, nullptr);

    return OptimizeFacesImpl<uint32_t>(indices, static_cast<uint32_t>(nFaces * 3), faceRemap, lruCacheSize);
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::OptimizeFacesLRUEx(
    const uint16_t* indices, size_t nFaces, const uint32_t* attributes,
    uint32_t* faceRemap, uint32_t lruCacheSize)
{
    if (!indices || !nFaces || !attributes || !faceRemap)
        return E_INVALIDARG;

    if (!lruCacheSize || lruCacheSize > kMaxVertexCacheSize)
        return E_INVALIDARG;

    if ((uint64_t(nFaces) * 3) >= UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    InitOnceExecuteOnce(&s_initOnce, ComputeVertexScores, nullptr, nullptr);

    // TODO -
    return E_NOTIMPL;
}

_Use_decl_annotations_
HRESULT DirectX::OptimizeFacesLRUEx(
    const uint32_t* indices, size_t nFaces, const uint32_t* attributes,
    uint32_t* faceRemap, uint32_t lruCacheSize)
{
    if (!indices || !nFaces || !attributes || !faceRemap)
        return E_INVALIDARG;

    if (!lruCacheSize || lruCacheSize > kMaxVertexCacheSize)
        return E_INVALIDARG;

    if ((uint64_t(nFaces) * 3) >= UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    InitOnceExecuteOnce(&s_initOnce, ComputeVertexScores, nullptr, nullptr);

    // TODO -
    return E_NOTIMPL;
}
