---
name: change-commit
description: 在本仓库提交代码时使用。覆盖“提交现在的工作区”“帮我提交”“commit 一下”等请求，以及需要撰写中文提交信息（全文不超过 300 字）、按 Agent/模型署名、处理 Git 身份未配置、避开 PowerShell 中文乱码、在提交前完成 CMake 构建验证等场景。也用于修正、精简或合并尚未推送的提交信息。
---

# 变更提交流程（change-commit）

本仓库的提交有四项硬性要求：**中文提交信息**、**简洁概述本次变更（全文不超过
300 字）**、**署名执行的 Agent 与模型**、**提交前完成构建验证**。同时本机环境有
两个坑：Git 未配置提交者身份，PowerShell 控制台是 GBK 代码页。下面是完整流程。

## 硬性约定

1. **提交信息用中文**，且**全文不超过 300 字**（含标题、正文、署名，按字符数计，
   中英文与标点一律计 1）。写清改了什么、为什么改，但只保留必要信息：
   - 标题一行，不超过 50 字。
   - 正文按主题分节（`一、`/`二、`…），每节 1~3 条，每条一行写完。
   - 不复述文件全路径、不粘贴命令、不罗列产物字节数、不展开背景论述；
     验证结论压缩成一句。
   - 超出 300 字就合并同类条目或删掉细节，不要靠删减署名来省字数。
2. **正文末尾固定署名两行**：

   ```
   提交方式：由 opencode CLI 的 <agent> 代理执行
   使用模型：<model-id>（<provider>/<model-id>）
   ```

   当前会话的取值写实际值，例如 `build` 代理与
   `ark-code-latest（volcengine-plan/ark-code-latest）`。
3. **未经用户明确要求，不得 push**，也不得 `git commit --amend` 已推送的提交。
4. **提交前必须构建验证**（见下文），并把验证结论写进提交信息。

## 步骤

### 1. 查看工作区

```powershell
git status
git log --oneline -10
git diff --stat
```

先读懂改动内容再动手，不要盲目 `git add -A`。新增的未跟踪文件要逐个确认是否
该入库（构建产物、`build*/`、`compile_commands.json` 一律不提交）。

### 2. 解决 Git 身份缺失

本机没有 `~/.gitconfig`，直接 commit 会失败：

```
Author identity unknown
fatal: unable to auto-detect email address
```

不要擅自写 `git config --global`。先读历史提交者：

```powershell
git log -3 --format="%an <%ae>"
```

本仓库历史身份为 `cenpengzhu <609357446@qq.com>`。**先问用户**是沿用该身份
（临时 `-c` 参数）还是写入仓库级 `git config`。默认推荐临时 `-c`，不污染配置。

### 3. 构建验证

提交前跑一次 Release 构建，细节见 `release-build` 技能。要点：x86 专用、
`NMake Makefiles` 生成器、`cmake` 与 `cl` 都不在 `PATH`。

```powershell
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
$cmake  = "C:\Program Files\CMake\bin\cmake.exe"
cmd /c "`"$vcvars`" x86 && `"$cmake`" -S . -B build-verify -G `"NMake Makefiles`" -DCMAKE_BUILD_TYPE=Release"
cmd /c "`"$vcvars`" x86 && `"$cmake`" --build build-verify"
Remove-Item -Recurse -Force build-verify
```

以 `[100%] Built target CurlDownloader` 结束即通过。C4005/C4244/C4700 是既有
警告，不算回归。验证用的构建目录用完即删，避免污染工作区。

验证结论写进提交信息时压成一句，例如「Release 构建（x86 + NMake）通过，仅既有
C4005/C4244 警告」，不要罗列产物大小与完整命令。

若工具链确实缺失，在提交信息里写明“未能构建验证”及原因，不要假装验证过。

### 4. 撰写提交信息（关键：避开 GBK 乱码）

**绝对不要**用 `git commit -m "中文…"`。PowerShell 控制台代码页是 GBK，中文
经命令行参数传递会被截断成乱码。正确做法是写入临时文件再用 `-F`：

1. 用 Write 工具把提交信息写到
   `C:\Users\ADMINI~1\AppData\Local\Temp\opencode\commitmsg.txt`
   （Write 工具输出 UTF-8，可靠）。
2. 提交：

   ```powershell
   git -c user.name="cenpengzhu" -c user.email="609357446@qq.com" -c i18n.commitEncoding=UTF-8 commit -F "C:\Users\ADMINI~1\AppData\Local\Temp\opencode\commitmsg.txt"
   ```

3. 删除临时文件。

信息结构模板（整体控制在 300 字内）：

```
<一句话概述本次变更>

一、<主题一>
- <改动及原因，一行写完>

二、<主题二>
- …

三、验证
<一句话构建/测试结论>

提交方式：由 opencode CLI 的 build 代理执行
使用模型：ark-code-latest（volcengine-plan/ark-code-latest）
```

写完后先数字数，超了就精简再提交：

```powershell
$p = "C:\Users\ADMINI~1\AppData\Local\Temp\opencode\commitmsg.txt"
$n = ([System.IO.File]::ReadAllText($p) -replace '\s','').Length
"字数：$n"; if ($n -gt 300) { "超出 300 字，需精简" }
```

（空白字符不计入，其余一律计 1。）

### 5. 校验提交结果

`git log` 在 GBK 控制台里显示中文一定是乱码，**这不代表提交坏了**。不要据此
重做提交。改为校验原始字节是否为合法 UTF-8：

```powershell
$p = "C:\Users\ADMINI~1\AppData\Local\Temp\opencode\head.txt"
cmd /c "git cat-file commit HEAD > `"$p`""
$b = [System.IO.File]::ReadAllBytes($p)
$enc = New-Object System.Text.UTF8Encoding($false,$true)
try { [void]$enc.GetString($b); "VALID UTF-8: yes" } catch { "VALID UTF-8: NO" }
Remove-Item $p
```

同理，校验源文件内容是否正确应使用 Read 工具（按 UTF-8 解码），而不是
`Get-Content` 或控制台回显。

最后 `git status --short` 确认工作区干净，并告知用户提交哈希、文件数与
「未推送」状态。

## 重写未推送的提交

用户对提交信息不满意时（比如要求改中文、要求合并），先确认远端状态，再回退到
远端最后一个提交并重新提交：

```powershell
git log --oneline -5
git reset --soft <远端最后一个提交>
git status --short
```

`--soft` 保留全部暂存内容，随后按第 4 步重新生成中文提交信息即可。多个未推送
提交是合并成一个还是分别改写，**先问用户**。

## 编码规约

本仓库全部文本文件为 **UTF-8 无 BOM**，MSVC 侧通过 `/utf-8` 编译选项保证含中文
字符串字面量的源码正确编译（`CMakeLists.txt` 与 `CurlDownloader.vcxproj` 各一
处）。提交前确认没有引入 GBK 或带 BOM 的新文件：

```powershell
$files = git ls-files | Where-Object { $_ -notmatch '\.(lib|pdb)$' }
foreach ($f in $files) {
  $b = [System.IO.File]::ReadAllBytes((Join-Path (Get-Location) $f))
  $enc = New-Object System.Text.UTF8Encoding($false,$true)
  $ok = $true; try { [void]$enc.GetString($b) } catch { $ok = $false }
  $bom = ($b.Length -ge 3 -and $b[0] -eq 0xEF -and $b[1] -eq 0xBB -and $b[2] -eq 0xBF)
  if (-not $ok -or $bom) { "$f  ValidUTF8=$ok  BOM=$bom" }
}
```

无输出即全部合规。若需转换旧文件，先做 GBK 往返校验
（`GetBytes(GetString(bytes))` 与原字节逐字节相等）再写回，防止误伤。

## 禁止事项

- 不要 `git commit -m` 带中文。
- 不要写超过 300 字的提交信息，也不要为凑字数堆砌背景与产物清单。
- 不要因控制台乱码就重做提交。
- 不要未经允许 push、amend 已推送提交、`--force`、跳过 hook。
- 不要提交 `build*/`、`compile_commands.json`、`*.pdb` 等生成物。
- 不要写 `git config --global`。
