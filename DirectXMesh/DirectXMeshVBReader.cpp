//-------------------------------------------------------------------------------------
// DirectXMeshVBReader.cpp
//  
// DirectX Mesh Geometry Library - Vertex Buffer Reader
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

using namespace DirectX::PackedVector;

namespace DirectX
{

class VBReader::Impl
{
public:
    Impl() : mTempSize(0) {}

    HRESULT Initialize( _In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl );
    HRESULT AddStream( _In_reads_bytes_(stride*nVerts) const void* vb, _In_ size_t nVerts, _In_ size_t inputSlot, _In_ size_t stride );
    HRESULT Read( _Out_writes_(count) XMVECTOR* buffer, _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex, _In_ size_t count ) const;

    void Release()
    {
        mInputDesc.clear();
        mSemantics.clear();
        memset( mStrides, 0, sizeof(mStrides) );
        memset( mBuffers, 0, sizeof(mBuffers) );
        memset( mVerts, 0, sizeof(mVerts) );
        memset( mDefaultStrides, 0, sizeof(mDefaultStrides) );
        mTempBuffer.reset();
    }

    const D3D11_INPUT_ELEMENT_DESC* GetElement( _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex ) const
    {
        auto range = mSemantics.equal_range(semanticName);

        auto it = range.first;
        for (; it != range.second; ++it)
        {
            if (mInputDesc[it->second].SemanticIndex == semanticIndex)
                break;
        }

        if (it == range.second)
            return nullptr;

        return &mInputDesc[it->second];
    }

    XMVECTOR* GetTemporaryBuffer( size_t count ) const
    {
        if ( !mTempBuffer || ( mTempSize < count ) )
        {
            mTempSize = count;

            for( size_t j = 0; j < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++j )
            {
                if ( mVerts[ j ] > mTempSize )
                    mTempSize = mVerts[ j ];
            }

            mTempBuffer.reset( reinterpret_cast<XMVECTOR*>( _aligned_malloc( sizeof(XMVECTOR) * mTempSize, 16 ) ) );
            if ( !mTempBuffer )
                mTempSize = 0;
        }

        return mTempBuffer.get();
    }

private:
    typedef std::multimap<std::string,uint32_t> SemanticMap;

    std::vector<D3D11_INPUT_ELEMENT_DESC>   mInputDesc;
    SemanticMap                             mSemantics;
    uint32_t                                mStrides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    const void*                             mBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    size_t                                  mVerts[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    uint32_t                                mDefaultStrides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    mutable size_t                          mTempSize;
    mutable ScopedAlignedArrayXMVECTOR      mTempBuffer;
};


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT VBReader::Impl::Initialize( const D3D11_INPUT_ELEMENT_DESC* vbDecl, size_t nDecl )
{
    Release();

    if ( !IsValid( vbDecl, nDecl ) )
        return E_INVALIDARG;

    assert( nDecl < D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT );
    _Analysis_assume_( nDecl < D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT );

    uint32_t offsets[ D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT ];
    ComputeInputLayout( vbDecl, nDecl, offsets, mDefaultStrides );

    for( uint32_t j = 0; j < nDecl; ++j )
    {
        if ( vbDecl[ j ].InputSlotClass == D3D11_INPUT_PER_INSTANCE_DATA )
        {
            // Does not currently support instance data layouts
            Release();
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        }

        mInputDesc.push_back( vbDecl[ j ] );

        mInputDesc[ j ].AlignedByteOffset = offsets[ j ];

        mSemantics.insert( SemanticMap::value_type( vbDecl[ j ].SemanticName, j ) );

        // Add common aliases
        if ( _stricmp( vbDecl[ j ].SemanticName, "POSITION" ) == 0 )
        {
            mSemantics.insert( SemanticMap::value_type( "SV_Position", j ) );
        }
        else if ( _stricmp( vbDecl[ j ].SemanticName, "SV_Position" ) == 0 )
        {
            mSemantics.insert( SemanticMap::value_type( "POSITION", j ) );
        }
    }

    return S_OK;
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT VBReader::Impl::AddStream( const void* vb, size_t nVerts, size_t inputSlot, size_t stride )
{
    if ( !vb || !nVerts )
        return E_INVALIDARG;

    if ( nVerts >= UINT32_MAX )
        return E_INVALIDARG;

    if ( inputSlot >= D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT )
        return E_INVALIDARG;

    if ( stride > D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES )
        return E_INVALIDARG;

    mStrides[ inputSlot ] = ( stride > 0 ) ? uint32_t( stride ) : mDefaultStrides[ inputSlot ];
    mBuffers[ inputSlot ] = vb;
    mVerts[ inputSlot ] = nVerts;

    return S_OK;
}


//-------------------------------------------------------------------------------------
#define LOAD_VERTS( type, func )\
        for( size_t icount = 0; icount < count; ++icount )\
        {\
            if ( ( ptr + sizeof(type) ) > eptr )\
                return E_UNEXPECTED;\
            *buffer++ = func( reinterpret_cast<const type*>(ptr) );\
            ptr += stride;\
        }\
        break;

_Use_decl_annotations_
HRESULT VBReader::Impl::Read( XMVECTOR* buffer, LPCSTR semanticName, UINT semanticIndex, size_t count ) const
{
    if ( !buffer || !semanticName || !count )
        return E_INVALIDARG;

    auto range = mSemantics.equal_range( semanticName );
    
    auto it = range.first;
    for( ; it != range.second; ++it )
    {
        if ( mInputDesc[ it->second ].SemanticIndex == semanticIndex )
            break;
    }

    if ( it == range.second )
        return HRESULT_FROM_WIN32( ERROR_INVALID_NAME );

    uint32_t inputSlot = mInputDesc[ it->second ].InputSlot;

    auto vb = reinterpret_cast<const uint8_t*>( mBuffers[ inputSlot ] );
    if ( !vb )
        return E_FAIL;

    if ( count > mVerts[ inputSlot ] )
        return E_BOUNDS;

    uint32_t stride = mStrides[ inputSlot ];
    if ( !stride )
        return E_UNEXPECTED;

    const uint8_t* eptr = vb + stride * mVerts[ inputSlot ];
    const uint8_t* ptr = vb + mInputDesc[ it->second ].AlignedByteOffset;

    switch( mInputDesc[ it->second ].Format )
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        LOAD_VERTS( XMFLOAT4, XMLoadFloat4 )

    case DXGI_FORMAT_R32G32B32A32_UINT:
        LOAD_VERTS( XMUINT4, XMLoadUInt4 )

    case DXGI_FORMAT_R32G32B32A32_SINT:
        LOAD_VERTS( XMINT4, XMLoadSInt4 )

    case DXGI_FORMAT_R32G32B32_FLOAT:
        LOAD_VERTS( XMFLOAT3, XMLoadFloat3 )

    case DXGI_FORMAT_R32G32B32_UINT:
        LOAD_VERTS( XMUINT3, XMLoadUInt3 )

    case DXGI_FORMAT_R32G32B32_SINT:
        LOAD_VERTS( XMINT3, XMLoadSInt3 )

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        LOAD_VERTS( XMHALF4, XMLoadHalf4 )

    case DXGI_FORMAT_R16G16B16A16_UNORM:
        LOAD_VERTS( XMUSHORTN4, XMLoadUShortN4 ) 

    case DXGI_FORMAT_R16G16B16A16_UINT:
        LOAD_VERTS( XMUSHORT4, XMLoadUShort4 )

    case DXGI_FORMAT_R16G16B16A16_SNORM:
        LOAD_VERTS( XMSHORTN4, XMLoadShortN4 )

    case DXGI_FORMAT_R16G16B16A16_SINT:
        LOAD_VERTS( XMSHORT4, XMLoadShort4 )

    case DXGI_FORMAT_R32G32_FLOAT:
        LOAD_VERTS( XMFLOAT2, XMLoadFloat2 )

    case DXGI_FORMAT_R32G32_UINT:
        LOAD_VERTS( XMUINT2, XMLoadUInt2 )

    case DXGI_FORMAT_R32G32_SINT:
        LOAD_VERTS( XMINT2, XMLoadSInt2 )

    case DXGI_FORMAT_R10G10B10A2_UNORM:
        LOAD_VERTS( XMUDECN4, XMLoadUDecN4 );

    case DXGI_FORMAT_R10G10B10A2_UINT:
        LOAD_VERTS( XMUDEC4, XMLoadUDec4 );

    case DXGI_FORMAT_R11G11B10_FLOAT:
        LOAD_VERTS( XMFLOAT3PK, XMLoadFloat3PK );

    case DXGI_FORMAT_R8G8B8A8_UNORM:
        LOAD_VERTS( XMUBYTEN4, XMLoadUByteN4 )

    case DXGI_FORMAT_R8G8B8A8_UINT:
        LOAD_VERTS( XMUBYTE4, XMLoadUByte4 )

    case DXGI_FORMAT_R8G8B8A8_SNORM:
        LOAD_VERTS( XMBYTEN4, XMLoadByteN4 )

    case DXGI_FORMAT_R8G8B8A8_SINT:
        LOAD_VERTS( XMBYTE4, XMLoadByte4 )

    case DXGI_FORMAT_R16G16_FLOAT:
        LOAD_VERTS( XMHALF2, XMLoadHalf2 )

    case DXGI_FORMAT_R16G16_UNORM:
        LOAD_VERTS( XMUSHORTN2, XMLoadUShortN2 )

    case DXGI_FORMAT_R16G16_UINT:
        LOAD_VERTS( XMUSHORT2, XMLoadUShort2 )

    case DXGI_FORMAT_R16G16_SNORM:
        LOAD_VERTS( XMSHORTN2, XMLoadShortN2 )

    case DXGI_FORMAT_R16G16_SINT:
        LOAD_VERTS( XMSHORT2, XMLoadShort2 )

    case DXGI_FORMAT_R32_FLOAT:
        LOAD_VERTS( float, XMLoadFloat )

    case DXGI_FORMAT_R32_UINT:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(uint32_t) ) > eptr )
                return E_UNEXPECTED;
            XMVECTOR v = XMLoadInt( reinterpret_cast<const uint32_t*>(ptr) );
            *buffer++ = XMConvertVectorUIntToFloat( v, 0 );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_R32_SINT:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(int32_t) ) > eptr )
                return E_UNEXPECTED;
            XMVECTOR v = XMLoadInt( reinterpret_cast<const uint32_t*>(ptr) );
            *buffer++ = XMConvertVectorIntToFloat( v, 0 );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_R8G8_UNORM:
        LOAD_VERTS( XMUBYTEN2, XMLoadUByteN2 )

    case DXGI_FORMAT_R8G8_UINT:
        LOAD_VERTS( XMUBYTE2, XMLoadUByte2 )

    case DXGI_FORMAT_R8G8_SNORM:
        LOAD_VERTS( XMBYTEN2, XMLoadByteN2 )

    case DXGI_FORMAT_R8G8_SINT:
        LOAD_VERTS( XMBYTE2, XMLoadByte2 )

    case DXGI_FORMAT_R16_FLOAT:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(HALF) ) > eptr )
                return E_UNEXPECTED;
            float v = XMConvertHalfToFloat( *reinterpret_cast<const HALF*>(ptr) );
            *buffer++ = XMVectorSet( v, 0.f, 0.f, 0.f );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_R16_UNORM:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(uint16_t) ) > eptr )
                return E_UNEXPECTED;
            auto v = *reinterpret_cast<const uint16_t*>(ptr);
            *buffer++ = XMVectorSet( static_cast<float>(v) / 65535.f, 0.f, 0.f, 0.f );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_R16_UINT:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(uint16_t) ) > eptr )
                return E_UNEXPECTED;
            auto v = *reinterpret_cast<const uint16_t*>(ptr);
            *buffer++ = XMVectorSet( static_cast<float>(v), 0.f, 0.f, 0.f );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_R16_SNORM:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(int16_t) ) > eptr )
                return E_UNEXPECTED;
            auto v = *reinterpret_cast<const int16_t*>(ptr);
            *buffer++ = XMVectorSet( static_cast<float>(v) / 32767.f, 0.f, 0.f, 0.f );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_R16_SINT:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(int16_t) ) > eptr )
                return E_UNEXPECTED;
            auto v = *reinterpret_cast<const int16_t*>(ptr);
               *buffer++ = XMVectorSet( static_cast<float>(v), 0.f, 0.f, 0.f );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_R8_UNORM:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(uint8_t) ) > eptr )
                return E_UNEXPECTED;
            auto v = *reinterpret_cast<const uint8_t*>(ptr);
            *buffer++ = XMVectorSet( static_cast<float>(v) / 255.f, 0.f, 0.f, 0.f );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_R8_UINT:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(uint8_t) ) > eptr )
                return E_UNEXPECTED;
            auto v = *reinterpret_cast<const uint8_t*>(ptr);
            *buffer++ = XMVectorSet( static_cast<float>(v), 0.f, 0.f, 0.f );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_R8_SNORM:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(int8_t) ) > eptr )
                return E_UNEXPECTED;
            auto v = *reinterpret_cast<const int8_t*>(ptr);
            *buffer++ = XMVectorSet( static_cast<float>(v) / 127.f, 0.f, 0.f, 0.f );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_R8_SINT:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(int8_t) ) > eptr )
                return E_UNEXPECTED;
            auto v = *reinterpret_cast<const int8_t*>(ptr);
               *buffer++ = XMVectorSet( static_cast<float>(v), 0.f, 0.f, 0.f );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_B5G6R5_UNORM:
        {
            static XMVECTORF32 s_Scale = { 1.f/31.f, 1.f/63.f, 1.f/31.f, 1.f };
            for( size_t icount = 0; icount < count; ++icount )
            {
                if ( ( ptr + sizeof(XMU565) ) > eptr )
                    return E_UNEXPECTED;
                XMVECTOR v = XMLoadU565( reinterpret_cast<const XMU565*>(ptr) );
                v = XMVectorMultiply( v, s_Scale );
                *buffer++ = XMVectorSwizzle<2, 1, 0, 3>( v );
                ptr += stride;
            }
        }
        break;

    case DXGI_FORMAT_B5G5R5A1_UNORM:
        {
            static XMVECTORF32 s_Scale = { 1.f/31.f, 1.f/31.f, 1.f/31.f, 1.f };
            for( size_t icount = 0; icount < count; ++icount )
            {
                if ( ( ptr + sizeof(XMU555) ) > eptr )
                    return E_UNEXPECTED;
                XMVECTOR v = XMLoadU555( reinterpret_cast<const XMU555*>(ptr) );
                v = XMVectorMultiply( v, s_Scale );
                *buffer++ = XMVectorSwizzle<2, 1, 0, 3>( v );
                ptr += stride;
            }
        }
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(XMUBYTEN4) ) > eptr )
                return E_UNEXPECTED;
            XMVECTOR v = XMLoadUByteN4( reinterpret_cast<const XMUBYTEN4*>(ptr) );
            *buffer++ = XMVectorSwizzle<2, 1, 0, 3>( v );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
        for( size_t icount = 0; icount < count; ++icount )
        {
            if ( ( ptr + sizeof(XMUBYTEN4) ) > eptr )
                return E_UNEXPECTED;
            XMVECTOR v = XMLoadUByteN4( reinterpret_cast<const XMUBYTEN4*>(ptr) );
            v = XMVectorSwizzle<2, 1, 0, 3>( v );
            *buffer++ = XMVectorSelect( g_XMZero, v, g_XMSelect1110 );
            ptr += stride;
        }
        break;

    case DXGI_FORMAT_B4G4R4A4_UNORM:
        {
            static XMVECTORF32 s_Scale = { 1.f/15.f, 1.f/15.f, 1.f/15.f, 1.f/15.f };
            for( size_t icount = 0; icount < count; ++icount )
            {
                if ( ( ptr + sizeof(XMUNIBBLE4) ) > eptr )
                    return E_UNEXPECTED;
                XMVECTOR v = XMLoadUNibble4( reinterpret_cast<const XMUNIBBLE4*>(ptr) );
                v = XMVectorMultiply( v, s_Scale );
                *buffer++ = XMVectorSwizzle<2, 1, 0, 3>( v );
                ptr += stride;
            }
        }
        break;

    default:
        return E_FAIL;
    }

    return S_OK;
}


//=====================================================================================
// Entry-points
//=====================================================================================

// Public constructor.
VBReader::VBReader()
  : pImpl( new Impl() )
{
}


// Move constructor.
VBReader::VBReader(VBReader&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
VBReader& VBReader::operator= (VBReader&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
VBReader::~VBReader()
{
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT VBReader::Initialize( const D3D11_INPUT_ELEMENT_DESC* vbDecl, size_t nDecl )
{
    return pImpl->Initialize( vbDecl, nDecl );
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT VBReader::AddStream( const void* vb, size_t nVerts, size_t inputSlot, size_t stride )
{
    return pImpl->AddStream( vb, nVerts, inputSlot, stride );
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT VBReader::Read( XMVECTOR* buffer, LPCSTR semanticName, UINT semanticIndex, size_t count ) const
{
    return pImpl->Read( buffer, semanticName, semanticIndex, count );
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT VBReader::Read( float* buffer, LPCSTR semanticName, UINT semanticIndex, size_t count ) const
{
    XMVECTOR* temp = pImpl->GetTemporaryBuffer( count );
    if ( !temp )
        return E_OUTOFMEMORY;

    HRESULT hr = pImpl->Read( temp, semanticName, semanticIndex, count );
    if ( FAILED(hr) )
        return hr;

    float* dptr = buffer;
    for( size_t j = 0; j < count; ++j )
    {
        XMVECTOR v = *(temp)++;
        XMStoreFloat( dptr++, v );
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT VBReader::Read( XMFLOAT2* buffer, LPCSTR semanticName, UINT semanticIndex, size_t count ) const
{
    XMVECTOR* temp = pImpl->GetTemporaryBuffer( count );
    if ( !temp )
        return E_OUTOFMEMORY;

    HRESULT hr = pImpl->Read( temp, semanticName, semanticIndex, count );
    if ( FAILED(hr) )
        return hr;

    XMFLOAT2* dptr = buffer;
    for( size_t j = 0; j < count; ++j )
    {
        XMVECTOR v = *(temp)++;
        XMStoreFloat2( dptr++, v );
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT VBReader::Read( XMFLOAT3* buffer, LPCSTR semanticName, UINT semanticIndex, size_t count ) const
{
    XMVECTOR* temp = pImpl->GetTemporaryBuffer( count );
    if ( !temp )
        return E_OUTOFMEMORY;

    HRESULT hr = pImpl->Read( temp, semanticName, semanticIndex, count );
    if ( FAILED(hr) )
        return hr;

    XMFLOAT3* dptr = buffer;
    for( size_t j = 0; j < count; ++j )
    {
        XMVECTOR v = *(temp)++;
        XMStoreFloat3( dptr++, v );
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT VBReader::Read( XMFLOAT4* buffer, LPCSTR semanticName, UINT semanticIndex, size_t count ) const
{
    XMVECTOR* temp = pImpl->GetTemporaryBuffer( count );
    if ( !temp )
        return E_OUTOFMEMORY;

    HRESULT hr = pImpl->Read( temp, semanticName, semanticIndex, count );
    if ( FAILED(hr) )
        return hr;

    XMFLOAT4* dptr = buffer;
    for( size_t j = 0; j < count; ++j )
    {
        XMVECTOR v = *(temp)++;
        XMStoreFloat4( dptr++, v );
    }

    return S_OK;
}


//-------------------------------------------------------------------------------------
void VBReader::Release()
{
    pImpl->Release();
}


//-------------------------------------------------------------------------------------
const D3D11_INPUT_ELEMENT_DESC* VBReader::GetElement( _In_z_ LPCSTR semanticName, _In_ UINT semanticIndex ) const
{
    return pImpl->GetElement( semanticName, semanticIndex );
}

} // namespace