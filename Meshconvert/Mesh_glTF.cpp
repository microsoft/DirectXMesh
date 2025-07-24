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
#include "json.hpp"

#include <fstream>
#include <vector>

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

    HRESULT ParseJSON(const std::string& str)
    {
        using json = nlohmann::json;

        try
        {
            auto meta = json::parse(str.cbegin(), str.cend());

            auto asset = meta.find("asset");
            if (asset == meta.end())
                return E_FAIL;

            // TODO - Check asset.version == 2.0

            auto meshes = meta.find("meshes");
            if (meshes == meta.end())
                return E_FAIL;

            auto materials = meta.find("materials");
            if (materials == meta.end())
                return E_FAIL;

            auto buffers = meta.find("buffers");
            if (buffers == meta.end())
                return E_FAIL;

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
    std::vector<Mesh::Material>& inMaterial)
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
        if (!inFile)
            return E_FAIL;

        inFile.close();
    }

    HRESULT hr = ParseJSON(jsonData);

    // TODO - .bin file
    
    return hr;
}

HRESULT LoadFrom_glTFBinary(
    const wchar_t* szFilename,
    std::unique_ptr<Mesh>& inMesh,
    std::vector<Mesh::Material>& inMaterial)
{
    std::string jsonData;
    std::vector<uint8_t> binData;

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

        if (header.magic != 0x46546C67
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
                jsonData.resize(chunk.chunkLength);
                inFile.read(const_cast<char*>(jsonData.data()), chunk.chunkLength);
                if (!inFile)
                    return E_FAIL;
                break;

            case 0x004E4942: // .bin
                binData.resize(chunk.chunkLength);
                inFile.read(reinterpret_cast<char*>(binData.data()), chunk.chunkLength);
                if (!inFile)
                    return E_FAIL;
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

    if (jsonData.empty())
        return E_FAIL;

    HRESULT hr = ParseJSON(jsonData);

    // TODO - .glb contains both JSON and chunks together

    return hr;
}
