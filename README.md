# DiskToggle — 磁盘禁用/启用图形化工具

一个用 C++ Win32 API 编写的轻量级 Windows 图形界面工具，用于在双硬盘双系统场景下，一键禁用或启用指定物理磁盘。

## 功能

- 🖥️ **图形界面**：原生 Win32 窗口，无需 .NET、Qt 等任何运行库
- 🔍 **自动枚举**：列出系统中所有物理磁盘及其当前状态（使用中 / 已禁用）
- 🚫 **一键禁用**：选中磁盘后点击按钮即可将其禁用，重启后依然生效
- ✅ **一键启用**：随时恢复被禁用的磁盘
- 🔐 **自动提权**：双击运行自动申请管理员权限（UAC 弹窗）
- ⚡ **单文件**：编译后仅约 140 KB，无任何依赖

## 适用场景

两块物理硬盘各装一个 Windows 系统（如 Win10 + Win11），希望：

- 进入系统 A 时，系统 B 的硬盘在资源管理器、磁盘管理中完全不可见
- 进入系统 B 时，系统 A 的硬盘完全不可见
- 避免误操作写入对方系统盘
- 两个系统的禁用设置相互独立，互不影响

## 使用方法

1. 双击 `DiskToggle.exe`
2. 在 UAC 弹窗中点击「是」授权管理员权限
3. 在列表中选择要操作的磁盘
4. 点击「禁用选中磁盘」或「启用选中磁盘」
5. 操作前会弹出确认警告，请**务必确认不要禁用当前正在运行系统的那块硬盘**

> ⚠️ **警告**：禁用当前系统所在的硬盘会导致系统立即崩溃、数据丢失。请务必认准磁盘名称再操作。

## 编译方法

### 依赖

- Visual Studio 2022 Build Tools（MSVC v143）
- Windows SDK

### 命令行编译

```cmd
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cl /EHsc /O2 /W3 /utf-8 /DUNICODE /D_UNICODE DiskToggle.cpp ^
   user32.lib gdi32.lib setupapi.lib newdev.lib advapi32.lib shell32.lib cfgmgr32.lib ^
   /Fe:DiskToggle.exe /link /SUBSYSTEM:WINDOWS
```

## 技术原理

- 使用 Windows SetupAPI（`SetupDiGetClassDevs` / `SetupDiEnumDeviceInfo`）枚举磁盘驱动器类设备（GUID `{4d36e967-e325-11ce-bfc1-08002be10318}`）
- 通过 `SetupDiCallClassInstaller(DIF_PROPERTYCHANGE)` 发送 `DICS_DISABLE` / `DICS_ENABLE` 请求来禁用或启用设备
- 设备禁用状态保存在当前系统注册表中，重启后持久生效
- 两个系统的设置互不干扰：在系统 A 禁用磁盘 B，不会影响系统 B 的正常启动

## 局限性

- 本工具实现的是**系统层设备禁用**，资源管理器、磁盘管理、普通软件均无法访问该磁盘
- DiskGenius 等自带底层磁盘驱动的专业工具仍可探测到物理磁盘本体，但无法读写未加密数据
- 若需要让数据完全不可读取（即使磁盘被拆到其他机器），请配合 **BitLocker 整盘加密**使用
- 若需要实现 BIOS 级别的完全不可见，需在主板 BIOS/UEFI 中手动禁用对应 SATA/NVMe 通道

## 文件说明

| 文件 | 说明 |
|------|------|
| `DiskToggle.cpp` | C++ 源代码 |
| `DiskToggle.exe` | 编译好的可执行文件（64位） |
| `README.md` | 中文说明 |
| `README_EN.md` | English documentation |

## 许可证

MIT License

---

> 🤖 本项目由 AI 创作。
