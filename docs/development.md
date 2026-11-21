# NAS Games 开发与构建

## 构建环境

项目在 Ubuntu x86-64 上构建。`make bootstrap` 将 PS5 Payload SDK v0.50、Clang/LLD 18 和 `socat` 放在项目的 `.toolchain/` 下。Ubuntu 24.04 需启用提供 `clang-18` 的 Universe 软件源。

```bash
sudo apt-get install build-essential python3 curl jq netcat-openbsd
make bootstrap
make nasgames
```

`make nasgames` 构建 `apps/nasgames/nasgames.elf`，并将 `apps/nasgames/ui/` 中的资源嵌入 ELF。首次构建会获取 libsmb2 6.2。工具链、依赖下载、生成的资源数组和构建输出均在本地生成。

重新构建发布包时，先把 `nasgames-<版本>-source.tar.gz` 中归档的 `.deps/libsmb2-6.2` 放到解压目录的对应位置，再运行 `make bootstrap` 和 `make nasgames`。`make bootstrap` 会验证并获取 SDK、Clang/LLD 和 socat。归档中的 `.deps/libsmb2-6.2` 与上游 SDK 源码是重新构建所需工具的源码和许可材料。

## 本地调试

```bash
make -C apps/nasgames host
apps/nasgames/build/nasgames-host ./apps/nasgames/build/host-data
```

在 `http://127.0.0.1:8098/` 打开页面。主机版适合测试界面、FTP 或本地目录、HTTP Range 和任务队列。它模拟 PKG 安装。SMB、系统安装接口和「媒体」快捷方式需要 PS5 构建验证。

## 实机部署

复制 `.env.example` 到 `config.env`，将 `PS5_HOST` 改成 PS5 局域网地址。也可在命令行传入 `PS5_HOST`。`NASGAMES_PORT` 应与待更新服务正在监听的端口一致。

| 命令 | 用途 |
| --- | --- |
| `make pm-upload-nasgames` | 将 ELF 上传或替换到 Payload Manager 的 `local/nasgames` 槽位 |
| `make pm-autoload-nasgames` | 上传并将 `local/nasgames` 加入 Autoload |
| `make release-nasgames` | 生成版本化 ELF 和校验文件，在 Payload Manager 更新运行中的服务并验证版本 |
| `make deploy-nasgames` | 通过 `elfldr` 加载 ELF |
| `make nasgames-health` | 检查 NAS Games HTTP 服务 |

Payload Manager 默认使用 `8084`，可用 `PM_URL` 指定管理地址。`elfldr` 默认使用 `9021`。更新前等待安装和扫描结束。`make release-nasgames-clean` 适用于实机开发环境的清理，会移除项目的测试样本与对应的临时来源。

GitHub Release 由 `v<版本号>` 标签触发。标签版本与 `NG_VERSION` 一致时，工作流会构建 PS5 ELF、校验和及带依赖源码的归档，然后发布。

## 兼容性和诊断

- `/api/health` 返回服务版本。`/api/state` 返回目录、存储和安装状态，`catalogRevision` 用于界面缓存失效。`/api/log` 提供扫描和传输诊断。
- SMB 使用异步读取流水线。大文件的预读与写入或 HTTP 发送重叠，SMB 2.0 使用较小的读取块。日志包含协商协议、块大小和吞吐量。
- 完整安装基准使用 TLOU 基础版：47,171,305,472 字节，621 秒，系统状态平均约 72.4 MiB/s，采样速度中位数约 74.8 MiB/s。NAS、SMB 版本和磁盘缓存会影响实际速度。
- PKG 元数据优先读取包内的 `param.json` 或 `param.sfo`。特殊包可以在同目录添加一个同名的 `.meta.json`，例如为 `Example.pkg` 添加 `Example.meta.json`。

```json
{
  "titleId": "CUSA00000",
  "titleName": "Example Game",
  "version": "01.00",
  "icon": "Example-icon.png",
  "pic0": "Example-background.png"
}
```

图片路径相对于 PKG 所在目录。复制文件夹时先写入临时目录，校验完成后再提交到安装目标。PKG 完成状态与取消结果由 PS5 系统安装器返回。

## 第三方依赖

- SMB 客户端为 libsmb2 6.2，使用 LGPL-2.1-or-later。发行版的 `nasgames-<版本>-source.tar.gz` 包含项目源码、libsmb2 源码及许可证。修改包内 `.deps/libsmb2-6.2` 后运行 `make bootstrap` 和 `make nasgames` 可重新链接。
- PS5 构建使用 PS5 Payload SDK v0.50。源码包包含这一版本的 SDK 上游源码和构建说明。
- 图片解码、缩放和 JPEG 写入使用 `apps/nasgames/third_party/stb/` 内的头文件，各头文件随附其许可文本。
- `apps/hello` 和 `apps/browser` 保留各自源文件中的版权与 GPL-3.0-or-later 许可声明。
