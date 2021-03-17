//--------------------------------------------------------------------------------------
// File: MeshX.cpp
//
// Helper code for loading Mesh data from legacy .X files
// (Uses XFile COM interface in legacy D3DX9 DLL)
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
// http://go.microsoft.com/fwlink/?LinkID=512686
//--------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include "Mesh.h"

#include <map>
#include <new>

#include <objbase.h>
#include <d3d9.h>

#include <wrl/client.h>

#define D3DX9_DLL_W L"d3dx9_43.dll"

#include <rmxfguid.h>
#include <rmxftmpl.h>

// dxguid.lib in the Windows 10 SDK does not include the DXFILEOBJ_* GUIDs, so we instance them in this module
#undef DEFINE_GUID
#include <initguid.h>
#include <d3dx9xof.h>
#include <d3dx9mesh.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    template<class T> class Lock
    {
    public:
        explicit Lock(T* p = 0) noexcept : _pointer(p) {}

        Lock(const Lock&) = delete;
        Lock& operator=(const Lock&) = delete;

        Lock(Lock&&) = delete;
        Lock& operator=(Lock&&) = delete;

        ~Lock()
        {
            if (_pointer)
            {
                _pointer->Unlock();
                _pointer = nullptr;
            }
        }

    private:
        T* _pointer;
    };

    constexpr size_t MAX_TEXCOORD = 8;

    class ScratchMesh
    {
    public:
        ScratchMesh() noexcept : m_texcoordsCount{} {}
        ~ScratchMesh() { Release(); }

        ScratchMesh(ScratchMesh&&) = default;
        ScratchMesh& operator= (ScratchMesh&&) = default;

        ScratchMesh(ScratchMesh const&) = delete;
        ScratchMesh& operator= (ScratchMesh const&) = delete;

        void Release()
        {
            m_indices.clear();
            m_attributes.clear();
            m_positions.clear();
            m_normals.clear();
            m_colors.clear();
            m_tangents.clear();
            m_binormals.clear();
            m_bones.clear();
            m_weights.clear();

            for (size_t i = 0; i < MAX_TEXCOORD; ++i)
                m_texcoords[i].clear();

            memset(&m_texcoordsCount, 0, sizeof(m_texcoordsCount));

            m_materials.clear();
            m_vdups.clear();
        }

        // Methods
        HRESULT Merge(_In_ const ScratchMesh& mesh, _In_ CXMMATRIX xform)
        {
            if (!mesh.IsValid())
                return E_INVALIDARG;

            size_t nVerts = mesh.m_positions.size();
            size_t nFaces = mesh.m_indices.size() / 3;

            if ((uint64_t(m_positions.size()) + uint64_t(nVerts)) > UINT32_MAX)
                return E_FAIL;

            uint32_t vbase = static_cast<uint32_t>(m_positions.size());

            m_positions.reserve(m_positions.size() + nVerts);
            m_normals.reserve(m_normals.size() + nVerts);
            m_colors.reserve(m_colors.size() + nVerts);
            m_tangents.reserve(m_tangents.size() + nVerts);
            m_binormals.reserve(m_binormals.size() + nVerts);
            m_bones.reserve(m_bones.size() + nVerts);
            m_weights.reserve(m_weights.size() + nVerts);

            for (size_t tc = 0; tc < MAX_TEXCOORD; ++tc)
            {
                m_texcoords[tc].reserve(m_texcoords[tc].size() + nVerts);
            }

            for (size_t ivert = 0; ivert < nVerts; ++ivert)
            {
                XMVECTOR vp = XMLoadFloat3(&mesh.m_positions[ivert]);
                vp = XMVector3TransformCoord(vp, xform);
                XMFLOAT3 tmp3;
                XMStoreFloat3(&tmp3, vp);
                m_positions.push_back(tmp3);

                if (!mesh.m_normals.empty())
                {
                    XMVECTOR vn = XMLoadFloat3(&mesh.m_normals[ivert]);
                    vn = XMVector3TransformNormal(vn, xform);
                    XMStoreFloat3(&tmp3, vn);
                    m_normals.push_back(tmp3);
                }

                if (!mesh.m_colors.empty())
                {
                    m_colors.push_back(mesh.m_colors[ivert]);
                }

                if (!mesh.m_tangents.empty())
                {
                    XMVECTOR vn = XMLoadFloat3(&mesh.m_tangents[ivert]);
                    vn = XMVector3TransformNormal(vn, xform);
                    XMStoreFloat3(&tmp3, vn);
                    m_tangents.push_back(tmp3);
                }

                if (!mesh.m_binormals.empty())
                {
                    XMVECTOR vn = XMLoadFloat3(&mesh.m_binormals[ivert]);
                    vn = XMVector3TransformNormal(vn, xform);
                    XMStoreFloat3(&tmp3, vn);
                    m_binormals.push_back(tmp3);
                }

                if (!mesh.m_bones.empty())
                {
                    // TODO - How do we merge such that the bone indices are consisently pointing to the correct bone?
                    m_bones.push_back(mesh.m_bones[ivert]);
                }

                if (!mesh.m_weights.empty())
                {
                    m_weights.push_back(mesh.m_weights[ivert]);
                }

                for (size_t tc = 0; tc < MAX_TEXCOORD; ++tc)
                {
                    if (m_texcoords[tc].empty() || mesh.m_texcoords[tc].empty())
                        continue;

                    m_texcoords[tc].push_back(mesh.m_texcoords[tc][ivert]);
                    m_texcoordsCount[tc] = std::max(m_texcoordsCount[tc], mesh.m_texcoordsCount[tc]);
                }
            }

            m_indices.reserve(m_indices.size() + (nFaces * 3));
            if (!mesh.m_attributes.empty())
                m_attributes.reserve(m_attributes.size() + nFaces);

            uint32_t maxvert = static_cast<uint32_t>(vbase + nVerts);

            for (size_t iface = 0; iface < nFaces; ++iface)
            {
                uint32_t index0 = vbase + mesh.m_indices[iface * 3];
                uint32_t index1 = vbase + mesh.m_indices[iface * 3 + 1];
                uint32_t index2 = vbase + mesh.m_indices[iface * 3 + 2];

                if (index0 >= maxvert || index1 >= maxvert || index2 >= maxvert)
                    return E_FAIL;

                m_indices.push_back(index0);
                m_indices.push_back(index1);
                m_indices.push_back(index2);

                if (!mesh.m_attributes.empty())
                {
                    m_attributes.push_back(mesh.m_attributes[iface]);
                }
            }

            size_t abase = m_attributes.size();

            // Merge materials
            if (!mesh.m_materials.empty())
            {
                if ((uint64_t(m_materials.size()) + uint64_t(mesh.m_materials.size())) > UINT32_MAX)
                    return E_FAIL;

                auto mbase = static_cast<uint32_t>(m_materials.size());

                m_materials.reserve(m_materials.size() + mesh.m_materials.size());

                for (size_t i = 0; i < mesh.m_materials.size(); ++i)
                {
                    m_materials.push_back(mesh.m_materials[i]);
                }

                // Fix-up materal IDs in attributes from new mesh
                for (size_t ai = abase; ai < m_attributes.size(); ++ai)
                {
                    m_attributes[ai] += mbase;
                }
            }

            // Merge vertex duplication records
            for (auto it = mesh.m_vdups.cbegin(); it != mesh.m_vdups.cend(); ++it)
            {
                m_vdups.insert(std::make_pair<uint32_t, uint32_t>(it->first + vbase, it->second + vbase));
            }

            // Note: this style of merge means that duplicated vertices are not always at the end of the vertex array

            return S_OK;
        }

        void Transform(_In_ CXMMATRIX matrix)
        {
            if (m_positions.empty())
                return;

            XMVector3TransformCoordStream(m_positions.data(), sizeof(XMFLOAT3),
                m_positions.data(), sizeof(XMFLOAT3), m_positions.size(), matrix);

            if (!m_normals.empty())
            {
                XMVector3TransformNormalStream(m_normals.data(), sizeof(XMFLOAT3),
                    m_normals.data(), sizeof(XMFLOAT3), m_normals.size(), matrix);
            }

            if (!m_tangents.empty())
            {
                XMVector3TransformNormalStream(m_tangents.data(), sizeof(XMFLOAT3),
                    m_tangents.data(), sizeof(XMFLOAT3), m_tangents.size(), matrix);
            }

            if (!m_binormals.empty())
            {
                XMVector3TransformNormalStream(m_binormals.data(), sizeof(XMFLOAT3),
                    m_binormals.data(), sizeof(XMFLOAT3), m_binormals.size(), matrix);
            }
        }

        HRESULT SetColors(_In_ size_t nverts, _In_count_(nverts) const XMFLOAT4* colors)
        {
            if (m_positions.empty() || m_indices.empty())
                return E_FAIL;

            size_t overts = m_positions.size() - m_vdups.size();

            if (nverts != overts)
                return E_INVALIDARG;

            if (!colors)
                return E_POINTER;

            size_t maxverts = m_positions.size();

            m_colors.clear();
            m_colors.resize(maxverts, XMFLOAT4(0, 0, 0, 1.f));

            for (size_t i = 0; i < nverts; ++i)
            {
                m_colors[i] = colors[i];
            }

            // Set colors in any duplicated vertices
            for (auto it = m_vdups.cbegin(); it != m_vdups.cend(); ++it)
            {
                if (it->first < nverts)
                {
                    if (it->second >= maxverts)
                        return E_FAIL;

                    m_colors[it->second] = colors[it->first];
                }
            }

            return S_OK;
        }

        HRESULT SetTexCoords(_In_ size_t tset, _In_ size_t tcount, _In_ size_t nverts, _In_count_(nverts) const XMFLOAT4* coords)
        {
            if (m_positions.empty() || m_indices.empty())
                return E_FAIL;

            if (tset >= MAX_TEXCOORD)
                return E_INVALIDARG;

            if (tcount < 1 || tcount > 4)
                return E_INVALIDARG;

            size_t overts = m_positions.size() - m_vdups.size();

            if (nverts != overts)
                return E_INVALIDARG;

            if (!coords)
                return E_POINTER;

            size_t maxverts = m_positions.size();

            m_texcoords[tset].clear();
            m_texcoords[tset].resize(maxverts, XMFLOAT4(0, 0, 0, 0));

            for (size_t i = 0; i < nverts; ++i)
            {
                m_texcoords[tset][i] = coords[i];
            }

            m_texcoordsCount[tset] = static_cast<uint32_t>(tcount);

            // Set texture coordinates in any duplicated vertices
            for (auto it = m_vdups.cbegin(); it != m_vdups.cend(); ++it)
            {
                if (it->first < nverts)
                {
                    if (it->second >= maxverts)
                        return E_FAIL;

                    m_texcoords[tset][it->second] = coords[it->first];
                }
            }

            return S_OK;
        }

        HRESULT SetNormals(_In_ size_t nfaces, _In_count_x_(nfaces * 3) const XMFLOAT3* normals)
        {
            if (m_positions.empty() || m_indices.empty())
                return E_FAIL;

            if ((nfaces * 3) != m_indices.size())
                return E_INVALIDARG;

            if (!normals)
                return E_POINTER;

            m_normals.clear();
            m_normals.resize(m_positions.size(), XMFLOAT3(0, 0, 0));

            const XMFLOAT3* nptr = normals;
            for (size_t face = 0; face < nfaces; ++face)
            {
                for (size_t index = 0; index < 3; ++index)
                {
                    uint32_t i = m_indices[face * 3 + index];

                    if (i > m_positions.size())
                        return E_FAIL;

                    if (m_normals[i].x == 0 && m_normals[i].y == 0 && m_normals[i].z == 0)
                    {
                        // First reference to this vertex, so just set it
                        m_normals[i] = *nptr;

                        // Check for any previous vertex duplications that also have no normal information
                        std::pair< vdups_t::const_iterator, vdups_t::const_iterator > fixups = m_vdups.equal_range(i);
                        for (vdups_t::const_iterator it = fixups.first; it != fixups.second; ++it)
                        {
                            uint32_t ix = it->second;

                            if (m_normals[ix].x == 0 && m_normals[ix].y == 0 && m_normals[ix].z == 0)
                            {
                                m_normals[ix] = *nptr;
                            }
                        }
                    }
                    else
                    {
                        XMVECTOR v1 = XMLoadFloat3(&m_normals[i]);
                        XMVECTOR vn = XMLoadFloat3(nptr);

                        if (!XMVector3NearEqual(v1, vn, g_XMEpsilon))
                        {
                            // First see if another previously duplicated vertex has the same normal
                            std::pair< vdups_t::const_iterator, vdups_t::const_iterator > fixups = m_vdups.equal_range(i);
                            vdups_t::const_iterator it;
                            for (it = fixups.first; it != fixups.second; ++it)
                            {
                                XMVECTOR v2 = XMLoadFloat3(&m_normals[it->second]);

                                if (XMVector3NearEqual(v2, vn, g_XMEpsilon))
                                {
                                    // Use this one
                                    m_indices[face * 3 + index] = static_cast<uint32_t>(it->second);
                                    break;
                                }
                            }

                            if (it == fixups.second)
                            {
                                // No other suitable duplicate, so create a new vertex
                                size_t nvert;
                                HRESULT hr = DuplicateVert(i, nvert);
                                if (FAILED(hr))
                                    return hr;

                                m_normals[nvert] = *nptr;

                                m_indices[face * 3 + index] = static_cast<uint32_t>(nvert);
                            }
                        }
                    }

                    ++nptr;
                }
            }

            return S_OK;
        }


        HRESULT DuplicateVert(_In_ size_t vert, _Out_ size_t& nvert)
        {
            if (m_positions.empty() || m_indices.empty())
                return E_FAIL;

            size_t pos = m_positions.size();

            if (vert >= pos)
                return E_INVALIDARG;

            // Since indices only contains uint32_t, we can't add another
            if (pos >= UINT32_MAX)
                return E_OUTOFMEMORY;

            // Duplicate position
            m_positions.push_back(m_positions[vert]);

            nvert = pos;

            assert(nvert < m_positions.size());

            // Record duplication of vertex for future fix-ups
            m_vdups.insert(std::make_pair<uint32_t, uint32_t>(static_cast<uint32_t>(vert), static_cast<uint32_t>(pos)));

            // Duplicate auxiliary vertex data
            if (!m_normals.empty() && (m_normals.size() == pos))
            {
                m_normals.push_back(m_normals[vert]);
            }

            if (!m_colors.empty() && (m_colors.size() == pos))
            {
                m_colors.push_back(m_colors[vert]);
            }

            if (!m_tangents.empty() && (m_tangents.size() == pos))
            {
                m_tangents.push_back(m_tangents[vert]);
            }

            if (!m_binormals.empty() && (m_binormals.size() == pos))
            {
                m_binormals.push_back(m_binormals[vert]);
            }

            if (!m_bones.empty() && (m_bones.size() == pos))
            {
                m_bones.push_back(m_bones[vert]);
            }

            if (!m_weights.empty() && (m_weights.size() == pos))
            {
                m_weights.push_back(m_weights[vert]);
            }

            for (size_t tc = 0; tc < MAX_TEXCOORD; ++tc)
            {
                if (!m_texcoords[tc].empty() && (m_texcoords[tc].size() == pos))
                {
                    m_texcoords[tc].push_back(m_texcoords[tc][vert]);
                }
            }

            return S_OK;
        }

        void Reserve(_In_ size_t nverts, _In_ size_t nfaces)
        {
            m_indices.reserve(nfaces * 3);
            m_positions.reserve(nverts);
        }

        size_t AddVertex(_In_ const XMFLOAT3& v)
        {
            assert(m_vdups.empty());

            size_t ind = m_positions.size();
            m_positions.push_back(v);
            return ind;
        }

        void AddTriangle(_In_ uint32_t i0, _In_ uint32_t i1, _In_ uint32_t i2)
        {
            m_indices.push_back(i0);
            m_indices.push_back(i1);
            m_indices.push_back(i2);
        }

        // Accessors
        bool IsEmpty() const
        {
            return (m_indices.empty() || m_positions.empty()) != 0;
        }

        bool IsValid() const
        {
            if (m_indices.empty() || m_positions.empty())
                return false;

            // Only support triangle lists (not strips or fans)
            if ((m_indices.size() % 3) != 0)
                return false;

            size_t nfaces = m_indices.size() / 3;

            if (!m_attributes.empty() && (m_attributes.size() != nfaces))
                return false;

            size_t nverts = m_positions.size();

            // Since indices only contains uint32_t
            if (nverts > UINT32_MAX)
                return false;

            if (!m_normals.empty() && (m_normals.size() != nverts))
                return false;

            if (!m_colors.empty() && (m_colors.size() != nverts))
                return false;

            if (!m_tangents.empty() && (m_tangents.size() != nverts))
                return false;

            if (!m_binormals.empty() && (m_binormals.size() != nverts))
                return false;

            if (!m_bones.empty() && (m_bones.size() != nverts))
                return false;

            if (!m_weights.empty() && (m_weights.size() != nverts))
                return false;

            for (size_t i = 0; i < MAX_TEXCOORD; ++i)
            {
                if (!m_texcoords[i].empty() && (m_texcoords[i].size() != nverts))
                    return false;
            }

            // Validate vertex indices
            for (auto it : m_indices)
            {
                if (it >= nverts)
                    return false;
            }

            // TODO - validate bones index information

            // Validate materials (which are optional, but if present are indexed by attributes)
            if (!m_materials.empty())
            {
                size_t nmats = m_materials.size();

                // Since attributes only contains uint32_t
                if (nmats > UINT32_MAX)
                    return false;

                for (auto it = m_attributes.begin(); it != m_attributes.end(); ++it)
                {
                    if (*it >= nmats)
                        return false;
                }
            }

            // Validate vertex duplication records
            for (auto it = m_vdups.begin(); it != m_vdups.end(); ++it)
            {
                if (it->first >= nverts)
                    return false;

                if (it->second >= nverts)
                    return false;
            }

            return true;
        }

        const Mesh::Material* GetMaterials() const
        {
            return (m_materials.empty() ? nullptr : m_materials.data());
        }
        size_t GetMaterialCount() const
        {
            return m_materials.size();
        }

        HRESULT GetMesh(std::unique_ptr<Mesh>& mesh) const
        {
            if (!IsValid())
                return E_FAIL;

            mesh.reset(new (std::nothrow) Mesh);
            if (!mesh)
                return E_OUTOFMEMORY;

            HRESULT hr = mesh->SetIndexData(m_indices.size() / 3, m_indices.data(),
                m_attributes.empty() ? nullptr : m_attributes.data());
            if (FAILED(hr))
                return hr;

            if (m_texcoords[0].empty() || m_texcoordsCount[0] != 2)
            {
                hr = mesh->SetVertexData(m_positions.size(), m_positions.data(),
                    m_normals.empty() ? nullptr : m_normals.data(),
                    nullptr);

                // TODO - needs FVF or DeclData if m_texcoordsCount[0] != 2
            }
            else
            {
                std::unique_ptr<XMFLOAT2[]> temp(new (std::nothrow) XMFLOAT2[m_texcoords[0].size()]);
                if (!temp)
                    return E_OUTOFMEMORY;

                for (size_t j = 0; j < m_texcoords[0].size(); ++j)
                {
                    temp[j].x = m_texcoords[0][j].x;
                    temp[j].y = m_texcoords[0][j].y;
                }

                hr = mesh->SetVertexData(m_positions.size(), m_positions.data(),
                    m_normals.empty() ? nullptr : m_normals.data(),
                    temp.get());
            }
            if (FAILED(hr))
                return hr;

            return S_OK;
        }

    private:
        using indices_t = std::vector<uint32_t>;
        using attributes_t = std::vector<uint32_t>;

        using materials_t = std::vector<Mesh::Material>;
        using vdups_t = std::multimap<uint32_t, uint32_t>;

        using positions_t = std::vector<XMFLOAT3>;
        using normals_t = std::vector<XMFLOAT3>;
        using colors_t = std::vector<XMFLOAT4>;
        using tangents_t = std::vector<XMFLOAT3>;
        using binormals_t = std::vector<XMFLOAT3>;
        using bones_t = std::vector<PackedVector::XMUBYTE4>;
        using weights_t = std::vector<XMFLOAT4>;
        using texcoords_t = std::vector<XMFLOAT4>;

        indices_t       m_indices;
        attributes_t    m_attributes;
        positions_t     m_positions;
        normals_t       m_normals;
        colors_t        m_colors;
        tangents_t      m_tangents;
        binormals_t     m_binormals;
        bones_t         m_bones;
        weights_t       m_weights;
        texcoords_t     m_texcoords[MAX_TEXCOORD];
        uint32_t        m_texcoordsCount[MAX_TEXCOORD];

        materials_t     m_materials;
        vdups_t         m_vdups;
    };

    //-------------------------------------------------------------------------------------
    // Creates an X File reader object (using the legacy D3DX9 DLL if it can be found)
    //-------------------------------------------------------------------------------------
    HRESULT SetupXFile(_COM_Outptr_ ID3DXFile** ppXFile)
    {
        if (!ppXFile)
            return E_INVALIDARG;

        *ppXFile = nullptr;

        HMODULE hMod = LoadLibraryW(D3DX9_DLL_W);
        if (!hMod)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        typedef HRESULT(STDAPICALLTYPE* fpDirectXFile)(_Deref_out_ ID3DXFile** lplpDirectXFile);

        auto pfXFile = reinterpret_cast<fpDirectXFile>(static_cast<void*>(GetProcAddress(hMod, "D3DXFileCreate")));
        if (!pfXFile)
        {
            FreeLibrary(hMod);
            return E_NOINTERFACE;
        }

        ComPtr<ID3DXFile> pXFile;
        HRESULT hr = pfXFile(&pXFile);
        if (FAILED(hr))
            return hr;

        hr = pXFile->RegisterTemplates(reinterpret_cast<LPVOID>(D3DRM_XTEMPLATES), D3DRM_XTEMPLATE_BYTES);
        if (FAILED(hr))
            return hr;

        hr = pXFile->RegisterTemplates(reinterpret_cast<LPCVOID>(XSKINEXP_TEMPLATES), sizeof(XSKINEXP_TEMPLATES) - 1);
        if (FAILED(hr))
            return hr;

        hr = pXFile->RegisterTemplates(reinterpret_cast<LPCVOID>(XEXTENSIONS_TEMPLATES), sizeof(XEXTENSIONS_TEMPLATES) - 1);
        if (FAILED(hr))
            return hr;

        *ppXFile = pXFile.Detach();

        return S_OK;
    }


    //-------------------------------------------------------------------------------------
    // Parses out a single mesh from the X File data block
    //-------------------------------------------------------------------------------------
    HRESULT ParseMesh(_In_ LPD3DXFILEDATA pCurrent, _Inout_ ScratchMesh& mesh)
    {
        SIZE_T cbSize;
        uint8_t* pMeshData = nullptr;
        HRESULT hr = pCurrent->Lock(&cbSize, (LPCVOID*)&pMeshData);
        if (FAILED(hr))
            return hr;

        Lock<ID3DXFileData> unlock(pCurrent);

        if (cbSize < sizeof(DWORD))
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        const uint8_t* pEnd = pMeshData + cbSize;

        // First DWORD is count of verts
        size_t nVerts = *reinterpret_cast<DWORD*>(pMeshData);
        if (!nVerts)
            return E_FAIL;

        auto verts = reinterpret_cast<const XMFLOAT3*>(pMeshData + sizeof(DWORD));

        uint64_t cb = uint64_t(nVerts) * 3 * sizeof(float) + sizeof(DWORD);
        if ((cb + sizeof(DWORD)) > UINT32_MAX)
            return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

        // Verify we have enough space for those verts plus the face count
        if (cbSize < (cb + sizeof(DWORD)))
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        size_t nPolygons = *reinterpret_cast<DWORD*>(pMeshData + cb);
        if (!nPolygons)
            return E_FAIL;

        auto polys = reinterpret_cast<const DWORD*>(pMeshData + cb + sizeof(DWORD));

        cb += (uint64_t(nPolygons) * sizeof(DWORD)) + sizeof(DWORD);
        if (cb > UINT32_MAX)
            return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

        // Verify we have enough space for the polygons
        if (cbSize < cb)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        // Count up triangles
        size_t nTris = 0;

        const DWORD* pptr = polys;
        for (size_t iface = 0; iface < nPolygons; ++iface)
        {
            if (pptr + 1 > reinterpret_cast<const DWORD*>(pEnd))
                return E_FAIL;

            size_t ind = *pptr;
            if (ind < 3)
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

            nTris += (ind - 2);

            pptr += (ind + 1);
        }

        mesh.Reserve(nVerts, nTris);

        //--- Process vertices ---------------------------------------------------------
        const XMFLOAT3* vptr = verts;
        for (size_t ivert = 0; ivert < nVerts; ++ivert)
        {
            mesh.AddVertex(*vptr++);
        }

        //--- Process faces, convertings polys to tris ---------------------------------
        pptr = polys;
        size_t aTris = 0;
        for (size_t iface = 0; iface < nPolygons; ++iface)
        {
            size_t ind = *pptr;

            const DWORD* i = pptr + 1;

            DWORD i0 = i[0];
            if (i0 > nVerts)
                return E_FAIL;

            for (size_t index = 0; index < (ind - 2); ++index)
            {
                DWORD i1 = i[index + 1];
                DWORD i2 = i[index + 2];
                if (i1 > nVerts || i2 > nVerts)
                    return E_FAIL;

                mesh.AddTriangle(i0, i1, i2);
                ++aTris;
            }

            pptr += (ind + 1);
        }

        assert(nTris == aTris);

        size_t nOrigIndices = pptr - polys;

        SIZE_T nchildren = 0;
        hr = pCurrent->GetChildren(&nchildren);
        if (FAILED(hr))
            return hr;

        std::unique_ptr<XMFLOAT3[]> normals;
        for (size_t ichild = 0; ichild < nchildren; ++ichild)
        {
            ComPtr<ID3DXFileData> pChild;
            hr = pCurrent->GetChild(ichild, &pChild);
            if (FAILED(hr))
                return hr;

            GUID type;
            hr = pChild->GetType(&type);
            if (FAILED(hr))
                return hr;

            const uint8_t* dataPtr;
            hr = pChild->Lock(&cbSize, reinterpret_cast<LPCVOID*>(&dataPtr));
            if (FAILED(hr))
                return hr;

            Lock<ID3DXFileData> unlockc(pChild.Get());

            if (type == DXFILEOBJ_PMInfo)
            {
                // ProgressiveMesh data is not supported.
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
            else if (type == DXFILEOBJ_PatchMesh9)
            {
                // PatchMesh data is not supported.
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
            else if (type == TID_D3DRMMeshVertexColors)
            {
                //--- Load vertex colors -----------------------------------------------
#ifdef _DEBUG
                OutputDebugStringA("XFileMesh: found vertex colors in .X file\n");
#endif

                if (cbSize < sizeof(DWORD))
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                pptr = reinterpret_cast<const DWORD*>(dataPtr);
                size_t nColors = *pptr;

                uint64_t cb2 = sizeof(DWORD) + (sizeof(DWORD) * uint64_t(nColors)) + (sizeof(float) * 4 * uint64_t(nColors));
                if (cb2 > UINT32_MAX)
                    return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

                if (cbSize < cb2)
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                ++pptr;

                std::unique_ptr<XMFLOAT4[]> colors(new (std::nothrow) XMFLOAT4[nVerts]);
                if (!colors)
                    return E_OUTOFMEMORY;

                for (size_t i = 0; i < nVerts; ++i)
                {
                    colors[i] = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
                }

                for (size_t i = 0; i < nColors; ++i)
                {
                    DWORD index = *pptr;
                    ++pptr;

                    if (index > nVerts)
                        return E_FAIL;

                    auto clr = reinterpret_cast<const XMFLOAT4*>(pptr);
                    colors[index] = *clr;
                    pptr += 4;
                }

                hr = mesh.SetColors(nVerts, colors.get());
                if (FAILED(hr))
                    return hr;
            }
            else if (type == TID_D3DRMMeshNormals)
            {
                //--- Load normals -----------------------------------------------------
#ifdef _DEBUG
                OutputDebugStringA("XFileMesh: found normals in .X file\n");
#endif
                if (cbSize < sizeof(DWORD))
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                size_t nNormals = *reinterpret_cast<const DWORD*>(dataPtr);
                if (!nNormals)
                    return E_FAIL;

                uint64_t cb2 = sizeof(DWORD) + (uint64_t(nNormals) * sizeof(float) * 3);
                if ((cb2 + sizeof(DWORD)) > UINT32_MAX)
                    return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

                if (cbSize < (cb2 + sizeof(DWORD)))
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                size_t nFaceNormals = *reinterpret_cast<const DWORD*>(dataPtr + cb2);
                if (nPolygons != nFaceNormals)
                    return E_FAIL;

                cb2 += uint64_t(nOrigIndices) * sizeof(DWORD);
                if (cb2 > UINT32_MAX)
                    return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

                if (cbSize < cb2)
                    return E_FAIL;

                normals.reset(new (std::nothrow) XMFLOAT3[nTris * 3]);
                if (!normals)
                    return E_OUTOFMEMORY;

                auto n = reinterpret_cast<const XMFLOAT3*>(dataPtr + sizeof(DWORD));
                auto nptr = reinterpret_cast<const DWORD*>(dataPtr + 2 * sizeof(DWORD) + uint64_t(nNormals) * 3 * sizeof(float));
                const DWORD* mptr = polys;
                aTris = 0;
                for (size_t iface = 0; iface < nPolygons; ++iface)
                {
                    size_t ind = *nptr;
                    if ((ind < 3) || (ind != *mptr))
                        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                    const DWORD* ni = nptr + 1;

                    DWORD ni0 = ni[0];
                    if (ni0 > nNormals)
                        return E_FAIL;

                    for (size_t index = 0; index < (ind - 2); ++index)
                    {
                        DWORD ni1 = ni[index + 1];
                        DWORD ni2 = ni[index + 2];

                        if (ni1 > nNormals || ni2 > nNormals)
                            return E_FAIL;

                        normals[aTris * 3] = n[ni0];
                        normals[aTris * 3 + 1] = n[ni1];
                        normals[aTris * 3 + 2] = n[ni2];
                        ++aTris;
                    }

                    nptr += (ind + 1);
                    mptr += (ind + 1);
                }

                assert(nTris == aTris);
            }
            else if (type == TID_D3DRMMeshTextureCoords)
            {
                //--- Load (simple) texture coordinates --------------------------------
#ifdef _DEBUG
                OutputDebugStringA("XFileMesh: found 1 coords in .X file\n");
#endif

                if (cbSize < sizeof(DWORD))
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                pptr = reinterpret_cast<const DWORD*>(dataPtr);

                if (*pptr != nVerts)
                    return E_FAIL;

                ++pptr;

                uint64_t cb2 = (sizeof(DWORD) + (sizeof(float) * 2 * uint64_t(nVerts)));
                if (cb2 > UINT32_MAX)
                    return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

                if (cbSize < cb2)
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                std::unique_ptr<XMFLOAT4[]> coords(new (std::nothrow) XMFLOAT4[nVerts]);
                if (!coords)
                    return E_OUTOFMEMORY;

                for (DWORD i = 0; i < nVerts; ++i)
                {
                    auto tex = reinterpret_cast<const XMFLOAT2*>(pptr);
                    coords[i].x = tex->x;
                    coords[i].y = tex->y;
                    coords[i].z = coords[i].w = 0.f;
                    pptr += 2;
                }

                hr = mesh.SetTexCoords(0, 2, nVerts, coords.get());
                if (FAILED(hr))
                    return hr;
            }
            else if (type == TID_D3DRMMeshMaterialList)
            {
                //--- Load materials/attributes ----------------------------------------
#ifdef _DEBUG
                OutputDebugStringA("XFileMesh: found materials list in .X file\n");
#endif

                // TODO - load m_attributes and m_materials
            }
            else if (type == DXFILEOBJ_FVFData)
           {
                //--- Additional vertex data using FVF ---------------------------------
                if (cbSize < (sizeof(DWORD) * 2))
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                pptr = reinterpret_cast<const DWORD*>(dataPtr);

                DWORD fvf = *pptr++;
                if (!fvf)
                    return E_FAIL;

                DWORD ndw = *pptr++;
                if (!ndw)
                    return E_FAIL;

#ifdef _DEBUG
                char buff[128] = {};
                sprintf_s(buff, "XFileMesh: found FVF data in .X file: FVF %08X, bytes-per-vertex %zu\n", fvf, (ndw * sizeof(DWORD)) / nVerts);
                OutputDebugStringA(buff);
#endif

                uint64_t cb2 = sizeof(DWORD) * (uint64_t(ndw) + 2);
                if (cb2 > UINT32_MAX)
                    return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

                if (cbSize < cb2)
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                // TODO - Multiple and/or !float2 texcoords, weights, specular,
                return E_NOTIMPL;
            }
            else if (type == DXFILEOBJ_DeclData)
            {
                //--- Additional vertex data via Decl ----------------------------------
#ifdef _DEBUG
                OutputDebugStringA("XFileMesh: found DeclData data in .X file\n");
#endif
                // TODO - Multiple and/or !float2 texcoords, tangents, binormals, specular, weights, etc.
                return E_NOTIMPL;
            }
            else if (type == DXFILEOBJ_XSkinMeshHeader)
            {
                // TODO - DXFILEOBJ_XSkinMeshHeader

#ifdef _DEBUG
                OutputDebugStringA("XFileMesh: found SkinMeshHeader data in .X file\n");
#endif
            }
            else if (type == DXFILEOBJ_SkinWeights)
            {
                // TODO - DXFILEOBJ_SkinWeights
#ifdef _DEBUG
                OutputDebugStringA("XFileMesh: found SkinWeights data in .X file\n");
#endif
            }
#ifdef _DEBUG
            else if (type == DXFILEOBJ_VertexDuplicationIndices)
            {
                OutputDebugStringA("XFileMesh: found VertexDups data in .X file\n");
            }
            else if (type == DXFILEOBJ_FaceAdjacency)
            {
                OutputDebugStringA("XFileMesh: found FaceAdjacency data in .X file\n");
            }
            else if (type == DXFILEOBJ_VertexElement)
            {
                OutputDebugStringA("XFileMesh: found VertexElement data in .X file\n");
            }
            else
            {
                char buff[128] = {};
                sprintf_s(buff, "XFileMesh: found unknown GUID (%08lX-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X) data in .X file\n",
                    type.Data1, type.Data2, type.Data3,
                    type.Data4[0], type.Data4[1], type.Data4[2], type.Data4[3],
                    type.Data4[4], type.Data4[5], type.Data4[6], type.Data4[7]);
                OutputDebugStringA(buff);
            }
#endif

            if (FAILED(hr))
                return hr;
        }

#ifdef _DEBUG
        OutputDebugStringA("XFileMesh: eof\n");
#endif

        // Set face information last to ensure proper vertex duplication
        if (normals)
        {
            hr = mesh.SetNormals(nTris, normals.get());
            if (FAILED(hr))
                return hr;
        }

        return S_OK;
    }


    //-------------------------------------------------------------------------------------
    // Recursively load mesh data from an X file, merging each mesh together
    //-------------------------------------------------------------------------------------
    HRESULT LoadMeshesFromX(_In_ LPD3DXFILEDATA pCurrent, _Inout_ XMMATRIX& matrix, _Inout_ ScratchMesh& mesh)
    {
        if (!pCurrent)
            return E_POINTER;

        GUID type;
        HRESULT hr = pCurrent->GetType(&type);
        if (FAILED(hr))
            return hr;

        if (type == TID_D3DRMMesh)
        {
            if (mesh.IsEmpty())
            {
                hr = ParseMesh(pCurrent, mesh);
                if (FAILED(hr))
                    return hr;

                mesh.Transform(matrix);

                return S_OK;
            }
            else
            {
                ScratchMesh temp;
                hr = ParseMesh(pCurrent, temp);
                if (FAILED(hr))
                    return hr;

                return mesh.Merge(temp, matrix);
            }
        }
        else if (type == TID_D3DRMFrameTransformMatrix)
        {
            SIZE_T cbSize;
            XMFLOAT4X4* pMatrix = nullptr;

            hr = pCurrent->Lock(&cbSize, (LPCVOID*)&pMatrix);
            if (FAILED(hr))
                return hr;

            Lock<ID3DXFileData> unlock(pCurrent);
            if (cbSize < sizeof(XMFLOAT4X4))
            {
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            XMMATRIX nmatrix = XMLoadFloat4x4(pMatrix);
            matrix = XMMatrixMultiply(nmatrix, matrix);
        }
        else if (type == TID_D3DRMFrame)
        {
            XMMATRIX lmatrix = matrix;

            SIZE_T nchildren = 0;
            hr = pCurrent->GetChildren(&nchildren);
            if (FAILED(hr))
                return hr;

            for (size_t ichild = 0; ichild < nchildren; ++ichild)
            {
                ComPtr<ID3DXFileData> pChild;
                hr = pCurrent->GetChild(ichild, &pChild);
                if (FAILED(hr))
                    return hr;

                hr = LoadMeshesFromX(pChild.Get(), lmatrix, mesh);

                if (FAILED(hr))
                    return hr;
            }
        }
#ifdef _DEBUG
        else if (type == TID_D3DRMAnimationSet)
        {
            OutputDebugStringA("XFile: found AnimationSet data in.X file\n");
        }
        else
        {
            char buff[128] = {};
            sprintf_s(buff, "XFile: found unknown GUID (%08lX-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X) data in .X file\n",
                type.Data1, type.Data2, type.Data3,
                type.Data4[0], type.Data4[1], type.Data4[2], type.Data4[3],
                type.Data4[4], type.Data4[5], type.Data4[6], type.Data4[7]);
            OutputDebugStringA(buff);
        }
#endif

        return S_OK;
    }

    HRESULT LoadFromXFile(_In_ ID3DXFileEnumObject* pXFileEnum, _Inout_ ScratchMesh& mesh)
    {
        if (!pXFileEnum)
            return E_POINTER;

        SIZE_T nchildren = 0;
        HRESULT hr = pXFileEnum->GetChildren(&nchildren);
        if (FAILED(hr))
            return hr;

        for (size_t ichild = 0; ichild < nchildren; ++ichild)
        {
            ComPtr<ID3DXFileData> pChild;
            hr = pXFileEnum->GetChild(ichild, &pChild);
            if (FAILED(hr))
                return hr;

            XMMATRIX mat = XMMatrixIdentity();

            hr = LoadMeshesFromX(pChild.Get(), mat, mesh);
            if (FAILED(hr))
                return hr;
        }

        return S_OK;
    }
}

//--------------------------------------------------------------------------------------
HRESULT LoadFromX(
    const wchar_t* szFilename,
    std::unique_ptr<Mesh>& inMesh,
    std::vector<Mesh::Material>& inMaterial,
    bool ccw,
    bool dds)
{
    if (!szFilename)
        return E_INVALIDARG;

    ComPtr<ID3DXFile> xFile;
    HRESULT hr = SetupXFile(&xFile);
    if (FAILED(hr))
        return hr;

    ComPtr<ID3DXFileEnumObject> xfileEnum;
    hr = xFile->CreateEnumObject(szFilename, D3DXF_FILELOAD_FROMWFILE, &xfileEnum);
    if (FAILED(hr))
        return hr;

    ScratchMesh mesh;
    hr = LoadFromXFile(xfileEnum.Get(), mesh);
    if (FAILED(hr))
        return hr;

    return mesh.GetMesh(inMesh);
}
