---
name: test-build
description: 在 Windows 上配置并编译 CurlDownloader 的 GoogleTest 测试工程时使用。覆盖“构建测试”“编译测试”“build 测试”“测试编译不过”、CMake 配置测试工程（FetchContent 拉取 gtest）、NMake 构建 CurlDownloaderTests、测试目标链接 OBJECT 库与 MT CRT 的报错排查。测试工程的初始化/接入见 test-init 技能，跑测试见 test-run 技能；x86/MT/vcvarsall 通用约束见 release-build 技能。
---

# CurlDownloader 测试构建

测试工程是根 CMake 的一个可选目标，由 `CURLDOWNLOADER_BUILD_TESTS`（默认 ON）控制，
产物是 `CurlDownloaderTests.exe`。它与正式 DLL 共享同一套工具链约束（x86、静态 MT、
NMake Makefiles、vcvarsall），**先读 release-build 技能的硬约束**，本技能只讲测试
侧的增量内容。

## 构建目录

测试用**独立**构建目录 `build-tests/`，不要和正式发布目录 `build/` 混用（两者 CMake
缓存选项不同，且首次配置会下载 gtest）。该目录已被 git 忽略（`build*/`），属生成物，
用完可直接删。

## 配置（首次较慢）

在仓库根目录 `E:\Workspace\CurlDownloader`，PowerShell：

```powershell
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
$cmake  = "C:\Program Files\CMake\bin\cmake.exe"

cmd /c "`"$vcvars`" x86 && `"$cmake`" -S . -B build-tests -G `"NMake Makefiles`" -DCMAKE_BUILD_TYPE=Release"
```

首次配置会经 FetchContent 用 git 浅克隆 googletest v1.14.0 到
`build-tests/_deps/`，耗时主要在下载（本机实测约 3~4 分钟，取决于 GitHub 连通性；
网络不通见 github-push 技能的 hosts/SSH 链路）。配置成功结尾为：

```
-- Configuring done (...)
-- Generating done (...)
-- Build files have been written to: E:/Workspace/CurlDownloader/build-tests
```

之后增删测试文件、改 `tests/CMakeLists.txt` 不需要手动重配，`cmake --build` 会自动
重新运行 CMake。

Debug 测试构建：`-B build-tests-debug -DCMAKE_BUILD_TYPE=Debug`（NMake 单配置，
Debug/Release 不能共用目录；Debug 下 gtest 与组件都切 `/MTd`）。

## 编译

```powershell
cmd /c "`"$vcvars`" x86 && `"$cmake`" --build build-tests"
```

预期目标依次出现：

```
[ 27%] Built target CurlDownloader_core     # OBJECT 库（组件核心源码）
[ 34%] Built target CurlDownloader          # 正式 DLL
[ 41%] Built target gtest                   # gtest/gmock 静态库
[ 68%] Built target CurlDownloaderTests     # ← 测试可执行文件
... gtest_main / gmock ...
[100%] Built target gtest_main
```

产物：`build-tests\bin\CurlDownloaderTests.exe`。
（输出目录由根 CMake 统一到 `build-tests/bin`、`build-tests/lib`。）

首次编译 gtest 时会刷若干行 `cl : command line warning D9025: 正在重写“/W4”...用“/W0”`，
这是 tests/CMakeLists 有意把 gtest 的告警级别压到 /W0，**属正常**。

组件源码侧的 C4005（宏重定义）、C4244（`__int64` 窄化）、C4700（`dDecimal` 未初始化）
都是 release-build 技能记录的既有警告，不阻塞。

## 只重编测试（迭代时）

改了 `tests/*.cpp` 后，`cmake --build build-tests` 只重编受影响文件并重链
`CurlDownloaderTests`，通常几秒内完成；改组件 `src/` 则会先重编
`CurlDownloader_core`。

## 常见构建报错

| 现象 | 原因 / 处理 |
| --- | --- |
| `could not find any instance of Visual Studio` | 用了 VS 生成器；必须 `-G "NMake Makefiles"`。 |
| `LNK1112 module machine type 'x86' conflicts ... x64` | 误用 x64 环境；`vcvarsall.bat x86` 后用全新目录重配。 |
| 配置期卡在 `Cloning into ... googletest` 后失败 | GitHub 网络不通；按 github-push 技能打通 hosts/SSH 后重试，或删 `build-tests/_deps` 重来。 |
| 编译 gtest 报需要 C++17 | gtest 被解析成了新版；确认 `GIT_TAG v1.14.0`，删 `build-tests` 全量重配。 |
| 测试目标 `LNK2019: el::base::...` / `__iob_func` | 没链到 `CurlDownloader_core`（漏了 OBJECT 库或 legacy_stdio_definitions）；检查 tests/CMakeLists 的 `target_link_libraries`。 |
| `C2664 CreateMutexW ... const char[25] 无法转 LPCWSTR` | 测试目标缺 `UNICODE/_UNICODE` 宏（见 test-init 技能）。 |
| LNK4098 / CRT 重复符号 | `gtest_force_shared_crt` 未设 OFF，或被别处改成了 ON。 |
| 改了 tests/CMakeLists 但没生效 | NMake 下 `cmake --build` 会自动重配；若怀疑缓存脏，删 `build-tests/CMakeCache.txt` 或整个目录重配。 |

## 验证构建结果

```powershell
Test-Path build-tests\bin\CurlDownloaderTests.exe
```

构建通过不等于测试通过——编译链接成功后按 test-run 技能执行
`CurlDownloaderTests.exe` 或 `ctest`。
