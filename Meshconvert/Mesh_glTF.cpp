//--------------------------------------------------------------------------------------
// File: Mesh_glTF.cpp
//
// Helper code for loading Mesh data from Khronos Group glTF 2.0
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
//--------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include "Mesh.h"

#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <wincrypt.h>

using namespace DirectX;

namespace
{
#pragma pack(push,1)

    struct glbHeader
    {
        uint32_t magic;
        uint32_t version;
        uint32_t length;
    };

    struct glbChunk
    {
        uint32_t chunkLength;
        uint32_t chunkType;
    };

#pragma pack(pop)

    using BinDataEntryType = std::vector<uint8_t>;
    using BinDataType = std::vector<BinDataEntryType>;

    using AccessorEntryType = std::pair<const uint8_t*, size_t>;
    using AccessorType = std::vector<AccessorEntryType>;

    const char c_mimeApplicationOS[] = "data:application/octet-stream;";
    const char c_mimeApplicationHASH64[] = "data:application/octet-stream;base64,";


    HRESULT ParseJSON(
        const std::string& str,
        BinDataType& binData,
        AccessorType& accessorData,
        const wchar_t* gltfPath,
        bool isGLB)
    {
        using json = nlohmann::json;

        try
        {
            auto meta = json::parse(str.cbegin(), str.cend());

            auto version = meta[ "asset" ][ "version" ];
            std::string value;
            version.get_to(value);
            if (strcmp(value.c_str(), "2.0") != 0)
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

            auto exts = meta[ "extensionsRequired" ];
            if (!exts.empty())
            {
                // TODO: allow KHR_materials_unlit, KHR_mesh_quantization
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }

            // Load all data buffers
            auto buffers = meta[ "buffers" ];
            if (!buffers.is_array() || buffers.empty())
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

            size_t bufferCount = 0;
            for(auto it = buffers.begin(); it != buffers.end(); ++it)
            {
                ++bufferCount;

                size_t length = it->value("byteLength", 0);
                if (!length)
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                std::string uri;
                if (it->contains("uri"))
                {
                    uri = it->value("uri", "");
                }

                if (uri.empty())
                {
                    if (!isGLB || (bufferCount != 1))
                        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                    if (binData.empty())
                        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                    if (binData[0].size() < length)
                        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }
                else if (strncmp(uri.c_str(), c_mimeApplicationOS, sizeof(c_mimeApplicationOS) - 1) == 0)
                {
                    if (!strncmp(uri.c_str(), c_mimeApplicationHASH64, sizeof(c_mimeApplicationHASH64) - 1) == 0)
                        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                    BinDataEntryType data;
                    data.resize(length);
                        // HASH64 encoded data is always longer than the original binary, so this is safe.
                    auto len  = static_cast<DWORD>(length);
                    if (!CryptStringToBinaryA(
                        &uri.c_str()[sizeof(c_mimeApplicationHASH64) - 1],
                        0,
                        CRYPT_STRING_BASE64,
                        data.data(),
                        &len,
                        nullptr,
                        nullptr))
                    {
                        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    }

                    if (length != len)
                        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                    if (binData.size() != (bufferCount - 1))
                        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                    data.resize(length);
                    binData.emplace_back(std::move(data));
                }
                else
                {
                    std::filesystem::path npath(gltfPath);
                    npath.append(uri);

                    std::ifstream inFile(npath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
                    if (!inFile)
                        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

                    std::streampos len = inFile.tellg();

                    if (len != length)
                        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

                    inFile.seekg(0, std::ios::beg);
                    if (!inFile)
                        return E_FAIL;

                    BinDataEntryType data;
                    data.resize(length);

                    inFile.read(reinterpret_cast<char*>(data.data()), len);
                    binData.emplace_back(std::move(data));
                }
            }

            // TODO - parse the JSON data
        }
        catch (json::exception& e)
        {
#ifdef _DEBUG
            OutputDebugStringA(e.what());
            OutputDebugStringA("\n");
#endif
            return E_FAIL;
        }

        return E_NOTIMPL;
    }
}

//--------------------------------------------------------------------------------------
// Entry-point
//--------------------------------------------------------------------------------------
HRESULT LoadFrom_glTF(
    const wchar_t* szFilename,
    std::unique_ptr<Mesh>& inMesh,
    std::vector<Mesh::Material>& inMaterial,
    const wchar_t* path)
{
    std::string jsonData;

    {
        std::ifstream inFile(szFilename, std::ios::in | std::ios::ate);
        if (!inFile)
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        std::streampos len = inFile.tellg();
        jsonData.resize(size_t(len));

        inFile.seekg(0, std::ios::beg);
        if (!inFile)
            return E_FAIL;

        inFile.read(const_cast<char*>(jsonData.data()), len);
        if (inFile.bad())
            return E_FAIL;

        inFile.close();
    }

    BinDataType binData;
    AccessorType accessorData;
    HRESULT hr = ParseJSON(jsonData, binData, accessorData, path, false);

    // TODO -

    return hr;
}

HRESULT LoadFrom_glTFBinary(
    const wchar_t* szFilename,
    std::unique_ptr<Mesh>& inMesh,
    std::vector<Mesh::Material>& inMaterial,
    const wchar_t* path)
{
    std::string jsonData;
    BinDataType binData;

    {
        std::ifstream inFile(szFilename, std::ios::in | std::ios::binary | std::ios::ate);
        if (!inFile)
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        std::streampos len = inFile.tellg();

        glbHeader header = {};
        if (len < sizeof(glbHeader))
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        inFile.seekg(0, std::ios::beg);
        if (!inFile)
            return E_FAIL;

        inFile.read(reinterpret_cast<char*>(&header), sizeof(glbHeader));
        if (!inFile)
            return E_FAIL;

        if (header.magic != 0x46546C67 // "glTF"
            || header.version != 2)
            return E_FAIL;

        if (header.length > len)
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

        uint32_t curLength = sizeof(glbHeader);

        while (curLength < header.length)
        {
            glbChunk chunk = {};
            if ((curLength + sizeof(glbChunk)) > header.length)
                return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

            inFile.read(reinterpret_cast<char*>(&chunk), sizeof(glbChunk));
            if (!inFile)
                return E_FAIL;

            curLength += sizeof(glbChunk);

            switch (chunk.chunkType)
            {
            case 0x4E4F534A: // .gltf
                if (jsonData.empty())
                {
                    jsonData.resize(chunk.chunkLength);
                    inFile.read(const_cast<char*>(jsonData.data()), chunk.chunkLength);
                    if (!inFile)
                        return E_FAIL;
                }
                else
                {
                    // Only one json chunk is valid.
                    return E_FAIL;
                }
                break;

            case 0x004E4942: // .bin
                if (binData.empty())
                {
                    BinDataEntryType data;
                    data.resize(chunk.chunkLength);
                    inFile.read(reinterpret_cast<char*>(data.data()), chunk.chunkLength);
                    if (!inFile)
                        return E_FAIL;
                    binData.emplace_back(std::move(data));
                }
                else
                {
                    // Only one bin chunk is valid.
                    return E_FAIL;
                }
                break;

            default:
                // Skip unknown chunk
                inFile.seekg(chunk.chunkLength, std::ios::cur);
                if (!inFile)
                    return E_FAIL;
                break;
            }

            curLength += chunk.chunkLength;
        }

        inFile.close();
    }

    if (jsonData.empty() || binData.empty())
        return E_FAIL;

    AccessorType accessorData;
    HRESULT hr = ParseJSON(jsonData, binData, accessorData, path, true);

    // TODO

    return hr;
}
