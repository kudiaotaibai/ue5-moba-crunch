# Crunch 项目上传 GitHub 说明

## 一句话结论

- `Crunch` 整个 UE 项目：建一个仓库上传完整工程
- `RDGStarter` 渲染插件：单独建第二个仓库上传插件源码和文档

## 你现在已经准备好的内容

根目录已经补好了：

- `.gitignore`
- `.gitattributes`

它们的作用分别是：

- `.gitignore`：忽略 UE 自动生成目录、编译产物、临时文件
- `.gitattributes`：让 `.uasset` 和 `.umap` 走 Git LFS

另外我还单独准备了一个插件仓库目录：

- `github-ready/ue5-rdg-render-plugin`

这个目录可以直接作为第二个 GitHub 仓库的本地工作目录。

## 上传整个 Crunch 项目

在 `E:\ue\Crunch` 目录下执行：

```powershell
git init
git lfs install
git lfs track "*.uasset"
git lfs track "*.umap"
git add .gitattributes
git add .gitignore
git add .
git status
git commit -m "Initial commit: Crunch UE project"
```

然后去 GitHub 新建仓库，比如：

- `Crunch`
- 或 `ue5-moba-crunch`

再执行：

```powershell
git remote add origin https://github.com/你的用户名/Crunch.git
git branch -M main
git push -u origin main
```

## 上传 RDGStarter 插件仓库

插件单独仓库建议使用：

- `E:\ue\Crunch\github-ready\ue5-rdg-render-plugin`

进入这个目录执行：

```powershell
git init
git add .
git status
git commit -m "Initial commit: UE5 RDG render plugin"
```

然后去 GitHub 新建仓库，比如：

- `ue5-rdg-render-plugin`

再执行：

```powershell
git remote add origin https://github.com/你的用户名/ue5-rdg-render-plugin.git
git branch -M main
git push -u origin main
```

## 上传前你最好看一眼的地方

### Crunch 主项目

```powershell
git status
```

你要重点确认：

- `Binaries/`
- `Intermediate/`
- `Saved/`
- `DerivedDataCache/`

这些目录没有被加入暂存区。

### RDGStarter 插件仓库

插件仓库里应该重点保留：

- `RDGStarter.uplugin`
- `Source/`
- `Shaders/`
- `docs/`
- `README.md`

不需要带：

- `Binaries/`
- `Intermediate/`

## 推荐仓库命名

### 整项目

- `Crunch`
- `ue5-moba-crunch`

### 插件

- `ue5-rdg-render-plugin`

## 推荐你简历里放的链接

如果两个仓库都上传成功，简历里优先放：

1. `ue5-rdg-render-plugin`
2. `Crunch`

原因是渲染插件仓库更聚焦、更容易让面试官快速看懂你的技术亮点。
