//--------------------------------------------------------------------------------------
// File: Meshconvert.cpp
//
// Meshconvert command-line tool (sample for DirectXMesh library)
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
//--------------------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4005)
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOMCX
#define NOSERVICE
#define NOHELP
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if __cplusplus < 201703L
#error Requires C++17 (and /Zc:__cplusplus with MSVC)
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <locale>
#include <memory>
#include <set>
#include <string>
#include <tuple>

#include "Mesh.h"

#define TOOL_VERSION DIRECTX_MESH_VERSION
#include "CmdLineHelpers.h"

using namespace Helpers;
using namespace DirectX;

namespace
{
    const wchar_t* g_ToolName = L"meshconvert";
    const wchar_t* g_Description = L"Microsoft (R) MeshConvert Command-line Tool [DirectXMesh]";

    enum OPTIONS : uint32_t
    {
        OPT_RECURSIVE = 1,
        OPT_TOPOLOGICAL_ADJ,
        OPT_GEOMETRIC_ADJ,
        OPT_NORMALS,
        OPT_WEIGHT_BY_AREA,
        OPT_WEIGHT_BY_EQUAL,
        OPT_TANGENTS,
        OPT_CTF,
        OPT_OPTIMIZE,
        OPT_OPTIMIZE_LRU,
        OPT_CLEAN,
        OPT_TOLOWER,
        OPT_CLOCKWISE,
        OPT_FORCE_32BIT_IB,
        OPT_OVERWRITE,
        OPT_NODDS,
        OPT_FLIP,
        OPT_FLIPU,
        OPT_FLIPV,
        OPT_FLIPZ,
        OPT_NOLOGO,
        OPT_FLAGS_MAX,
        OPT_FILETYPE,
        OPT_OUTPUTFILE,
        OPT_FILELIST,
        OPT_VERT_NORMAL_FORMAT,
        OPT_VERT_UV_FORMAT,
        OPT_VERT_COLOR_FORMAT,
        OPT_SDKMESH,
        OPT_SDKMESH_V2,
        OPT_CMO,
        OPT_VBO,
        OPT_WAVEFRONT_OBJ,
        OPT_VERSION,
        OPT_HELP,
    };

    static_assert(OPT_FLAGS_MAX <= 32, "dwOptions is a unsigned int bitfield");

    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    const SValue<uint32_t> g_pOptions[] =
    {
        { L"r",         OPT_RECURSIVE },
        { L"ta",        OPT_TOPOLOGICAL_ADJ },
        { L"ga",        OPT_GEOMETRIC_ADJ },
        { L"n",         OPT_NORMALS },
        { L"na",        OPT_WEIGHT_BY_AREA },
        { L"ne",        OPT_WEIGHT_BY_EQUAL },
        { L"t",         OPT_TANGENTS },
        { L"tb",        OPT_CTF },
        { L"op",        OPT_OPTIMIZE },
        { L"oplru",     OPT_OPTIMIZE_LRU },
        { L"c",         OPT_CLEAN },
        { L"o",         OPT_OUTPUTFILE },
        { L"l",         OPT_TOLOWER },
        { L"cw",        OPT_CLOCKWISE },
        { L"ib32",      OPT_FORCE_32BIT_IB },
        { L"y",         OPT_OVERWRITE },
        { L"ft",        OPT_FILETYPE },
        { L"nodds",     OPT_NODDS },
        { L"flip",      OPT_FLIP },
        { L"fn",        OPT_VERT_NORMAL_FORMAT },
        { L"fuv",       OPT_VERT_UV_FORMAT },
        { L"fc",        OPT_VERT_COLOR_FORMAT },
        { L"nologo",    OPT_NOLOGO },
        { L"flist",     OPT_FILELIST },

        // Legacy selection switches for file type (use -ft instead)
        { L"sdkmesh",   OPT_SDKMESH },
        { L"sdkmesh2",  OPT_SDKMESH_V2 },
        { L"cmo",       OPT_CMO },
        { L"vbo",       OPT_VBO },
        { L"wf",        OPT_WAVEFRONT_OBJ },

        // Deprecated options (recommend using new -- alternatives)
        { L"flipu",     OPT_FLIPU },
        { L"flipv",     OPT_FLIPV },
        { L"flipz",     OPT_FLIPZ },
        { nullptr,      0 }
    };

    const SValue<uint32_t> g_pOptionsLong[] =
    {
        { L"clean",                     OPT_CLEAN },
        { L"clockwise",                 OPT_CLOCKWISE },
        { L"color-format",              OPT_VERT_COLOR_FORMAT },
        { L"file-list",                 OPT_FILELIST },
        { L"file-type",                 OPT_FILETYPE },
        { L"flip-face-winding",         OPT_FLIP },
        { L"flip-u",                    OPT_FLIPU },
        { L"flip-v",                    OPT_FLIPV },
        { L"flip-z",                    OPT_FLIPZ },
        { L"geometric-adjacency",       OPT_GEOMETRIC_ADJ },
        { L"help",                      OPT_HELP },
        { L"index-buffer-32-bit",       OPT_FORCE_32BIT_IB },
        { L"normal-format",             OPT_VERT_NORMAL_FORMAT },
        { L"normals-by-angle",          OPT_NORMALS },
        { L"normals-by-area",           OPT_WEIGHT_BY_AREA },
        { L"normals-by-equal",          OPT_WEIGHT_BY_EQUAL },
        { L"optimize-lru",              OPT_OPTIMIZE_LRU },
        { L"optimize",                  OPT_OPTIMIZE },
        { L"overwrite",                 OPT_OVERWRITE },
        { L"tangent-frame",             OPT_CTF },
        { L"tangents",                  OPT_TANGENTS },
        { L"to-lowercase",              OPT_TOLOWER },
        { L"topological-adjacency",     OPT_TOPOLOGICAL_ADJ },
        { L"uv-format",                 OPT_VERT_UV_FORMAT },
        { L"version",                   OPT_VERSION },
        { nullptr,                      0 }
    };

    const SValue<DXGI_FORMAT> g_vertexNormalFormats[] =
    {
        { L"float3",    DXGI_FORMAT_R32G32B32_FLOAT },
        { L"float16_4", DXGI_FORMAT_R16G16B16A16_FLOAT },
        { L"r11g11b10", DXGI_FORMAT_R11G11B10_FLOAT },
        { nullptr,      DXGI_FORMAT_UNKNOWN }
    };

    const SValue<DXGI_FORMAT> g_vertexUVFormats[] =
    {
        { L"float2",    DXGI_FORMAT_R32G32_FLOAT },
        { L"float16_2", DXGI_FORMAT_R16G16_FLOAT },
        { nullptr,      DXGI_FORMAT_UNKNOWN }
    };

    const SValue<DXGI_FORMAT> g_vertexColorFormats[] =
    {
        { L"bgra",      DXGI_FORMAT_B8G8R8A8_UNORM },
        { L"rgba",      DXGI_FORMAT_R8G8B8A8_UNORM },
        { L"float4",    DXGI_FORMAT_R32G32B32A32_FLOAT },
        { L"float16_4", DXGI_FORMAT_R16G16B16A16_FLOAT },
        { L"rgba_10",   DXGI_FORMAT_R10G10B10A2_UNORM },
        { L"r11g11b10", DXGI_FORMAT_R11G11B10_FLOAT },
        { nullptr,      DXGI_FORMAT_UNKNOWN }
    };

    enum MESH_CODEC : uint32_t
    {
        CODEC_SDKMESH = 1,
        CODEC_SDKMESH_V2,
        CODEC_CMO,
        CODEC_VBO,
        CODEC_WAVEFRONT_OBJ,
    };

    const SValue<uint32_t> g_pMeshFileTypes[] = // valid formats to write to
    {
        { L"sdkmesh",   CODEC_SDKMESH },
        { L"sdkmesh2",  CODEC_SDKMESH_V2 },
        { L"cmo",       CODEC_CMO },
        { L"vbo",       CODEC_VBO },
        { L"obj",       CODEC_WAVEFRONT_OBJ },
        { L"_obj",      CODEC_WAVEFRONT_OBJ },
        { nullptr,      0 }
    };
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

HRESULT LoadFromOBJ(const wchar_t* szFilename,
    std::unique_ptr<Mesh>& inMesh, std::vector<Mesh::Material>& inMaterial,
    bool ccw, bool dds);

HRESULT LoadFrom_glTF(const wchar_t* szFilename,
    std::unique_ptr<Mesh>& inMesh,
    std::vector<Mesh::Material>& inMaterial);
HRESULT LoadFrom_glTFBinary(const wchar_t* szFilename,
    std::unique_ptr<Mesh>& inMesh,
    std::vector<Mesh::Material>& inMaterial);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

namespace
{
    void PrintUsage()
    {
        PrintLogo(false, g_ToolName, g_Description);

        static const wchar_t* const s_usage =
            L"Usage: meshconvert <options> [--] <files>\n"
            L"\n"
            L"   Input file type must be Wavefront Object (.obj)\n"
            L"\n"
            L"   -ft <filetype>, --file-type <filetype>  output file type\n"
            L"       sdkmesh:  DirectX SDK .sdkmesh format (default)\n"
            L"       sdkmesh2: sdkmesh format version 2 (PBR materials)\n"
            L"       cmo:      Visual Studio Content Pipeline .cmo format\n"
            L"       vbo:      Vertex Buffer Object (.vbo) format\n"
            L"       obj:      WaveFront Object (.obj) format\n"
            L"\n"
            L"   -r                  wildcard filename search is recursive\n"
            L"   -flist <filename>, --file-list <filename>\n"
            L"                       use text file with a list of input files (one per line)\n"
            L"\n"
            L"   -n, --normal-by-angle   -na, --normal-by-area   -ne, --normal-by-equal\n"
            L"                                  generate normals weighted by angle/area/equal\n"
            L"   -t, --tangents                 generate tangents\n"
            L"   -tb, --tangent-frame           generate tangents & bi-tangents\n"
            L"   -cw, --clockwise               faces are clockwise (defaults to counter-clockwise)\n"
            L"\n"
            L"   -op, --optimize   -oplru, --optimize-lru\n"
            L"                                  vertex cache optimize the mesh (implies -c)\n"
            L"   -c, --clean                    mesh cleaning including vertex dups for attribute sets\n"
            L"   -ta, --topological-adjacency -or- -ga, --geometric-adjacency\n"
            L"                                  generate topological vs. geometric adjacency (def: ta)\n"
            L"\n"
            L"   -nodds                         prevents extension renaming in exported materials\n"
            L"   -flip, --flip-face-winding     reverse winding of faces\n"
            L"   --flip-u                       inverts the u texcoords\n"
            L"   --flip-v                       inverts the v texcoords\n"
            L"   --flip-z                       flips the handedness of the positions/normals\n"
            L"   -o <filename>                  output filename\n"
            L"   -l, --to-lowercase             force output filename to lower case\n"
            L"   -y, --overwrite                overwrite existing output file (if any)\n"
            L"   -nologo                        suppress copyright message\n"
            L"\n"
            L"       (sdkmesh/sdkmesh2 only)\n"
            L"   -ib32, --index-buffer-32-bit   use 32-bit index buffer\n"
            L"   -fn <normal-format>, --normal-format <normal-format>\n"
            L"                                  format to use for writing normals/tangents/binormals\n"
            L"   -fuv <uv-format>, --uv-format <uv-format>\n"
            L"                                  format to use for texture coordinates\n"
            L"   -fc <color-format>, --color-format <color-format>\n"
            L"                                  format to use for writing colors\n"
            L"\n"
            L"   '-- ' is needed if any input filepath starts with the '-' or '/' character\n";

        wprintf(L"%ls", s_usage);

        wprintf(L"\n   <normal-format>: ");
        PrintList(13, g_vertexNormalFormats);

        wprintf(L"\n   <uv-format>: ");
        PrintList(13, g_vertexUVFormats);

        wprintf(L"\n   <color-format>: ");
        PrintList(13, g_vertexColorFormats);
    }
}

//--------------------------------------------------------------------------------------
// Entry-point
//--------------------------------------------------------------------------------------
#ifdef __PREFAST__
#pragma prefast(disable : 28198, "Command-line tool, frees all memory on exit")
#endif

int __cdecl wmain(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
    // Parameters and defaults
    DXGI_FORMAT normalFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    DXGI_FORMAT uvFormat = DXGI_FORMAT_R32G32_FLOAT;
    DXGI_FORMAT colorFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    uint32_t fileType = 0;

    std::wstring outputFile;

    // Set locale for output since GetErrorDesc can get localized strings.
    std::locale::global(std::locale(""));

    // Process command line
    uint32_t dwOptions = 0;
    std::list<SConversion> conversion;
    bool allowOpts = true;

    for (int iArg = 1; iArg < argc; iArg++)
    {
        PWSTR pArg = argv[iArg];

        if (allowOpts && (('-' == pArg[0]) || ('/' == pArg[0])))
        {
            uint32_t dwOption = 0;
            PWSTR pValue = nullptr;

            if (('-' == pArg[0]) && ('-' == pArg[1]))
            {
                if (pArg[2] == 0)
                {
                    // "-- " is the POSIX standard for "end of options" marking to escape the '-' and '/' characters at the start of filepaths.
                    allowOpts = false;
                    continue;
                }
                else
                {
                    pArg += 2;

                    for (pValue = pArg; *pValue && (':' != *pValue) && ('=' != *pValue); ++pValue);

                    if (*pValue)
                        *pValue++ = 0;

                    dwOption = LookupByName(pArg, g_pOptionsLong);
                }
            }
            else
            {
                pArg++;

                for (pValue = pArg; *pValue && (':' != *pValue) && ('=' != *pValue); ++pValue);

                if (*pValue)
                    *pValue++ = 0;

                dwOption = LookupByName(pArg, g_pOptions);

                if (!dwOption)
                {
                    if (LookupByName(pArg, g_pOptionsLong))
                    {
                        wprintf(L"ERROR: did you mean `--%ls` (with two dashes)?\n", pArg);
                        return 1;
                    }
                }
            }

            switch (dwOption)
            {
            case 0:
                wprintf(L"ERROR: Unknown option: `%ls`\n\nUse %ls --help\n", pArg, g_ToolName);
                return 1;

            case OPT_FILETYPE:
            case OPT_OUTPUTFILE:
            case OPT_FILELIST:
            case OPT_VERT_NORMAL_FORMAT:
            case OPT_VERT_UV_FORMAT:
            case OPT_VERT_COLOR_FORMAT:
            case OPT_SDKMESH:
            case OPT_SDKMESH_V2:
            case OPT_CMO:
            case OPT_VBO:
            case OPT_WAVEFRONT_OBJ:
                // These don't use flag bits
                break;

            case OPT_VERSION:
                PrintLogo(true, g_ToolName, g_Description);
                return 0;

            case OPT_HELP:
                PrintUsage();
                return 0;

            default:
                if (dwOptions & (UINT32_C(1) << dwOption))
                {
                    wprintf(L"ERROR: Duplicate option: `%ls`\n\n", pArg);
                    return 1;
                }

                dwOptions |= (UINT32_C(1) << dwOption);
                break;
            }

            // Handle options with additional value parameter
            switch (dwOption)
            {
            case OPT_OUTPUTFILE:
            case OPT_VERT_NORMAL_FORMAT:
            case OPT_VERT_UV_FORMAT:
            case OPT_VERT_COLOR_FORMAT:
            case OPT_FILELIST:
            case OPT_FILETYPE:
                if (!*pValue)
                {
                    if ((iArg + 1 >= argc))
                    {
                        wprintf(L"ERROR: missing value for command-line option '%ls'\n\n", pArg);
                        PrintUsage();
                        return 1;
                    }

                    iArg++;
                    pValue = argv[iArg];
                }
                break;

            default:
                break;
            }

            switch (dwOption)
            {
            case OPT_OPTIMIZE_LRU:
                dwOptions |= (UINT32_C(1) << OPT_OPTIMIZE);
                break;

            case OPT_WEIGHT_BY_AREA:
                if (dwOptions & (UINT32_C(1) << OPT_WEIGHT_BY_EQUAL))
                {
                    wprintf(L"Cannot use both na and ne at the same time\n");
                    return 1;
                }
                dwOptions |= (UINT32_C(1) << OPT_NORMALS);
                break;

            case OPT_WEIGHT_BY_EQUAL:
                if (dwOptions & (UINT32_C(1) << OPT_WEIGHT_BY_AREA))
                {
                    wprintf(L"Cannot use both na and ne at the same time\n");
                    return 1;
                }
                dwOptions |= (UINT32_C(1) << OPT_NORMALS);
                break;

            case OPT_OUTPUTFILE:
                {
                    std::filesystem::path path(pValue);
                    outputFile = path.make_preferred().native();
                }
                break;

            case OPT_FILETYPE:
                fileType = LookupByName(pValue, g_pMeshFileTypes);
                if (!fileType)
                {
                    wprintf(L"Invalid value specified with -ft (%ls)\n\n", pValue);
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_TOPOLOGICAL_ADJ:
                if (dwOptions & (UINT32_C(1) << OPT_GEOMETRIC_ADJ))
                {
                    wprintf(L"Cannot use both ta and ga at the same time\n");
                    return 1;
                }
                break;

            case OPT_GEOMETRIC_ADJ:
                if (dwOptions & (UINT32_C(1) << OPT_TOPOLOGICAL_ADJ))
                {
                    wprintf(L"Cannot use both ta and ga at the same time\n");
                    return 1;
                }
                break;

            case OPT_SDKMESH:
                if (fileType != 0 && fileType != CODEC_SDKMESH)
                {
                    wprintf(L"Can only use one of sdkmesh, cmo, vbo, or wf\n");
                    return 1;
                }
                fileType = CODEC_SDKMESH;
                break;

            case OPT_SDKMESH_V2:
                if (fileType != 0 && fileType != CODEC_SDKMESH && fileType != CODEC_SDKMESH_V2)
                {
                    wprintf(L"-sdkmesh2 requires sdkmesh\n");
                    return 1;
                }
                fileType = CODEC_SDKMESH_V2;
                break;

            case OPT_CMO:
                if (fileType != 0 && fileType != CODEC_CMO)
                {
                    wprintf(L"Can only use one of sdkmesh, cmo, vbo, or wf\n");
                    return 1;
                }
                fileType = CODEC_CMO;
                break;

            case OPT_VBO:
                if (fileType != 0 && fileType != CODEC_VBO)
                {
                    wprintf(L"Can only use one of sdkmesh, cmo, vbo, or wf\n");
                    return 1;
                }
                fileType = CODEC_VBO;
                break;

            case OPT_WAVEFRONT_OBJ:
                if (fileType != 0 && fileType != CODEC_WAVEFRONT_OBJ)
                {
                    wprintf(L"Can only use one of sdkmesh, cmo, vbo, or wf\n");
                    return 1;
                }
                fileType = CODEC_WAVEFRONT_OBJ;
                break;

            case OPT_VERT_NORMAL_FORMAT:
                normalFormat = LookupByName(pValue, g_vertexNormalFormats);
                if (!normalFormat)
                {
                    wprintf(L"Invalid value specified with -fn (%ls)\n\n", pValue);
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_VERT_UV_FORMAT:
                uvFormat = LookupByName(pValue, g_vertexUVFormats);
                if (!uvFormat)
                {
                    wprintf(L"Invalid value specified with -fuv (%ls)\n\n", pValue);
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_VERT_COLOR_FORMAT:
                colorFormat = LookupByName(pValue, g_vertexColorFormats);
                if (!colorFormat)
                {
                    wprintf(L"Invalid value specified with -fc (%ls)\n\n", pValue);
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_FILELIST:
                {
                    std::filesystem::path path(pValue);
                    std::wifstream inFile(path.make_preferred().c_str());
                    if (!inFile)
                    {
                        wprintf(L"Error opening -flist file %ls\n", pValue);
                        return 1;
                    }

                    inFile.imbue(std::locale::classic());

                    ProcessFileList(inFile, conversion);
                }
                break;

            default:
                break;
            }
        }
        else if (wcspbrk(pArg, L"?*") != nullptr)
        {
            const size_t count = conversion.size();
            std::filesystem::path path(pArg);
            SearchForFiles(path.make_preferred(), conversion, (dwOptions & (UINT32_C(1) << OPT_RECURSIVE)) != 0, nullptr);
            if (conversion.size() <= count)
            {
                wprintf(L"No matching files found for %ls\n", pArg);
                return 1;
            }
        }
        else
        {
            SConversion conv = {};
            std::filesystem::path path(pArg);
            conv.szSrc = path.make_preferred().native();
            conversion.push_back(conv);
        }
    }

    if (conversion.empty())
    {
        PrintUsage();
        return 0;
    }

    if (!outputFile.empty() && conversion.size() > 1)
    {
        wprintf(L"Cannot use -o with multiple input files\n");
        return 1;
    }

    if (~dwOptions & (UINT32_C(1) << OPT_NOLOGO))
        PrintLogo(false, g_ToolName, g_Description);

    if (!fileType)
        fileType = CODEC_SDKMESH;

    // Process files
    for (auto pConv = conversion.begin(); pConv != conversion.end(); ++pConv)
    {
        std::filesystem::path curpath(pConv->szSrc);
        const auto ext = curpath.extension();

        if (pConv != conversion.begin())
            wprintf(L"\n");

        wprintf(L"reading %ls", curpath.c_str());
        fflush(stdout);

        std::unique_ptr<Mesh> inMesh;
        std::vector<Mesh::Material> inMaterial;
        HRESULT hr = E_NOTIMPL;
        if (_wcsicmp(ext.c_str(), L".vbo") == 0)
        {
            hr = Mesh::CreateFromVBO(curpath.c_str(), inMesh);
        }
        else if (_wcsicmp(ext.c_str(), L".sdkmesh") == 0)
        {
            wprintf(L"\nERROR: Importing SDKMESH files not supported\n");
            return 1;
        }
        else if (_wcsicmp(ext.c_str(), L".cmo") == 0)
        {
            wprintf(L"\nERROR: Importing Visual Studio CMO files not supported\n");
            return 1;
        }
        else if (_wcsicmp(ext.c_str(), L".x") == 0)
        {
            wprintf(L"\nERROR: Legacy Microsoft X files not supported\n");
            return 1;
        }
        else if (_wcsicmp(ext.c_str(), L".fbx") == 0)
        {
            wprintf(L"\nERROR: Autodesk FBX files not supported\n");
            return 1;
        }
        else if (_wcsicmp(ext, L".gltf") == 0)
        {
            hr = LoadFrom_glTF(pConv->szSrc, inMesh, inMaterial);
        }
        else if (_wcsicmp(ext, L".glb") == 0)
        {
            hr = LoadFrom_glTFBinary(pConv->szSrc, inMesh, inMaterial);
        }
        else
        {
            hr = LoadFromOBJ(curpath.c_str(), inMesh, inMaterial,
                (dwOptions & (UINT32_C(1) << OPT_CLOCKWISE)) ? false : true,
                (dwOptions & (UINT32_C(1) << OPT_NODDS)) ? false : true);
        }
        if (FAILED(hr))
        {
            wprintf(L" FAILED (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
            return 1;
        }

        size_t nVerts = inMesh->GetVertexCount();
        const size_t nFaces = inMesh->GetFaceCount();

        if (!nVerts || !nFaces)
        {
            wprintf(L"\nERROR: Invalid mesh\n");
            return 1;
        }

        assert(inMesh->GetPositionBuffer() != nullptr);
        assert(inMesh->GetIndexBuffer() != nullptr);

        wprintf(L"\n%zu vertices, %zu faces", nVerts, nFaces);

        if (dwOptions & (UINT32_C(1) << OPT_FLIPU))
        {
            hr = inMesh->InvertUTexCoord();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed inverting u texcoord (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
        }

        if (dwOptions & (UINT32_C(1) << OPT_FLIPV))
        {
            hr = inMesh->InvertVTexCoord();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed inverting v texcoord (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
        }

        if (dwOptions & (UINT32_C(1) << OPT_FLIPZ))
        {
            hr = inMesh->ReverseHandedness();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed reversing handedness (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
        }

        // Prepare mesh for processing
        if (dwOptions & ((UINT32_C(1) << OPT_OPTIMIZE) | (UINT32_C(1) << OPT_CLEAN)))
        {
            // Adjacency
            const float epsilon = (dwOptions & (UINT32_C(1) << OPT_GEOMETRIC_ADJ)) ? 1e-5f : 0.f;

            hr = inMesh->GenerateAdjacency(epsilon);
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed generating adjacency (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }

            // Validation
            std::wstring msgs;
            hr = inMesh->Validate(VALIDATE_BACKFACING, &msgs);
            if (!msgs.empty())
            {
                wprintf(L"\nWARNING: \n");
                wprintf(L"%ls", msgs.c_str());
            }

            // Clean (also handles attribute reuse split if needed)
            hr = inMesh->Clean();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed mesh clean (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
            else
            {
                const size_t nNewVerts = inMesh->GetVertexCount();
                if (nVerts != nNewVerts)
                {
                    wprintf(L" [%zu vertex dups] ", nNewVerts - nVerts);
                    nVerts = nNewVerts;
                }
            }
        }

        if (!inMesh->GetNormalBuffer())
        {
            dwOptions |= UINT32_C(1) << OPT_NORMALS;
        }

        if (!inMesh->GetTangentBuffer() && (fileType == CODEC_CMO))
        {
            dwOptions |= UINT32_C(1) << OPT_TANGENTS;
        }

        // Compute vertex normals from faces
        if ((dwOptions & (UINT32_C(1) << OPT_NORMALS))
            || ((dwOptions & ((UINT32_C(1) << OPT_TANGENTS) | (UINT32_C(1) << OPT_CTF))) && !inMesh->GetNormalBuffer()))
        {
            CNORM_FLAGS flags = CNORM_DEFAULT;

            if (dwOptions & (UINT32_C(1) << OPT_WEIGHT_BY_EQUAL))
            {
                flags |= CNORM_WEIGHT_EQUAL;
            }
            else if (dwOptions & (UINT32_C(1) << OPT_WEIGHT_BY_AREA))
            {
                flags |= CNORM_WEIGHT_BY_AREA;
            }

            if (dwOptions & (UINT32_C(1) << OPT_CLOCKWISE))
            {
                flags |= CNORM_WIND_CW;
            }

            hr = inMesh->ComputeNormals(flags);
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed computing normals (flags:%X, %08X%ls)\n",
                    static_cast<unsigned int>(flags),
                    static_cast<unsigned int>(hr),
                    GetErrorDesc(hr));
                return 1;
            }
        }

        // Compute tangents and bitangents
        if (dwOptions & ((UINT32_C(1) << OPT_TANGENTS) | (UINT32_C(1) << OPT_CTF)))
        {
            if (!inMesh->GetTexCoordBuffer())
            {
                wprintf(L"\nERROR: Computing tangents/bi-tangents requires texture coordinates\n");
                return 1;
            }

            hr = inMesh->ComputeTangentFrame((dwOptions & (UINT32_C(1) << OPT_CTF)) ? true : false);
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed computing tangent frame (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
        }

        // Perform attribute and vertex-cache optimization
        if (dwOptions & (UINT32_C(1) << OPT_OPTIMIZE))
        {
            assert(inMesh->GetAdjacencyBuffer() != nullptr);

            float acmr, atvr;
            ComputeVertexCacheMissRate(inMesh->GetIndexBuffer(), nFaces, nVerts, OPTFACES_V_DEFAULT, acmr, atvr);

            wprintf(L" [ACMR %f, ATVR %f] ", acmr, atvr);

            hr = inMesh->Optimize((dwOptions & (UINT32_C(1) << OPT_OPTIMIZE_LRU)) ? true : false);
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed vertex-cache optimization (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
        }

        if (dwOptions & (UINT32_C(1) << OPT_FLIP))
        {
            hr = inMesh->ReverseWinding();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed reversing winding (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
        }

        // Write results
        wprintf(L"\n\t->\n");

        if (dwOptions & (UINT32_C(1) << OPT_OPTIMIZE))
        {
            float acmr, atvr;
            ComputeVertexCacheMissRate(inMesh->GetIndexBuffer(), nFaces, nVerts, OPTFACES_V_DEFAULT, acmr, atvr);

            wprintf(L" [ACMR %f, ATVR %f] ", acmr, atvr);
        }
        wchar_t outputExt[_MAX_EXT] = {};

        if (!outputFile.empty())
        {
            std::filesystem::path npath(outputFile);
            wcscpy_s(outputExt, npath.extension().c_str());
        }
        else
        {
            switch (fileType)
            {
            case CODEC_VBO:
                wcscpy_s(outputExt, L".vbo");
                break;

            case CODEC_CMO:
                wcscpy_s(outputExt, L".cmo");
                break;

            case CODEC_WAVEFRONT_OBJ:
                wcscpy_s(outputExt, L".obj");
                break;

            default:
                wcscpy_s(outputExt, L".sdkmesh");
                break;
            }

            outputFile.assign(curpath.stem());
            outputFile.append(outputExt);
        }

        if (dwOptions & (UINT32_C(1) << OPT_TOLOWER))
        {
            std::transform(outputFile.begin(), outputFile.end(), outputFile.begin(), towlower);
        }

        if (~dwOptions & (UINT32_C(1) << OPT_OVERWRITE))
        {
            if (GetFileAttributesW(outputFile.c_str()) != INVALID_FILE_ATTRIBUTES)
            {
                wprintf(L"\nERROR: Output file already exists, use -y to overwrite:\n'%ls'\n", outputFile.c_str());
                return 1;
            }
        }

        if (!_wcsicmp(outputExt, L".vbo"))
        {
            if (!inMesh->GetNormalBuffer() || !inMesh->GetTexCoordBuffer())
            {
                wprintf(L"\nERROR: VBO requires position, normal, and texcoord\n");
                return 1;
            }

            if (!inMesh->Is16BitIndexBuffer() || (dwOptions & (UINT32_C(1) << OPT_FORCE_32BIT_IB)))
            {
                wprintf(L"\nERROR: VBO only supports 16-bit indices\n");
                return 1;
            }

            hr = inMesh->ExportToVBO(outputFile.c_str());
        }
        else if (!_wcsicmp(outputExt, L".sdkmesh"))
        {
            hr = inMesh->ExportToSDKMESH(
                outputFile.c_str(),
                inMaterial.size(), inMaterial.empty() ? nullptr : inMaterial.data(),
                (dwOptions & (UINT32_C(1) << OPT_FORCE_32BIT_IB)) ? true : false,
                (fileType == CODEC_SDKMESH_V2) ? true : false,
                normalFormat,
                uvFormat,
                colorFormat);
        }
        else if (!_wcsicmp(outputExt, L".cmo"))
        {
            if (!inMesh->GetNormalBuffer() || !inMesh->GetTexCoordBuffer() || !inMesh->GetTangentBuffer())
            {
                wprintf(L"\nERROR: Visual Studio CMO requires position, normal, tangents, and texcoord\n");
                return 1;
            }

            if (!inMesh->Is16BitIndexBuffer() || (dwOptions & (UINT32_C(1) << OPT_FORCE_32BIT_IB)))
            {
                wprintf(L"\nERROR: Visual Studio CMO only supports 16-bit indices\n");
                return 1;
            }

            hr = inMesh->ExportToCMO(outputFile.c_str(), inMaterial.size(), inMaterial.empty() ? nullptr : inMaterial.data());
        }
        else if (!_wcsicmp(outputExt, L".obj") || !_wcsicmp(outputExt, L"._obj"))
        {
            hr = inMesh->ExportToOBJ(outputFile.c_str(), inMaterial.size(), inMaterial.empty() ? nullptr : inMaterial.data());
        }
        else if (!_wcsicmp(outputExt, L".x"))
        {
            wprintf(L"\nERROR: Legacy Microsoft X files not supported\n");
            return 1;
        }
        else
        {
            wprintf(L"\nERROR: Unknown output file type '%ls'\n", outputExt);
            return 1;
        }

        if (FAILED(hr))
        {
            wprintf(L"\nERROR: Failed write (%08X%ls):-> '%ls'\n",
                static_cast<unsigned int>(hr), GetErrorDesc(hr), outputFile.c_str());
            return 1;
        }

        wprintf(L" %zu vertices, %zu faces written:\n'%ls'\n", nVerts, nFaces, outputFile.c_str());
    }

    return 0;
}
