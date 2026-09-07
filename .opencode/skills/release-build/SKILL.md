---
name: release-build
description: Use when building, compiling, configuring, or troubleshooting the CurlDownloader project on Windows — including CMake configure/build, Release or Debug builds, "帮我构建", "编译一下", linker errors (LNK2019/LNK1112/LNK4098), CRT mismatch, x86 vs x64 architecture problems, or "could not find any instance of Visual Studio". Covers the exact toolchain paths, the x86-only constraint imposed by the vendored libcurl/tinyxml2 archives, and the vcvarsall + NMake workaround required on this machine.
---

# CurlDownloader Release Build

Windows-only C++14 DLL built with CMake + MSVC. This skill encodes the toolchain
layout, the hard architecture constraint, and the generator workaround that this
project requires.

## Hard constraints — read first

1. **x86 (Win32) only.** The vendored archives under
   `CurlDownloader/CurlDownloader/dependency/` are 32-bit
   (`dumpbin /HEADERS` reports `14C machine (x86)`). Building x64 fails with
   `LNK1112: module machine type 'x86' conflicts with target machine type 'x64'`.
   Always initialize the environment with `vcvarsall.bat x86`.

2. **Static multithreaded CRT.** The `*_mt.lib` archives link against `/MT`
   (`/MTd` for Debug). The top-level `CMakeLists.txt` sets this via
   `CMAKE_MSVC_RUNTIME_LIBRARY`. Changing it produces `LNK4098` /
   duplicate-symbol floods.

3. **`Visual Studio 17 2022` generator does not work here.** CMake 4.4 cannot
   detect the standalone BuildTools install and fails with
   `could not find any instance of Visual Studio`, even though `vswhere.exe`
   resolves it correctly. Use `NMake Makefiles` instead.

## Toolchain locations on this machine

| Component | Path |
| --- | --- |
| CMake | `C:\Program Files\CMake\bin\cmake.exe` (4.4.3) |
| vcvarsall | `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat` |
| MSVC | `...\BuildTools\VC\Tools\MSVC\14.44.35207` (cl 19.44) |
| dumpbin (x86) | `...\MSVC\14.44.35207\bin\Hostx86\x86\dumpbin.exe` |
| Windows SDK | `C:\Program Files (x86)\Windows Kits\10` (10.0.26100.0) |

Neither `cmake` nor `cl` is on the default `PATH`. Always invoke `cmake.exe` by
absolute path inside the `vcvarsall` shell — prepending
`C:\Program Files\CMake\bin` to `%PATH%` inside `cmd /c` breaks on the space in
the path.

## Build commands

Run from the repo root (`E:\Workspace\CurlDownloader`). PowerShell:

```powershell
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
$cmake  = "C:\Program Files\CMake\bin\cmake.exe"

# Configure (Release)
cmd /c "`"$vcvars`" x86 && `"$cmake`" -S . -B build -G `"NMake Makefiles`" -DCMAKE_BUILD_TYPE=Release"

# Build
cmd /c "`"$vcvars`" x86 && `"$cmake`" --build build"
```

Debug build: swap `-DCMAKE_BUILD_TYPE=Debug` and use a separate binary dir
(`-B build-debug`). `NMake Makefiles` is single-config, so Debug and Release
cannot share one build tree.

Clean reconfigure:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
```

## Expected output

| Artifact | Approx size |
| --- | --- |
| `build\bin\CurlDownloader.dll` | ~1.9 MB |
| `build\lib\CurlDownloader.lib` | ~31 KB |
| `build\lib\CurlDownloader.exp` | ~18 KB |

Build ends with `[100%] Built target CurlDownloader`.

## Verification

Confirm the export is present:

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x86\dumpbin.exe" /EXPORTS "build\bin\CurlDownloader.dll"
```

Expect the decorated symbol `?CurlDownloadFile@@YAHPBD0AA_J1H@Z`.

Check a vendored archive's architecture before debugging link errors:

```powershell
& "...\Hostx86\x86\dumpbin.exe" /HEADERS "CurlDownloader\CurlDownloader\dependency\curl\lib\libcurl_mt.lib" | Select-String machine
```

## CMake layout

- `CMakeLists.txt` (root) — project decl, C++14, `CMAKE_MSVC_RUNTIME_LIBRARY`,
  output dirs (`build/bin`, `build/lib`), `add_subdirectory`.
- `CurlDownloader/CurlDownloader/CMakeLists.txt` — `vendor::curl` and
  `vendor::tinyxml2` as `IMPORTED STATIC` targets with
  `IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"` selecting `libcurl_mt.lib` /
  `libcurld_mt.lib` and `tinyxml2_mt.lib` / `tinyxml2d_mt.lib`; the
  `CurlDownloader` SHARED target; install rules.

The legacy `.sln` / `.vcxproj` remain in the tree but are not the build of
record. `compile_flags.txt` feeds clangd for editor IntelliSense only.

## Known warnings (non-blocking)

- **C4005 x3** — `ELPP_STL_LOGGING`, `ELPP_NO_DEFAULT_LOG_FILE`, and
  `DOWNLOADER_EXPORT` are defined both in `src/stdafx.h` (lines 45, 46, 66) and
  in `target_compile_definitions`. Harmless; removing the three from
  `CMakeLists.txt` silences them.
- **C4700** at `src/CDownloadTaskManager.cpp:423` — uninitialized `dDecimal`.
  Pre-existing source defect, not a build-system issue.
- **C4244 x3** at `src/CLibcurlTool.cpp:35,40,41` — `__int64` narrowing.
  Pre-existing.

`legacy_stdio_definitions` is linked deliberately: `src/stdafx.cpp` reimplements
`__iob_func` for the pre-VS2015 CRT the vendored archives were built against.
Do not remove it.

## Troubleshooting

| Symptom | Cause / fix |
| --- | --- |
| `could not find any instance of Visual Studio` | VS generator unsupported here — use `-G "NMake Makefiles"`. |
| `'cmake' 不是内部或外部命令` | `cmake` not on `PATH`; use the absolute path, do not append to `%PATH%` inside `cmd /c`. |
| `LNK1112: module machine type conflict` | Built x64. Reconfigure in a fresh dir under `vcvarsall.bat x86`. |
| `LNK4098` / duplicate CRT symbols | CRT mismatch. Verify `CMAKE_MSVC_RUNTIME_LIBRARY` still resolves to `MultiThreaded[Debug]`. |
| Unresolved `curl_*` / `tinyxml2::*` | Check the `vendor::*` `IMPORTED_LOCATION*` paths and that `CURL_STATICLIB` is still on `vendor::curl`'s interface. |
| Unresolved `__iob_func` | `legacy_stdio_definitions` was dropped from the link line. |

## If the toolchain is missing

```powershell
winget install --id Kitware.CMake --accept-source-agreements --accept-package-agreements
winget install --id Microsoft.VisualStudio.2022.BuildTools --accept-source-agreements --accept-package-agreements --override "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

BuildTools takes a long time and pulls several GB. After it finishes, start a
fresh shell — `vswhere.exe` and the MSVC paths are not visible to already-running
shells.
