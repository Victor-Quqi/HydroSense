下面这份指南面向**完全没有计算机基础**的入门者，力求每一步都讲清楚：**你在做什么、为什么要这么做、以及在 mac 的界面里要点哪里**。同时穿插解释 Git / GitHub 的关键概念、`~/Projects` 的含义、如何打开终端与 VS Code 的命令面板等。

> **阅读方法**：建议由上至下跟着做；每一节都标注「你在做什么 / 为什么 / 怎么做」。  
> **提示**：菜单名字可能一开始是英文，安装中文语言包后会变中文；本指南同时给出中英文菜单。

---

## A. 先把几个核心概念讲清楚（看懂后再动手）

- **GitHub 是什么**：一个把代码放到云端、多人协作的平台；你在本地编辑代码，用 Git 把修改**推送（push）**到 GitHub，别人就能拉取（pull）你的修改。VS Code 可以直接和 GitHub 协同工作（支持登录、克隆仓库、提 Issue/PR 等）。  
- **仓库（Repository）**：项目代码的“云端家”。你把它**克隆（clone）**到本机，得到一个**本地仓库**；之后的提交（commit）与推送（push）都是对这个仓库进行。VS Code 自带 Git 视图，能完成克隆、提交、推拉与同步。  
- **分支（Branch）**：同一个项目的不同“开发轨道”，通常 `main` 放稳定版本，开发在 `develop` 或其它分支上进行。VS Code 状态栏可一键切换分支。  
- **提交（Commit）**：一次代码快照，要写清楚“做了什么”；推送（Push）就是把这些提交发回 GitHub，拉取（Pull）就是把云端更新拿到本地。VS Code 的“**同步**（Sync）”会依次**先拉再推**，更安全。

---

## B. 路径与文件夹：`~/Projects` 里的 `~` 是什么？

- **你在做什么**：决定把代码放到你电脑上的哪个文件夹（建议统一放到 `~/Projects`）。  
- **为什么**：克隆时需要一个**明确的本地位置**，以后你知道代码“住在哪里”，不迷路。  
- **`~` 的含义**：在 mac 的终端里，`~` 表示**你的用户“家目录”（Home）**，通常路径是 `/Users/你的用户名`；因此 `~/Projects` 就是“你的家目录里的 `Projects` 文件夹”。Apple 的终端说明里也用 `~` 表示当前用户的家目录。

**怎么做（在 Finder 里）**：

1. 打开 **Finder**（桌面最左侧蓝白笑脸图标）。  
2. 左侧边栏点你的**用户名**（这是“家目录”）。  
3. 在空白处右键 → **新建文件夹** → 命名为 **Projects**（以后所有项目都放这里）。  
   > 记住：这个 `Projects` 的完整路径就是 **`~/Projects`**（家目录 + Projects）。上面的 `~` 就是你的家目录的快捷写法。

---

## C. 打开终端（Terminal）与它的几种打开方式

- **你在做什么**：打开“终端”这个工具，后续要用它执行少量命令（例如配置 Git 身份）。  
- **为什么**：很多设置用终端更直接；即便将来主要用 VS Code 的图形界面，认识终端仍然有用。  
- **怎么做（四种方法，任选其一）**：  
  1) **Launchpad**：点 Dock 里的 **Launchpad** → 在上方搜索框输入 **Terminal** → 点 **Terminal** 图标。  
  2) **Spotlight**：按 **⌘ + 空格** → 输入 **Terminal** → 回车。  
  3) **Finder**：顶部菜单 **前往（Go）** → **实用工具（Utilities）** → 双击 **Terminal**。  
  4) **应用程序文件夹**：Finder → 左侧 **应用程序** → 打开 **实用工具** → 双击 **Terminal**。

> 终端里看到提示如 `michael@MacBook-Pro ~ %`，其中 `~` 说明你当前就在**家目录**。

---

## D. 把 VS Code 切换成中文界面（更容易认菜单）

- **你在做什么**：安装中文（简体）语言包，把 VS Code 的界面汉化。  
- **为什么**：更容易理解菜单与提示。  
- **怎么做（界面操作）**：  
  1) 打开 VS Code → 顶部菜单 **View（视图）** → **Command Palette…（命令面板）**。  
  2) 在弹出的搜索框输入 **Configure Display Language**（配置显示语言） → 选择 **中文（简体） zh‑cn** → 按提示**重启 VS Code**。  
  > 也可以在左侧 **扩展**里搜索并安装 **Chinese (Simplified) Language Pack**，效果相同。

> **命令面板**的打开方式除了菜单外，还能用快捷键 **⌘⇧P** 或 **F1**；若快捷键冲突，菜单方式一定可用。

---

## E. 在 VS Code 里克隆（Clone）团队仓库到 `~/Projects`

- **你在做什么**：把 GitHub 上的项目复制到你的电脑（本地仓库）。  
- **为什么**：只有克隆到本地，你才能在 VS Code 里编辑与提交。  
- **怎么做（纯界面操作，几乎不需要终端）**：  
  1) 打开 VS Code → 左侧点击 **源代码管理（Source Control）**。  
  2) 右侧看到 **Clone Repository（克隆仓库）** 按钮；或按 **⌘⇧P** 在命令面板输入 **Git: Clone**。  
  3) 选择 **Clone from GitHub（从 GitHub 克隆）**（若提示登录，会自动打开浏览器完成 GitHub 登录授权，然后返回 VS Code）。  
  4) 在提示的输入框粘贴仓库地址：`https://github.com/Victor-Quqi/HydroSense.git`。  
  5) 弹窗会让你选择**保存位置**：请点进 **用户 →（你的用户名）→ Projects**，然后选择 **打开（Open）**。  
  6) 克隆完成后，VS Code 会问是否打开该仓库，点 **打开**即可。

> **说明**：VS Code 的 GitHub 集成会在需要时引导你**浏览器登录 GitHub**并返回 VS Code，这一步用于 VS Code 与 GitHub 的连接与授权（例如克隆、查看 PR/Issues）。

---

## F. 在 VS Code 里打开“集成终端”（以后少量命令都在这里敲）

- **你在做什么**：在 VS Code 下面打开一个终端面板。  
- **为什么**：不必来回切换应用，直接在编辑器里运行 Git 命令。  
- **怎么做（界面操作）**：  
  - 顶部菜单 **Terminal（终端）** → **New Terminal（新终端）**；或 **View（视图）→ Terminal（终端）**。  
  - 快捷键：**⌃`**（Control + 反引号），或在命令面板里输入 **View: Toggle Terminal**。

---

## G. 第一次必须做的两件事：设置 Git“身份” 与 选择认证方式

### G1. 设置 Git“身份”（提交显示是谁）

- **你在做什么**：告诉 Git 你的名字和邮箱（会写进每次提交的记录里）。  
- **为什么**：团队要知道是谁做了修改；GitHub 也用邮箱与你账号关联贡献。  
- **怎么做（VS Code 集成终端里敲）**：  
  ```bash
  git config --global user.name "你的GitHub用户名或姓名"
  git config --global user.email "你的GitHub邮箱"
  git config --global --list   # 检查是否设置成功
  ```  
  这属于 Git 的标准初始化步骤。

### G2. 选择简单的认证方式（让推送/拉取不再卡住）

#### **最省事路线之一**：HTTPS + 个人访问令牌（PAT）＋ macOS 钥匙串
1) **在浏览器里创建 PAT（细粒度，安全）**  
   - GitHub → 头像 **Settings** → **Developer settings** → **Personal access tokens** → 选择 **Fine‑grained tokens** → **Generate new token**。  
   - 将 **仓库访问**限定到团队仓库（例如 `HydroSense`），把 **Repository permissions → Contents** 设置为 **Read and write**（允许克隆与推送）。  
   - 复制令牌（只显示一次）；细粒度 PAT 在 2025 年已**正式 GA**并默认开启，更安全。

2) **让 Git 把令牌记到 macOS 钥匙串**（以后不再重复输入）：  
   - 在 VS Code 的终端执行：  
     ```bash
     git config --global credential.helper osxkeychain
     ```
   - **第一次**执行 `git pull` / `git push` 时，Git 会弹出“用户名/密码”输入：**用户名填你的 GitHub 用户名，密码处粘贴刚复制的 PAT**；此后会被存到**钥匙串**，不再重复询问。

> 如果之前输错过密码/令牌，导致一直失败：在终端执行  
> `git credential-osxkeychain erase`（回车两次）清除旧记录，再推送一次重新保存正确令牌。

#### **备选路线（也很简单）**：SSH 密钥（免输令牌）
1) 在 VS Code 终端生成密钥：  
   ```bash
   ssh-keygen -t ed25519 -C "你的GitHub邮箱"   # 连续回车接受默认位置 ~/.ssh/id_ed25519
   eval "$(ssh-agent -s)"
   /usr/bin/ssh-add --apple-use-keychain ~/.ssh/id_ed25519
   ```
2) 复制公钥并添加到 GitHub：  
   ```bash
   pbcopy < ~/.ssh/id_ed25519.pub
   ```
   打开 GitHub → **Settings → SSH and GPG keys** → **New SSH key** → 粘贴保存。  
3) 测试并把仓库地址改为 SSH：  
   ```bash
   ssh -T git@github.com        # 成功会问候你
   git remote set-url origin git@github.com:Victor-Quqi/HydroSense.git
   ```  
   之后 `push/pull` 都不用再输密码或令牌。

> **为什么我们要准备这两条路线**：GitHub 已废止密码登录 Git 操作，必须用 PAT 或 SSH。遇到“`Support for password authentication was removed`”之类错误，按上述两法即可修复。

---

## H. 每次工作的最少步骤（一步步点哪里）

1. **打开项目**：VS Code → **文件（File）→ 打开文件夹（Open Folder…）** → 选择 `~/Projects/HydroSense`。VS Code 会识别到这是一个 Git 仓库。  
2. **先同步**：左下角状态栏点 **同步（Sync）**，它会**先拉取再推送**，避免冲突；或从命令面板选择 **Git: Sync**。  
3. **切分支**：状态栏点击当前分支名 → 选择 `develop`（在开发分支上改动）。  
4. **修改代码**：在左侧资源管理器编辑文件。  
5. **提交与推送**：左侧 **源代码管理** → 对修改文件点 **`+` 暂存** → 在顶部输入清晰的提交信息 → 点 **提交（Commit）** → 状态栏点 **同步（Sync）**。

---

## I. 如果只想**全程用界面**（不碰命令），也可以这样做

- **克隆**：**源代码管理 → Clone Repository → Clone from GitHub → 选择保存位置**（`~/Projects`），流程里会自动引导浏览器授权登录 GitHub；克隆完成后点 **Open** 打开。  
- **提交/推送**：在 **源代码管理**视图里完成暂存与提交；然后点状态栏的 **同步**。如果你用 **HTTPS** 地址，**第一次推送**可能会弹出“输入用户名/密码”的对话框——**密码处粘贴 PAT**，并由钥匙串记住（前面 G2 已设置）。

---

## J. 你可能会问的两个“为什么”

1) **“我在 VS Code 里已经登录 GitHub，为何还要 PAT/SSH？”**  
   - VS Code 的登录让编辑器能访问 GitHub 的功能（PR、Issues、克隆列表等），但**Git 的推拉**依旧由系统 Git 客户端执行；在 **GitHub.com** 上通过 **HTTPS** 推拉需要 **PAT**（或改用 **SSH**）。VS Code 会在需要时引导你浏览器登录，但最终的 Git 凭据通常仍由系统的**凭据助手/钥匙串**管理。

2) **“为什么说密码不行了？”**  
   - GitHub 在 **2021‑08‑13** 起移除 **账户密码**用于 Git 操作（HTTPS）；必须用 **PAT** 或 **SSH**。

---

## K. PlatformIO（了解即可）

- **你在做什么**：在 VS Code 安装 **PlatformIO IDE** 扩展；它会识别 `platformio.ini` 并自动拉取工具链与库。  
- **为什么**：团队项目使用它做嵌入式/IoT 构建。  
- **怎么做**：左侧 **扩展** → 搜索 **PlatformIO IDE** → **安装**；首次打开项目它会自动完成依赖准备。

---

## L. 一页速查（含中英文菜单）

- **家目录与路径**：`~/Projects` 中 `~` 表示你的家目录（Home）；Apple 的终端文档和社区答复均如此说明。  
- **打开终端**：Launchpad / Spotlight / Finder → Utilities → Terminal。  
- **命令面板**：**View → Command Palette…**（或 **⌘⇧P** / **F1**）。  
- **VS Code 终端**：**Terminal → New Terminal** 或 **View → Terminal**（快捷 **⌃`**）。  
- **VS Code 设中文**：**Configure Display Language → 选择 zh‑cn → 重启**。  
- **克隆仓库**：**源代码管理 → Clone Repository → Clone from GitHub → 选 `~/Projects`**。  
- **Git 身份**（在 VS Code 终端）：  
  ```bash
  git config --global user.name "YourName"
  git config --global user.email "you@example.com"
  ```
    
- **认证（HTTPS）**：创建 **细粒度 PAT**（仓库限定、`Contents: Read & write`），并启用钥匙串助手：  
  ```bash
  git config --global credential.helper osxkeychain
  ```
  首次推送时把 **PAT 当密码**；钥匙串会记住。  
- **认证（SSH）**：  
  ```bash
  ssh-keygen -t ed25519 -C "you@example.com"
  /usr/bin/ssh-add --apple-use-keychain ~/.ssh/id_ed25519
  pbcopy < ~/.ssh/id_ed25519.pub  # GitHub → Settings → SSH keys → New
  ssh -T git@github.com
  ```
  然后把远程改成 `git@github.com:...`。  
- **遇到密码报错**（支持密码已移除）：改用 PAT 或 SSH。