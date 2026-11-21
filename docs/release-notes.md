# Game Center 1.0.5

Game Center 可在 PS5 上浏览 NAS 游戏库，通过系统安装器流式安装 PKG，并从详情页直接启动已安装游戏。

- 产品界面、通知和 PS5「媒体」入口统一更名为 Game Center。
- 已安装游戏可在详情页直接启动；启动时隐藏自身界面并切换到游戏，必要时先退出当前前台应用。
- 启动请求异步执行，重复请求会返回明确状态，失败时保留日志并发送 PS5 通知。
- 修复格式筛选在部分缓存和手柄焦点路径下无法切换的问题，并补齐菜单内方向键导航与确认选择。
- 静态界面资源加入版本参数，避免 PS5 浏览器继续使用旧版脚本和样式。
- 保留 `nasgames.elf`、`local/nasgames` 和 `/data/nasgames`，兼容已有 Autoload、Payload Manager 槽位和配置。

## 下载

- `nasgames.elf` 上传到 Payload Manager 后运行。默认地址为 `http://<PS5-IP>:8098/`。
- `SHA256SUMS` 可用 `sha256sum -c SHA256SUMS` 核对下载。
- `nasgames-1.0.5-source.tar.gz` 包含此版本的项目源码、构建脚本及链接依赖源码。

兼容性取决于 PS5 固件和系统安装接口。普通文件夹可导入本地库。ZSTD 和 LZ4 目录可复制到 `/data/homebrew` 后用 Kraken Mount 挂载。

发行版使用 libsmb2 6.2（LGPL-2.1-or-later）和 PS5 Payload SDK v0.50。源码包附带第三方源码、许可文本和重新构建说明。[第三方声明](https://github.com/hxhb/nas-games-ps5/blob/v1.0.5/THIRD_PARTY_NOTICES.md)
