# Docker 与多服务器架构详解

> 协调器如何管理多个游戏服务器？Docker 的作用是什么？

---

## 目录

1. [当前架构分析](#当前架构分析)
2. [Docker 是什么](#docker-是什么)
3. [多服务器架构](#多服务器架构)
4. [Docker 部署方案](#docker-部署方案)
5. [实战演示](#实战演示)

---

## 当前架构分析

### 你的协调器目前是怎么工作的？

**查看代码：`Coordinator/coordinator.py`**

```python
def CreateSeverLocalTest(sessionName, sessionSearchId):
    global nextAvailablePort
    subprocess.Popen([
        "E:/uey/UnrealEngine-5.5/Engine/Binaries/Win64/UnrealEditor.exe",
        "E:/ue/Crunch/Crunch.uproject",
        "-server",
        # ...
    ])
    # ...
```

**当前方式：直接启动进程**

```
客户端1请求创建房间
    ↓
协调器调用 subprocess.Popen()
    ↓
直接在本机启动 UnrealEditor.exe
    ↓
游戏服务器1启动（端口 7777）
    ↓
客户端2请求创建房间
    ↓
协调器再次调用 subprocess.Popen()
    ↓
直接在本机启动另一个 UnrealEditor.exe
    ↓
游戏服务器2启动（端口 7778）
```

**特点：**
- ✅ 简单直接
- ✅ 适合开发测试
- ❌ 没有使用 Docker
- ❌ 所有服务器运行在同一台机器上
- ❌ 资源隔离不好
- ❌ 不适合生产环境

---

## Docker 是什么

### 通俗解释

**Docker = 轻量级虚拟机**

```
传统方式（直接运行程序）：
┌─────────────────────────────┐
│      你的 Windows 系统       │
│  ┌─────┐ ┌─────┐ ┌─────┐   │
│  │服务器1│ │服务器2│ │服务器3│   │
│  └─────┘ └─────┘ └─────┘   │
│  共享系统资源，互相影响      │
└─────────────────────────────┘

Docker 方式（容器化）：
┌─────────────────────────────┐
│      你的 Windows 系统       │
│  ┌─────┐ ┌─────┐ ┌─────┐   │
│  │容器1 │ │容器2 │ │容器3 │   │
│  │服务器1│ │服务器2│ │服务器3│   │
│  └─────┘ └─────┘ └─────┘   │
│  资源隔离，互不影响          │
└─────────────────────────────┘
```

### Docker 的核心概念

#### 1. 镜像（Image）

**类比：游戏安装包**

```
镜像 = 游戏服务器的"安装包"
包含：
- Linux 操作系统
- UE5 引擎
- 游戏服务器程序
- 所有依赖库
```

**创建镜像：**
```dockerfile
# Dockerfile（镜像配方）
FROM ubuntu:22.04

# 安装依赖
RUN apt-get update && apt-get install -y \
    libicu-dev \
    libssl-dev

# 复制游戏服务器文件
COPY CrunchServer /app/CrunchServer

# 设置启动命令
CMD ["/app/CrunchServer", "-server"]
```

#### 2. 容器（Container）

**类比：游戏运行实例**

```
容器 = 从镜像"安装"出来的运行实例

1个镜像 → 可以创建多个容器
就像：
1个游戏安装包 → 可以安装到多台电脑上
```

**容器特点：**
```
✅ 轻量级（几秒启动）
✅ 隔离性（互不影响）
✅ 可移植（在任何机器上运行）
✅ 易管理（一键启动/停止）
```

#### 3. Docker Compose

**类比：游戏启动器**

```
Docker Compose = 批量管理容器的工具

一个配置文件 → 管理多个容器
就像：
一个启动器 → 管理多个游戏
```

**配置示例：`docker-compose.yaml`**
```yaml
version: '3'
services:
  coordinator:
    image: coordinator:latest
    ports:
      - "7777:7777"
  
  game-server-1:
    image: crunch-server:latest
    ports:
      - "7778:7777"
  
  game-server-2:
    image: crunch-server:latest
    ports:
      - "7779:7777"
```

---

## 多服务器架构

### 问题：多个房间如何处理？

**是的！每个房间都是一个独立的服务器进程！**

```
房间1（5v5）
    ↓
游戏服务器进程1（端口 7777）
    ↓
10个玩家连接到这个服务器

房间2（5v5）
    ↓
游戏服务器进程2（端口 7778）
    ↓
10个玩家连接到这个服务器

房间3（5v5）
    ↓
游戏服务器进程3（端口 7779）
    ↓
10个玩家连接到这个服务器
```

### 架构对比

#### 方案1：单服务器架构（不推荐）

```
所有玩家连接到同一个服务器
    ↓
服务器内部管理多个房间
    ↓
问题：
- 一个房间卡顿，所有房间都卡
- 一个房间崩溃，所有房间都崩溃
- 无法水平扩展
```

#### 方案2：多服务器架构（推荐，你的项目用的）

```
每个房间一个独立服务器
    ↓
房间之间互不影响
    ↓
优点：
- 房间1卡顿，不影响房间2
- 房间1崩溃，不影响房间2
- 可以水平扩展（加机器）
```

### 完整流程图

```
┌─────────────────────────────────────────────────┐
│                   协调器                         │
│  (Python Flask, 端口 7777)                      │
│  - 接收创建房间请求                              │
│  - 分配端口号                                    │
│  - 启动游戏服务器                                │
└────────────┬────────────────────────────────────┘
             │
             ├─────────────┬─────────────┬─────────────┐
             ↓             ↓             ↓             ↓
    ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐
    │游戏服务器1  │ │游戏服务器2  │ │游戏服务器3  │ │游戏服务器N  │
    │端口: 7777  │ │端口: 7778  │ │端口: 7779  │ │端口: 777N  │
    │房间: 房间1  │ │房间: 房间2  │ │房间: 房间3  │ │房间: 房间N  │
    └────────────┘ └────────────┘ └────────────┘ └────────────┘
         ↓              ↓              ↓              ↓
    10个玩家       10个玩家       10个玩家       10个玩家
```

### 资源消耗

**每个游戏服务器占用：**
```
CPU：1-2 核（取决于玩家数量）
内存：2-4 GB
网络：10-50 Mbps（取决于玩家数量）
```

**示例：**
```
1台服务器（16核，64GB内存）
可以同时运行：
- 10-15 个游戏服务器
- 支持 100-150 个玩家同时在线
```

---

## Docker 部署方案

### 为什么要用 Docker？

**开发环境 vs 生产环境**

```
开发环境（你现在的方式）：
- Windows 系统
- 直接运行 UnrealEditor.exe
- 手动管理进程
- 适合测试

生产环境（推荐用 Docker）：
- Linux 服务器
- Docker 容器运行
- 自动管理进程
- 适合部署
```

**Docker 的优势：**

| 特性 | 不用 Docker | 用 Docker |
|------|------------|-----------|
| 部署 | 手动配置环境 | 一键部署 |
| 隔离 | 进程共享资源 | 容器隔离 |
| 扩展 | 手动启动进程 | 自动扩展 |
| 管理 | 手动管理 | 自动管理 |
| 回滚 | 困难 | 一键回滚 |
| 监控 | 需要自己写 | 内置监控 |

### Docker 架构设计

#### 方案1：静态容器（简单）

**每个房间一个容器，手动创建**

```yaml
# docker-compose.yaml
version: '3'
services:
  coordinator:
    image: coordinator:latest
    ports:
      - "7777:7777"
    volumes:
      - ./coordinator:/app
  
  game-server-1:
    image: crunch-server:latest
    ports:
      - "7778:7777"
    environment:
      - SESSION_NAME=房间1
      - SESSION_SEARCH_ID=abc-123
      - PORT=7777
  
  game-server-2:
    image: crunch-server:latest
    ports:
      - "7779:7777"
    environment:
      - SESSION_NAME=房间2
      - SESSION_SEARCH_ID=def-456
      - PORT=7777
```

**启动：**
```bash
docker-compose up -d
```

**缺点：**
- 需要预先创建容器
- 不能动态增加房间
- 资源浪费（空房间也占用资源）

#### 方案2：动态容器（推荐）

**协调器动态创建容器**

**修改 `coordinator.py`：**
```python
import docker

# 创建 Docker 客户端
docker_client = docker.from_env()

def CreateServerDocker(sessionName, sessionSearchId):
    global nextAvailablePort
    
    # 创建并启动容器
    container = docker_client.containers.run(
        image='crunch-server:latest',
        detach=True,  # 后台运行
        ports={
            '7777/tcp': nextAvailablePort  # 容器内 7777 映射到主机端口
        },
        environment={
            'SESSION_NAME': sessionName,
            'SESSION_SEARCH_ID': sessionSearchId,
            'PORT': '7777'
        },
        name=f'game-server-{sessionSearchId}'
    )
    
    usedPort = nextAvailablePort
    nextAvailablePort += 1
    
    return usedPort, container.id
```

**优点：**
- ✅ 动态创建容器
- ✅ 按需分配资源
- ✅ 自动清理（游戏结束后删除容器）
- ✅ 易于扩展

#### 方案3：Kubernetes（大规模）

**适用场景：**
```
玩家数量：10000+ 同时在线
服务器数量：100+ 台
房间数量：1000+ 个
```

**Kubernetes 特点：**
- 自动扩展（根据负载自动增加/减少服务器）
- 自动恢复（服务器崩溃自动重启）
- 负载均衡（自动分配玩家到不同服务器）
- 滚动更新（不停机更新）

**架构：**
```
┌─────────────────────────────────────┐
│         Kubernetes 集群              │
│  ┌─────────────────────────────┐   │
│  │      协调器 Pod              │   │
│  └─────────────────────────────┘   │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐  │
│  │服务器1│ │服务器2│ │服务器3│ │服务器N│  │
│  │ Pod │ │ Pod │ │ Pod │ │ Pod │  │
│  └─────┘ └─────┘ └─────┘ └─────┘  │
└─────────────────────────────────────┘
```

---

## 实战演示

### 步骤1：创建 Dockerfile

**为游戏服务器创建镜像**

**文件：`ServerDeploy/server/Dockerfile`**
```dockerfile
# 使用 Ubuntu 22.04 作为基础镜像
FROM ubuntu:22.04

# 安装依赖
RUN apt-get update && apt-get install -y \
    libicu-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# 创建工作目录
WORKDIR /app

# 复制游戏服务器文件
COPY Binaries/Linux/CrunchServer /app/CrunchServer
COPY Content/ /app/Content/
COPY Config/ /app/Config/

# 设置执行权限
RUN chmod +x /app/CrunchServer

# 暴露端口
EXPOSE 7777

# 启动命令
CMD ["/app/CrunchServer", "-server", "-log"]
```

### 步骤2：构建镜像

```bash
# 进入 ServerDeploy/server 目录
cd ServerDeploy/server

# 构建镜像
docker build -t crunch-server:latest .

# 查看镜像
docker images
```

### 步骤3：创建协调器镜像

**文件：`ServerDeploy/coordinator/Dockerfile`**
```dockerfile
FROM python:3.11-slim

WORKDIR /app

# 安装依赖
COPY requirements.txt .
RUN pip install -r requirements.txt

# 复制代码
COPY coordinator.py .
COPY consts.py .

# 暴露端口
EXPOSE 7777

# 启动命令
CMD ["python", "coordinator.py"]
```

**文件：`ServerDeploy/coordinator/requirements.txt`**
```
flask==3.0.0
docker==7.0.0
```

### 步骤4：创建 docker-compose.yaml

**文件：`ServerDeploy/docker-compose.yaml`**
```yaml
version: '3.8'

services:
  # 协调器
  coordinator:
    build: ./coordinator
    image: coordinator:latest
    container_name: crunch-coordinator
    ports:
      - "7777:7777"
    volumes:
      - /var/run/docker.sock:/var/run/docker.sock  # 允许协调器创建容器
    networks:
      - crunch-network
    restart: unless-stopped

networks:
  crunch-network:
    driver: bridge
```

### 步骤5：启动服务

```bash
# 启动协调器
docker-compose up -d

# 查看日志
docker-compose logs -f coordinator

# 查看运行的容器
docker ps
```

### 步骤6：测试创建房间

```bash
# 发送请求创建房间
curl -X POST http://localhost:7777/Sessions \
  -H "Content-Type: application/json" \
  -d '{"SESSION_NAME":"测试房间","SESSION_SEARCH_ID":"test-123"}'

# 查看新创建的游戏服务器容器
docker ps | grep game-server
```

### 步骤7：监控和管理

```bash
# 查看容器状态
docker ps

# 查看容器日志
docker logs game-server-test-123

# 停止容器
docker stop game-server-test-123

# 删除容器
docker rm game-server-test-123

# 查看资源占用
docker stats
```

---

## 架构演进

### 阶段1：开发阶段（你现在的状态）

```
协调器：直接启动进程
服务器：Windows 本地运行
管理：手动管理
适用：开发测试
```

### 阶段2：小规模部署

```
协调器：Docker 容器
服务器：Docker 容器（动态创建）
管理：Docker Compose
适用：100-500 玩家
```

### 阶段3：中等规模部署

```
协调器：Docker 容器
服务器：Docker 容器（动态创建）
管理：Docker Swarm
负载均衡：Nginx
适用：500-5000 玩家
```

### 阶段4：大规模部署

```
协调器：Kubernetes Pod
服务器：Kubernetes Pod（自动扩展）
管理：Kubernetes
负载均衡：Kubernetes Ingress
监控：Prometheus + Grafana
日志：ELK Stack
适用：5000+ 玩家
```

---

## 常见问题

### Q1：不用 Docker 可以吗？

**A：可以，但不推荐生产环境**

```
开发测试：
✅ 不用 Docker，直接运行
✅ 简单快速

生产部署：
❌ 不用 Docker，管理困难
✅ 用 Docker，自动化管理
```

### Q2：Docker 会影响性能吗？

**A：影响很小（1-5%）**

```
性能对比：
- 直接运行：100% 性能
- Docker 容器：95-99% 性能
- 虚拟机：60-80% 性能

Docker 几乎没有性能损失！
```

### Q3：一台服务器能运行多少个游戏服务器？

**A：取决于硬件配置**

```
示例配置：
CPU：16 核
内存：64 GB
网络：1 Gbps

可以运行：
- 10-15 个游戏服务器
- 每个服务器 10 个玩家
- 总共 100-150 个玩家
```

### Q4：如何自动清理空房间？

**A：在协调器中添加清理逻辑**

```python
import time
import threading

# 记录容器信息
containers = {}  # {container_id: {'created_at': time, 'players': 0}}

def cleanup_empty_servers():
    while True:
        time.sleep(60)  # 每分钟检查一次
        
        for container_id, info in list(containers.items()):
            # 如果房间空了超过 5 分钟，删除容器
            if info['players'] == 0 and time.time() - info['created_at'] > 300:
                docker_client.containers.get(container_id).stop()
                docker_client.containers.get(container_id).remove()
                del containers[container_id]

# 启动清理线程
threading.Thread(target=cleanup_empty_servers, daemon=True).start()
```

### Q5：如何实现跨服务器通信？

**A：使用消息队列（如 Redis）**

```
服务器1 → Redis → 服务器2
服务器1 → Redis → 协调器
协调器 → Redis → 所有服务器
```

---

## 总结

### 关键点

```
1. ✅ 每个房间 = 一个独立的服务器进程
2. ✅ 多个房间 = 多个服务器进程同时运行
3. ✅ 协调器负责创建和管理服务器
4. ✅ Docker 用于容器化部署（生产环境推荐）
5. ✅ 开发阶段可以不用 Docker
```

### 架构选择

| 阶段 | 玩家数 | 推荐方案 |
|------|--------|----------|
| 开发测试 | 10-50 | 直接运行进程 |
| 小规模 | 100-500 | Docker Compose |
| 中等规模 | 500-5000 | Docker Swarm |
| 大规模 | 5000+ | Kubernetes |

### 下一步

```
1. 开发阶段：继续用现在的方式（直接运行）
2. 测试阶段：学习 Docker，容器化部署
3. 上线阶段：使用 Docker Compose 或 Kubernetes
```

---

**希望这份文档帮你理解了 Docker 和多服务器架构！** 🚀
