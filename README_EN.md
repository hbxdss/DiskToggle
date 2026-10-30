# DiskToggle — Disk Disable/Enable GUI Tool

A lightweight Windows GUI tool written in C++ Win32 API, designed for dual-disk dual-boot scenarios. It lets you disable or enable a physical disk with one click.

## Features

- 🖥️ **Native GUI**: Pure Win32 window, no .NET, Qt, or any runtime required
- 🔍 **Auto-enumerate**: Lists all physical disks with their current status (Running / Disabled)
- 🚫 **One-click disable**: Disable a selected disk; the state persists across reboots
- ✅ **One-click enable**: Re-enable a disabled disk at any time
- 🔐 **Auto UAC elevation**: Double-click to run; it automatically requests administrator privileges
- ⚡ **Single file**: Only ~140 KB compiled, zero dependencies

## Use Case

You have two physical disks, each running its own Windows installation (e.g., Win10 + Win11). You want:

- When booting into System A, the disk containing System B is completely invisible in File Explorer and Disk Management
- When booting into System B, the disk containing System A is completely invisible
- Prevent accidental writes to the other system's disk
- Each system's disable settings are independent and do not affect the other

## How to Use

1. Double-click `DiskToggle.exe`
2. Click "Yes" on the UAC prompt to grant administrator rights
3. Select a disk from the list
4. Click "Disable Selected Disk" or "Enable Selected Disk"
5. A confirmation dialog will appear — **make sure you are NOT disabling the disk that the current system is running from**

> ⚠️ **Warning**: Disabling the disk containing the running system will cause an immediate system crash and potential data loss. Always verify the disk name before proceeding.

## Building

### Prerequisites

- Visual Studio 2022 Build Tools (MSVC v143)
- Windows SDK

### Command-line build

```cmd
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cl /EHsc /O2 /W3 /utf-8 /DUNICODE /D_UNICODE DiskToggle.cpp ^
   user32.lib gdi32.lib setupapi.lib newdev.lib advapi32.lib shell32.lib cfgmgr32.lib ^
   /Fe:DiskToggle.exe /link /SUBSYSTEM:WINDOWS
```

## How It Works

- Uses Windows SetupAPI (`SetupDiGetClassDevs` / `SetupDiEnumDeviceInfo`) to enumerate disk drive class devices (GUID `{4d36e967-e325-11ce-bfc1-08002be10318}`)
- Sends `DICS_DISABLE` / `DICS_ENABLE` via `SetupDiCallClassInstaller(DIF_PROPERTYCHANGE)`
- The disabled state is stored in the current system's registry and persists across reboots
- Settings are independent per system: disabling disk B in System A does not affect System B's normal boot

## Limitations

- This tool performs a **system-level device disable**. File Explorer, Disk Management, and regular software cannot access the disk.
- Professional tools like DiskGenius (which ship their own low-level disk drivers) can still detect the physical disk, but cannot read unencrypted data.
- To make data completely unreadable (even if the disk is moved to another machine), combine this with **BitLocker full-disk encryption**.
- For BIOS-level invisibility (no software can detect the disk at all), you must manually disable the SATA/NVMe port in your motherboard BIOS/UEFI.

## Files

| File | Description |
|------|-------------|
| `DiskToggle.cpp` | C++ source code |
| `DiskToggle.exe` | Compiled 64-bit executable |
| `README.md` | Chinese documentation |
| `README_EN.md` | This file |

## License

MIT License

---

> 🤖 This project was created by AI.
