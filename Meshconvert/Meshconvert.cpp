//--------------------------------------------------------------------------------------
// File: Meshconvert.cpp
//
// Meshconvert command-line tool (sample for DirectXMesh library)
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <fstream>
#include <memory>
#include <list>

#include "Mesh.h"

using namespace DirectX;

enum OPTIONS
{
    OPT_RECURSIVE = 1,
    OPT_NORMALS,
    OPT_WEIGHT_BY_AREA,
    OPT_WEIGHT_BY_EQUAL,
    OPT_TANGENTS,
    OPT_CTF,
    OPT_OPTIMIZE,
    OPT_OPTIMIZE_LRU,
    OPT_CLEAN,
    OPT_TOPOLOGICAL_ADJ,
    OPT_GEOMETRIC_ADJ,
    OPT_OUTPUTFILE,
    OPT_SDKMESH,
    OPT_CMO,
    OPT_VBO,
    OPT_CLOCKWISE,
    OPT_FORCE_32BIT_IB,
    OPT_OVERWRITE,
    OPT_NODDS,
    OPT_FLIP,
    OPT_FLIPU,
    OPT_FLIPV,
    OPT_FLIPZ,
    OPT_NOLOGO,
    OPT_FILELIST,
    OPT_MAX
};

static_assert(OPT_MAX <= 32, "dwOptions is a DWORD bitfield");

struct SConversion
{
    wchar_t szSrc[MAX_PATH];
};

struct SValue
{
    LPCWSTR pName;
    DWORD dwValue;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

const SValue g_pOptions[] =
{
    { L"r",         OPT_RECURSIVE },
    { L"n",         OPT_NORMALS },
    { L"na",        OPT_WEIGHT_BY_AREA },
    { L"ne",        OPT_WEIGHT_BY_EQUAL },
    { L"t",         OPT_TANGENTS },
    { L"tb",        OPT_CTF },
    { L"op",        OPT_OPTIMIZE },
    { L"oplru",     OPT_OPTIMIZE_LRU },
    { L"c",         OPT_CLEAN },
    { L"ta",        OPT_TOPOLOGICAL_ADJ },
    { L"ga",        OPT_GEOMETRIC_ADJ },
    { L"o",         OPT_OUTPUTFILE },
    { L"sdkmesh",   OPT_SDKMESH },
    { L"cmo",       OPT_CMO },
    { L"vbo",       OPT_VBO },
    { L"cw",        OPT_CLOCKWISE },
    { L"ib32",      OPT_FORCE_32BIT_IB },
    { L"y",         OPT_OVERWRITE },
    { L"nodds",     OPT_NODDS },
    { L"flip",      OPT_FLIP },
    { L"flipu",     OPT_FLIPU },
    { L"flipv",     OPT_FLIPV },
    { L"flipz",     OPT_FLIPZ },
    { L"nologo",    OPT_NOLOGO },
    { L"flist",     OPT_FILELIST },
    { nullptr,      0 }
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

namespace
{
    inline HANDLE safe_handle(HANDLE h) { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

    struct find_closer { void operator()(HANDLE h) { assert(h != INVALID_HANDLE_VALUE); if (h) FindClose(h); } };

    typedef std::unique_ptr<void, find_closer> ScopedFindHandle;

#pragma prefast(disable : 26018, "Only used with static internal arrays")

    DWORD LookupByName(const wchar_t *pName, const SValue *pArray)
    {
        while (pArray->pName)
        {
            if (!_wcsicmp(pName, pArray->pName))
                return pArray->dwValue;

            pArray++;
        }

        return 0;
    }


    const wchar_t* LookupByValue(DWORD pValue, const SValue *pArray)
    {
        while (pArray->pName)
        {
            if (pValue == pArray->dwValue)
                return pArray->pName;

            pArray++;
        }

        return L"";
    }


    void SearchForFiles(const wchar_t* path, std::list<SConversion>& files, bool recursive)
    {
        // Process files
        WIN32_FIND_DATA findData = {};
        ScopedFindHandle hFile(safe_handle(FindFirstFileExW(path,
            FindExInfoBasic, &findData,
            FindExSearchNameMatch, nullptr,
            FIND_FIRST_EX_LARGE_FETCH)));
        if (hFile)
        {
            for (;;)
            {
                if (!(findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)))
                {
                    wchar_t drive[_MAX_DRIVE] = {};
                    wchar_t dir[_MAX_DIR] = {};
                    _wsplitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);

                    SConversion conv;
                    _wmakepath_s(conv.szSrc, drive, dir, findData.cFileName, nullptr);
                    files.push_back(conv);
                }

                if (!FindNextFile(hFile.get(), &findData))
                    break;
            }
        }

        // Process directories
        if (recursive)
        {
            wchar_t searchDir[MAX_PATH] = {};
            {
                wchar_t drive[_MAX_DRIVE] = {};
                wchar_t dir[_MAX_DIR] = {};
                _wsplitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
                _wmakepath_s(searchDir, drive, dir, L"*", nullptr);
            }

            hFile.reset(safe_handle(FindFirstFileExW(searchDir,
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
                        wchar_t subdir[MAX_PATH] = {};

                        {
                            wchar_t drive[_MAX_DRIVE] = {};
                            wchar_t dir[_MAX_DIR] = {};
                            wchar_t fname[_MAX_FNAME] = {};
                            wchar_t ext[_MAX_FNAME] = {};
                            _wsplitpath_s(path, drive, dir, fname, ext);
                            wcscat_s(dir, findData.cFileName);
                            _wmakepath_s(subdir, drive, dir, fname, ext);
                        }

                        SearchForFiles(subdir, files, recursive);
                    }
                }

                if (!FindNextFile(hFile.get(), &findData))
                    break;
            }
        }
    }


    void PrintLogo()
    {
        wprintf(L"Microsoft (R) MeshConvert Command-line Tool\n");
        wprintf(L"Copyright (C) Microsoft Corp. All rights reserved.\n");
#ifdef _DEBUG
        wprintf(L"*** Debug build ***\n");
#endif
        wprintf(L"\n");
    }


    void PrintUsage()
    {
        PrintLogo();

        wprintf(L"Usage: meshconvert <options> <files>\n");
        wprintf(L"\n");
        wprintf(L"   -r                  wildcard filename search is recursive\n");
        wprintf(L"   -n | -na | -ne      generate normals weighted by angle/area/equal\n");
        wprintf(L"   -t                  generate tangents\n");
        wprintf(L"   -tb                 generate tangents & bi-tangents\n");
        wprintf(L"   -cw                 faces are clockwise (defaults to counter-clockwise)\n");
        wprintf(L"   -ib32               use 32-bit index buffer (SDKMESH only)\n");
        wprintf(L"   -op | -oplru        vertex cache optimize the mesh (implies -c)\n");
        wprintf(L"   -c                  mesh cleaning including vertex dups for atttribute sets\n");
        wprintf(L"   -ta | -ga           generate topological vs. geometric adjancecy (def: ta)\n");
        wprintf(L"   -sdkmesh|-cmo|-vbo  output file type\n");
        wprintf(L"   -nodds              prevents extension renaming in exported materials\n");
        wprintf(L"   -flip               reverse winding of faces\n");
        wprintf(L"   -flipu              inverts the u texcoords\n");
        wprintf(L"   -flipv              inverts the v texcoords\n");
        wprintf(L"   -flipz              flips the handedness of the positions/normals\n");
        wprintf(L"   -o <filename>       output filename\n");
        wprintf(L"   -y                  overwrite existing output file (if any)\n");
        wprintf(L"   -nologo             suppress copyright message\n");
        wprintf(L"   -flist <filename>   use text file with a list of input files (one per line)\n");

        wprintf(L"\n");
    }
}

extern HRESULT LoadFromOBJ(const wchar_t* szFilename, std::unique_ptr<Mesh>& inMesh, std::vector<Mesh::Material>& inMaterial, bool ccw, bool dds);

//--------------------------------------------------------------------------------------
// Entry-point
//--------------------------------------------------------------------------------------
#pragma prefast(disable : 28198, "Command-line tool, frees all memory on exit")

int __cdecl wmain(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
    // Parameters and defaults
    wchar_t szOutputFile[MAX_PATH] = {};

    // Process command line
    DWORD dwOptions = 0;
    std::list<SConversion> conversion;

    for (int iArg = 1; iArg < argc; iArg++)
    {
        PWSTR pArg = argv[iArg];

        if (('-' == pArg[0]) || ('/' == pArg[0]))
        {
            pArg++;
            PWSTR pValue;

            for (pValue = pArg; *pValue && (':' != *pValue); pValue++);

            if (*pValue)
                *pValue++ = 0;

            DWORD dwOption = LookupByName(pArg, g_pOptions);

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
                wcscpy_s(szOutputFile, MAX_PATH, pValue);
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
                if (dwOptions & ((1 << OPT_VBO) | (1 << OPT_CMO)))
                {
                    wprintf(L"Can only use one of sdkmesh, cmo, or vbo\n");
                    return 1;
                }
                break;

            case OPT_CMO:
                if (dwOptions & ((1 << OPT_VBO) | (1 << OPT_SDKMESH)))
                {
                    wprintf(L"Can only use one of sdkmesh, cmo, or vbo\n");
                    return 1;
                }
                break;

            case OPT_VBO:
                if (dwOptions & ((1 << OPT_SDKMESH) | (1 << OPT_CMO)))
                {
                    wprintf(L"Can only use one of sdkmesh, cmo, or vbo\n");
                    return 1;
                }
                break;

            case OPT_FILELIST:
                {
                    std::wifstream inFile(pValue);
                    if (!inFile)
                    {
                        wprintf(L"Error opening -flist file %ls\n", pValue);
                        return 1;
                    }
                    wchar_t fname[1024] = {};
                    for (;;)
                    {
                        inFile >> fname;
                        if (!inFile)
                            break;

                        if (*fname == L'#')
                        {
                            // Comment
                        }
                        else if (*fname == L'-')
                        {
                            wprintf(L"Command-line arguments not supported in -flist file\n");
                            return 1;
                        }
                        else if (wcspbrk(fname, L"?*") != nullptr)
                        {
                            wprintf(L"Wildcards not supported in -flist file\n");
                            return 1;
                        }
                        else
                        {
                            SConversion conv;
                            wcscpy_s(conv.szSrc, MAX_PATH, fname);
                            conversion.push_back(conv);
                        }

                        inFile.ignore(1000, '\n');
                    }
                    inFile.close();
                }
                break;
            }
        }
        else if (wcspbrk(pArg, L"?*") != nullptr)
        {
            size_t count = conversion.size();
            SearchForFiles(pArg, conversion, (dwOptions & (1 << OPT_RECURSIVE)) != 0);
            if (conversion.size() <= count)
            {
                wprintf(L"No matching files found for %ls\n", pArg);
                return 1;
            }
        }
        else
        {
            SConversion conv;
            wcscpy_s(conv.szSrc, MAX_PATH, pArg);

            conversion.push_back(conv);
        }
    }

    if (conversion.empty())
    {
        PrintUsage();
        return 0;
    }

    if (*szOutputFile && conversion.size() > 1)
    {
        wprintf(L"Cannot use -o with multiple input files\n");
        return 1;
    }

    if (~dwOptions & (1 << OPT_NOLOGO))
        PrintLogo();

    // Process files
    for (auto pConv = conversion.begin(); pConv != conversion.end(); ++pConv)
    {
        wchar_t ext[_MAX_EXT];
        wchar_t fname[_MAX_FNAME];
        _wsplitpath_s(pConv->szSrc, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, ext, _MAX_EXT);

        if (pConv != conversion.begin())
            wprintf(L"\n");

        wprintf(L"reading %ls", pConv->szSrc);
        fflush(stdout);

        std::unique_ptr<Mesh> inMesh;
        std::vector<Mesh::Material> inMaterial;
        HRESULT hr = E_NOTIMPL;
        if (_wcsicmp(ext, L".vbo") == 0)
        {
            hr = Mesh::CreateFromVBO(pConv->szSrc, inMesh);
        }
        else if (_wcsicmp(ext, L".sdkmesh") == 0)
        {
            wprintf(L"\nERROR: Importing SDKMESH files not supported\n");
            return 1;
        }
        else if (_wcsicmp(ext, L".cmo") == 0)
        {
            wprintf(L"\nERROR: Importing Visual Studio CMO files not supported\n");
            return 1;
        }
        else if (_wcsicmp(ext, L".x") == 0)
        {
            wprintf(L"\nERROR: Legacy Microsoft X files not supported\n");
            return 1;
        }
        else if (_wcsicmp(ext, L".fbx") == 0)
        {
            wprintf(L"\nERROR: Autodesk FBX files not supported\n");
            return 1;
        }
        else
        {
            hr = LoadFromOBJ(pConv->szSrc, inMesh, inMaterial,
                (dwOptions & (1 << OPT_CLOCKWISE)) ? false : true,
                (dwOptions & (1 << OPT_NODDS)) ? false : true);
        }
        if (FAILED(hr))
        {
            wprintf(L" FAILED (%08X)\n", hr);
            return 1;
        }

        size_t nVerts = inMesh->GetVertexCount();
        size_t nFaces = inMesh->GetFaceCount();

        if (!nVerts || !nFaces)
        {
            wprintf(L"\nERROR: Invalid mesh\n");
            return 1;
        }

        assert(inMesh->GetPositionBuffer() != 0);
        assert(inMesh->GetIndexBuffer() != 0);

        wprintf(L"\n%zu vertices, %zu faces", nVerts, nFaces);

        if (dwOptions & (1 << OPT_FLIPU))
        {
            hr = inMesh->InvertUTexCoord();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed inverting u texcoord (%08X)\n", hr);
                return 1;
            }
        }

        if (dwOptions & (1 << OPT_FLIPV))
        {
            hr = inMesh->InvertVTexCoord();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed inverting v texcoord (%08X)\n", hr);
                return 1;
            }
        }

        if (dwOptions & (1 << OPT_FLIPZ))
        {
            hr = inMesh->ReverseHandedness();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed reversing handedness (%08X)\n", hr);
                return 1;
            }
        }

        // Prepare mesh for processing
        if (dwOptions & ((1 << OPT_OPTIMIZE) | (1 << OPT_CLEAN)))
        {
            // Adjacency
            float epsilon = (dwOptions & (1 << OPT_GEOMETRIC_ADJ)) ? 1e-5f : 0.f;

            hr = inMesh->GenerateAdjacency(epsilon);
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed generating adjacency (%08X)\n", hr);
                return 1;
            }

            // Validation
            std::wstring msgs;
            hr = inMesh->Validate(VALIDATE_BACKFACING, &msgs);
            if (!msgs.empty())
            {
                wprintf(L"\nWARNING: \n");
                wprintf(msgs.c_str());
            }

            // Clean (also handles attribute reuse split if needed)
            hr = inMesh->Clean();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed mesh clean (%08X)\n", hr);
                return 1;
            }
            else
            {
                size_t nNewVerts = inMesh->GetVertexCount();
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
            DWORD flags = CNORM_DEFAULT;

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
                wprintf(L"\nERROR: Failed computing normals (flags:%1X, %08X)\n", flags, hr);
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
                wprintf(L"\nERROR: Failed computing tangent frame (%08X)\n", hr);
                return 1;
            }
        }

        // Perform attribute and vertex-cache optimization
        if (dwOptions & (1 << OPT_OPTIMIZE))
        {
            assert(inMesh->GetAdjacencyBuffer() != 0);

            float acmr, atvr;
            ComputeVertexCacheMissRate(inMesh->GetIndexBuffer(), nFaces, nVerts, OPTFACES_V_DEFAULT, acmr, atvr);

            wprintf(L" [ACMR %f, ATVR %f] ", acmr, atvr);

            hr = inMesh->Optimize((dwOptions & (1 << OPT_OPTIMIZE_LRU)) ? true : false);
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed vertex-cache optimization (%08X)\n", hr);
                return 1;
            }
        }

        if (dwOptions & (1 << OPT_FLIP))
        {
            hr = inMesh->ReverseWinding();
            if (FAILED(hr))
            {
                wprintf(L"\nERROR: Failed reversing winding (%08X)\n", hr);
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


        wchar_t outputPath[MAX_PATH] = {};
        wchar_t outputExt[_MAX_EXT] = {};

        if (*szOutputFile)
        {
            wcscpy_s(outputPath, szOutputFile);

            _wsplitpath_s(szOutputFile, nullptr, 0, nullptr, 0, nullptr, 0, outputExt, _MAX_EXT);
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
            else
            {
                wcscpy_s(outputExt, L".sdkmesh");
            }

            wchar_t outFilename[_MAX_FNAME] = {};
            wcscpy_s(outFilename, fname);

            _wmakepath_s(outputPath, nullptr, nullptr, outFilename, outputExt);
        }

        if (~dwOptions & (1 << OPT_OVERWRITE))
        {
            if (GetFileAttributesW(outputPath) != INVALID_FILE_ATTRIBUTES)
            {
                wprintf(L"\nERROR: Output file already exists, use -y to overwrite:\n'%ls'\n", outputPath);
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

            hr = inMesh->ExportToVBO(outputPath);
        }
        else if (!_wcsicmp(outputExt, L".sdkmesh"))
        {
            hr = inMesh->ExportToSDKMESH(
                outputPath,
                inMaterial.size(), inMaterial.empty() ? nullptr : inMaterial.data(),
                (dwOptions & (1 << OPT_FORCE_32BIT_IB)) ? true : false);
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

            hr = inMesh->ExportToCMO(outputPath, inMaterial.size(), inMaterial.empty() ? nullptr : inMaterial.data());
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
            wprintf(L"\nERROR: Failed write (%08X):-> '%ls'\n", hr, outputPath);
            return 1;
        }

        wprintf(L" %zu vertices, %zu faces written:\n'%ls'\n", nVerts, nFaces, outputPath);
    }

    return 0;
}
