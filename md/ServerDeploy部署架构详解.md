# ServerDeploy 部署架构详解

> 详细解析项目中的 Docker 部署配置

---

## 目录

1. [文件结构分析](#文件结构分析)
2. [docker-compose.yaml 详解](#docker-composeyaml-详解)
3. [Coordinator Dockerfile 详解](#coordinator-dockerfile-详解)
4. [Server Dockerfile 详解](#server-dockerfile-详解)
5. [部署流程](#部署流程)
6. [问题和改进建议](#问题和改进建议)

---

## 文件结构分析

### 当前目录结构

```
ServerDeploy/
├── docker-compose.yaml          # Docker Compose 配置文件
├── coordinator/                 # 协调器容器配置
│   ├── Dockerfile              # 协调器镜像构建文件
│   └── coordinator/            # 协调器代码目录
│       ├── coordinator.py
│       └── consts.py
└── server/                      # 游戏服务器容器配置
    ├── Dockerfile              # 服务器镜像构建文件
    └── server/                 # 服务器文件目录
        └── Crunch/
            └── Binaries/Linux/
                └── CrunchServer  # Linux 服务器可执行文件
```

### 架构图

```
┌─────────────────────────────────────────────┐
│           Docker Compose                    │
│  ┌────────────────┐  ┌──────────────────┐  │
│  │  Coordinator   │  │   Server Image   │  │
│  │   Container    │  │   (Template)     │  │
│  │                │  │                  │  │
│  │  - Flask API   │  │  - CrunchServer  │  │
│  │  - 端口 80     │  │  - Linux Binary  │  │
│  │  - 创建服务器  │  │                  │  │
│  └────────┬───────┘  └──────────────────┘  │
│           │                                 │
│           │ 动态创建                        │
│           ↓                                 │
│  ┌────────────────┐  ┌──────────────────┐  │
│  │ Game Server 1  │  │ Game Server 2    │  │
│  │ (Container)    │  │ (Container)      │  │
│  └────────────────┘  └──────────────────┘  │
└─────────────────────────────────────────────┘
```

---

## docker-compose.yaml 详解

### 完整配置

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

### 逐行解析

#### 1. services 定义

```yaml
services:
```
- **作用：** 定义要运行的服务（容器）
- **通俗解释：** 就像定义"这个项目需要哪些程序"

#### 2. server 服务（游戏服务器镜像）

```yaml
  server:
    build: ./server
    image: server
```

**逐行解释：**

```yaml
server:
```
- **服务名：** server
- **作用：** 定义游戏服务器镜像
- **注意：** 这里只是构建镜像，不会启动容器

```yaml
build: ./server
```
- **作用：** 从 `./server` 目录构建镜像
- **查找：** 会使用 `./server/Dockerfile`
- **通俗解释：** 告诉 Docker "去 server 文件夹找 Dockerfile，按照它的指令构建镜像"

```yaml
image: server
```
- **作用：** 构建出来的镜像名称为 `server`
- **用途：** 协调器会使用这个镜像来创建游戏服务器容器

**关键理解：**
```
这个 server 服务不会启动容器！
它只是构建一个镜像（模板）
协调器会用这个镜像动态创建游戏服务器容器
```

#### 3. coordinator 服务（协调器）

```yaml
  coordinator:
    build: ./coordinator
    image: coordinator
    container_name: servercoordinator
    volumes:
      - /var/run/docker.sock:/var/run/docker.sock
    ports:
      - '80:80'
```

**逐行解释：**

```yaml
coordinator:
```
- **服务名：** coordinator
- **作用：** 定义协调器服务

```yaml
build: ./coordinator
```
- **作用：** 从 `./coordinator` 目录构建镜像
- **查找：** 使用 `./coordinator/Dockerfile`

```yaml
image: coordinator
```
- **作用：** 镜像名称为 `coordinator`

```yaml
container_name: servercoordinator
```
- **作用：** 容器名称为 `servercoordinator`
- **区别：**
  - `image: coordinator` - 镜像名（模板）
  - `container_name: servercoordinator` - 容器名（运行实例）

```yaml
volumes:
  - /var/run/docker.sock:/var/run/docker.sock
```
- **作用：** 挂载 Docker Socket
- **格式：** `主机路径:容器路径`
- **关键：** 这让协调器容器能够控制主机的 Docker

**Docker Socket 详解：**
```
/var/run/docker.sock = Docker 的"遥控器"

挂载后，协调器容器可以：
✅ 创建新容器（游戏服务器）
✅ 停止容器
✅ 删除容器
✅ 查看容器列表

就像给协调器一个"遥控器"，让它能控制 Docker
```

```yaml
ports:
  - '80:80'
```
- **作用：** 端口映射
- **格式：** `主机端口:容器端口`
- **效果：** 访问主机的 80 端口 = 访问容器的 80 端口

**端口映射图：**
```
外部请求 → 主机 80 端口 → 容器 80 端口 → Flask 应用
```

---

## Coordinator Dockerfile 详解

### 完整配置

```dockerfile
FROM python:3.9-slim

RUN \
    echo "deb http://mirrors.aliyun.com/debian/ bullseye main contrib non-free" > /etc/apt/sources.list && \
    echo "deb http://mirrors.aliyun.com/debian-security bullseye-security main contrib non-free" >> /etc/apt/sources.list && \
    rm -rf /etc/apt/sources.list.d/* && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        curl \
        docker.io \
    && \
    rm -rf /var/lib/apt/lists/*

COPY . /
WORKDIR /coordinator

RUN pip3 install flask -i https://pypi.tuna.tsinghua.edu.cn/simple

ENTRYPOINT ["python", "coordinator.py"]
```

### 逐行解析

#### 1. 基础镜像

```dockerfile
FROM python:3.9-slim
```
- **FROM：** 指定基础镜像
- **python:3.9-slim：** Python 3.9 的精简版
- **slim：** 体积小（约 50MB），只包含必要组件
- **通俗解释：** 就像选择一个"操作系统模板"

**镜像对比：**
```
python:3.9        - 完整版（约 900MB）
python:3.9-slim   - 精简版（约 50MB）  ← 你用的
python:3.9-alpine - 超精简版（约 20MB）
```

#### 2. 配置软件源（中国镜像）

```dockerfile
RUN \
    echo "deb http://mirrors.aliyun.com/debian/ bullseye main contrib non-free" > /etc/apt/sources.list && \
    echo "deb http://mirrors.aliyun.com/debian-security bullseye-security main contrib non-free" >> /etc/apt/sources.list && \
    rm -rf /etc/apt/sources.list.d/* && \
```

**解释：**
- **作用：** 配置阿里云镜像源（加速下载）
- **> /etc/apt/sources.list：** 覆盖写入（清空原内容）
- **>> /etc/apt/sources.list：** 追加写入
- **rm -rf /etc/apt/sources.list.d/*：** 删除旧的配置文件

**为什么要这样做？**
```
默认源：国外服务器（慢）
阿里云源：国内服务器（快）

下载速度对比：
默认源：100 KB/s
阿里云源：10 MB/s（快100倍！）
```

#### 3. 安装依赖

```dockerfile
    apt-get update && \
    apt-get install -y --no-install-recommends \
        curl \
        docker.io \
    && \
    rm -rf /var/lib/apt/lists/*
```

**逐行解释：**

```dockerfile
apt-get update
```
- **作用：** 更新软件包列表
- **类比：** 就像刷新应用商店的软件列表

```dockerfile
apt-get install -y --no-install-recommends curl docker.io
```
- **install -y：** 自动确认安装（不需要手动输入 yes）
- **--no-install-recommends：** 不安装推荐的软件包（减小体积）
- **curl：** 命令行下载工具
- **docker.io：** Docker 客户端（让协调器能控制 Docker）

**为什么要安装 docker.io？**
```
协调器需要创建游戏服务器容器
需要 Docker 客户端来控制 Docker
```

```dockerfile
rm -rf /var/lib/apt/lists/*
```
- **作用：** 删除 apt 缓存
- **效果：** 减小镜像体积（约 50MB）
- **通俗解释：** 安装完软件后，删除安装包

#### 4. 复制文件

```dockerfile
COPY . /
WORKDIR /coordinator
```

**解释：**

```dockerfile
COPY . /
```
- **格式：** `COPY 源路径 目标路径`
- **. ：** 当前目录（`ServerDeploy/coordinator/`）
- **/ ：** 容器的根目录
- **效果：** 把 `coordinator/` 下的所有文件复制到容器的 `/` 目录

**复制结果：**
```
主机：ServerDeploy/coordinator/coordinator/coordinator.py
容器：/coordinator/coordinator.py
```

```dockerfile
WORKDIR /coordinator
```
- **作用：** 设置工作目录
- **效果：** 后续命令都在 `/coordinator` 目录执行
- **类比：** 就像 `cd /coordinator`

#### 5. 安装 Python 依赖

```dockerfile
RUN pip3 install flask -i https://pypi.tuna.tsinghua.edu.cn/simple
```

**解释：**
- **pip3 install flask：** 安装 Flask 框架
- **-i https://pypi.tuna.tsinghua.edu.cn/simple：** 使用清华大学镜像源
- **为什么用镜像源：** 加速下载（国内快）

**速度对比：**
```
默认源（PyPI）：100 KB/s
清华源：10 MB/s
```

#### 6. 启动命令

```dockerfile
ENTRYPOINT ["python", "coordinator.py"]
```

**解释：**
- **ENTRYPOINT：** 容器启动时执行的命令
- **["python", "coordinator.py"]：** 运行 Python 脚本
- **效果：** 容器启动后自动运行协调器

**对比 CMD 和 ENTRYPOINT：**
```dockerfile
CMD ["python", "coordinator.py"]
# 可以被覆盖：docker run myimage python other.py

ENTRYPOINT ["python", "coordinator.py"]
# 不能被覆盖，总是运行这个命令
```

---

## Server Dockerfile 详解

### 完整配置

```dockerfile
FROM gcr.io/distroless/nodejs22-debian12:nonroot

COPY --chown=nonroot:nonroot ./server /home/nonroot/server

ENTRYPOINT [ "/home/nonroot/server/Crunch/Binaries/Linux/CrunchServer", "Crunch" ]
```

### 逐行解析

#### 1. 基础镜像

```dockerfile
FROM gcr.io/distroless/nodejs22-debian12:nonroot
```

**什么是 Distroless？**
```
Distroless = 无发行版镜像
只包含应用运行所需的最小依赖
不包含：
- Shell（bash、sh）
- 包管理器（apt、yum）
- 其他工具

优点：
✅ 体积极小（约 20MB）
✅ 安全性高（攻击面小）
✅ 启动快

缺点：
❌ 无法进入容器调试（没有 shell）
❌ 无法安装额外软件
```

**为什么选择 nodejs22？**
```
⚠️ 这里有问题！
游戏服务器是 C++ 程序，不需要 Node.js
应该使用：
- gcr.io/distroless/cc-debian12:nonroot（C++ 运行时）
- 或 ubuntu:22.04（如果需要调试）
```

**nonroot 的含义：**
```
nonroot = 非 root 用户运行
安全性更高（即使被攻击，权限也有限）
```

#### 2. 复制文件

```dockerfile
COPY --chown=nonroot:nonroot ./server /home/nonroot/server
```

**解释：**
- **COPY：** 复制文件
- **--chown=nonroot:nonroot：** 设置文件所有者为 nonroot 用户
- **./server：** 主机上的 `ServerDeploy/server/server/` 目录
- **/home/nonroot/server：** 容器内的目标路径

**为什么要 chown？**
```
容器以 nonroot 用户运行
文件所有者也必须是 nonroot
否则可能没有权限执行
```

#### 3. 启动命令

```dockerfile
ENTRYPOINT [ "/home/nonroot/server/Crunch/Binaries/Linux/CrunchServer", "Crunch" ]
```

**解释：**
- **ENTRYPOINT：** 容器启动命令
- **第一个参数：** 可执行文件路径
- **第二个参数：** 传递给程序的参数（项目名称）

**等价于：**
```bash
/home/nonroot/server/Crunch/Binaries/Linux/CrunchServer Crunch
```

---

## 部署流程

### 步骤1：准备文件

```bash
ServerDeploy/
├── coordinator/
│   └── coordinator/
│       ├── coordinator.py  # 协调器代码
│       └── consts.py
└── server/
    └── server/
        └── Crunch/
            └── Binaries/Linux/
                └── CrunchServer  # Linux 服务器可执行文件
```

### 步骤2：构建镜像

```bash
# 进入 ServerDeploy 目录
cd ServerDeploy

# 构建镜像
docker-compose build

# 查看构建的镜像
docker images
```

**输出：**
```
REPOSITORY      TAG       SIZE
coordinator     latest    200MB
server          latest    500MB
```

### 步骤3：启动服务

```bash
# 启动协调器
docker-compose up -d

# 查看运行的容器
docker ps
```

**输出：**
```
CONTAINER ID   IMAGE         PORTS                NAMES
abc123         coordinator   0.0.0.0:80->80/tcp   servercoordinator
```

### 步骤4：测试

```bash
# 测试协调器
curl http://localhost:80/

# 创建游戏服务器
curl -X POST http://localhost:80/Sessions \
  -H "Content-Type: application/json" \
  -d '{"SESSION_NAME":"测试房间","SESSION_SEARCH_ID":"test-123"}'
```

### 步骤5：查看游戏服务器容器

```bash
# 查看所有容器（包括动态创建的游戏服务器）
docker ps -a
```

---

## 问题和改进建议

### 问题1：Server 镜像使用了错误的基础镜像

**当前：**
```dockerfile
FROM gcr.io/distroless/nodejs22-debian12:nonroot
```

**问题：**
- 游戏服务器是 C++ 程序，不需要 Node.js
- 可能缺少必要的 C++ 运行时库

**建议修改：**
```dockerfile
# 方案1：使用 C++ 运行时镜像
FROM gcr.io/distroless/cc-debian12:nonroot

# 方案2：使用 Ubuntu（方便调试）
FROM ubuntu:22.04

# 安装依赖
RUN apt-get update && apt-get install -y \
    libicu-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

COPY ./server /app/server
WORKDIR /app/server

ENTRYPOINT ["/app/server/Crunch/Binaries/Linux/CrunchServer", "Crunch"]
```

### 问题2：协调器端口配置不一致

**docker-compose.yaml：**
```yaml
ports:
  - '80:80'  # 映射到 80 端口
```

**coordinator.py：**
```python
app.run(host="0.0.0.0", port=7777)  # 监听 7777 端口
```

**问题：**
- 配置不一致！
- 外部访问 80 端口，但容器内监听 7777 端口
- 会导致连接失败

**建议修改：**

**方案1：修改 docker-compose.yaml**
```yaml
ports:
  - '7777:7777'  # 改成 7777
```

**方案2：修改 coordinator.py**
```python
app.run(host="0.0.0.0", port=80)  # 改成 80
```

### 问题3：缺少环境变量配置

**建议添加：**

```yaml
# docker-compose.yaml
coordinator:
  build: ./coordinator
  image: coordinator
  container_name: servercoordinator
  environment:
    - COORDINATOR_PORT=7777
    - SERVER_IMAGE=server:latest
    - NEXT_AVAILABLE_PORT=7777
  volumes:
    - /var/run/docker.sock:/var/run/docker.sock
  ports:
    - '7777:7777'
```

### 问题4：缺少网络配置

**建议添加：**

```yaml
# docker-compose.yaml
version: '3.8'

services:
  coordinator:
    # ...
    networks:
      - crunch-network

networks:
  crunch-network:
    driver: bridge
```

### 改进后的完整配置

**docker-compose.yaml：**
```yaml
version: '3.8'

services:
  # 游戏服务器镜像（模板）
  server:
    build: ./server
    image: crunch-server:latest
    # 不启动容器，只构建镜像

  # 协调器
  coordinator:
    build: ./coordinator
    image: crunch-coordinator:latest
    container_name: crunch-coordinator
    environment:
      - COORDINATOR_PORT=7777
      - SERVER_IMAGE=crunch-server:latest
    volumes:
      - /var/run/docker.sock:/var/run/docker.sock
    ports:
      - '7777:7777'
    networks:
      - crunch-network
    restart: unless-stopped

networks:
  crunch-network:
    driver: bridge
```

**coordinator/Dockerfile：**
```dockerfile
FROM python:3.9-slim

# 配置阿里云镜像源
RUN echo "deb http://mirrors.aliyun.com/debian/ bullseye main" > /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        curl \
        docker.io \
    && rm -rf /var/lib/apt/lists/*

# 复制代码
COPY ./coordinator /app
WORKDIR /app

# 安装 Python 依赖
RUN pip3 install flask docker -i https://pypi.tuna.tsinghua.edu.cn/simple

# 暴露端口
EXPOSE 7777

# 启动命令
ENTRYPOINT ["python", "coordinator.py"]
```

**server/Dockerfile：**
```dockerfile
FROM ubuntu:22.04

# 安装依赖
RUN apt-get update && apt-get install -y \
    libicu-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# 复制服务器文件
COPY ./server/Crunch /app/Crunch
WORKDIR /app

# 设置执行权限
RUN chmod +x /app/Crunch/Binaries/Linux/CrunchServer

# 暴露端口
EXPOSE 7777

# 启动命令
ENTRYPOINT ["/app/Crunch/Binaries/Linux/CrunchServer", "Crunch", "-server", "-log"]
```

---

## 总结

### 当前架构

```
1. ✅ 使用 Docker Compose 管理容器
2. ✅ 协调器可以动态创建游戏服务器容器
3. ✅ 使用阿里云镜像源加速下载
4. ⚠️ 存在一些配置问题需要修复
```

### 关键点

```
1. server 服务只构建镜像，不启动容器
2. coordinator 服务启动并持续运行
3. 协调器通过 Docker Socket 控制 Docker
4. 游戏服务器容器由协调器动态创建
```

### 改进建议

```
1. 修复 Server 镜像的基础镜像（改用 Ubuntu 或 distroless/cc）
2. 统一端口配置（7777）
3. 添加环境变量配置
4. 添加网络配置
5. 添加健康检查
6. 添加日志管理
```

---

**现在你应该完全理解 ServerDeploy 的部署架构了！** 🚀
