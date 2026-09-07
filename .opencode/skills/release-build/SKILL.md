---
name: release-build
description: 在 Windows 上构建、编译、配置或排查 CurlDownloader 项目时使用。覆盖 CMake 配置与构建、Release 与 Debug 构建、“帮我构建”“编译一下”“build 一下”、链接错误（LNK2019/LNK1112/LNK4098）、CRT 运行库不匹配、x86 与 x64 架构冲突，以及“could not find any instance of Visual Studio”。包含本机确切的工具链路径、随仓库携带的 libcurl/tinyxml2 预编译库导致的 x86 专用约束，以及本机必需的 vcvarsall + NMake 变通方案。
---

# CurlDownloader Release 构建

仅支持 Windows 的 C++14 DLL，使用 CMake + MSVC 构建。本技能记录了工具链布局、
架构硬约束，以及本项目必需的生成器变通方案。

## 硬约束 —— 先读这里

1. **仅支持 x86（Win32）。** `CurlDownloader/CurlDownloader/dependency/` 下随仓库
   携带的预编译库均为 32 位（`dumpbin /HEADERS` 显示 `14C machine (x86)`）。
   构建 x64 会失败并报
   `LNK1112: module machine type 'x86' conflicts with target machine type 'x64'`。
   务必用 `vcvarsall.bat x86` 初始化环境。

2. **静态多线程 CRT。** `*_mt.lib` 系列库链接的是 `/MT`（Debug 为 `/MTd`）。
   根 `CMakeLists.txt` 通过 `CMAKE_MSVC_RUNTIME_LIBRARY` 设定该运行库，改动它会
   引发 `LNK4098` 与大量重复符号错误。

3. **`Visual Studio 17 2022` 生成器在本机不可用。** CMake 4.4 无法探测独立安装的
   BuildTools，会报 `could not find any instance of Visual Studio`，即使
   `vswhere.exe` 能正确定位也一样。改用 `NMake Makefiles`。

## 本机工具链位置

| 组件 | 路径 |
| --- | --- |
| CMake | `C:\Program Files\CMake\bin\cmake.exe`（4.4.3） |
| vcvarsall | `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat` |
| MSVC | `...\BuildTools\VC\Tools\MSVC\14.44.35207`（cl 19.44） |
| dumpbin（x86） | `...\MSVC\14.44.35207\bin\Hostx86\x86\dumpbin.exe` |
| Windows SDK | `C:\Program Files (x86)\Windows Kits\10`（10.0.26100.0） |

`cmake` 和 `cl` 都不在默认 `PATH` 中。始终在 `vcvarsall` 环境内用绝对路径调用
`cmake.exe` —— 在 `cmd /c` 内把 `C:\Program Files\CMake\bin` 追加到 `%PATH%` 会
因路径含空格而失败。

## 构建命令

在仓库根目录（`E:\Workspace\CurlDownloader`）执行。PowerShell：

```powershell
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
$cmake  = "C:\Program Files\CMake\bin\cmake.exe"

# 配置（Release）
cmd /c "`"$vcvars`" x86 && `"$cmake`" -S . -B build -G `"NMake Makefiles`" -DCMAKE_BUILD_TYPE=Release"

# 构建
cmd /c "`"$vcvars`" x86 && `"$cmake`" --build build"
```

Debug 构建：换成 `-DCMAKE_BUILD_TYPE=Debug` 并使用独立的构建目录
（`-B build-debug`）。`NMake Makefiles` 是单配置生成器，Debug 与 Release 不能共用
同一个构建树。

清理后重新配置：

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
```

## 预期产物

| 产物 | 大致大小 |
| --- | --- |
| `build\bin\CurlDownloader.dll` | 约 1.9 MB |
| `build\lib\CurlDownloader.lib` | 约 31 KB |
| `build\lib\CurlDownloader.exp` | 约 18 KB |

构建以 `[100%] Built target CurlDownloader` 结束。

## 验证

确认导出符号存在：

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x86\dumpbin.exe" /EXPORTS "build\bin\CurlDownloader.dll"
```

应能看到修饰后的符号 `?CurlDownloadFile@@YAHPBD0AA_J1H@Z`。

排查链接错误前，先检查随仓库携带库的架构：

```powershell
& "...\Hostx86\x86\dumpbin.exe" /HEADERS "CurlDownloader\CurlDownloader\dependency\curl\lib\libcurl_mt.lib" | Select-String machine
```

## CMake 结构

- `CMakeLists.txt`（根）—— 工程声明、C++14、`CMAKE_MSVC_RUNTIME_LIBRARY`、
  输出目录（`build/bin`、`build/lib`）、`add_subdirectory`。
- `CurlDownloader/CurlDownloader/CMakeLists.txt` —— 将 `vendor::curl` 与
  `vendor::tinyxml2` 声明为 `IMPORTED STATIC` 目标，通过
  `IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"` 分别选用 `libcurl_mt.lib` /
  `libcurld_mt.lib` 与 `tinyxml2_mt.lib` / `tinyxml2d_mt.lib`；定义
  `CurlDownloader` SHARED 目标；以及 install 规则。

旧的 `.sln` / `.vcxproj` 仍保留在仓库中，但不再是权威构建方式。
`compile_flags.txt` 仅用于给 clangd 提供编辑器智能补全。

## 已知警告（不阻塞构建）

- **C4005 三处** —— `ELPP_STL_LOGGING`、`ELPP_NO_DEFAULT_LOG_FILE` 与
  `DOWNLOADER_EXPORT` 在 `src/stdafx.h`（第 45、46、66 行）与
  `target_compile_definitions` 中重复定义。无害；从 `CMakeLists.txt` 中移除这三个
  宏即可消除警告。
- **C4700** 位于 `src/CDownloadTaskManager.cpp:423` —— `dDecimal` 未初始化。
  属源码既有缺陷，与构建系统无关。
- **C4244 三处** 位于 `src/CLibcurlTool.cpp:35,40,41` —— `__int64` 窄化转换。
  既有问题。

`legacy_stdio_definitions` 是刻意链接的：`src/stdafx.cpp` 为随仓库携带库所依赖的
VS2015 之前版本 CRT 重新实现了 `__iob_func`。不要移除。

## 故障排查

| 现象 | 原因 / 处理 |
| --- | --- |
| `could not find any instance of Visual Studio` | 本机不支持 VS 生成器 —— 改用 `-G "NMake Makefiles"`。 |
| `'cmake' 不是内部或外部命令` | `cmake` 不在 `PATH` 中；使用绝对路径，不要在 `cmd /c` 内追加 `%PATH%`。 |
| `LNK1112: module machine type conflict` | 误按 x64 构建。在 `vcvarsall.bat x86` 下用全新目录重新配置。 |
| `LNK4098` / CRT 符号重复 | 运行库不匹配。确认 `CMAKE_MSVC_RUNTIME_LIBRARY` 仍解析为 `MultiThreaded[Debug]`。 |
| `curl_*` / `tinyxml2::*` 无法解析 | 检查 `vendor::*` 的 `IMPORTED_LOCATION*` 路径，以及 `CURL_STATICLIB` 是否仍在 `vendor::curl` 的接口定义上。 |
| `__iob_func` 无法解析 | 链接行中漏掉了 `legacy_stdio_definitions`。 |

## 工具链缺失时

```powershell
winget install --id Kitware.CMake --accept-source-agreements --accept-package-agreements
winget install --id Microsoft.VisualStudio.2022.BuildTools --accept-source-agreements --accept-package-agreements --override "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

BuildTools 安装耗时较长且需下载数 GB。安装完成后请开启新的 shell ——
`vswhere.exe` 与 MSVC 路径对已运行的 shell 不可见。
