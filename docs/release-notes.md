# Game Center 1.0.13

Game Center 可在 PS5 上浏览 NAS 游戏库，通过系统安装器流式安装 PKG，并从详情页直接启动已安装游戏。

## 主要更新

- **已安装详情与启动体验**：精简已安装游戏详情页（隐藏无意义的空间检查、状态标签和冗余文案，补丁与附加内容仅在存在时显示）；打开后立即聚焦“启动游戏”，首次按 X 直接启动；启动后通过 PS5 应用接口关闭浏览器窗口，避免遮挡前台游戏；PC 远程浏览器启动不再出现 12 秒遮罩误报。
- **SMB 传输与安装稳定性**：预读区间内保持 8×1 MiB 异步请求跨槽位连续在途，消除 8 MiB 槽位边界排空停顿；文件夹安装遇 SMB 超时自动重连并从断点续传（最多 3 次）；小文件落盘改为按批次 `fsync()` 并加入目录递归深度保护；安装完成后等待扫描结束再刷新游戏库。
- **服务端架构与内存安全**：`/api/state` 支持按 `catalogRevision` 与 `instance` 增量轮询；扫描线程改为事件驱动条件变量唤醒；嵌入式静态资源支持 `ETag`/`304` 与版本化长缓存；消除各线程栈上大块 `NgConfig` 与 128 KiB 局部缓冲，HTTP 线程栈收敛至 512 KiB，优雅退出时完整释放来源与目录内存。
- **反向代理与前端健壮性**：支持反向代理 HTTPS 外部域名访问（兼容 `X-Forwarded-Proto`/`X-Forwarded-Host` 与专用请求头 CSRF 校验）；前端 `api()` 增加超时保护，后台或无手柄时自动降频轮询与暂停手柄循环；修复空状态重复图标与清空筛选未同步格式菜单的问题。
- **版本缓存与兼容**：页面检测到 payload 版本变化自动刷新，媒体入口带版本号避免旧缓存；保留 `nasgames.elf`、`local/nasgames` 和 `/data/nasgames`，兼容已有 Autoload、Payload Manager 槽位和配置。

## 下载

- `nasgames.elf` 上传到 Payload Manager 后运行。默认地址为 `http://<PS5-IP>:8098/`。
- `SHA256SUMS` 可用 `sha256sum -c SHA256SUMS` 核对下载。
- `nasgames-1.0.13-source.tar.gz` 包含此版本的项目源码、构建脚本及链接依赖源码。

兼容性取决于 PS5 固件和系统安装接口。普通文件夹可导入本地库。ZSTD 和 LZ4 目录可复制到 `/data/homebrew` 后用 Kraken Mount 挂载。

发行版使用 libsmb2 6.2（LGPL-2.1-or-later）和 PS5 Payload SDK v0.50。源码包附带第三方源码、许可文本和重新构建说明。[第三方声明](https://github.com/hxhb/game-center-ps5/blob/v1.0.13/THIRD_PARTY_NOTICES.md)
