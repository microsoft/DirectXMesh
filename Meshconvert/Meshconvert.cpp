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

using namespace DirectX;

namespace
{
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
        OPT_OUTPUTFILE,
        OPT_TOLOWER,
        OPT_SDKMESH,
        OPT_SDKMESH_V2,
        OPT_CMO,
        OPT_VBO,
        OPT_WAVEFRONT_OBJ,
        OPT_CLOCKWISE,
        OPT_FORCE_32BIT_IB,
        OPT_OVERWRITE,
        OPT_NODDS,
        OPT_FLIP,
        OPT_FLIPU,
        OPT_FLIPV,
        OPT_FLIPZ,
        OPT_VERT_NORMAL_FORMAT,
        OPT_VERT_UV_FORMAT,
        OPT_VERT_COLOR_FORMAT,
        OPT_NOLOGO,
        OPT_FILELIST,
        OPT_MAX
    };

    static_assert(OPT_MAX <= 32, "dwOptions is a unsigned int bitfield");

    struct SConversion
    {
        std::wstring szSrc;
    };

    struct SValue
    {
        const wchar_t*  name;
        uint32_t        value;
    };

    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    const SValue g_pOptions[] =
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
        { L"sdkmesh",   OPT_SDKMESH },
        { L"sdkmesh2",  OPT_SDKMESH_V2 },
        { L"cmo",       OPT_CMO },
        { L"vbo",       OPT_VBO },
        { L"wf",        OPT_WAVEFRONT_OBJ },
        { L"cw",        OPT_CLOCKWISE },
        { L"ib32",      OPT_FORCE_32BIT_IB },
        { L"y",         OPT_OVERWRITE },
        { L"nodds",     OPT_NODDS },
        { L"flip",      OPT_FLIP },
        { L"flipu",     OPT_FLIPU },
        { L"flipv",     OPT_FLIPV },
        { L"flipz",     OPT_FLIPZ },
        { L"fn",        OPT_VERT_NORMAL_FORMAT },
        { L"fuv",       OPT_VERT_UV_FORMAT },
        { L"fc",        OPT_VERT_COLOR_FORMAT },
        { L"nologo",    OPT_NOLOGO },
        { L"flist",     OPT_FILELIST },
        { nullptr,      0 }
    };

    const SValue g_vertexNormalFormats[] =
    {
        { L"float3",    DXGI_FORMAT_R32G32B32_FLOAT },
        { L"float16_4", DXGI_FORMAT_R16G16B16A16_FLOAT },
        { L"r11g11b10", DXGI_FORMAT_R11G11B10_FLOAT },
        { nullptr,      0 }
    };

    const SValue g_vertexUVFormats[] =
    {
        { L"float2",    DXGI_FORMAT_R32G32_FLOAT },
        { L"float16_2", DXGI_FORMAT_R16G16_FLOAT },
        { nullptr,      0 }
    };

    const SValue g_vertexColorFormats[] =
    {
        { L"bgra",      DXGI_FORMAT_B8G8R8A8_UNORM },
        { L"rgba",      DXGI_FORMAT_R8G8B8A8_UNORM },
        { L"float4",    DXGI_FORMAT_R32G32B32A32_FLOAT },
        { L"float16_4", DXGI_FORMAT_R16G16B16A16_FLOAT },
        { L"rgba_10",   DXGI_FORMAT_R10G10B10A2_UNORM },
        { L"r11g11b10", DXGI_FORMAT_R11G11B10_FLOAT },
        { nullptr,      0 }
    };
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

HRESULT LoadFromOBJ(const wchar_t* szFilename,
    std::unique_ptr<Mesh>& inMesh, std::vector<Mesh::Material>& inMaterial,
    bool ccw, bool dds);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

namespace
{
    inline HANDLE safe_handle(HANDLE h) noexcept { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

    struct find_closer { void operator()(HANDLE h) noexcept { assert(h != INVALID_HANDLE_VALUE); if (h) FindClose(h); } };

    using ScopedFindHandle = std::unique_ptr<void, find_closer>;

#ifdef __PREFAST__
#pragma prefast(disable : 26018, "Only used with static internal arrays")
#endif

    uint32_t LookupByName(const wchar_t* pName, const SValue* pArray)
    {
        while (pArray->name)
        {
            if (!_wcsicmp(pName, pArray->name))
                return pArray->value;

            pArray++;
        }

        return 0;
    }

    void SearchForFiles(const std::filesystem::path& path, std::list<SConversion>& files, bool recursive)
    {
        // Process files
        WIN32_FIND_DATAW findData = {};
        ScopedFindHandle hFile(safe_handle(FindFirstFileExW(path.c_str(),
            FindExInfoBasic, &findData,
            FindExSearchNameMatch, nullptr,
            FIND_FIRST_EX_LARGE_FETCH)));
        if (hFile)
        {
            for (;;)
            {
                if (!(findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)))
                {
                    SConversion conv = {};
                    conv.szSrc = path.parent_path().append(findData.cFileName).native();
                    files.push_back(conv);
                }

                if (!FindNextFileW(hFile.get(), &findData))
                    break;
            }
        }

        // Process directories
        if (recursive)
        {
            auto searchDir = path.parent_path().append(L"*");

            hFile.reset(safe_handle(FindFirstFileExW(searchDir.c_str(),
                FindExInfoBasic, &findData,
                FindExSearchLimitToDirectories, nullptr,
                FIND_FIRST_EX_LARGE_FETCH)));
            if (!hFile)
                return;

            for (;;)
            {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (findData.cFileName[0] != L'.')
                    {
                        auto subdir = path.parent_path().append(findData.cFileName).append(path.filename().c_str());

                        SearchForFiles(subdir, files, recursive);
                    }
                }

                if (!FindNextFileW(hFile.get(), &findData))
                    break;
            }
        }
    }

    void ProcessFileList(std::wifstream& inFile, std::list<SConversion>& files)
    {
        std::list<SConversion> flist;
        std::set<std::wstring> excludes;

        auto fname = std::make_unique<wchar_t[]>(32768);
        for (;;)
        {
            inFile >> fname.get();
            if (!inFile)
                break;

            if (fname[0] == L'#')
            {
                // Comment
            }
            else if (fname[0] == L'-')
            {
                if (flist.empty())
                {
                    wprintf(L"WARNING: Ignoring the line '%ls' in -flist\n", fname.get());
                }
                else
                {
                    std::filesystem::path path(fname.get() + 1);
                    auto& npath = path.make_preferred();
                    if (wcspbrk(fname.get(), L"?*") != nullptr)
                    {
                        std::list<SConversion> removeFiles;
                        SearchForFiles(npath, removeFiles, false);

                        for (auto& it : removeFiles)
                        {
                            std::wstring name = it.szSrc;
                            std::transform(name.begin(), name.end(), name.begin(), towlower);
                            excludes.insert(name);
                        }
                    }
                    else
                    {
                        std::wstring name = npath.c_str();
                        std::transform(name.begin(), name.end(), name.begin(), towlower);
                        excludes.insert(name);
                    }
                }
            }
            else if (wcspbrk(fname.get(), L"?*") != nullptr)
            {
                std::filesystem::path path(fname.get());
                SearchForFiles(path.make_preferred(), flist, false);
            }
            else
            {
                SConversion conv = {};
                std::filesystem::path path(fname.get());
                conv.szSrc = path.make_preferred().native();
                flist.push_back(conv);
            }

            inFile.ignore(1000, '\n');
        }

        inFile.close();

        if (!excludes.empty())
        {
            // Remove any excluded files
            for (auto it = flist.begin(); it != flist.end();)
            {
                std::wstring name = it->szSrc;
                std::transform(name.begin(), name.end(), name.begin(), towlower);
                auto item = it;
                ++it;
                if (excludes.find(name) != excludes.end())
                {
                    flist.erase(item);
                }
            }
        }

        if (flist.empty())
        {
            wprintf(L"WARNING: No file names found in -flist\n");
        }
        else
        {
            files.splice(files.end(), flist);
        }
    }

    void PrintList(size_t cch, const SValue* pValue)
    {
        while (pValue->name)
        {
            const size_t cchName = wcslen(pValue->name);

            if (cch + cchName + 2 >= 80)
            {
                wprintf(L"\n      ");
                cch = 6;
            }

            wprintf(L"%ls ", pValue->name);
            cch += cchName + 2;
            pValue++;
        }

        wprintf(L"\n");
    }

    void PrintLogo(bool versionOnly)
    {
        wchar_t version[32] = {};

        wchar_t appName[_MAX_PATH] = {};
        if (GetModuleFileNameW(nullptr, appName, _MAX_PATH))
        {
            const DWORD size = GetFileVersionInfoSizeW(appName, nullptr);
            if (size > 0)
            {
                auto verInfo = std::make_unique<uint8_t[]>(size);
                if (GetFileVersionInfoW(appName, 0, size, verInfo.get()))
                {
                    LPVOID lpstr = nullptr;
                    UINT strLen = 0;
                    if (VerQueryValueW(verInfo.get(), L"\\StringFileInfo\\040904B0\\ProductVersion", &lpstr, &strLen))
                    {
                        wcsncpy_s(version, reinterpret_cast<const wchar_t*>(lpstr), strLen);
                    }
                }
            }
        }

        if (!*version || wcscmp(version, L"1.0.0.0") == 0)
        {
            swprintf_s(version, L"%03d (library)", DIRECTX_MESH_VERSION);
        }

        if (versionOnly)
        {
            wprintf(L"meshconvert version %ls\n", version);
        }
        else
        {
            wprintf(L"Microsoft (R) MeshConvert Command-line Tool Version %ls\n", version);
            wprintf(L"Copyright (C) Microsoft Corp.\n");
        #ifdef _DEBUG
            wprintf(L"*** Debug build ***\n");
        #endif
            wprintf(L"\n");
        }
    }

    void PrintUsage()
    {
        PrintLogo(false);

        static const wchar_t* const s_usage =
            L"Usage: meshconvert <options> [--] <files>\n"
            L"\n"
            L"   Input file type must be Wavefront Object (.obj)\n"
            L"\n"
            L"   Output file type:\n"
            L"       -sdkmesh        DirectX SDK .sdkmesh format (default)\n"
            L"       -sdkmesh2       .sdkmesh format version 2 (PBR materials)\n"
            L"       -cmo            Visual Studio Content Pipeline .cmo format\n"
            L"       -vbo            Vertex Buffer Object (.vbo) format\n"
            L"       -wf             WaveFront Object (.obj) format\n"
            L"\n"
            L"   -r                  wildcard filename search is recursive\n"
            L"   -n | -na | -ne      generate normals weighted by angle/area/equal\n"
            L"   -t                  generate tangents\n"
            L"   -tb                 generate tangents & bi-tangents\n"
            L"   -cw                 faces are clockwise (defaults to counter-clockwise)\n"
            L"   -op | -oplru        vertex cache optimize the mesh (implies -c)\n"
            L"   -c                  mesh cleaning including vertex dups for atttribute sets\n"
            L"   -ta | -ga           generate topological vs. geometric adjancecy (def: ta)\n"
            L"   -nodds              prevents extension renaming in exported materials\n"
            L"   -flip               reverse winding of faces\n"
            L"   -flipu              inverts the u texcoords\n"
            L"   -flipv              inverts the v texcoords\n"
            L"   -flipz              flips the handedness of the positions/normals\n"
            L"   -o <filename>       output filename\n"
            L"   -l                  force output filename to lower case\n"
            L"   -y                  overwrite existing output file (if any)\n"
            L"   -nologo             suppress copyright message\n"
            L"   -flist <filename>   use text file with a list of input files (one per line)\n"
            L"\n"
            L"       (sdkmesh/sdkmesh2 only)\n"
            L"   -ib32               use 32-bit index buffer\n"
            L"   -fn <normal-format> format to use for writing normals/tangents/normals\n"
            L"   -fuv <uv-format>    format to use for texture coordinates\n"
            L"   -fc <color-format>  format to use for writing colors\n"
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

    const wchar_t* GetErrorDesc(HRESULT hr)
    {
        static wchar_t desc[1024] = {};

        LPWSTR errorText = nullptr;

        const DWORD result = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            nullptr, static_cast<DWORD>(hr),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&errorText), 0, nullptr);

        *desc = 0;

        if (result > 0 && errorText)
        {
            swprintf_s(desc, L": %ls", errorText);

            size_t len = wcslen(desc);
            if (len >= 1)
            {
                desc[len - 1] = 0;
            }

            if (errorText)
                LocalFree(errorText);

            for (wchar_t* ptr = desc; *ptr != 0; ++ptr)
            {
                if (*ptr == L'\r' || *ptr == L'\n')
                {
                    *ptr = L' ';
                }
            }
        }

        return desc;
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

        if (allowOpts
            && ('-' == pArg[0]) && ('-' == pArg[1]))
        {
            if (pArg[2] == 0)
            {
                // "-- " is the POSIX standard for "end of options" marking to escape the '-' and '/' characters at the start of filepaths.
                allowOpts = false;
            }
            else if (!_wcsicmp(pArg, L"--version"))
            {
                PrintLogo(true);
                return 0;
            }
            else if (!_wcsicmp(pArg, L"--help"))
            {
                PrintUsage();
                return 0;
            }
            else
            {
                wprintf(L"Unknown option: %ls\n", pArg);
                return 1;
            }
        }
        else if (allowOpts
            && (('-' == pArg[0]) || ('/' == pArg[0])))
        {
            pArg++;
            PWSTR pValue;

            for (pValue = pArg; *pValue && (':' != *pValue); pValue++);

            if (*pValue)
                *pValue++ = 0;

            const uint32_t dwOption = LookupByName(pArg, g_pOptions);

            if (!dwOption || (dwOptions & (1 << dwOption)))
            {
                wprintf(L"ERROR: unknown command-line option '%ls'\n\n", pArg);
                PrintUsage();
                return 1;
            }

            dwOptions |= (1 << dwOption);

            // Handle options with additional value parameter
            switch (dwOption)
            {
            case OPT_OUTPUTFILE:
            case OPT_VERT_NORMAL_FORMAT:
            case OPT_VERT_UV_FORMAT:
            case OPT_VERT_COLOR_FORMAT:
            case OPT_FILELIST:
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
            }

            switch (dwOption)
            {
            case OPT_OPTIMIZE_LRU:
                dwOptions |= (1 << OPT_OPTIMIZE);
                break;

            case OPT_WEIGHT_BY_AREA:
                if (dwOptions & (1 << OPT_WEIGHT_BY_EQUAL))
                {
                    wprintf(L"Cannot use both na and ne at the same time\n");
                    return 1;
                }
                dwOptions |= (1 << OPT_NORMALS);
                break;

            case OPT_WEIGHT_BY_EQUAL:
                if (dwOptions & (1 << OPT_WEIGHT_BY_AREA))
                {
                    wprintf(L"Cannot use both na and ne at the same time\n");
                    return 1;
                }
                dwOptions |= (1 << OPT_NORMALS);
                break;

            case OPT_OUTPUTFILE:
                {
                    std::filesystem::path path(pValue);
                    outputFile = path.make_preferred().native();
                }
                break;

            case OPT_TOPOLOGICAL_ADJ:
                if (dwOptions & (1 << OPT_GEOMETRIC_ADJ))
                {
                    wprintf(L"Cannot use both ta and ga at the same time\n");
                    return 1;
                }
                break;

            case OPT_GEOMETRIC_ADJ:
                if (dwOptions & (1 << OPT_TOPOLOGICAL_ADJ))
                {
                    wprintf(L"Cannot use both ta and ga at the same time\n");
                    return 1;
                }
                break;

            case OPT_SDKMESH:
            case OPT_SDKMESH_V2:
                if (dwOptions & ((1 << OPT_VBO) | (1 << OPT_CMO) | (1 << OPT_WAVEFRONT_OBJ)))
                {
                    wprintf(L"Can only use one of sdkmesh, cmo, vbo, or wf\n");
                    return 1;
                }
                if (dwOption == OPT_SDKMESH_V2)
                {
                    dwOptions |= (1 << OPT_SDKMESH);
                }
                break;

            case OPT_CMO:
                if (dwOptions & ((1 << OPT_VBO) | (1 << OPT_SDKMESH) | (1 << OPT_WAVEFRONT_OBJ)))
                {
                    wprintf(L"Can only use one of sdkmesh, cmo, vbo, or wf\n");
                    return 1;
                }
                break;

            case OPT_VBO:
                if (dwOptions & ((1 << OPT_SDKMESH) | (1 << OPT_CMO) | (1 << OPT_WAVEFRONT_OBJ)))
                {
                    wprintf(L"Can only use one of sdkmesh, cmo, vbo, or wf\n");
                    return 1;
                }
                break;

            case OPT_WAVEFRONT_OBJ:
                if (dwOptions & ((1 << OPT_VBO) | (1 << OPT_SDKMESH) | (1 << OPT_CMO)))
                {
                    wprintf(L"Can only use one of sdkmesh, cmo, vbo, or wf\n");
                    return 1;
                }
                break;

            case OPT_VERT_NORMAL_FORMAT:
                normalFormat = static_cast<DXGI_FORMAT>(LookupByName(pValue, g_vertexNormalFormats));
                if (!normalFormat)
                {
                    wprintf(L"Invalid value specified with -fn (%ls)\n", pValue);
                    wprintf(L"\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_VERT_UV_FORMAT:
                uvFormat = static_cast<DXGI_FORMAT>(LookupByName(pValue, g_vertexUVFormats));
                if (!uvFormat)
                {
                    wprintf(L"Invalid value specified with -fuv (%ls)\n", pValue);
                    wprintf(L"\n");
                    PrintUsage();
                    return 1;
                }
                break;

            case OPT_VERT_COLOR_FORMAT:
                colorFormat = static_cast<DXGI_FORMAT>(LookupByName(pValue, g_vertexColorFormats));
                if (!colorFormat)
                {
                    wprintf(L"Invalid value specified with -fc (%ls)\n", pValue);
                    wprintf(L"\n");
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
            }
        }
        else if (wcspbrk(pArg, L"?*") != nullptr)
        {
            const size_t count = conversion.size();
            std::filesystem::path path(pArg);
            SearchForFiles(path.make_preferred(), conversion, (dwOptions & (1 << OPT_RECURSIVE)) != 0);
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

    if (~dwOptions & (1 << OPT_NOLOGO))
        PrintLogo(false);

    // Process files
    for (auto pConv = conversion.begin(); pConv != conversion.end(); ++pConv)
    {
        std::filesystem::path curpath(pConv->szSrc);
        auto const ext = curpath.extension();

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
        else
        {
            hr = LoadFromOBJ(curpath.c_str(), inMesh, inMaterial,
                (dwOptions & (1 << OPT_CLOCKWISE)) ? false : true,
                (dwOptions & (1 << OPT_NODDS)) ? false : true);
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

        if (dwOptions & (1 << OPT_FLIPU))
        {
            hr = inMesh->InvertUTexCoord();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed inverting u texcoord (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
        }

        if (dwOptions & (1 << OPT_FLIPV))
        {
            hr = inMesh->InvertVTexCoord();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed inverting v texcoord (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
        }

        if (dwOptions & (1 << OPT_FLIPZ))
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
        if (dwOptions & ((1 << OPT_OPTIMIZE) | (1 << OPT_CLEAN)))
        {
            // Adjacency
            const float epsilon = (dwOptions & (1 << OPT_GEOMETRIC_ADJ)) ? 1e-5f : 0.f;

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
            dwOptions |= 1 << OPT_NORMALS;
        }

        if (!inMesh->GetTangentBuffer() && (dwOptions & (1 << OPT_CMO)))
        {
            dwOptions |= 1 << OPT_TANGENTS;
        }

        // Compute vertex normals from faces
        if ((dwOptions & (1 << OPT_NORMALS))
            || ((dwOptions & ((1 << OPT_TANGENTS) | (1 << OPT_CTF))) && !inMesh->GetNormalBuffer()))
        {
            CNORM_FLAGS flags = CNORM_DEFAULT;

            if (dwOptions & (1 << OPT_WEIGHT_BY_EQUAL))
            {
                flags |= CNORM_WEIGHT_EQUAL;
            }
            else if (dwOptions & (1 << OPT_WEIGHT_BY_AREA))
            {
                flags |= CNORM_WEIGHT_BY_AREA;
            }

            if (dwOptions & (1 << OPT_CLOCKWISE))
            {
                flags |= CNORM_WIND_CW;
            }

            hr = inMesh->ComputeNormals(flags);
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed computing normals (flags:%lX, %08X%ls)\n",
                    flags, static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
        }

        // Compute tangents and bitangents
        if (dwOptions & ((1 << OPT_TANGENTS) | (1 << OPT_CTF)))
        {
            if (!inMesh->GetTexCoordBuffer())
            {
                wprintf(L"\nERROR: Computing tangents/bi-tangents requires texture coordinates\n");
                return 1;
            }

            hr = inMesh->ComputeTangentFrame((dwOptions & (1 << OPT_CTF)) ? true : false);
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed computing tangent frame (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
        }

        // Perform attribute and vertex-cache optimization
        if (dwOptions & (1 << OPT_OPTIMIZE))
        {
            assert(inMesh->GetAdjacencyBuffer() != nullptr);

            float acmr, atvr;
            ComputeVertexCacheMissRate(inMesh->GetIndexBuffer(), nFaces, nVerts, OPTFACES_V_DEFAULT, acmr, atvr);

            wprintf(L" [ACMR %f, ATVR %f] ", acmr, atvr);

            hr = inMesh->Optimize((dwOptions & (1 << OPT_OPTIMIZE_LRU)) ? true : false);
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed vertex-cache optimization (%08X%ls)\n",
                    static_cast<unsigned int>(hr), GetErrorDesc(hr));
                return 1;
            }
        }

        if (dwOptions & (1 << OPT_FLIP))
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

        if (dwOptions & (1 << OPT_OPTIMIZE))
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
            if (dwOptions & (1 << OPT_VBO))
            {
                wcscpy_s(outputExt, L".vbo");
            }
            else if (dwOptions & (1 << OPT_CMO))
            {
                wcscpy_s(outputExt, L".cmo");
            }
            else if (dwOptions & (1 << OPT_WAVEFRONT_OBJ))
            {
                wcscpy_s(outputExt, L".obj");
            }
            else
            {
                wcscpy_s(outputExt, L".sdkmesh");
            }

            outputFile.assign(curpath.stem());
            outputFile.append(outputExt);
        }

        if (dwOptions & (1 << OPT_TOLOWER))
        {
            std::transform(outputFile.begin(), outputFile.end(), outputFile.begin(), towlower);
        }

        if (~dwOptions & (1 << OPT_OVERWRITE))
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

            if (!inMesh->Is16BitIndexBuffer() || (dwOptions & (1 << OPT_FORCE_32BIT_IB)))
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
                (dwOptions & (1 << OPT_FORCE_32BIT_IB)) ? true : false,
                (dwOptions & (1 << OPT_SDKMESH_V2)) ? true : false,
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

            if (!inMesh->Is16BitIndexBuffer() || (dwOptions & (1 << OPT_FORCE_32BIT_IB)))
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
