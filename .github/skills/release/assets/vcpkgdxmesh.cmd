@set VCPKG_BINARY_SOURCES=clear
@set VCPKG_ROOT=%cd%
@if %1.==xbox. goto xbox
@if %1.==clang. goto clang
.\vcpkg install directxmesh:x86-windows
@if errorlevel 1 goto error
.\vcpkg install directxmesh[tools]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh[dx12]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh[spectre]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x86-windows-static
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x86-windows-static-md
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-windows
@if errorlevel 1 goto error
.\vcpkg install directxmesh[tools]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh[dx12]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh[spectre]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-windows-static
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-windows-static-md
@if errorlevel 1 goto error
.\vcpkg install directxmesh:arm64-windows
@if errorlevel 1 goto error
.\vcpkg install directxmesh[tools]:arm64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh[spectre]:arm64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh:arm64-windows-static
@if errorlevel 1 goto error
.\vcpkg install directxmesh:arm64-windows-static-md
@if errorlevel 1 goto error
.\vcpkg install directxmesh:arm64ec-windows
@if errorlevel 1 goto error
.\vcpkg install directxmesh[tools]:arm64ec-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh[spectre]:arm64ec-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x86-uwp
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-uwp
@if errorlevel 1 goto error
.\vcpkg install directxmesh:arm64-uwp
@if errorlevel 1 goto error
@where /Q x86_64-w64-mingw32-g++.exe
@if errorlevel 1 goto skipgcc64
.\vcpkg install directxmesh:x64-mingw-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxmesh[dx12]:x64-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh[tools]:x64-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-mingw-static
@if errorlevel 1 goto error
.\vcpkg install directxmesh[dx12]:x64-mingw-static --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh[tools]:x64-mingw-static --recurse
@if errorlevel 1 goto error
:skipgcc64
@where /Q i686-w64-mingw32-g++.exe
@if errorlevel 1 goto skipgcc32
.\vcpkg install directxmesh:x86-mingw-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxmesh[dx12]:x86-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh[tools]:x86-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x86-mingw-static
@if errorlevel 1 goto error
.\vcpkg install directxmesh[dx12]:x86-mingw-static --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh[tools]:x86-mingw-static --recurse
@if errorlevel 1 goto error
:skipgcc32
@if "%GXDKLatest%."=="." goto finish
:xbox
.\vcpkg install directxmesh:x64-xbox-scarlett
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-xbox-scarlett-static
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-xbox-xboxone
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-xbox-xboxone-static
@if errorlevel 1 goto error
@goto finish
:clang
.\vcpkg install directxmesh:x64-clangcl-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxmesh[tools]:x64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh[dx12]:x64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-clangcl-static
@if errorlevel 1 goto error
.\vcpkg install directxmesh:arm64-clangcl-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxmesh[tools]:arm64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxmesh:arm64-clangcl-static
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-clangcl-uwp
@if errorlevel 1 goto error
.\vcpkg install directxmesh:arm64-clangcl-uwp
@if errorlevel 1 goto error
@if "%GXDKLatest%."=="." goto finish
.\vcpkg install directxmesh:x64-clangcl-scarlett
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-clangcl-scarlett-static
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-clangcl-xboxone
@if errorlevel 1 goto error
.\vcpkg install directxmesh:x64-clangcl-xboxone-static
@if errorlevel 1 goto error
:finish
@echo SUCCEEDED
@if %1.==xbox. goto eof
@if %1.==clang. goto eof
@echo .
@echo . Run on x64-linux and arm64-linux
@echo .   ./vcpkg install directxmesh
@echo .   ./vcpkg install directxmesh[dx12] --recurse
@where /Q x86_64-w64-mingw32-g++.exe
@if NOT errorlevel 1 goto gcc64
@echo .
@echo . Run for MinGW64
@echo .   .\vcpkg install directxmesh:x64-mingw-dynamic
@echo .   .\vcpkg install directxmesh[dx12]:x64-mingw-dynamic --recurse
@echo .   .\vcpkg install directxmesh[tools]:x64-mingw-dynamic --recurse
@echo .   .\vcpkg install directxmesh:x64-mingw-static
@echo .   .\vcpkg install directxmesh[dx12]:x64-mingw-static --recurse
@echo .   .\vcpkg install directxmesh[tools]:x64-mingw-static --recurse
:gcc64
@where /Q i686-w64-mingw32-g++.exe
@if NOT errorlevel 1 goto gcc32
@echo .
@echo . Run for MinGW32
@echo .   .\vcpkg install directxmesh:x86-mingw-dynamic
@echo .   .\vcpkg install directxmesh[dx12]:x86-mingw-dynamic --recurse
@echo .   .\vcpkg install directxmesh[tools]:x86-mingw-dynamic --recurse
@echo .   .\vcpkg install directxmesh:x86-mingw-dynamic
@echo .   .\vcpkg install directxmesh[dx12]:x86-mingw-dynamic --recurse
@echo .   .\vcpkg install directxmesh[tools]:x86-mingw-dynamic --recurse
:gcc32
@goto eof
:error
@echo FAILED
:eof
