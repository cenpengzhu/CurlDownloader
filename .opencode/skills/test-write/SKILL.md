---
name: test-write
description: 在 CurlDownloader 项目中编写或改写 GoogleTest 单元测试代码时使用。覆盖“写测试”“加测试用例”“新增单元测试”“把测试改成中文”“中文测试名/中文注释”“TEST/TEST_F 怎么命名”，以及中文套件名/用例名/夹具的写法与技术前提、被测符号保留英文的边界、测试文件骨架、离线确定性/内存所有权/特征化断言等编写约定，和控制台中文显示成问号的甄别。工程接入见 test-init，构建见 test-build，运行见 test-run。
---

# CurlDownloader 测试代码编写

本技能讲**怎么写一个测试文件/用例**（命名、语言、骨架、断言约定）。工程怎么接入
CMake/gtest 见 test-init，怎么编译见 test-build，怎么跑/排查见 test-run。

## 核心语言约定：测试代码用中文书写

本仓库 `tests/` 下的测试**统一用中文书写**，已在 MSVC + gtest 1.14.0 下验证可行
（81 个用例全部通过）。具体边界：

| 元素 | 用什么语言 | 例子 |
| --- | --- | --- |
| 测试套件名（`TEST`/`TEST_F` 第一个参数） | **中文** | `任务管理器测试`、`线程测试`、`下载信息测试` |
| 测试用例名（第二个参数） | **中文** | `构造后处于暂停状态`、`压入一个待办任务` |
| 夹具类名（`class ... : public ::testing::Test`） | **中文** | `class 下载任务管理器测试 : public ::testing::Test` |
| 夹具内辅助方法 / 局部变量（测试自造的） | **中文** | `添加分片(...)`、`排布分片(...)`、`m_已持有线程` |
| 注释 | **中文** | 说明意图、离线化原因、特征化断言锁定的缺陷 |
| **被测类/结构体名** | **保持英文** | `CTask`、`CThreadManager`、`CDownloadTask` |
| **被测成员/方法名** | **保持英文** | `m_vecPTasks`、`pushOneTask()`、`m_nTaskStatus` |
| 宏 / 枚举常量 | **保持英文** | `TASK_TODO`、`THREAD_RUN`、`MEGABYTES`、`inconsisdent` |
| gtest 断言宏 | **保持英文** | `EXPECT_EQ`、`ASSERT_NE`、`TEST_F` |
| 字符串测试数据 | 按被测逻辑需要 | URL/路径/`"512B"` 等保持原样，不强行翻译 |

原则：**描述测试意图的名字用中文，与被测源码一一对应的符号一律保留英文**，这样
改名不影响与源码的对照，也避免把 `m_` 成员、枚举值译错。

### 为什么中文标识符能编译通过

- 测试目标在 `tests/CMakeLists.txt` 里带了 `target_compile_options(... /utf-8)`，
  源文件按 UTF-8 解析，MSVC 接受中文标识符（C++ 允许实现定义的字符集）。
- gtest 的 `TEST(套件, 用例)` 宏内部用 `##` 把两个参数拼接成 C++ 类名，中文拼接
  已实测可正常注册/运行/输出。
- **务必确认文件以 UTF-8（无 BOM 亦可）保存**；若编辑器存成 GBK，`/utf-8` 下会
  产生乱码标识符或编译错误。

## 文件骨架（照此新建）

文件名与被测源文件对应：`tests/test_<模块名>.cpp`（如测 `CTaskManager` 就用
`test_ctask_manager.cpp`）。

```cpp
#include <stdafx.h>            // 必须第一行

#include <CTaskManager.h>      // 被测头文件
#include <gtest/gtest.h>

#include <vector>              // 其它标准库

namespace {

// 夹具：需要共用 Setup/TearDown 或辅助方法时用 TEST_F；纯函数用 TEST 即可。
class 任务管理器测试 : public ::testing::Test {
protected:
    void SetUp() override { /* 构造测试数据 */ }
    void TearDown() override { /* 释放“未交给被测对象”的资源，见内存所有权 */ }

    CTaskManager m_manager;    // 被测对象成员名可保留英文，与源码对照
};

TEST_F(任务管理器测试, 初始为空) {
    EXPECT_EQ(0, m_manager.m_nTasksCount);
    EXPECT_TRUE(m_manager.m_vecPTasks.empty());
}

TEST(错误码测试, 取值符合文档约定) {   // 无夹具时用 TEST
    EXPECT_EQ(1, noerror);
}

}  // namespace
```

命名要点：

- 套件/用例名**不能含空格、点号、引号**；gtest 用 `.` 分隔“套件.用例”，用例名里
  再出现 `.` 会破坏 `--gtest_filter` 与显示。中文顿号、括号也尽量避免。
- 用例名用“**被测行为/预期结果**”的陈述短语（`压入空指针不做任何事`、
  `时间加倍则平均速度减半`），不要用 `Test1`、`case_a` 这类无意义名。
- 一个源文件可含多个套件（如 `test_cthread_manager.cpp` 同时有
  `线程管理器测试` 与 `线程测试`）。

## 编写约定（本项目踩过的坑，务必遵守）

1. **每个测试文件第一行 `#include <stdafx.h>`**，再 include 被测头与 gtest。
2. **全部离线、确定性**：
   - 需要文件时在 `%TEMP%` 生成唯一名（`GetTempPathA` + PID + 自增计数器），
     夹具析构里 `remove()`；可直接仿 `test_cdownload_task_manager.cpp` 的
     `临时文件`（原 `TempFile`）辅助类。
   - 不发网络请求：任务/线程对象直接 `new` 后注入字段，不经过
     `generateDownloadTask()`/`createThreads()`（会真 HEAD 请求、真建线程）。
   - 联网函数（如 `checkTaskInfo()`）只测离线分支：不可达主机
     `getContentLength()` 返回 0，据此构造断言。
3. **内存所有权（最易踩，崩溃见 test-run）**：`CTaskManager::clearTask()`（及子类
   `clearDownloadTask()`）会 `delete` 掉 `m_vecPTasks` 里的全部任务。夹具 TearDown
   只能 delete **未进 manager** 的任务；进了 manager 的交给 `clearTask()`。手动调
   过 `clearTask()` 后，要把本地裸指针容器里对应的悬垂指针移除。参照
   `tests/test_ctask_manager.cpp` 的 `任务管理器测试::TearDown`。
4. **不启动真实工作线程**：线程池测试只操作 `m_vecFreeThreads/m_vecBusyThreads`
   簿记与状态字段；可构造 `CThread` 对象，但不要调 `init()`（会 `_beginthreadex`）。
5. **既有缺陷写“特征化断言”**：README「已知问题」里的 bug（如
   `getTotalDownloadedLength()` 每分片多计 1 字节、`getAverageSpeed()` 先除后乘丢
   精度、`writeData` 返回 `nmemb` 且遇 NUL 截断）按**当前实际行为**断言，并加中文
   注释说明“这是锁定的既有缺陷”。日后修复时测试会立即提醒行为变化；不要为了让
   测试变绿去改组件源码，除非任务本身就是修该 bug。
6. **断言字面量类型匹配**：`size()` 返回无符号，写 `EXPECT_EQ(1u, vec.size())`；
   `long long` 字面量带 `LL`（`1048576LL`），避免 signed/unsigned 与窄化告警。
7. **优先 ASSERT 保护后续**：指针/容器取值前用 `ASSERT_NE(nullptr, ...)`、
   `ASSERT_EQ(n, vec.size())` 先挡住，再用 `EXPECT_*` 做细节断言。
8. **新增测试文件后**，把文件名加进 `tests/CMakeLists.txt` 的
   `CURLDOWNLOADER_TEST_SOURCES`；NMake 构建会自动检测 CMakeLists 变化并重配置。

## 中文输出的甄别（重要）

gtest 运行时输出的套件/用例名是**标准 UTF-8**。但在 Windows PowerShell 里直接跑或
经管道（`|`、`>`、`Select-Object`）捕获时，PowerShell 常按系统代码页（GBK/936）
解码，于是中文行末出现 `?` 或乱码，例如：

```
[ RUN      ] 线程管理器测?全部忙碌时回收线程为空操?
```

这**只是控制台解码的显示问题，测试名本身完好**，不影响注册、过滤、通过与否。
确认真实名称的方法：

- 让可执行文件直接重定向（不经 PowerShell 管道），再按 UTF-8 读字节：
  ```powershell
  cmd /c "build-tests\bin\CurlDownloaderTests.exe --gtest_list_tests > %TEMP%\list.txt 2>&1"
  [System.IO.File]::ReadAllText("$env:TEMP\list.txt", [System.Text.Encoding]::UTF8)
  ```
- 或先 `chcp 65001` 切到 UTF-8 代码页再运行。
- `--gtest_filter` 用中文套件/用例名通配同样有效，例如
  `--gtest_filter="任务管理器测试.*"`、`--gtest_filter="*平均速度*"`（在 PowerShell
  里注意用引号包裹，避免中文被不当解析）。

## 自检清单（写完后）

- [ ] 文件名 `test_<模块>.cpp` 并已加入 `CURLDOWNLOADER_TEST_SOURCES`。
- [ ] 首行 `#include <stdafx.h>`；文件以 UTF-8 保存。
- [ ] 套件/用例/夹具/辅助名为中文且无空格、`.` 等非法字符；被测类名、成员、宏、
      枚举保持英文。
- [ ] 无网络、不 `init()` 起线程；临时文件在 `%TEMP%` 且析构清理。
- [ ] 进了 manager 的任务不在 TearDown 重复 delete。
- [ ] 既有缺陷用特征化断言 + 中文注释，而非按理想行为写期望值。
- [ ] 无符号比较告警（`1u`/`LL` 字面量）。
- [ ] 按 test-build 重新构建、按 test-run 跑通（结尾 `[  PASSED  ] N tests.`）。
