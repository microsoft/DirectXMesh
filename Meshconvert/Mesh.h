//--------------------------------------------------------------------------------------
// File: Mesh.h
//
// Mesh processing helper class
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkID=324981
// http://go.microsoft.com/fwlink/?LinkID=512686
//--------------------------------------------------------------------------------------

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>

#include <d3d11_1.h>

#include <DirectXMath.h>

#include "DirectXMesh.h"

class Mesh
{
public:
    Mesh() noexcept
        : mnFaces(0),
          mnVerts(0)
    {}
    Mesh(Mesh&& moveFrom) noexcept;
    Mesh& operator=(Mesh&& moveFrom) noexcept;

    Mesh(Mesh const&)            = delete;
    Mesh& operator=(Mesh const&) = delete;

    // Methods
    void Clear() noexcept;

    void SetMTLFileName(std::wstring const& name) noexcept { mtlFileName = name; }

    HRESULT SetIndexData(_In_ size_t           nFaces,
        _In_reads_(nFaces * 3) const uint16_t* indices,
        _In_reads_opt_(nFaces) const uint32_t* attributes = nullptr) noexcept;
    HRESULT SetIndexData(_In_ size_t           nFaces,
        _In_reads_(nFaces * 3) const uint32_t* indices,
        _In_reads_opt_(nFaces) const uint32_t* attributes = nullptr) noexcept;

    HRESULT SetVertexData(DirectX::VBReader const& reader, _In_ size_t nVerts) noexcept;

    HRESULT Validate(_In_ DirectX::VALIDATE_FLAGS flags, _In_opt_ std::wstring* msgs) const noexcept;

    HRESULT Clean() noexcept;

    HRESULT GenerateAdjacency(_In_ float epsilon) noexcept;

    HRESULT ComputeNormals(_In_ DirectX::CNORM_FLAGS flags) noexcept;

    HRESULT ComputeTangentFrame(_In_ bool bitangents) noexcept;

    HRESULT Optimize(bool lru) noexcept;

    HRESULT ReverseWinding() noexcept;

    HRESULT InvertUTexCoord() noexcept;
    HRESULT InvertVTexCoord() noexcept;

    HRESULT ReverseHandedness() noexcept;

    // Accessors
    uint32_t const*          GetAttributeBuffer() const noexcept { return mAttributes.get(); }
    uint32_t const*          GetAdjacencyBuffer() const noexcept { return mAdjacency.get(); }
    DirectX::XMFLOAT3 const* GetPositionBuffer() const noexcept { return mPositions.get(); }
    DirectX::XMFLOAT3 const* GetNormalBuffer() const noexcept { return mNormals.get(); }
    DirectX::XMFLOAT2 const* GetTexCoordBuffer() const noexcept { return mTexCoords.get(); }
    DirectX::XMFLOAT4 const* GetTangentBuffer() const noexcept { return mTangents.get(); }
    DirectX::XMFLOAT4 const* GetColorBuffer() const noexcept { return mColors.get(); }

    size_t GetFaceCount() const noexcept { return mnFaces; }
    size_t GetVertexCount() const noexcept { return mnVerts; }

    bool Is16BitIndexBuffer() const noexcept;

    uint32_t const*             GetIndexBuffer() const noexcept { return mIndices.get(); }
    std::unique_ptr<uint16_t[]> GetIndexBuffer16() const noexcept;

    HRESULT GetVertexBuffer(DirectX::VBWriter const& writer) const noexcept;

    // Save mesh to file
    struct Material
    {
        std::wstring      name;
        bool              perVertexColor;
        float             specularPower;
        float             alpha;
        DirectX::XMFLOAT3 ambientColor;
        DirectX::XMFLOAT3 diffuseColor;
        DirectX::XMFLOAT3 specularColor;
        DirectX::XMFLOAT3 emissiveColor;
        std::wstring      texture;
        std::wstring      normalTexture;
        std::wstring      specularTexture;
        std::wstring      emissiveTexture;
        std::wstring      rmaTexture;

        Material() noexcept
            : perVertexColor(false),
              specularPower(1.f),
              alpha(1.f),
              ambientColor{},
              diffuseColor{},
              specularColor{},
              emissiveColor{}
        {}

        Material(wchar_t const*      iname,
            bool                     pvc,
            float                    power,
            float                    ialpha,
            DirectX::XMFLOAT3 const& ambient,
            DirectX::XMFLOAT3 const  diffuse,
            DirectX::XMFLOAT3 const& specular,
            DirectX::XMFLOAT3 const& emissive,
            wchar_t const*           txtname)
            : name(iname),
              perVertexColor(pvc),
              specularPower(power),
              alpha(ialpha),
              ambientColor(ambient),
              diffuseColor(diffuse),
              specularColor(specular),
              emissiveColor(emissive),
              texture(txtname)
        {}
    };

    HRESULT ExportToOBJ(wchar_t const* szFileName, _In_ size_t nMaterials, _In_reads_opt_(nMaterials) const Material* materials) const;
    HRESULT ExportToVBO(_In_z_ const wchar_t* szFileName) const noexcept;
    HRESULT ExportToCMO(_In_z_ const wchar_t*      szFileName,
        _In_ size_t                                nMaterials,
        _In_reads_opt_(nMaterials) const Material* materials) const noexcept;
    HRESULT ExportToSDKMESH(_In_z_ const wchar_t*  szFileName,
        _In_ size_t                                nMaterials,
        _In_reads_opt_(nMaterials) const Material* materials,
        bool                                       force32bit   = false,
        bool                                       version2     = false,
        DXGI_FORMAT                                normalFormat = DXGI_FORMAT_R32G32B32_FLOAT,
        DXGI_FORMAT                                uvFormat     = DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT                                colorFormat  = DXGI_FORMAT_B8G8R8A8_UNORM) const noexcept;

    // Create mesh from file
    static HRESULT CreateFromVBO(_In_z_ const wchar_t* szFileName, _Inout_ std::unique_ptr<Mesh>& result) noexcept;

private:
    size_t                               mnFaces;
    size_t                               mnVerts;
    std::unique_ptr<uint32_t[]>          mIndices;
    std::unique_ptr<uint32_t[]>          mAttributes;
    std::unique_ptr<uint32_t[]>          mAdjacency;
    std::unique_ptr<DirectX::XMFLOAT3[]> mPositions;
    std::unique_ptr<DirectX::XMFLOAT3[]> mNormals;
    std::unique_ptr<DirectX::XMFLOAT4[]> mTangents;
    std::unique_ptr<DirectX::XMFLOAT3[]> mBiTangents;
    std::unique_ptr<DirectX::XMFLOAT2[]> mTexCoords;
    std::unique_ptr<DirectX::XMFLOAT4[]> mColors;
    std::unique_ptr<DirectX::XMFLOAT4[]> mBlendIndices;
    std::unique_ptr<DirectX::XMFLOAT4[]> mBlendWeights;

    std::wstring mtlFileName;

    void ExportToOBJ(std::wostream& os, _In_ size_t nMaterials, _In_reads_opt_(nMaterials) const Material* materials) const;
};
