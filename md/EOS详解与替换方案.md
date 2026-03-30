# EOS 详解与替换方案

> EOS 到底做了什么？如何实现无需登录的局域网联机？

---

## 目录

1. [EOS 是什么](#eos-是什么)
2. [EOS 帮你做了什么](#eos-帮你做了什么)
3. [为什么需要登录](#为什么需要登录)
4. [如何去掉 EOS](#如何去掉-eos)
5. [局域网联机方案](#局域网联机方案)
6. [完整实现](#完整实现)

---

## EOS 是什么

### Epic Online Services（EOS）

**通俗解释：**
```
EOS = Epic 提供的免费在线服务平台
就像微信提供的登录、好友、聊天功能
但这是给游戏用的
```

**EOS 提供的服务：**
```
1. 账号系统（登录、注册）
2. 好友系统（添加好友、查看在线状态）
3. 会话系统（创建房间、搜索房间、加入房间）
4. 匹配系统（自动匹配玩家）
5. 语音聊天（玩家语音通话）
6. 排行榜（记录玩家成绩）
7. 成就系统（解锁成就）
8. 云存档（保存游戏进度）
```

---

## EOS 帮你做了什么

### 1. 账号登录

**没有 EOS：**
```
问题：怎么知道玩家是谁？
- 玩家A说："我是赵云飞"
- 玩家B也说："我是赵云飞"
- 谁是真的赵云飞？无法验证！
```

**有 EOS：**
```
1. 玩家用 Epic 账号登录
2. EOS 验证账号密码
3. EOS 返回唯一的玩家ID（如：abc123）
4. 服务器知道：abc123 = 赵云飞
5. 无法冒充（因为需要密码）
```

### 2. 会话管理

**没有 EOS：**
```
问题：怎么找到其他玩家的房间？
- 玩家A创建了房间，IP：192.168.1.100
- 玩家B怎么知道这个IP？
- 需要手动告诉玩家B这个IP
```

**有 EOS：**
```
1. 玩家A创建房间
2. EOS 记录：房间ID=room123，IP=192.168.1.100
3. 玩家B搜索房间
4. EOS 返回：找到 room123
5. 玩家B点击加入
6. EOS 告诉玩家B：连接到 192.168.1.100
```

### 3. NAT 穿透

**什么是 NAT？**
```
你的电脑 → 路由器 → 互联网

内网IP：192.168.1.100（只有你家能访问）
外网IP：123.45.67.89（全世界都能访问）

问题：
- 玩家A在家里，内网IP：192.168.1.100
- 玩家B在另一个城市
- 玩家B无法直接连接到 192.168.1.100
```

**EOS 的解决方案：**
```
1. 玩家A创建房间
2. EOS 记录玩家A的外网IP：123.45.67.89
3. 玩家B搜索房间
4. EOS 返回外网IP：123.45.67.89
5. 玩家B连接到外网IP
6. 路由器转发到玩家A的内网IP
```

### 4. 玩家身份验证

**代码示例：**

**你的项目中：**
```cpp
// Source/Crunch/Private/Framework/CGameInstance.cpp
void UCGameInstance::ClientAccountPortalLogin()
{
    ClientLogin("AccountPortal", "", "");
}
```

**EOS 做了什么：**
```
1. 弹出 Epic 登录窗口
2. 玩家输入账号密码
3. EOS 验证账号
4. 返回玩家信息：
   - 玩家ID：abc123
   - 玩家昵称：赵云飞
   - 玩家头像：https://...
5. 游戏保存玩家ID
```

### 5. 会话搜索

**代码示例：**

**你的项目中：**
```cpp
// Source/Crunch/Private/Framework/CGameInstance.cpp
void UCGameInstance::FindCreatedSession(FGuid SessionSearchId)
{
    SessionSearch->QuerySettings.Set(
        UCNetStatics::GetSessionSearchIdKey(),
        SessionSearchId.ToString(),
        EOnlineComparisonOp::Equals
    );
    
    SessionPtr->FindSessions(0, SessionSearch.ToSharedRef());
}
```

**EOS 做了什么：**
```
1. 接收搜索请求：找 SESSION_SEARCH_ID = "abc-123" 的房间
2. 在 EOS 服务器上查询数据库
3. 找到匹配的房间
4. 返回房间信息：
   - 房间名称：赵云飞的房间
   - 房间ID：abc-123
   - 服务器IP：123.45.67.89
   - 端口：7777
   - 当前玩家数：5/10
```

---

## 为什么需要登录

### 场景对比

#### 场景1：不需要登录（局域网游戏）

```
适用场景：
- 局域网联机（同一个WiFi）
- 朋友来家里玩
- 网吧联机

特点：
✅ 简单，不需要账号
✅ 快速，直接连接
❌ 只能局域网
❌ 无法保存数据
❌ 无法防作弊
```

**例子：**
```
《我的世界》局域网模式
《CS 1.6》局域网对战
《魔兽争霸3》局域网对战
```

#### 场景2：需要登录（在线游戏）

```
适用场景：
- 互联网联机（不同城市）
- 保存玩家数据
- 排行榜、成就
- 防作弊

特点：
✅ 可以互联网联机
✅ 保存玩家数据
✅ 防作弊
❌ 需要账号系统
❌ 需要服务器
```

**例子：**
```
《英雄联盟》
《王者荣耀》
《绝地求生》
```

---

## 如何去掉 EOS

### 方案1：使用 NULL 子系统（最简单）

**什么是 NULL 子系统？**
```
NULL = 空的在线子系统
不需要登录
不需要账号
只能局域网联机
```

**修改步骤：**

#### 步骤1：修改配置文件

**文件：`Config/DefaultEngine.ini`**

```ini
[OnlineSubsystem]
; 注释掉 EOS
; DefaultPlatformService=EOS

; 使用 NULL 子系统
DefaultPlatformService=NULL

[OnlineSubsystemNull]
bEnabled=true
```

#### 步骤2：修改登录代码

**文件：`Source/Crunch/Private/Framework/CGameInstance.cpp`**

```cpp
void UCGameInstance::ClientAccountPortalLogin()
{
    // 不再需要登录，直接标记为已登录
    OnLoginCompleted.Broadcast(true, "LocalPlayer", "");
}

bool UCGameInstance::IsLoggedIn() const
{
    // 总是返回 true（不需要登录）
    return true;
}
```

#### 步骤3：修改会话创建

**不需要修改！NULL 子系统也支持会话创建。**

#### 步骤4：修改会话搜索

**使用局域网搜索：**

```cpp
void UCGameInstance::FindCreatedSession(FGuid SessionSearchId)
{
    SessionSearch = MakeShareable(new FOnlineSessionSearch);
    
    // 关键：设置为局域网搜索
    SessionSearch->bIsLanQuery = true;  // ← 改成 true
    SessionSearch->MaxSearchResults = 10;
    
    // 不需要设置 QuerySettings（局域网自动发现）
    
    SessionPtr->FindSessions(0, SessionSearch.ToSharedRef());
}
```

#### 步骤5：修改会话设置

**文件：`Source/Crunch/Private/Network/CNetStatics.cpp`**

```cpp
FOnlineSessionSettings UCNetStatics::GenerateOnlineSessionSettings(
    const FName& SessionName,
    const FString& SessionSearchId,
    int Port
)
{
    FOnlineSessionSettings OnlineSessionSettings{};
    
    // 关键：设置为局域网
    OnlineSessionSettings.bIsLANMatch = true;  // ← 改成 true
    OnlineSessionSettings.NumPublicConnections = 10;
    OnlineSessionSettings.bShouldAdvertise = true;
    
    // 其他设置...
    
    return OnlineSessionSettings;
}
```

---

## 局域网联机方案

### 方案2：完全去掉在线子系统（最简单）

**不使用任何在线子系统，直接连接IP**

#### 步骤1：创建服务器

**服务器端：**
```cpp
void AMyGameMode::StartServer()
{
    // 直接监听端口
    GetWorld()->Listen(FURL(nullptr, TEXT(""), TRAVEL_Absolute));
    
    UE_LOG(LogTemp, Warning, TEXT("Server started on port 7777"));
}
```

#### 步骤2：客户端连接

**客户端：**
```cpp
void AMyPlayerController::ConnectToServer(const FString& ServerIP)
{
    // 直接连接到服务器IP
    FString TravelURL = FString::Printf(TEXT("%s:7777"), *ServerIP);
    ClientTravel(TravelURL, TRAVEL_Absolute);
}
```

#### 步骤3：UI界面

**创建简单的UI：**

```cpp
// 服务器列表UI
class UServerListWidget : public UUserWidget
{
public:
    // 输入框：输入服务器IP
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* ServerIPInput;
    
    // 按钮：连接
    UPROPERTY(meta = (BindWidget))
    UButton* ConnectButton;
    
    UFUNCTION()
    void OnConnectClicked()
    {
        FString ServerIP = ServerIPInput->GetText().ToString();
        
        // 连接到服务器
        APlayerController* PC = GetOwningPlayer();
        PC->ClientTravel(ServerIP + TEXT(":7777"), TRAVEL_Absolute);
    }
};
```

**效果：**
```
┌─────────────────────────────┐
│      加入游戏                │
│                              │
│  服务器IP：[192.168.1.100]   │
│                              │
│  [  连接  ]                  │
└─────────────────────────────┘
```

---

## 完整实现

### 方案3：自己实现简单的会话系统

**不依赖 EOS，自己实现房间列表**

#### 架构设计

```
┌─────────────────────────────────────┐
│         房间列表服务器               │
│  (Python Flask, 端口 8888)          │
│  - 记录所有房间信息                  │
│  - 提供房间列表API                   │
└────────────┬────────────────────────┘
             │
             ├─────────────┬─────────────┐
             ↓             ↓             ↓
    ┌────────────┐ ┌────────────┐ ┌────────────┐
    │游戏服务器1  │ │游戏服务器2  │ │游戏服务器3  │
    │IP: 192...  │ │IP: 192...  │ │IP: 192...  │
    │端口: 7777  │ │端口: 7778  │ │端口: 7779  │
    └────────────┘ └────────────┘ └────────────┘
```

#### 实现：房间列表服务器

**文件：`RoomListServer/server.py`**

```python
from flask import Flask, request, jsonify
import time

app = Flask(__name__)

# 存储房间列表
rooms = {}
# 格式：{
#   "room-123": {
#       "name": "赵云飞的房间",
#       "ip": "192.168.1.100",
#       "port": 7777,
#       "players": 5,
#       "max_players": 10,
#       "created_at": 1234567890
#   }
# }

@app.route('/rooms', methods=['GET'])
def get_rooms():
    """获取房间列表"""
    # 清理超时的房间（5分钟没更新）
    current_time = time.time()
    expired_rooms = [
        room_id for room_id, room in rooms.items()
        if current_time - room['created_at'] > 300
    ]
    for room_id in expired_rooms:
        del rooms[room_id]
    
    return jsonify(list(rooms.values()))

@app.route('/rooms', methods=['POST'])
def create_room():
    """创建房间"""
    data = request.get_json()
    
    room_id = data['room_id']
    rooms[room_id] = {
        'name': data['name'],
        'ip': data['ip'],
        'port': data['port'],
        'players': data.get('players', 0),
        'max_players': data.get('max_players', 10),
        'created_at': time.time()
    }
    
    return jsonify({'status': 'success', 'room_id': room_id})

@app.route('/rooms/<room_id>', methods=['PUT'])
def update_room(room_id):
    """更新房间信息（玩家数量）"""
    if room_id not in rooms:
        return jsonify({'error': 'Room not found'}), 404
    
    data = request.get_json()
    rooms[room_id]['players'] = data.get('players', 0)
    rooms[room_id]['created_at'] = time.time()  # 更新时间
    
    return jsonify({'status': 'success'})

@app.route('/rooms/<room_id>', methods=['DELETE'])
def delete_room(room_id):
    """删除房间"""
    if room_id in rooms:
        del rooms[room_id]
    return jsonify({'status': 'success'})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8888)
```

#### 实现：UE客户端

**文件：`Source/Crunch/Public/Network/CRoomListClient.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Http.h"

USTRUCT(BlueprintType)
struct FRoomInfo
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadOnly)
    FString RoomID;
    
    UPROPERTY(BlueprintReadOnly)
    FString Name;
    
    UPROPERTY(BlueprintReadOnly)
    FString IP;
    
    UPROPERTY(BlueprintReadOnly)
    int32 Port;
    
    UPROPERTY(BlueprintReadOnly)
    int32 Players;
    
    UPROPERTY(BlueprintReadOnly)
    int32 MaxPlayers;
};

class CRUNCH_API UCRoomListClient
{
public:
    // 获取房间列表
    static void GetRoomList(TFunction<void(TArray<FRoomInfo>)> Callback);
    
    // 创建房间
    static void CreateRoom(const FString& RoomName, int32 Port, TFunction<void(bool)> Callback);
    
    // 更新房间信息
    static void UpdateRoom(const FString& RoomID, int32 Players);
    
    // 删除房间
    static void DeleteRoom(const FString& RoomID);
    
private:
    static FString ServerURL;  // "http://192.168.1.100:8888"
};
```

**文件：`Source/Crunch/Private/Network/CRoomListClient.cpp`**

```cpp
#include "Network/CRoomListClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"

FString UCRoomListClient::ServerURL = TEXT("http://192.168.1.100:8888");

void UCRoomListClient::GetRoomList(TFunction<void(TArray<FRoomInfo>)> Callback)
{
    // 创建 HTTP 请求
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ServerURL + TEXT("/rooms"));
    Request->SetVerb(TEXT("GET"));
    
    // 绑定回调
    Request->OnProcessRequestComplete().BindLambda(
        [Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            TArray<FRoomInfo> Rooms;
            
            if (bSuccess && Response->GetResponseCode() == 200)
            {
                // 解析 JSON
                FString ResponseStr = Response->GetContentAsString();
                TSharedPtr<FJsonValue> JsonValue;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
                
                if (FJsonSerializer::Deserialize(Reader, JsonValue))
                {
                    TArray<TSharedPtr<FJsonValue>> JsonArray = JsonValue->AsArray();
                    
                    for (const TSharedPtr<FJsonValue>& Item : JsonArray)
                    {
                        TSharedPtr<FJsonObject> JsonObject = Item->AsObject();
                        
                        FRoomInfo Room;
                        Room.Name = JsonObject->GetStringField(TEXT("name"));
                        Room.IP = JsonObject->GetStringField(TEXT("ip"));
                        Room.Port = JsonObject->GetIntegerField(TEXT("port"));
                        Room.Players = JsonObject->GetIntegerField(TEXT("players"));
                        Room.MaxPlayers = JsonObject->GetIntegerField(TEXT("max_players"));
                        
                        Rooms.Add(Room);
                    }
                }
            }
            
            Callback(Rooms);
        }
    );
    
    // 发送请求
    Request->ProcessRequest();
}

void UCRoomListClient::CreateRoom(const FString& RoomName, int32 Port, TFunction<void(bool)> Callback)
{
    // 生成房间ID
    FString RoomID = FGuid::NewGuid().ToString();
    
    // 获取本机IP
    FString LocalIP = TEXT("192.168.1.100");  // 实际应该获取真实IP
    
    // 创建 JSON
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("room_id"), RoomID);
    JsonObject->SetStringField(TEXT("name"), RoomName);
    JsonObject->SetStringField(TEXT("ip"), LocalIP);
    JsonObject->SetNumberField(TEXT("port"), Port);
    JsonObject->SetNumberField(TEXT("players"), 0);
    JsonObject->SetNumberField(TEXT("max_players"), 10);
    
    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    // 创建 HTTP 请求
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ServerURL + TEXT("/rooms"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(RequestBody);
    
    // 绑定回调
    Request->OnProcessRequestComplete().BindLambda(
        [Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            bool bCreated = bSuccess && Response->GetResponseCode() == 200;
            Callback(bCreated);
        }
    );
    
    // 发送请求
    Request->ProcessRequest();
}
```

#### 实现：UI界面

**房间列表UI：**

```cpp
class UServerBrowserWidget : public UUserWidget
{
public:
    UPROPERTY(meta = (BindWidget))
    UListView* RoomListView;
    
    UPROPERTY(meta = (BindWidget))
    UButton* RefreshButton;
    
    UPROPERTY(meta = (BindWidget))
    UButton* CreateRoomButton;
    
    void NativeConstruct() override
    {
        Super::NativeConstruct();
        
        RefreshButton->OnClicked.AddDynamic(this, &UServerBrowserWidget::OnRefreshClicked);
        CreateRoomButton->OnClicked.AddDynamic(this, &UServerBrowserWidget::OnCreateRoomClicked);
        
        RefreshRoomList();
    }
    
    UFUNCTION()
    void OnRefreshClicked()
    {
        RefreshRoomList();
    }
    
    UFUNCTION()
    void OnCreateRoomClicked()
    {
        // 打开创建房间界面
        // ...
    }
    
    void RefreshRoomList()
    {
        UCRoomListClient::GetRoomList([this](TArray<FRoomInfo> Rooms)
        {
            RoomListView->ClearListItems();
            
            for (const FRoomInfo& Room : Rooms)
            {
                // 添加到列表
                URoomListItem* Item = NewObject<URoomListItem>();
                Item->RoomInfo = Room;
                RoomListView->AddItem(Item);
            }
        });
    }
    
    void JoinRoom(const FRoomInfo& Room)
    {
        // 连接到服务器
        FString TravelURL = FString::Printf(TEXT("%s:%d"), *Room.IP, Room.Port);
        GetOwningPlayer()->ClientTravel(TravelURL, TRAVEL_Absolute);
    }
};
```

---

## 方案对比

### 各方案优缺点

| 方案 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| EOS | 功能完整、免费、稳定 | 需要登录、依赖Epic | 正式发布的游戏 |
| NULL子系统 | 简单、无需登录 | 只能局域网 | 局域网联机 |
| 直接连接IP | 最简单 | 需要手动输入IP | 朋友之间联机 |
| 自建房间列表 | 灵活、可控 | 需要自己维护服务器 | 小型独立游戏 |

### 推荐方案

**开发阶段：**
```
使用 NULL 子系统或直接连接IP
快速测试，不需要登录
```

**发布阶段：**
```
小型游戏：自建房间列表服务器
中大型游戏：使用 EOS（免费且功能完整）
```

---

## 总结

### EOS 的作用

```
1. ✅ 账号登录和身份验证
2. ✅ 会话管理（创建、搜索、加入房间）
3. ✅ NAT 穿透（互联网联机）
4. ✅ 好友系统
5. ✅ 语音聊天
6. ✅ 排行榜、成就
```

### 如何去掉 EOS

```
方案1：使用 NULL 子系统（局域网）
方案2：直接连接IP（最简单）
方案3：自建房间列表服务器（推荐）
```

### 关键代码修改

```cpp
// 1. 修改配置
DefaultPlatformService=NULL

// 2. 设置局域网
OnlineSessionSettings.bIsLANMatch = true;
SessionSearch->bIsLanQuery = true;

// 3. 或者直接连接
ClientTravel("192.168.1.100:7777", TRAVEL_Absolute);
```

---

**现在你可以实现无需登录的局域网联机了！** 🎮
