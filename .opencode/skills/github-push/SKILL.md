---
name: github-push
description: 在本机把本仓库推送到 GitHub 时使用。覆盖“推送到 github”“帮我 push”“git push 失败”等请求，以及处理 DNS 把 github.com 解析到被封 IP 导致的“无法连接到远程服务器 20.205.243.166:443”、`could not read Username for 'https://github.com'`、`/dev/tty: No such device or address`、凭据管理器为空、需改用 SSH 密钥并配置 hosts/ssh config 等场景。也用于推送前确认待推提交与远端状态。
---

# 推送到 GitHub（github-push）

本机推送 GitHub 有两个固定障碍：**DNS 解析到被封 IP** 与 **无可用凭据且无法弹交互
窗口**。本技能记录已验证可行的处理路径与实测数据，照做即可推送成功。

## 前置约束

1. **push 必须用户明确要求**。change-commit 技能禁止自动推送；只有用户说了
   「推送」「push」才执行。
2. 推送前先确认待推内容与远端状态，不要盲推：

   ```powershell
   git remote -v
   git status -sb
   git log --oneline origin/master..HEAD
   ```

   把「将把 N 个提交推到 origin/master」告知用户后再动手。
3. 不使用 `--force`、`--force-with-lease`、不推送 `build*/` 等生成物分支。

## 障碍一：DNS 指向被封 IP

`git push` 报：

```
fatal: 发送请求时出错
fatal: 无法连接到远程服务器
fatal: 由于连接方在一段时间后没有正确答复……连接尝试失败。20.205.243.166:443
```

原因：本机 DNS 把 `github.com` 解析到 `20.205.243.166`（Azure 段），该 IP 在本网
络 443 不通。而 GitHub 的另一批 IP 是通的。

### 诊断

```powershell
Resolve-DnsName github.com -Type A | Select-Object IPAddress
@("140.82.113.3","140.82.114.3","20.27.177.113","4.237.22.38") | ForEach-Object {
  "$_ = $(Test-NetConnection -ComputerName $_ -Port 443 -InformationLevel Quiet -WarningAction SilentlyContinue)"
}
```

本机实测（2026-09）：

| 目标 | 443 | 22 |
| --- | --- | --- |
| `github.com` DNS 解析值 `20.205.243.166` | 不通 | — |
| `140.82.113.3` | 通 | 通 |
| `140.82.114.3` | 通 | 通 |
| `20.27.177.113` / `4.237.22.38` | 通 | — |
| `140.82.113.35`（api） | 通 | — |
| `codeload.github.com` | 通 | — |
| `ssh.github.com:443` | 不通 | — |

注意 `ssh.github.com:443`（常见的翻墙备用通道）在本机**不通**，别浪费时间；直连
`140.82.113.3:22` 反而是通的。

也要先排除代理：本机既无 `http.proxy`/`https.proxy`，也没有监听
7890/7897/10809/10808/1080/8888/2080 的本地代理。

### 确认 IP 可用（TLS 层）

只测端口不够，还要确认 SNI 能正常握手：

```powershell
try {
  $c = New-Object Net.Sockets.TcpClient("140.82.113.3",443)
  $s = New-Object Net.Security.SslStream($c.GetStream())
  $s.AuthenticateAsClient("github.com")
  "TLS OK subject=$($s.RemoteCertificate.Subject)"
  $s.Close(); $c.Close()
} catch { "TLS FAIL: $($_.Exception.Message)" }
```

期望 `CN=github.com`。

### 处理：写 hosts（需管理员，本机当前会话已是管理员）

**先备份再改**，用 ASCII 编码追加，改完刷新 DNS：

```powershell
$hosts = "$env:WINDIR\System32\drivers\etc\hosts"
Copy-Item $hosts "$hosts.bak-opencode" -Force
Add-Content -Path $hosts -Value "`n140.82.113.3 github.com`n140.82.113.4 api.github.com" -Encoding ASCII
ipconfig /flushdns | Out-Null
Resolve-DnsName github.com -Type A | Select-Object IPAddress
```

这是修改仓库外的系统文件，属于副作用，**必须在最终汇报里明确告知用户**改了什么、
备份在哪（`hosts.bak-opencode`）、以及 GitHub IP 会变、日后失效需重测更新。

若不愿改 hosts，替代方案是在 `~/.ssh/config` 里用 `HostName <可用IP>`（见下），
效果等价且不碰系统文件——**优先选这个**，hosts 仅在需要 HTTPS 通道时才动。

## 障碍二：凭据缺失且无法交互

HTTPS 推送在网络通了之后仍会失败：

```
bash: line 1: /dev/tty: No such device or address
error: failed to execute prompt script (exit code 1)
fatal: could not read Username for 'https://github.com': No such file or directory
```

原因：`credential.helper=manager`，但凭据库里没有 GitHub 条目
（`cmdkey /list:git:https://github.com` 显示「* 无 *」），而 opencode 的 shell 没
有 tty，弹不出登录窗口。且 `gh` 未安装、`GITHUB_TOKEN` 未设置。

此时**先问用户**选哪条路：

- **SSH 密钥（本机已验证可行，推荐）**：一次配置长期有效，无需 tty。
- **PAT**：用户提供后以临时环境变量方式使用，不落盘。
- **用户在外部终端手动 push**：让凭据管理器弹窗。

### SSH 方案完整步骤

1. 确认 22 端口可达（见上表），`ssh-keygen` 在
   `C:\Windows\System32\OpenSSH\ssh-keygen.exe`。
2. 生成专用密钥（PowerShell 下空口令要写 `-N '""'`，写 `-N ""` 会被吃掉参数）：

   ```powershell
   ssh-keygen -t ed25519 -C "cenpengzhu@CurlDownloader" -f "$env:USERPROFILE\.ssh\id_ed25519_github" -N '""'
   ```

3. 用 Read 工具读出 `id_ed25519_github.pub`，把公钥原文给用户，让其添加到
   <https://github.com/settings/ssh/new>（Key type 选 Authentication Key）。
   **绝不要**读取或外传私钥内容。
4. 追加 `~/.ssh/config`（保留已有条目，`HostName` 直接写可用 IP 以绕过 DNS）：

   ```
   Host github.com
     HostName 140.82.113.3
     User git
     IdentityFile C:\Users\Administrator\.ssh\id_ed25519_github
     IdentitiesOnly yes
     StrictHostKeyChecking accept-new
     ConnectTimeout 15
   ```

   `IdentitiesOnly yes` 防止本机既有的 `volc_key.pem` 被优先送出；
   `StrictHostKeyChecking accept-new` 避免首次连接卡在 yes/no 交互。
5. 验证认证（`BatchMode=yes` 保证不会等输入）：

   ```powershell
   ssh -T -o BatchMode=yes git@github.com
   ```

   看到 `Hi cenpengzhu! You've successfully authenticated` 即通过。
   ——注意 PowerShell 会把 ssh 的 stderr 包装成红色 `NativeCommandError`
   （例如 `Warning: Permanently added ... to the list of known hosts.`），
   **这不是失败**，看正文即可。
6. 切换 remote 并推送：

   ```powershell
   git remote set-url origin git@github.com:cenpengzhu/CurlDownloader.git
   git push origin master
   ```

## 校验推送结果

```powershell
git status -sb
git log --oneline -1 origin/master
```

`## master...origin/master` 后面没有 `[ahead N]` 即已同步。推送成功的输出形如
`458d0ee..54bdfb9  master -> master`（同样会被 PowerShell 标成红色，属正常）。
中文提交标题在 GBK 控制台显示乱码是编码显示问题，不代表推送有误（详见
change-commit 技能）。

## 汇报要求

推送完成后向用户说明：
- 推送范围（`<old>..<new>`，提交数）与远端分支；
- 为打通链路对**仓库外**做的改动：hosts 追加项及备份路径、新建的 SSH 密钥路径、
  `~/.ssh/config` 新增段、remote URL 由 HTTPS 改为 SSH；
- 这些改动的时效性提醒（GitHub IP 会变）。

## 禁止事项

- 不要未经用户要求 push，不要 `--force`。
- 不要 `git config --global`，需要时用仓库级或临时 `-c`。
- 不要把 PAT、私钥内容写进任何文件、提交信息或日志。
- 不要在未备份的情况下改 `hosts`，也不要删掉其中与本次无关的既有条目。
- 不要因 PowerShell 把 stderr 渲染成 `NativeCommandError` 就判定命令失败。
- 不要尝试 `ssh.github.com:443`，本机不通。
