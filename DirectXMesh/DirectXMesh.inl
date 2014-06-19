//-------------------------------------------------------------------------------------
// DirectXMesh.inl
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

//=====================================================================================
// DXGI Format Utilities
//=====================================================================================
_Use_decl_annotations_
inline bool IsValidVB( DXGI_FORMAT fmt )
{
    return BytesPerElement( fmt ) != 0;
}

_Use_decl_annotations_
inline bool IsValidIB( DXGI_FORMAT fmt )
{
    return ( fmt == DXGI_FORMAT_R32_UINT || fmt == DXGI_FORMAT_R16_UINT ) != 0;
}
