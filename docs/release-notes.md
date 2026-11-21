# NAS Games 1.0.2

NAS Games 可在 PS5 上浏览 NAS 游戏库，通过系统安装器流式安装 PKG，并从 Payload Manager 管理启动。

- 支持 SMB、FTP 和本地目录，多连接和多扫描路径可在网页中配置。
- 海报墙支持手柄、安装状态筛选、格式筛选和搜索。游戏详情展示缓存背景、空间检查和同一游戏的补丁及 DLC。
- 传输面板展示队列、累计大小、速度和近期结果。活动任务可请求取消，完成记录可手动清除。
- 支持 Payload Manager Autoload 和 PS5「媒体」快捷方式。加载 payload 时默认只启动后台服务。
- 修复超过 1 MiB 的小文件在第二次 SMB 读取时使用未分配缓冲区的问题，并保留底层网络错误详情。
- 修复复制完成后 `eboot.bin` 的权限检查误判，使用文件类型和大小校验。
- 安装文件夹后递归设置目录和文件权限为 `0777`，避免 `mkdtemp()` 创建的 `0700` 根目录在挂载后无法访问。
- 改进了 SMB 超时恢复、FTP 分段读取、取消后重试、配置保存和界面焦点恢复。

## 下载

- `nasgames.elf` 上传到 Payload Manager 后运行。默认地址为 `http://<PS5-IP>:8098/`。
- `SHA256SUMS` 可用 `sha256sum -c SHA256SUMS` 核对下载。
- `nasgames-1.0.2-source.tar.gz` 包含此版本的项目源码、构建脚本及链接依赖源码。

兼容性取决于 PS5 固件和系统安装接口。普通文件夹可导入本地库。ZSTD 和 LZ4 目录可复制到 `/data/homebrew` 后用 Kraken Mount 挂载。

发行版使用 libsmb2 6.2（LGPL-2.1-or-later）和 PS5 Payload SDK v0.50。源码包附带第三方源码、许可文本和重新构建说明。[第三方声明](https://github.com/hxhb/nas-games-ps5/blob/v1.0.2/THIRD_PARTY_NOTICES.md)
