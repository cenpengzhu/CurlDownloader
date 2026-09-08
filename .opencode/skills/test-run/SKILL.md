---
name: test-run
description: 运行、调试或排查 CurlDownloader 的 GoogleTest 单元测试时使用。覆盖“运行测试”“跑一下测试”“ctest”“测试挂了/崩溃”“测试失败”、直接执行 CurlDownloaderTests.exe、用 ctest 跑全部用例、gtest_filter 按中文套件/用例名筛选、控制台中文用例名显示成问号/乱码的甄别（实为 UTF-8 输出被 GBK 解码）、测试进程以 0xC0000374/负数退出码崩溃（多半是任务对象 double free）、以及把失败断言定位到期望值写错还是组件行为变化。测试代码写法见 test-write，工程初始化见 test-init，构建见 test-build。
---

# CurlDownloader 测试运行

测试可执行文件为 `build-tests\bin\CurlDownloaderTests.exe`，共 81 个用例、11 个测试套，
全部离线确定性运行，整套耗时 < 1 秒（ctest 启动开销另计，实测总时长约 9~12 秒）。

## 前提

`CurlDownloaderTests.exe` 已构建（见 test-build 技能）。运行**不需要** vcvarsall
环境——它是已链接完成的可执行文件，直接跑即可；只有用 `ctest --test-dir` 时才建议
在 vcvarsall 内调用（与构建保持同一 shell 习惯，实际也非必需）。

## 三种运行方式

在仓库根目录 `E:\Workspace\CurlDownloader`：

1. 直接跑全部（看逐条 [ OK ]/[ FAILED ]）：

```powershell
& ".\build-tests\bin\CurlDownloaderTests.exe"
```

2. 只跑部分用例（按测试套/用例名通配；套件与用例名均为中文）：

```powershell
& ".\build-tests\bin\CurlDownloaderTests.exe" --gtest_filter="任务管理器测试.*"
& ".\build-tests\bin\CurlDownloaderTests.exe" --gtest_filter="*平均速度*:*回收任务*"
& ".\build-tests\bin\CurlDownloaderTests.exe" --gtest_list_tests   # 只列出全部用例名
```

> PowerShell 里中文过滤器要用引号包裹。套件/用例名中**不含空格与 `.`**（gtest 用
> `.` 分隔“套件.用例”），通配规则与英文完全一致。

3. 经 CTest 跑全部（CI 友好，输出逐用例 Passed/Failed）：

```powershell
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
cmd /c "`"$vcvars`" x86 && `"C:\Program Files\CMake\bin\ctest.exe`" --test-dir build-tests --output-on-failure"
```

正常结尾：`100% tests passed out of 81`。

## 结果判读

- gtest 直接运行的退出码：**0 = 全部通过**，**1 = 有用例失败**。
- 结尾汇总：`[  PASSED  ] 81 tests.`；失败时追加
  `[  FAILED  ] N tests, listed below:` 并列出用例全名。
- 用例失败行会给出文件:行号与期望值/实际值，例如：

```
...tests\test_cdownload_info.cpp(64): error: Expected equality of these values:
  1048576LL  ... Which is: 1048576
  info.getAverageSpeed()  Which is: 1048000
```

## 控制台中文用例名显示成问号/乱码

本项目套件/用例名是中文（见 test-write 技能），gtest 输出为**标准 UTF-8**。在
Windows PowerShell 里直接跑或经管道（`|`、`>`、`Select-Object`）捕获时，常被按
系统代码页（GBK/936）解码，于是行末出现 `?` 或乱码：

```
[ RUN      ] 线程管理器测?全部忙碌时回收线程为空操?
```

这**只是显示问题，用例名本身完好**，不影响注册、过滤、通过与否。核实方法：

```powershell
# 不经 PowerShell 管道直接重定向，再按 UTF-8 读回
cmd /c "build-tests\bin\CurlDownloaderTests.exe --gtest_list_tests > %TEMP%\list.txt 2>&1"
[System.IO.File]::ReadAllText("$env:TEMP\list.txt", [System.Text.Encoding]::UTF8)
```

或先 `chcp 65001` 切到 UTF-8 代码页再运行。判断是否真失败只看退出码与结尾汇总行，
不要被乱码误导。

## 排查失败的用例

1. **先判断是断言写错还是组件 bug。** 本仓库测试里有一类“特征化断言”专门固化
   README「已知问题」记录的既有缺陷（每分片多计 1 字节、平均速度先除后乘丢精度、
   `writeData` 返回 nmemb 遇 NUL 截断等），文件内有注释说明。新增用例时若期望值按
   “理想行为”写，就会失败——核对注释与源码实际逻辑后修正期望值，不要为了让测试变绿
   去改组件源码，除非本次任务本来就是修该 bug。
2. **只重跑失败用例**定位：`--gtest_filter="<失败用例全名>"`。
3. 改完测试或源码后必须**重新构建**（test-build 技能）再跑，可执行文件不会自动更新。

## 进程崩溃（退出码为大负数 / 0xC0000374）

若 gtest 输出在某个用例中途截断、没有汇总行，且退出码类似
`-1073740940`（即 `0xC0000374 STATUS_HEAP_CORRUPTION`）或其它 `0xC0000xxx`，**不是
随机 flaky**，基本都是测试代码的内存所有权错误：

- **最常见：CTask 对象 double free。** `CTaskManager::clearTask()`（及子类
  `clearDownloadTask()`）会 `delete` 掉 `m_vecPTasks` 中的全部任务指针。fixture 若
  既把任务 `pushOneTask()` 进了 manager、又在 `TearDown()` 里 `delete` 同一批指针，
  就会二次释放。正确做法：TearDown 先记下 manager 持有的指针集合，调 `clearTask()`
  后只 delete 未进 manager 的那些（参照 `tests/test_ctask_manager.cpp` 的
  `任务管理器测试::TearDown`）。
- 测试中手动调过 `clearTask()` 后，务必把本地保存的裸指针容器里对应的悬垂指针移除，
  别让 TearDown 再碰。
- 崩溃位置可用 `--gtest_filter` 二分：先跑到哪个测试套为止正常，即可锁定肇事 fixture。

## 其它注意

- 测试会在 `%TEMP%` 下创建 `curldl_test_<PID>_<n>.xml/.bin` 临时文件，fixture 析构时
  删除；调试时若进程被强杀可能残留，可手动清理 `%TEMP%\curldl_test_*`。
- 测试不发网络请求、不监听端口；若某用例意外耗时数秒以上，说明误触发了真实网络
  （例如直接调了 `getContentLength()`/`loadDownloadTask()`），应改为注入数据。
- 组件日志在 `test_main.cpp` 中被全局关闭，正常运行**不应**有 easylogging++ 输出；
  若看到日志刷屏，说明 main 的 reconfigure 没生效。
- ctest 用例由 `gtest_discover_tests(... DISCOVERY_MODE PRE_TEST)` 在测试运行前枚举，
  新增/重命名用例后无需手动注册，但要先重新构建。
