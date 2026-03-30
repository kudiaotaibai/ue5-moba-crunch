# Docker 原理入门 - 程序员版

本文不讲常用命令手册，重点回答下面几个问题：

- Docker 到底是什么
- 镜像、容器、`Dockerfile`、`docker-compose.yaml` 分别是什么
- 容器为什么不是虚拟机
- 一个容器从 `docker build` 到 `docker run` 实际发生了什么
- 这些概念在你项目里的部署目录中分别对应什么

相关文件：

- [server Dockerfile](/E:/ue/Crunch/ServerDeploy/server/Dockerfile)
- [coordinator Dockerfile](/E:/ue/Crunch/ServerDeploy/coordinator/Dockerfile)
- [docker-compose.yaml](/E:/ue/Crunch/ServerDeploy/docker-compose.yaml)
- [coordinator.py](/E:/ue/Crunch/ServerDeploy/coordinator/coordinator/coordinator.py)

## 1. Docker 是什么

从工程角度看，Docker 不是“打包软件”这么简单。

更准确地说，Docker 是一套围绕下面能力构建的工程化工具链：

- 打包应用运行环境
- 分发这个运行环境
- 在隔离环境中启动进程
- 管理网络、挂载、资源限制和生命周期

如果只看运行时本质，Docker 做的事情可以概括成一句话：

**在宿主机内核之上，使用一组隔离机制启动一个有独立 root filesystem、进程视图、网络视图和资源限制的进程。**

所以容器的本质仍然是“进程”，不是一台完整虚拟机。

## 2. Docker 体系里几个核心对象

### 2.1 `Dockerfile`

`Dockerfile` 是镜像构建描述文件。

它不是镜像本身，也不是容器本身，而是“如何构建镜像”的脚本。

例如你的服务端镜像：

[server Dockerfile](/E:/ue/Crunch/ServerDeploy/server/Dockerfile)

```dockerfile
FROM gcr.io/distroless/nodejs22-debian12:nonroot
COPY --chown=nonroot:nonroot ./server /home/nonroot/server
ENTRYPOINT [ "/home/nonroot/server/Crunch/Binaries/Linux/CrunchServer", "Crunch" ]
```

它表达的是：

- 以某个基础镜像作为起点
- 把打包好的 Linux 服务端文件复制进去
- 设置容器启动后的入口进程

### 2.2 镜像 `image`

镜像是构建结果。

它可以理解为：

- 一个只读的 root filesystem 模板
- 外加元数据，例如环境变量、入口命令、工作目录等

镜像不是“运行中的程序”。

镜像更像：

- 一个可复用、可分发、可实例化的运行时模板

### 2.3 容器 `container`

容器是镜像的运行实例。

启动容器时，运行时会：

- 基于镜像组装文件系统
- 创建隔离环境
- 启动入口进程

因此：

- 镜像是模板
- 容器是实例

这和：

- C++ 类是定义
- 对象是实例

类似，但不要机械类比，因为容器还涉及进程、文件系统和网络命名空间。

### 2.4 `docker-compose.yaml`

`docker-compose.yaml` 用来描述多个容器如何一起运行。

你的项目里：

[docker-compose.yaml](/E:/ue/Crunch/ServerDeploy/docker-compose.yaml)

```yaml
services:
  server:
    build: ./server
    image: server

  coordinator:
    build: ./coordinator
    image: coordinator
    container_name: servercoordinator
    volumes:
      - /var/run/docker.sock:/var/run/docker.sock
    ports:
      - '80:80'
```

它表达的是：

- 有两个服务：`server` 和 `coordinator`
- 各自如何构建镜像
- `coordinator` 需要暴露端口 `80`
- `coordinator` 还挂载了宿主机的 Docker socket

## 3. 为什么容器不是虚拟机

这是理解 Docker 的关键。

### 3.1 虚拟机的模型

虚拟机通常是：

- Hypervisor 虚拟出硬件
- 每个虚拟机运行自己的 Guest OS
- Guest OS 再运行自己的进程

也就是说，虚拟机一般有自己独立的内核。

### 3.2 容器的模型

容器通常是：

- 不虚拟化整套硬件
- 不自带独立内核
- 直接共享宿主机内核
- 只把“进程可见世界”隔离开

所以容器启动快、占用轻，但隔离层级和虚拟机不同。

### 3.3 容器依赖哪些内核机制

Linux 容器最核心的基础设施主要是：

- `namespaces`
- `cgroups`
- `capabilities`
- `union filesystem / overlay filesystem`

Docker 自己不是发明这些机制的人，它更多是把这些机制工程化、产品化。

## 4. `namespaces`：隔离“看到的世界”

`namespace` 可以理解为“给进程一个被隔离的视图”。

常见类型有：

- `pid namespace`
  让容器内的进程看到独立的进程树
- `net namespace`
  让容器拥有自己的网络接口、IP、路由表
- `mnt namespace`
  让容器看到独立的挂载点视图
- `uts namespace`
  隔离 hostname 等系统标识
- `ipc namespace`
  隔离共享内存、消息队列等 IPC 资源
- `user namespace`
  隔离用户 ID 映射

因此，当你进入一个容器时，感觉像是“进入另一台机器”，本质上不是换了机器，而是：

- 当前进程被放进了另一组 namespace
- 看到的是另一套被隔离后的资源视图

## 5. `cgroups`：限制“能用多少资源”

`cgroups` 负责资源控制和统计。

例如可以限制：

- CPU
- 内存
- I/O
- 进程数量

没有 `cgroups`，容器即使能隔离进程视图，也难以做可靠的资源约束。

所以可以简单记：

- `namespaces` 解决“看见什么”
- `cgroups` 解决“能用多少”

## 6. 文件系统：镜像为什么能分层

### 6.1 镜像不是一个普通目录打包

镜像通常由多层只读 layer 组成。

例如：

- 基础系统一层
- 安装依赖一层
- 复制应用文件一层
- 设置元数据不是数据层，但会进入镜像配置

每条 `Dockerfile` 指令往往会对应新的构建层或新的镜像配置状态。

### 6.2 为什么需要分层

分层的好处：

- 便于缓存
- 便于复用
- 便于分发

例如两个镜像如果都基于同一个基础镜像，那么底层 layer 可以共享，不必重复下载。

### 6.3 容器启动时为什么还能写文件

镜像是只读的，但容器运行时通常会在镜像层上再叠加一层可写层。

常见实现思路是 copy-on-write。

也就是说：

- 读文件时，按层查找
- 写文件时，写入容器自己的可写层

因此：

- 同一个镜像可以启动多个容器
- 各容器运行时修改互不影响

## 7. 网络：端口映射本质上做了什么

在默认 bridge 网络模式下，可以这样理解：

- 容器有自己的 network namespace
- 容器里有自己的网卡和 IP
- 宿主机通过 bridge 和 NAT 与它通信

当你写：

```yaml
ports:
  - '80:80'
```

它的含义不是“容器直接占用宿主机 80 端口”那么简单。

更准确地说，它通常意味着：

- 宿主机监听某个端口
- 再把流量转发到容器网络命名空间中的对应端口

你项目里的协调器就是这样暴露出去的。

## 8. 挂载：为什么容器能访问宿主机文件

Docker 允许把宿主机路径挂载到容器里。

这不是“复制文件”，而是“把宿主机上的某个路径映射到容器文件系统视图里”。

例如你的 compose：

```yaml
volumes:
  - /var/run/docker.sock:/var/run/docker.sock
```

这句非常关键。

它表示：

- 把宿主机的 Docker Unix socket 文件
- 映射到容器内部同一路径

这样容器内的程序就能通过这个 socket 和宿主机 Docker daemon 通信。

## 9. Docker daemon、client、runtime 的关系

从逻辑上看，Docker 主要有三层：

### 9.1 Docker client

也就是你执行 `docker build`、`docker run` 的命令行客户端。

### 9.2 Docker daemon

负责真正执行构建、拉取镜像、创建容器、管理网络和存储。

客户端通常不是直接操作内核，而是向 daemon 发请求。

### 9.3 container runtime

底层真正负责创建容器进程、准备 rootfs、设置 namespace/cgroup 的运行时组件。

你可以把链路简化理解为：

```text
docker CLI -> Docker daemon -> container runtime -> Linux kernel
```

## 10. `docker build` 实际发生了什么

当你执行：

```bash
docker build -t server .
```

逻辑上大致发生这些事情：

1. Docker 读取当前目录的 `Dockerfile`
2. 发送 build context 给构建后端
3. 逐条执行构建步骤
4. 每步生成新的 layer 或镜像配置
5. 最终得到一个镜像 ID 和标签

以你的服务端镜像为例：

[server Dockerfile](/E:/ue/Crunch/ServerDeploy/server/Dockerfile)

```dockerfile
FROM gcr.io/distroless/nodejs22-debian12:nonroot
COPY --chown=nonroot:nonroot ./server /home/nonroot/server
ENTRYPOINT [ "/home/nonroot/server/Crunch/Binaries/Linux/CrunchServer", "Crunch" ]
```

可以理解为：

1. 选择一个基础根文件系统
2. 把 `./server` 目录写入镜像层
3. 设置默认入口进程

这里的 `ENTRYPOINT` 不是立即执行，它只是把“以后容器启动时的默认进程”写入镜像元数据。

## 11. `docker run` 实际发生了什么

当执行一个 `docker run` 时，运行时大致会做这些事情：

1. 找到目标镜像
2. 创建容器元数据
3. 为容器准备可写层
4. 设置 namespace
5. 设置 cgroups
6. 配置网络
7. 配置挂载
8. 启动入口进程

如果入口进程退出，容器通常也就结束了。

这也是容器和传统“后台服务管理器”思维不同的地方：

- 容器的核心生命周期通常绑定到 PID 1

## 12. 为什么说“容器里跑的是进程”

如果一个容器的 `ENTRYPOINT` 是：

```dockerfile
ENTRYPOINT ["python", "coordinator.py"]
```

那容器的本体就是这个 Python 进程。

在你的项目里：

[coordinator Dockerfile](/E:/ue/Crunch/ServerDeploy/coordinator/Dockerfile)

```dockerfile
ENTRYPOINT ["python", "coordinator.py"]
```

这意味着：

- 容器启动后，主进程是 `python coordinator.py`
- 这个进程退出，容器也结束

服务端镜像也是同理：

```dockerfile
ENTRYPOINT [ "/home/nonroot/server/Crunch/Binaries/Linux/CrunchServer", "Crunch" ]
```

容器的主进程就是 `CrunchServer`。

## 13. 结合你的项目：这套部署到底怎么工作

你的项目可以拆成两个角色：

### 13.1 `server` 镜像

[server Dockerfile](/E:/ue/Crunch/ServerDeploy/server/Dockerfile)

它的职责是：

- 提供一个可启动的 Linux UE 专用服务器运行环境
- 作为“游戏房间实例模板”

这个镜像本身通常不是长驻控制器，而是“被随时启动多个实例”的模板。

### 13.2 `coordinator` 容器

[coordinator.py](/E:/ue/Crunch/ServerDeploy/coordinator/coordinator/coordinator.py)

它的职责是：

- 收到建房请求
- 找可用端口
- 调用宿主机 Docker 启动新的 `server` 容器

关键代码是：

```python
subprocess.Popen([
    "docker",
    "run",
    "--rm",
    "-p", f"{port}:{port}/tcp",
    "-p", f"{port}:{port}/udp",
    "server",
    ...
])
```

这段代码的意义不是“容器里再跑一个小脚本”那么简单，而是：

- 协调器容器通过挂载进来的 `/var/run/docker.sock`
- 直接控制宿主机 Docker daemon
- 让宿主机再启动新的 `server` 容器

这是一种典型的“容器控制宿主机 Docker”的模式。

## 14. `docker.sock` 为什么危险

从权限角度看：

```yaml
- /var/run/docker.sock:/var/run/docker.sock
```

几乎等价于把“控制宿主机 Docker 的能力”交给了容器内进程。

因为一旦容器内程序可以自由调用 Docker API，它通常就有很高的宿主机控制能力。

所以这个挂载很方便，但也意味着：

- 协调器容器不能随便暴露给不可信代码
- 输入参数必须尽量可控
- 生产环境需要额外做权限和网络边界控制

## 15. `distroless` 是什么

你的服务端镜像基于：

```dockerfile
FROM gcr.io/distroless/nodejs22-debian12:nonroot
```

`distroless` 的含义是：

- 尽量不带完整发行版工具链
- 没有大量 shell、包管理器、调试工具
- 更接近“只保留运行应用需要的最小用户态环境”

它的优点：

- 镜像更小
- 攻击面更小
- 更适合纯运行时镜像

它的代价：

- 调试不方便
- 容器里通常不能像普通 Linux 一样随便 `bash` 进去查问题

## 16. Windows 上为什么还能跑 Linux 容器

你当前开发环境是 Windows。

但 Linux 容器依赖 Linux 内核特性，例如：

- namespaces
- cgroups

因此在 Windows 上跑 Linux 容器时，通常不是 Windows 内核直接原生提供这些能力，而是：

- Docker Desktop 启动一个 Linux VM
- Linux 容器实际跑在那个 Linux 虚拟环境里

所以对 Windows 用户来说，常见心智模型应当是：

- 你操作的是 Docker
- 但 Linux 容器最终还是需要 Linux 内核支撑

## 17. 你现在最应该建立的心智模型

建议把 Docker 拆成下面四层理解：

### 17.1 镜像是模板

它描述“应用运行所需文件系统和默认启动配置”。

### 17.2 容器是实例

它是镜像在隔离环境中的一次运行。

### 17.3 容器本质是被隔离的进程

不是一台完整机器。

### 17.4 Docker 是把底层内核机制工程化

底层依赖的是 Linux 提供的隔离和资源控制能力，不是魔法。

## 18. 如果把你现在的项目翻译成一句话

你现在这套部署可以概括成：

**用 Docker 把 UE 专用服务器做成可重复启动的镜像模板，再用一个 Flask 协调器容器通过宿主机 Docker API 按需拉起房间实例。**

这是一个合理的学习型原型架构。

## 19. 下一步最适合补什么

如果你已经理解这篇文档，接下来最自然的两篇是：

1. `Dockerfile` 每一条指令到底在镜像层上做了什么
2. `docker-compose.yaml` 在多容器编排里到底承担什么角色

如果只选一篇，建议先补：

**“Dockerfile 是如何一步一步变成镜像的”**

因为它和你项目里的 UE 服务端打包最直接。
