# CurlDownloader

Windows 平台上基于 libcurl 的多线程断点续传下载组件（DLL）。

- 语言／标准：C++14
- 目标平台：Windows **x86（Win32）**，静态多线程 CRT（`/MT`）
- 构建系统：CMake（>= 3.15）+ MSVC
- 第三方依赖：libcurl、tinyxml2（随仓库携带预编译静态库）、easylogging++（源码内联）

## 功能

| 能力 | 说明 |
| --- | --- |
| 文件大小探测 | HEAD 请求解析 `Content-Range` / `Content-Length`，循环跟随 `Location` 重定向 |
| 分块切分 | 按 1 MB 切分为下载任务，不足 1 MB 记为一个任务 |
| 并发下载 | 固定线程池，通过 HTTP `Range` 分段拉取（对外 API 内部固定 10 线程） |
| 单文件随机写 | 所有工作线程共享同一 `FILE*`，由命名互斥量串行化 `fseek` + `fwrite` |
| 断点续传 | 任务进度持久化到 `<目标文件>.xml`，重启后加载并校验，校验失败则重新切分 |
| 进度与测速 | 提供已下载字节数、百分比、瞬时速度、平均速度 |
| 失败检测 | 网络断开（`InternetGetConnectedState`）、连续 30 秒零速度 |
| 日志 | easylogging++ 输出到 `<目标文件>.log` 及标准输出 |

支持 HTTP / HTTPS（不校验证书与主机名）。

## 对外接口

组件仅导出一个函数，声明位于 `CurlDownloader/CurlDownloader/src/CurlDownloader.h`：

```cpp
int __declspec(dllexport) CurlDownloadFile(
    const char* szURL,            // in  远程 URL
    const char* szFilePath,       // in  本地保存路径
    long long&  llTotalContent,   // out 文件总字节数
    long long&  llCurrentContent, // out 已下载字节数
    int         ThreadCount = 1); // in  当前实现忽略此参数，内部固定 10 线程
```

返回值：

| 值 | 含义 |
| --- | --- |
| `1` | 下载成功 |
| `-1` | 下载器初始化失败，或结束时文件不完整 |
| `-2` | 网络连接失败 |
| `-3` | 超过 30 秒无下载速度 |

调用是**同步阻塞**的，函数返回即表示下载结束。`llTotalContent` /
`llCurrentContent` 在下载过程中被持续写入，可由另一线程读取以展示进度。

未使用 `extern "C"`，导出的是 C++ 修饰名 `?CurlDownloadFile@@YAHPBD0AA_J1H@Z`。

副产物：下载过程中会在目标文件同目录生成 `<目标文件>.xml`（续传信息）与
`<目标文件>.log`（日志）。

## 构建

工具链要求见 `.opencode/skills/release-build/SKILL.md`。核心约束：

1. **必须 x86。** 随仓库携带的 `dependency/` 预编译库均为 32 位，构建 x64 会报
   `LNK1112`。
2. **必须静态 MT CRT。** 由根 `CMakeLists.txt` 的 `CMAKE_MSVC_RUNTIME_LIBRARY`
   设定，改动会引发 `LNK4098` 与重复符号。
3. **使用 `NMake Makefiles` 生成器。** 本机 CMake 无法探测独立安装的 VS
   BuildTools，`Visual Studio 17 2022` 生成器不可用。

在仓库根目录执行（PowerShell）：

```powershell
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
$cmake  = "C:\Program Files\CMake\bin\cmake.exe"

cmd /c "`"$vcvars`" x86 && `"$cmake`" -S . -B build -G `"NMake Makefiles`" -DCMAKE_BUILD_TYPE=Release"
cmd /c "`"$vcvars`" x86 && `"$cmake`" --build build"
```

Debug 构建请改用独立目录（`-B build-debug -DCMAKE_BUILD_TYPE=Debug`），
`NMake Makefiles` 为单配置生成器。

产物：

| 文件 | 大小（约） |
| --- | --- |
| `build\bin\CurlDownloader.dll` | 1.9 MB |
| `build\lib\CurlDownloader.lib` | 31 KB |
| `build\lib\CurlDownloader.exp` | 18 KB |

`install` 目标会输出 `bin/CurlDownloader.dll`、`lib/CurlDownloader.lib` 与
`include/CurlDownloader.h`。

旧的 `.sln` / `.vcxproj` 仍在仓库中保留，但不再是权威构建方式；
`compile_flags.txt` 仅供 clangd 做编辑器补全。

## 架构

### 逻辑视图

```
CurlDownloadFile                     API 门面：配置日志、驱动下载、轮询状态
      │
      ▼
 CDownloader ──────────── CDownloadInfo          进度/速度值对象
   │  控制层：DOWNLOAD_RUN / PAUSE / STOP 状态机
   │  独立的 downloaderThread 调度循环
   ├──► CDownloadTaskManager : CTaskManager      任务域
   │        └── CDownloadTask : CTask            1 MB 分片
   │        └── tinyxml2 ──► <目标文件>.xml      续传持久化
   └──► CDownloadThreadManager : CThreadManager  线程域
            └── CDownloadThread : CThread        工作线程
                     └── CLibcurlTool ──► libcurl easy 接口
```

任务域与线程域相互解耦：基类 `CTask` / `CTaskManager` / `CThread` /
`CThreadManager` 提供通用的「线程池 + 任务队列」骨架，`CDownload*` 子类通过
`taskBusiness()`、`createThreads()` 等虚函数注入下载语义。

调度循环（`CDownloader::downloaderThread`）：回收暂停线程 → 将失败任务放回待办
队列 → 刷新下载信息 → 为空闲线程分配待办任务 → 每推进 10% 落盘一次 XML。

状态定义：

- 任务：`TASK_TODO(0)`、`TASK_COMPLETE(1)`、`TASK_DONE(失败, 2)`、`TASK_DOING(3)`
- 线程：`THREAD_STOP(0)`、`THREAD_RUN(1)`、`THREAD_PAUSE(2)`
- 下载：`DOWNLOAD_STOP(0)`、`DOWNLOAD_RUN(1)`、`DOWNLOAD_PAUSE(2)`

### 开发视图

```
CMakeLists.txt                        顶层：C++14、MT CRT、输出目录、compile_commands
CurlDownloader/CurlDownloader/
  CMakeLists.txt                      SHARED 库目标 + vendor::curl / vendor::tinyxml2
  CurlDownloader.vcxproj              遗留 VS 工程（非权威）
  dependency/curl/                    预编译 libcurl_mt / libcurld_mt + libssh2（x86）
  dependency/tinyxml2/                预编译 tinyxml2_mt / tinyxml2d_mt
  src/
    CurlDownloader.h/.cpp             导出 API、DllMain、日志配置
    CDownloader.h/.cpp                下载控制器与调度线程
    CTask.h、CDownloadTask.h          任务模型
    CTaskManager.h、CDownloadTaskManager.h/.cpp   任务管理与 XML 持久化
    CThread.h、CDownloadThread.h/.cpp             线程模型
    CThreadManager.h、CDownloadThreadManager.h    线程池与共享文件句柄
    CLibcurlTool.h/.cpp               libcurl 封装与写回调
    stdafx.h/.cpp                     预编译头、链库声明、__iob_func 兼容垫片
    easylogging++、md5                第三方源码
```

层次依赖单向：API → 控制 → （任务 | 线程）→ 工具 → 第三方。
`stdafx.cpp` 为随仓库携带的旧 CRT 静态库重新实现了 `__iob_func`，链接行中的
`legacy_stdio_definitions` 不可移除。

## 数据设计

内存结构：

| 类型 | 关键字段 |
| --- | --- |
| `CDownloadTask` | `m_nTaskId`、`m_nTaskStatus`、`m_llStartPos`、`m_llEndPos`、`m_llDownloadedPos`、`m_strRemotePath`、`m_strLocalPath` |
| `CDownloadTaskManager` | `m_llContentLength`、`m_llDownloadTime`、`m_nTasksCount`，以及全部／已完成／待办三个 `vector<CTask*>` |
| `CDownloadInfo` | `m_dwTime`、`m_llTotalDownloadedLength`、`m_dPercent`、`m_strSpeed`、`m_llTotalDownloadTime`、`m_strAverageSpeed` |
| `errorcode` | `noerror=1`、`inconsisdent`、`dividerror`、`downloadederror`、`filerror`、`remotefilerror`、`localfilerror` |

续传信息文件 `<目标文件>.xml`：

```xml
<TaskInfo>
  <TasksCount>N</TasksCount>
  <ContentLength>字节数</ContentLength>
  <DownloadTime>累计毫秒</DownloadTime>
  <Task>
    <StartPos/><EndPos/><DownloadedPos/>
    <RemotePath/><LocalPath/><TaskStatus/><TaskId/>
  </Task>
  <!-- Task 重复 N 次 -->
</TaskInfo>
```

续传校验规则（`CDownloadTaskManager::checkTaskInfo`）：

1. 远端 `Content-Length` 必须与文件中记录的一致；
2. 每个分片满足 `StartPos == (TaskId - 1) * 1MB`；
3. 相邻分片首尾连续；
4. 末片 `EndPos + 1 == ContentLength + 1`。

任一条不满足则清空任务并按远端文件重新切分。

磁盘文件：目标文件已存在时以 `rb+` 打开，否则以 `wb+` 新建；各线程按
`DownloadedPos` 偏移随机写入。

## 已知问题

- `CurlDownloader.cpp` 零速度判断分支误用赋值 `=` 而非比较 `==`。
- `CDownloadTaskManager::getTotalDownloadedLength()` 每个分片多计 1 字节。
- `CurlDownloadFile` 的 `ThreadCount` 参数未生效。
- `CDownloadThreadManager::m_pLocalFile` 在构造函数中未初始化。
- `md5.cpp/h` 参与编译但无任何调用点。
- 构建告警：`C4005`（宏重复定义）、`C4700`（`CDownloadTaskManager.cpp` 中
  `dDecimal` 未初始化）、`C4244`（`CLibcurlTool.cpp` 中 `__int64` 窄化）。
