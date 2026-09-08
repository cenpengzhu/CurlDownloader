---
name: test-init
description: 在 CurlDownloader 项目中初始化或改造 GoogleTest 单元测试工程时使用。覆盖“初始化测试工程”“搭建 gtest”“新增测试工程”“加单元测试”“gtest 怎么接入 CMake”，以及 FetchContent 拉取 gtest、gtest 与 C++14/静态 MT CRT 的版本兼容、OBJECT 库让 DLL 与测试共用源码、test_main.cpp 的 easylogging++ 初始化、测试用例离线化与内存所有权约定。测试代码统一中文书写（中文套件/用例/夹具名，依赖 /utf-8），具体命名与写法见 test-write 技能。
---

# CurlDownloader 测试工程初始化

测试技术栈：**GoogleTest 1.14.0 + CMake FetchContent + CTest**。本技能记录本项目
接入 gtest 时验证过的全部关键决策；`tests/` 目录已按此落地，新增测试文件或重建
工程时照此办理。

## 硬约束 —— 先读这里

1. **gtest 必须钉在 v1.14.0。** 它是最后一个支持 C++14 的发布版；v1.15 起最低要求
   C++17，而本项目 `CMAKE_CXX_STANDARD 14`，用新版会在编译 gtest 时直接报错。
2. **`gtest_force_shared_crt` 必须为 OFF。** 本项目全局静态 MT CRT（见 release-build
   技能）。若让 gtest 走默认的 `/MD` 动态 CRT，链接测试可执行文件时会报 LNK4098 与
   CRT 符号冲突。OFF 时 gtest 继承根 CMake 的 `CMAKE_MSVC_RUNTIME_LIBRARY`。
3. **测试链接的是 OBJECT 库，不是 DLL。** 组件 CMake 已把除 `src/CurlDownloader.cpp`
   （导出 API + DllMain）外的全部源码拆为 `CurlDownloader_core` OBJECT 库，DLL 与
   测试共用同一份编译产物。测试**不要**链接 `CurlDownloader.dll`，也不要把
   `CurlDownloader.cpp` 编进测试。
4. **x86 / NMake / vcvarsall 约束与正式构建完全相同**，见 release-build 技能。

## 工程结构（现状即模板）

```
CMakeLists.txt                       根：option(CURLDOWNLOADER_BUILD_TESTS ON) + enable_testing() + add_subdirectory(tests)
CurlDownloader/CurlDownloader/
  CMakeLists.txt                     定义 CurlDownloader_core(OBJECT) 与 CurlDownloader(SHARED)
tests/
  CMakeLists.txt                     FetchContent gtest + 测试可执行文件 + gtest_discover_tests
  test_main.cpp                      main()、INITIALIZE_EASYLOGGINGPP、关闭日志输出
  test_<模块名>.cpp                  每个被测模块一个文件，文件名与源文件对应
```

## tests/CMakeLists.txt 要点

```cmake
include(FetchContent)
set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON  CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(googletest)

# gtest 自身按 /W4 编译，在本项目 /W3 下刷 D9025 与大量告警，压到 /W0
foreach(t gtest gtest_main gmock gmock_main)
    if(TARGET ${t})
        target_compile_options(${t} PRIVATE /W0 /utf-8)
    endif()
endforeach()

add_executable(CurlDownloaderTests <测试源文件...>)
target_link_libraries(CurlDownloaderTests PRIVATE CurlDownloader_core GTest::gtest)
target_compile_definitions(CurlDownloaderTests PRIVATE
    WIN32 _WINDOWS UNICODE _UNICODE
    DOWNLOADER_STATIC            # 包含 CurlDownloader.h 时让 DOWNLOADERAPI 为空，而非 dllimport
    _CRT_SECURE_NO_WARNINGS ELPP_STL_LOGGING ELPP_NO_DEFAULT_LOG_FILE)
target_compile_options(CurlDownloaderTests PRIVATE /W3 /MP /utf-8)

include(GoogleTest)
gtest_discover_tests(CurlDownloaderTests DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 120)
```

关键解释：

- `target_link_libraries(... CurlDownloader_core)` 链接 OBJECT 库会**同时**带入它的
  对象文件与全部使用需求（src  include 目录、vendor::curl、vendor::tinyxml2、
  Wininet/winmm/comctl32、legacy_stdio_definitions），测试侧无需重复声明。
- 组件 CMake 里**每一个**使用 `stdafx.h` 的目标（`_core`、DLL、测试）都必须带
  `UNICODE/_UNICODE` 等同一套编译宏。`CThreadManager.h` 里的 `CreateMutexW` +
  `_T(...)` 缺 `UNICODE` 会报 `C2664: const char[25] 无法转换为 LPCWSTR`。
- `DISCOVERY_MODE PRE_TEST` 让用例在 ctest 运行时才枚举注册，避免构建期跨环境探测
  失败；超时设 120 秒兜底。
- 测试目标的 `/utf-8` 编译选项**不能省**：本项目测试代码统一用中文书写套件名/用例名/
  夹具名（见 test-write 技能），MSVC 需按 UTF-8 解析源文件才能接受中文标识符；gtest 的
  `TEST(套件, 用例)` 宏内部 `##` 拼接中文已实测可正常注册运行。源文件也必须存成 UTF-8。

## test_main.cpp 要点

```cpp
#include <stdafx.h>
#include <gtest/gtest.h>

// easylogging++ 的全局存储定义在 CurlDownloader.cpp（API TU）里，测试不链接它，
// 必须在测试二进制中补一份，否则 core 源码里的 LOG() 链接失败。
INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    el::Configurations conf;
    conf.setToDefault();
    conf.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
    conf.setGlobally(el::ConfigurationType::ToFile, "false");
    el::Loggers::reconfigureAllLoggers(conf);   // 压掉组件日志，保持 gtest 输出干净
    return RUN_ALL_TESTS();
}
```

用自带 `main` 而非 `gtest_main`，就是为了插入上面两行初始化。

## 编写用例的约定（本项目踩过的坑）

> 测试**代码用中文书写**：套件名/用例名/夹具类名/测试自造的辅助方法与变量/注释用中文；
> 被测类名、成员名、宏、枚举、gtest 断言宏保留英文。完整命名边界、文件骨架与自检清单见
> **test-write 技能**。下面只列与工程接入强相关的约定。

1. **每个测试文件第一行 `#include <stdafx.h>`**，再 include 被测头与 gtest。
2. **全部离线、确定性**：
   - 需要文件时在 `%TEMP%` 生成唯一名（`GetTempPathA` + PID + 自增计数器），
     fixture 析构里 `remove()`，参考 `test_cdownload_task_manager.cpp` 的 `TempFile`。
   - 不发网络请求：任务/线程对象直接 `new` 后注入字段，不经过
     `generateDownloadTask()`/`createThreads()`（后者会真的 HEAD 请求、真的建线程）。
   - `checkTaskInfo()` 等联网函数只测其离线分支（对不可达主机
     `getContentLength()` 返回 0，据此构造断言）。
3. **内存所有权**：`CTaskManager::clearTask()` 会 `delete` 掉 `m_vecPTasks` 里的全部
   任务。fixture 的 TearDown 只能 delete **未进 manager** 的任务；进了 manager 的
   交给 `clearTask()`，否则 double free，测试进程以 `0xC0000374` 堆崩溃退出（详见
   test-run 技能）。
4. **不启动真实工作线程**：线程池测试只操作 `m_vecFreeThreads/m_vecBusyThreads`
   簿记与状态字段；`CThread` 构造对象可以，但不要调 `init()`（会 `_beginthreadex`）。
5. **既有缺陷写“特征化断言”**：README「已知问题」里的 bug（如
   `getTotalDownloadedLength()` 每分片多计 1 字节、`getAverageSpeed()` 先除后乘丢
   精度、`writeData` 返回 `nmemb` 且遇 NUL 截断）按**当前实际行为**断言并加注释，
   日后修复时测试会立刻提醒行为变化。
6. **断言字面量类型**：`size()` 返回无符号，`EXPECT_EQ(1u, vec.size())`，避免
   signed/unsigned 比较告警。
7. **新增测试文件**后把文件名加进 `tests/CMakeLists.txt` 的
   `CURLDOWNLOADER_TEST_SOURCES`；NMake 构建会自动检测 CMakeLists 变化并重新配置。

## 故障排查

| 现象 | 原因 / 处理 |
| --- | --- |
| 编译 gtest 报 C++17 相关错误 | gtest 版本高于 v1.14.0；钉回 `GIT_TAG v1.14.0` 后清掉 `build-tests/_deps` 重配。 |
| LNK4098 / CRT 库冲突、重复符号 | `gtest_force_shared_crt` 没关；确认三处 set(... FORCE) 都在。 |
| LNK1112 machine type conflict | 误用 x64；必须 `vcvarsall.bat x86`（见 release-build）。 |
| `C2664 ... CreateMutexW ... LPCWSTR` | 测试目标漏 `UNICODE/_UNICODE` 宏。 |
| `unresolved external symbol el::base::...` 或 `INITIALIZE_EASYLOGGINGPP` 相关 | test_main.cpp 漏 `INITIALIZE_EASYLOGGINGPP`。 |
| FetchContent 拉取超时/失败 | 本机 GitHub 访问问题见 github-push 技能（SSH/hosts 链路打通后 HTTPS git clone 同样受益）。 |
