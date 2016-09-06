//--------------------------------------------------------------------------------------
// File: WaveFrontReader.h
//
// Code for loading basic mesh data from a WaveFront OBJ file
//
// http://en.wikipedia.org/wiki/Wavefront_.obj_file
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
//--------------------------------------------------------------------------------------

#pragma once

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <windows.h>

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

#include <stdint.h>

#include <directxmath.h>
#include <directxcollision.h>

template<class index_t>
class WaveFrontReader
{
public:
    typedef index_t index_t;

    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 textureCoordinate;
    };

    WaveFrontReader() : hasNormals(false), hasTexcoords(false) {}

    HRESULT Load( _In_z_ const wchar_t* szFileName, bool ccw = true )
    {
        Clear();

        static const size_t MAX_POLY = 64;

        using namespace DirectX;

        std::wifstream InFile( szFileName );
        if( !InFile )
            return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

        wchar_t fname[_MAX_FNAME];
        _wsplitpath_s( szFileName, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, nullptr, 0 );

        name = fname;

        std::vector<XMFLOAT3>   positions;
        std::vector<XMFLOAT3>   normals;
        std::vector<XMFLOAT2>   texCoords;

        VertexCache  vertexCache;

        Material defmat;

        wcscpy_s( defmat.strName, L"default" );
        materials.push_back( defmat );

        uint32_t curSubset = 0;

        wchar_t strCommand[256] = {};
        wchar_t strMaterialFilename[MAX_PATH] = {};
        for( ;; )
        {
            InFile >> strCommand;
            if( !InFile )
                break;

            if ( *strCommand == L'#' )
            {
                // Comment
            }
            else if( 0 == wcscmp( strCommand, L"o" ) )
            {
                // Object name ignored
            }
            else if( 0 == wcscmp( strCommand, L"g" ) )
            {
                // Group name ignored
            }
            else if( 0 == wcscmp( strCommand, L"s" ) )
            {
                // Smoothing group ignored
            }
            else if( 0 == wcscmp( strCommand, L"v" ) )
            {
                // Vertex Position
                float x, y, z;
                InFile >> x >> y >> z;
                positions.push_back( XMFLOAT3( x, y, z ) );
            }
            else if( 0 == wcscmp( strCommand, L"vt" ) )
            {
                // Vertex TexCoord
                float u, v;
                InFile >> u >> v;
                texCoords.push_back( XMFLOAT2( u, v ) );

                hasTexcoords = true;
            }
            else if( 0 == wcscmp( strCommand, L"vn" ) )
            {
                // Vertex Normal
                float x, y, z;
                InFile >> x >> y >> z;
                normals.push_back( XMFLOAT3( x, y, z ) );

                hasNormals = true;
            }
            else if( 0 == wcscmp( strCommand, L"f" ) )
            {
                // Face
                UINT iPosition, iTexCoord, iNormal;
                Vertex vertex;

                DWORD faceIndex[ MAX_POLY ];
                size_t iFace = 0;
                for(;;)
                {
                    if ( iFace >= MAX_POLY )
                    {
                        // Too many polygon verts for the reader
                        return E_FAIL;
                    }

                    memset( &vertex, 0, sizeof( vertex ) );

                    // OBJ format uses 1-based arrays
                    InFile >> iPosition;
                    if ( iPosition > positions.size() )
                        return E_FAIL;

                    vertex.position = positions[ iPosition - 1 ];

                    if( '/' == InFile.peek() )
                    {
                        InFile.ignore();

                        if( '/' != InFile.peek() )
                        {
                            // Optional texture coordinate
                            InFile >> iTexCoord;
                            if ( iTexCoord > texCoords.size() )
                                return E_FAIL;

                            vertex.textureCoordinate = texCoords[ iTexCoord - 1 ];
                        }

                        if( '/' == InFile.peek() )
                        {
                            InFile.ignore();

                            // Optional vertex normal
                            InFile >> iNormal;
                            if ( iNormal > normals.size() )
                                return E_FAIL;

                            vertex.normal = normals[ iNormal - 1 ];
                        }
                    }

                    // If a duplicate vertex doesn't exist, add this vertex to the Vertices
                    // list. Store the index in the Indices array. The Vertices and Indices
                    // lists will eventually become the Vertex Buffer and Index Buffer for
                    // the mesh.
                    DWORD index = AddVertex( iPosition, &vertex, vertexCache );
                    if ( index == (DWORD)-1 )
                       return E_OUTOFMEMORY;

#pragma warning( suppress : 4127 )
                    if ( sizeof(index_t) == 2 && ( index >= 0xFFFF ) )
                    {
                        // Too many indices for 16-bit IB!
                        return E_FAIL;
                    }
#pragma warning( suppress : 4127 )
                    else if ( sizeof(index_t) == 4 && ( index >= 0xFFFFFFFF ) )
                    {
                        // Too many indices for 32-bit IB!
                        return E_FAIL;
                    }

                    faceIndex[ iFace ] = index;
                    ++iFace;
   
                    // Check for more face data or end of the face statement
                    bool faceEnd = false;
                    for(;;)
                    {
                        wchar_t p = InFile.peek();
                    
                        if ( '\n' == p || !InFile )
                        {
                            faceEnd = true;
                            break;
                        }
                        else if ( isdigit( p ) )
                            break;

                        InFile.ignore();
                    }

                    if ( faceEnd )
                        break;
                }

                if ( iFace < 3 )
                {
                    // Need at least 3 points to form a triangle
                    return E_FAIL;
                }

                // Convert polygons to triangles
                DWORD i0 = faceIndex[0];
                DWORD i1 = faceIndex[1];

                for( size_t j = 2; j < iFace; ++ j )
                {
                    DWORD index = faceIndex[ j ];
                    indices.push_back( static_cast<index_t>( i0 ) );
                    if ( ccw )
                    {
                        indices.push_back( static_cast<index_t>( i1 ) );
                        indices.push_back( static_cast<index_t>( index ) );
                    }
                    else
                    {
                        indices.push_back( static_cast<index_t>( index ) );
                        indices.push_back( static_cast<index_t>( i1 ) );
                    }                        
                     
                    attributes.push_back( curSubset );

                    i1 = index;
                }

                assert( attributes.size()*3 == indices.size() );
            }
            else if( 0 == wcscmp( strCommand, L"mtllib" ) )
            {
                // Material library
                InFile >> strMaterialFilename;
            }
            else if( 0 == wcscmp( strCommand, L"usemtl" ) )
            {
                // Material
                wchar_t strName[MAX_PATH] = {};
                InFile >> strName;

                bool bFound = false;
                uint32_t count = 0;
                for( auto it = materials.cbegin(); it != materials.cend(); ++it, ++count )
                {
                    if( 0 == wcscmp( it->strName, strName ) )
                    {
                        bFound = true;
                        curSubset = count;
                        break;
                    }
                }

                if( !bFound )
                {
                    Material mat;
                    curSubset = static_cast<uint32_t>( materials.size() );
                    wcscpy_s( mat.strName, MAX_PATH - 1, strName );
                    materials.push_back( mat );
                }
            }
            else
            {
#ifdef _DEBUG
                // Unimplemented or unrecognized command
                OutputDebugStringW( strCommand );
#endif
            }

            InFile.ignore( 1000, '\n' );
        }

        // Cleanup
        InFile.close();

        BoundingBox::CreateFromPoints( bounds, positions.size(), positions.data(), sizeof(XMFLOAT3) );

        // If an associated material file was found, read that in as well.
        if( *strMaterialFilename )
        {
            wchar_t ext[_MAX_EXT];
            _wsplitpath_s( strMaterialFilename, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, ext, _MAX_EXT );

            wchar_t drive[_MAX_DRIVE];
            wchar_t dir[_MAX_DIR];
            _wsplitpath_s( szFileName, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0 );

            wchar_t szPath[ MAX_PATH ];
            _wmakepath_s( szPath, MAX_PATH, drive, dir, fname, ext );

            HRESULT hr = LoadMTL( szPath );
            if ( FAILED(hr) )
                return hr;
        }

        return S_OK;
    }

    HRESULT LoadMTL( _In_z_ const wchar_t* szFileName )
    {
        // Assumes MTL is in CWD along with OBJ
        std::wifstream InFile( szFileName );
        if( !InFile )
            return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

        auto curMaterial = materials.end();

        wchar_t strCommand[256] = {};
        for( ;; )
        {
            InFile >> strCommand;
            if( !InFile )
                break;

            if( 0 == wcscmp( strCommand, L"newmtl" ) )
            {
                // Switching active materials
                wchar_t strName[MAX_PATH] = {};
                InFile >> strName;

                curMaterial = materials.end();
                for( auto it = materials.begin(); it != materials.end(); ++it )
                {
                    if( 0 == wcscmp( it->strName, strName ) )
                    {
                        curMaterial = it;
                        break;
                    }
                }
            }

            // The rest of the commands rely on an active material
            if( curMaterial == materials.end() )
                continue;

            if( 0 == wcscmp( strCommand, L"#" ) )
            {
                // Comment
            }
            else if( 0 == wcscmp( strCommand, L"Ka" ) )
            {
                // Ambient color
                float r, g, b;
                InFile >> r >> g >> b;
                curMaterial->vAmbient = XMFLOAT3( r, g, b );
            }
            else if( 0 == wcscmp( strCommand, L"Kd" ) )
            {
                // Diffuse color
                float r, g, b;
                InFile >> r >> g >> b;
                curMaterial->vDiffuse = XMFLOAT3( r, g, b );
            }
            else if( 0 == wcscmp( strCommand, L"Ks" ) )
            {
                // Specular color
                float r, g, b;
                InFile >> r >> g >> b;
                curMaterial->vSpecular = XMFLOAT3( r, g, b );
            }
            else if( 0 == wcscmp( strCommand, L"d" ) ||
                     0 == wcscmp( strCommand, L"Tr" ) )
            {
                // Alpha
                InFile >> curMaterial->fAlpha;
            }
            else if( 0 == wcscmp( strCommand, L"Ns" ) )
            {
                // Shininess
                int nShininess;
                InFile >> nShininess;
                curMaterial->nShininess = nShininess;
            }
            else if( 0 == wcscmp( strCommand, L"illum" ) )
            {
                // Specular on/off
                int illumination;
                InFile >> illumination;
                curMaterial->bSpecular = ( illumination == 2 );
            }
            else if( 0 == wcscmp( strCommand, L"map_Kd" ) )
            {
                // Texture
                InFile >> curMaterial->strTexture;
            }
            else
            {
                // Unimplemented or unrecognized command
            }

            InFile.ignore( 1000, L'\n' );
        }

        InFile.close();

        return S_OK;
    }

    void Clear()
    {
        vertices.clear();
        indices.clear();
        attributes.clear();
        materials.clear();
        name.clear();
        hasNormals = false;
        hasTexcoords = false;

        bounds.Center.x = bounds.Center.y = bounds.Center.z = 0.f;
        bounds.Extents.x = bounds.Extents.y = bounds.Extents.z = 0.f;
    }

    HRESULT LoadVBO( _In_z_ const wchar_t* szFileName )
    {
        Clear();

        wchar_t fname[_MAX_FNAME];
        _wsplitpath_s( szFileName, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, nullptr, 0 );

        name = fname;

        Material defmat;
        wcscpy_s( defmat.strName, L"default" );
        materials.push_back( defmat );

        std::ifstream vboFile(szFileName, std::ifstream::in | std::ifstream::binary);
        if ( !vboFile.is_open() )
            return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

        hasNormals = hasTexcoords = true;

        uint32_t numVertices = 0;
        uint32_t numIndices = 0;

        vboFile.read( reinterpret_cast<char*>( &numVertices ), sizeof(uint32_t ) );
        if ( !numVertices )
            return E_FAIL;

        vboFile.read( reinterpret_cast<char*>( &numIndices ), sizeof(uint32_t ) );
        if ( !numIndices )
            return E_FAIL;

        vertices.resize( numVertices );
        vboFile.read( reinterpret_cast<char*>( vertices.data() ), sizeof(Vertex) * numVertices );

#pragma warning( suppress : 4127 )
        if ( sizeof( index_t ) == 2 )
        {
            indices.resize( numIndices );
            vboFile.read( reinterpret_cast<char*>( indices.data() ), sizeof(uint16_t) * numIndices );
        }
        else
        {
            std::vector<uint16_t> tmp;
            tmp.resize( numIndices );
            vboFile.read( reinterpret_cast<char*>( tmp.data() ), sizeof(uint16_t) * numIndices );

            indices.reserve( numIndices );
            for( auto it = tmp.cbegin(); it != tmp.cend(); ++it )
            {
                indices.push_back( *it );
            }
        }

        BoundingBox::CreateFromPoints( bounds, vertices.size(), reinterpret_cast<const XMFLOAT3*>( vertices.data() ), sizeof(Vertex) );

        vboFile.close();

        return S_OK;
    }

    struct Material
    {
        DirectX::XMFLOAT3 vAmbient;
        DirectX::XMFLOAT3 vDiffuse;
        DirectX::XMFLOAT3 vSpecular;
        uint32_t nShininess;
        float fAlpha;

        bool bSpecular;

        wchar_t strName[MAX_PATH];
        wchar_t strTexture[MAX_PATH];

        Material() :
            vAmbient( 0.2f, 0.2f, 0.2f ),
            vDiffuse( 0.8f, 0.8f, 0.8f ),
            vSpecular( 1.0f, 1.0f, 1.0f ),
            nShininess( 0 ),
            fAlpha( 1.f ),
            bSpecular( false )
            { memset(strName, 0, sizeof(strName)); memset(strTexture, 0, sizeof(strTexture)); } 
    };

    std::vector<Vertex>     vertices;
    std::vector<index_t>    indices;
    std::vector<uint32_t>   attributes;
    std::vector<Material>   materials;

    std::wstring            name;
    bool                    hasNormals;
    bool                    hasTexcoords;

    DirectX::BoundingBox    bounds;

private:
    typedef std::unordered_multimap<UINT, UINT> VertexCache;

    DWORD AddVertex( UINT hash, Vertex* pVertex, VertexCache& cache )
    {
        auto f = cache.equal_range( hash );

        for( auto it = f.first; it != f.second; ++it )
        {
            auto& tv = vertices[ it->second ];

            if ( 0 == memcmp( pVertex, &tv, sizeof(Vertex) ) )
            {
                return it->second;
            }
        }

        DWORD index = static_cast<UINT>( vertices.size() );
        vertices.push_back( *pVertex );

        VertexCache::value_type entry( hash, index );
        cache.insert( entry );
        return index;
    }
};
