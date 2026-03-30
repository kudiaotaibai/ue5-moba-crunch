# Unreal Engine 版本升级指南

> 从 UE 5.5.4 升级到更高版本（如 5.7）的完整流程

---

## 目录

1. [升级前的准备](#升级前的准备)
2. [UE版本类型说明](#ue版本类型说明)
3. [升级步骤](#升级步骤)
4. [常见问题](#常见问题)
5. [回退方案](#回退方案)

---

## 升级前的准备

### 1. 备份项目

**⚠️ 非常重要！升级前必须备份！**

```bash
# 方法1：复制整个项目文件夹
cp -r Crunch Crunch_Backup_5.5.4

# 方法2：使用 Git
git add .
git commit -m "备份：升级到 UE 5.7 之前的版本"
git tag v1.0-ue5.5.4
```

**建议备份的内容：**
- ✅ 整个项目文件夹
- ✅ `.uproject` 文件
- ✅ `Source/` 文件夹（C++ 代码）
- ✅ `Content/` 文件夹（资源文件）
- ✅ `Config/` 文件夹（配置文件）

### 2. 检查项目状态

**确保项目在当前版本能正常运行：**
```bash
# 1. 编译项目
# 在 Visual Studio 中：Build → Build Solution

# 2. 运行项目
# 双击 Crunch.uproject

# 3. 测试核心功能
# - 能否进入游戏
# - 能否创建服务器
# - 能否加入游戏
```

### 3. 记录当前配置

**记录以下信息：**
```
当前 UE 版本：5.5.4
项目路径：E:/ue/Crunch/
引擎路径：E:/uey/UnrealEngine-5.5/
使用的插件：
  - Enhanced Input
  - Gameplay Abilities
  - Online Subsystem EOS
  - 其他...
```

---

## UE版本类型说明

### 1. Launcher 版本（启动器版本）

**特点：**
- ✅ 官方预编译版本
- ✅ 下载即用，无需编译
- ✅ 稳定，经过测试
- ❌ 无法修改引擎源码
- ❌ 某些高级功能受限

**适用场景：**
- 普通游戏开发
- 不需要修改引擎
- 快速开发原型

**下载方式：**
```
Epic Games Launcher → Unreal Engine → 库 → 引擎版本 → 安装
```

### 2. Source 版本（源码版本）

**特点：**
- ✅ 完整源代码
- ✅ 可以修改引擎
- ✅ 可以调试引擎代码
- ✅ 支持 Linux 专用服务器
- ❌ 需要自己编译（耗时1-3小时）
- ❌ 占用空间大（100GB+）

**适用场景：**
- 需要修改引擎功能
- 需要 Linux 服务器
- 深度定制项目
- 学习引擎原理

**获取方式：**
```
GitHub → EpicGames/UnrealEngine → 下载源码 → 编译
```

---

## 升级步骤

### 方案1：使用 Launcher 版本升级（推荐新手）

#### 步骤1：安装新版本引擎

```
1. 打开 Epic Games Launcher
2. 进入 Unreal Engine → 库
3. 点击 "+" 号
4. 选择 5.7 版本
5. 点击 "安装"
6. 等待下载和安装完成（可能需要1-2小时）
```

#### 步骤2：升级项目

**方法A：右键升级（最简单）**
```
1. 找到项目文件：Crunch.uproject
2. 右键点击
3. 选择 "Switch Unreal Engine version..."
4. 选择 5.7
5. 点击确定
```

**方法B：直接打开（自动升级）**
```
1. 双击 Crunch.uproject
2. 如果引擎版本不匹配，会弹出提示
3. 选择 "More Options"
4. 选择 5.7 引擎
5. 点击 "Open Copy"（打开副本）或 "Open"（直接升级）
```

#### 步骤3：重新生成项目文件

```
1. 右键点击 Crunch.uproject
2. 选择 "Generate Visual Studio project files"
3. 等待完成
```

#### 步骤4：重新编译项目

```
1. 打开 Crunch.sln（Visual Studio 解决方案）
2. 选择 Development Editor 配置
3. 点击 Build → Rebuild Solution
4. 等待编译完成（可能需要10-30分钟）
```

#### 步骤5：测试项目

```
1. 编译成功后，双击 Crunch.uproject
2. 测试核心功能：
   - 能否打开项目
   - 能否编译蓝图
   - 能否运行游戏
   - 能否创建服务器
```

---

### 方案2：使用 Source 版本升级（需要 Linux 服务器）

#### 为什么需要 Source 版本？

**你的项目需要 Source 版本，因为：**
```
1. 你需要编译 Linux 专用服务器
2. Launcher 版本不支持 Linux 服务器编译
3. 你的协调器需要启动 Linux 服务器
```

#### 步骤1：下载 UE 5.7 源码

**前置要求：**
- GitHub 账号
- 已关联 Epic Games 账号
- 加入 EpicGames 组织

**下载步骤：**
```bash
# 1. 克隆仓库
git clone -b 5.7 https://github.com/EpicGames/UnrealEngine.git UE5.7

# 2. 进入目录
cd UE5.7

# 3. 下载依赖
./Setup.bat  # Windows
# 或
./Setup.sh   # Linux/Mac

# 4. 生成项目文件
./GenerateProjectFiles.bat  # Windows
```

#### 步骤2：编译引擎

**Windows 编译：**
```
1. 打开 UE5.sln
2. 选择 Development Editor 配置
3. 右键 UE5 项目 → Build
4. 等待编译完成（1-3小时，取决于电脑性能）
```

**编译配置建议：**
```
CPU：至少 8 核
内存：至少 32GB
硬盘：至少 150GB 可用空间
```

#### 步骤3：升级项目

**修改 .uproject 文件：**
```json
{
    "FileVersion": 3,
    "EngineAssociation": "E:/UE5.7/Engine",  // ← 改成新引擎路径
    "Category": "",
    "Description": "",
    "Modules": [
        {
            "Name": "Crunch",
            "Type": "Runtime",
            "LoadingPhase": "Default",
            "AdditionalDependencies": [
                "Engine",
                "GameplayAbilities"
            ]
        }
    ],
    "Plugins": [
        // ...
    ]
}
```

**或者使用命令行：**
```bash
# 切换引擎版本
E:/UE5.7/Engine/Binaries/Win64/UnrealVersionSelector.exe /switchversion "E:/ue/Crunch/Crunch.uproject" "E:/UE5.7"
```

#### 步骤4：重新生成和编译

```bash
# 1. 生成项目文件
右键 Crunch.uproject → Generate Visual Studio project files

# 2. 编译项目
打开 Crunch.sln → Build → Rebuild Solution
```

#### 步骤5：编译 Linux 服务器

**在 Windows 上交叉编译 Linux 服务器：**

```
1. 打开 Unreal Editor
2. 菜单：Platforms → Linux → Package Project
3. 选择输出目录
4. 等待打包完成
```

**或者使用命令行：**
```bash
E:/UE5.7/Engine/Build/BatchFiles/RunUAT.bat BuildCookRun ^
  -project="E:/ue/Crunch/Crunch.uproject" ^
  -platform=Linux ^
  -serverconfig=Development ^
  -cook ^
  -build ^
  -stage ^
  -pak ^
  -archive ^
  -archivedirectory="E:/ue/Crunch/Build/Linux"
```

---

## 常见问题

### Q1：升级后编译失败怎么办？

**A：按以下步骤排查：**

```
1. 清理项目
   - 删除 Binaries/ 文件夹
   - 删除 Intermediate/ 文件夹
   - 删除 Saved/ 文件夹
   - 删除 .vs/ 文件夹

2. 重新生成项目文件
   右键 .uproject → Generate Visual Studio project files

3. 重新编译
   打开 .sln → Rebuild Solution

4. 如果还是失败，查看错误信息
   - API 变更：查看 UE 5.7 的 Release Notes
   - 插件不兼容：禁用或更新插件
   - 代码过时：根据错误提示修改代码
```

### Q2：升级后资源丢失或损坏？

**A：可能的原因和解决方法：**

```
原因1：资源格式变更
解决：重新导入资源

原因2：蓝图节点过时
解决：打开蓝图，修复红色节点

原因3：材质系统变更
解决：重新编译材质

原因4：动画系统变更
解决：重新设置动画蓝图
```

### Q3：升级后性能下降？

**A：检查以下设置：**

```
1. 项目设置
   Edit → Project Settings → Engine → Rendering
   检查是否启用了新的渲染功能（如 Lumen、Nanite）

2. 关闭不需要的新功能
   - Lumen（如果不需要动态全局光照）
   - Nanite（如果不需要虚拟几何体）
   - Virtual Shadow Maps

3. 优化设置
   - 降低阴影质量
   - 减少后处理效果
   - 调整 LOD 设置
```

### Q4：插件不兼容怎么办？

**A：处理步骤：**

```
1. 查看插件是否支持新版本
   - 访问插件官网
   - 查看 Marketplace 页面
   - 查看 GitHub 仓库

2. 更新插件
   - Marketplace 插件：在 Launcher 中更新
   - 第三方插件：下载新版本

3. 临时禁用插件
   - 编辑 .uproject 文件
   - 注释掉不兼容的插件
   - 重新生成项目文件

4. 寻找替代方案
   - 寻找功能相似的插件
   - 自己实现功能
```

### Q5：协调器路径需要修改吗？

**A：需要！记得更新协调器中的引擎路径：**

**修改 `Coordinator/coordinator.py`：**
```python
def CreateSeverLocalTest(sessionName, sessionSearchId):
    global nextAvailablePort
    subprocess.Popen([
        # ← 改成新版本的路径
        "E:/UE5.7/Engine/Binaries/Win64/UnrealEditor.exe",
        "E:/ue/Crunch/Crunch.uproject",
        "-server",
        "-log",
        '-epicapp="ServerClient"',
        f'-SESSION_NAME="{sessionName}"',
        f'-SESSION_SEARCH_ID="{sessionSearchId}"',
        f'-PORT={nextAvailablePort}'
    ])
    # ...
```

---

## 回退方案

### 如果升级失败，如何回退？

#### 方法1：使用备份

```bash
# 1. 删除升级后的项目
rm -rf Crunch

# 2. 恢复备份
cp -r Crunch_Backup_5.5.4 Crunch

# 3. 切换回旧版本引擎
右键 .uproject → Switch Unreal Engine version → 5.5.4
```

#### 方法2：使用 Git

```bash
# 1. 查看提交历史
git log

# 2. 回退到升级前的版本
git reset --hard v1.0-ue5.5.4

# 3. 强制推送（如果已经推送到远程）
git push -f origin main
```

#### 方法3：重新克隆项目

```bash
# 如果项目在 Git 仓库中
git clone <仓库地址>
cd Crunch
git checkout <升级前的分支或标签>
```

---

## 升级建议

### 什么时候应该升级？

**✅ 应该升级的情况：**
- 新版本有你需要的功能
- 新版本修复了你遇到的 Bug
- 新版本性能更好
- 项目还在早期开发阶段

**❌ 不应该升级的情况：**
- 项目即将发布
- 团队成员还在使用旧版本
- 没有充足的测试时间
- 新版本刚发布（可能不稳定）

### 升级策略

**保守策略（推荐）：**
```
1. 等待新版本发布 1-2 个月
2. 查看社区反馈
3. 在测试分支上升级
4. 充分测试
5. 确认无问题后合并到主分支
```

**激进策略（不推荐）：**
```
1. 新版本一发布就升级
2. 直接在主分支升级
3. 边升级边修复问题
```

### 版本选择建议

**对于你的项目：**

```
当前版本：5.5.4
建议升级到：5.6.x（稳定版）

不建议：
- 5.7.0（刚发布，可能有 Bug）
- 5.8 Preview（预览版，不稳定）

推荐：
- 等 5.7.1 或 5.7.2（修复了初期 Bug）
```

---

## 总结

### 升级流程总结

```
1. 备份项目 ✅
2. 安装新版本引擎 ✅
3. 升级项目文件 ✅
4. 重新生成项目 ✅
5. 重新编译 ✅
6. 测试功能 ✅
7. 修复问题 ✅
8. 更新协调器路径 ✅
```

### 关键点

- ⚠️ 升级前必须备份
- ⚠️ 如果需要 Linux 服务器，必须用 Source 版本
- ⚠️ 升级后需要重新编译
- ⚠️ 记得更新协调器中的引擎路径
- ⚠️ 充分测试后再发布

### 时间估算

| 步骤 | Launcher 版本 | Source 版本 |
|------|---------------|-------------|
| 下载引擎 | 1-2 小时 | 30 分钟（源码） |
| 编译引擎 | 0（预编译） | 1-3 小时 |
| 升级项目 | 5 分钟 | 5 分钟 |
| 编译项目 | 10-30 分钟 | 10-30 分钟 |
| 测试修复 | 1-2 小时 | 1-2 小时 |
| **总计** | **2-4 小时** | **3-6 小时** |

---

**祝你升级顺利！如果遇到问题，记得查看 UE 官方文档和社区论坛。** 🎉
