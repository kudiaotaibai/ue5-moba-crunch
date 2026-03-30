FROM python:3.9-slim

RUN \
    # 核心修正：使用 > 覆盖创建新文件，这会隐式地删除旧内容。
    # 并且我们只保留最核心的 main 和 security 源。
    echo "deb http://mirrors.aliyun.com/debian/ bullseye main contrib non-free" > /etc/apt/sources.list && \
    echo "deb http://mirrors.aliyun.com/debian-security bullseye-security main contrib non-free" >> /etc/apt/sources.list && \
    \
    # 确保旧的碎片化配置文件目录被清空
    rm -rf /etc/apt/sources.list.d/* && \
    \
    # 更新软件包列表并安装curl
    apt-get update && \
    apt-get install -y --no-install-recommends \
        curl \
	docker.io\

    && \
    # 清理APT缓存
    rm -rf /var/lib/apt/lists/*
COPY . /
WORKDIR /coordinator


# 修正2：为 pip 添加国内源
RUN pip3 install flask -i https://pypi.tuna.tsinghua.edu.cn/simple

ENTRYPOINT ["python", "coordinator.py"]