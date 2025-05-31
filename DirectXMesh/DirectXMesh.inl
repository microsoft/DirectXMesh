//-------------------------------------------------------------------------------------
// DirectXMesh.inl
//
// DirectX Mesh Geometry Library
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
//-------------------------------------------------------------------------------------

#pragma once

//=====================================================================================
// Bitmask flags enumerator operators
//=====================================================================================
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#endif

DEFINE_ENUM_FLAG_OPERATORS(CNORM_FLAGS);
DEFINE_ENUM_FLAG_OPERATORS(VALIDATE_FLAGS);
DEFINE_ENUM_FLAG_OPERATORS(MESHLET_FLAGS);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

//=====================================================================================
// DXGI Format Utilities
//=====================================================================================
_Use_decl_annotations_
inline bool __cdecl IsValidVB(DXGI_FORMAT fmt) noexcept
{
    return BytesPerElement(fmt) != 0;
}

_Use_decl_annotations_
constexpr bool __cdecl IsValidIB(DXGI_FORMAT fmt) noexcept
{
    return (fmt == DXGI_FORMAT_R32_UINT || fmt == DXGI_FORMAT_R16_UINT) != 0;
}
