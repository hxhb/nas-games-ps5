# NAS Games for PS5

NAS Games 是一个 PS5 Homebrew payload。它在 PS5 上运行本地 Web 服务，从 SMB、FTP 和本机目录扫描游戏，并通过适合电视和手柄操作的海报墙查看、安装和管理资源。

[下载 NAS Games 1.0.3](https://github.com/hxhb/nas-games-ps5/releases/tag/v1.0.3) · [开发与构建](docs/development.md)

![NAS Games 游戏库。使用演示数据展示已安装和未安装的游戏](docs/screenshots/library.png)

## 功能

- **游戏库**支持多个连接和扫描目录，可按标题或 Title ID 搜索，按安装状态和格式筛选。同一 Title ID 的基础资源、补丁和 DLC 归到一张卡片。
- **封面缓存**从游戏元数据读取标题、图标和背景。详情页使用缓存的 JPEG 背景图，在手柄聚焦海报时预加载。
- **安装与传输**支持 PKG 流式安装、文件夹导入和压缩文件夹复制。安装前检查空间，传输面板展示队列、速度和进度，支持取消活动任务和清除最近记录。
- **连接管理**支持 SMB 共享枚举、目录浏览和草稿连接测试。设置中的「需要关注」列出待装的内容包、载荷不完整和连接异常。
- **主机入口**支持 Payload Manager 的 Autoload。启动 payload 后默认只启动服务，可从 PS5「媒体」中的「NAS 游戏库」手动打开。

![NAS Games 游戏详情。使用演示数据展示资源格式、可用空间与内容包](docs/screenshots/details.png)

## 安装与使用

1. 在能运行 PS5 Homebrew payload 的主机上启动 [Payload Manager](https://github.com/itsPLK/ps5-payload-manager)。从 [Release 页面](https://github.com/hxhb/nas-games-ps5/releases/latest) 下载 `nasgames.elf`。
2. 在 Payload Manager 上传并运行 `nasgames.elf`。需要随 Payload Manager 启动时，将 `local/nasgames` 加入 Autoload。
3. 在 PS5「媒体」中打开「NAS 游戏库」，或在同一局域网的浏览器打开 `http://<PS5-IP>:8098/`。
4. 打开「设置 → 连接」，添加来源并测试连接。选择扫描目录后保存，等待游戏出现在库中。

例如，NAS 上的路径是 `\\nas\Games\PS5\Library` 时，主机填写 `nas`，共享名填写 `Games`，扫描目录填写 `/PS5/Library`。SMB 默认端口为 `445`。

## 安装目标

| 资源 | 安装方式 |
| --- | --- |
| PKG 基础包 | 从来源流式传给 PS5 系统安装器 |
| PKG 补丁、DLC | 从游戏详情页单独安装，需要对应的基础游戏 |
| 普通文件夹 | 复制到 `/data/nasgames/library/<Title ID>`，由支持的固件接口注册到主界面 |
| ZSTD / LZ4 文件夹 | 复制到 `/data/homebrew/<Title ID>`，可使用 Kraken Mount 挂载 |

详情显示来源大小和目标盘可用空间。PKG 的安装占用由系统决定，可能超过包文件大小。PKG 的取消和文件夹的主界面注册依赖固件提供的接口。最近任务保留到 payload 重启，也可在传输面板手动清除。

Web 服务在受信任的局域网使用，当前 API 无登录认证。配置和封面缓存在主机的 `/data/nasgames`。

## 从源码构建

在 Ubuntu 24.04 或更新的 x86-64 Linux 环境中，使用 GNU Make 4.3+、Python 3 和系统构建工具。WSL 也可构建。

```bash
make bootstrap
make nasgames
```

构建输出为 `apps/nasgames/nasgames.elf`。源码构建、主机调试、依赖和部署命令见[开发文档](docs/development.md)。
