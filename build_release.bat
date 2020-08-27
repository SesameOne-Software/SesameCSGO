@echo off
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86
) else (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
    ) else (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x86
    )
)

set compilerflags=/Fo".\\bin\\release\\" /LD /GS- /MP /EHsc /O2 /Ob3 /std:c++17 /fp:fast

set linkerflags=/DLL /NXCOMPAT:NO /OUT:bin\release\hack.dll

set libs=kernel32.lib user32.lib advapi32.lib ws2_32.lib d3d9.lib d3dx9.lib comdlg32.lib winspool.lib gdi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib minhook.lib
set include_dirs=/I "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Include"

set library_dirs=/LIBPATH:"C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x86" /LIBPATH:"C:\\Users\\Arche\\blast-software\\csgo\\minhook"

set compile_dirs=csgo\*.cpp csgo\animations\*.cpp csgo\features\*.cpp csgo\hooks\*.cpp csgo\javascript\*.cpp csgo\menu\*.cpp csgo\renderer\*.cpp csgo\sdk\*.cpp csgo\security\*.cpp csgo\tinyxml2\*.cpp csgo\utils\*.cpp

set preprocessor=/D _MBCS /D _WINDLL /D _CRT_SECURE_NO_WARNINGS /D JM_XORSTR_DISABLE_AVX_INTRINSICS /D ANTI_DEBUG /D DEV_BUILD

cl.exe %preprocessor% %compilerflags% %include_dirs% %compile_dirs% /link %library_dirs% %libs% %linkerflags%