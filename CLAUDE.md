# CLAUDE.md

## 项目概述

悟空网络工程师工具箱 (WukongToolkit) — 一个面向网络工程师的集成式 IDE，将 30+ 网络工具整合到单一 Qt6 桌面应用中。

- **产品定位**: 网络工程师 IDE（类比 Visual Studio 之于程序员）
- **版本**: V1.0.0
- **语言**: C++20
- **GUI 框架**: Qt 6 Widgets
- **构建系统**: CMake 3.20+
- **数据库**: SQLite3
- **SSH 库**: libssh2 (Homebrew 安装)
- **平台**: macOS（当前）、Windows、Linux

## 构建命令

```bash
# 配置
cmake -B build -S WukongToolkit

# 编译
cmake --build build

# 运行
open build/WukongToolkit.app
```

## 架构设计

四层架构：

```
┌─────────────────────────────┐
│         UI Layer            │  主窗口、菜单树、主题、状态栏
├─────────────────────────────┤
│      Service Layer          │  各功能模块 Widget
├─────────────────────────────┤
│       Core Layer            │  AppCore, EventBus, TaskScheduler, DatabaseManager, Logger
├─────────────────────────────┤
│     Driver / Network        │  libssh2, Qt Network, Qt SerialPort, SQLite
└─────────────────────────────┘
```

### 核心单例

| 类 | 文件 | 职责 |
|---|---|---|
| `AppCore` | `src/core/AppCore.h` | 应用生命周期管理，持有 EventBus 和 TaskScheduler |
| `EventBus` | `src/core/EventBus.h` | 模块间事件发布/订阅（基于 QMetaMethod 类型安全分发） |
| `TaskScheduler` | `src/core/TaskScheduler.h` | 定时任务调度 |
| `DatabaseManager` | `src/database/DatabaseManager.h` | SQLite 数据库初始化和访问 |
| `Logger` | `src/log/Logger.h` | 日志系统，支持文件轮转，信号 `logMessage()` |
| `ConfigManager` | `src/config/ConfigManager.h` | 应用配置读写 |
| `ThemeManager` | `src/ui/ThemeManager.h` | 主题系统，暗色/亮色切换，全局样式表和字体 |
| `PluginManager` | `src/plugins/PluginManager.h` | 插件加载和管理 |
| `SSHManager` | `src/terminal/SSHManager.h` | SSH 会话管理 |

### 源码目录结构

```
WukongToolkit/src/
├── main.cpp                    # 入口
├── app/Application.h/.cpp      # QApplication 子类
├── core/                       # AppCore, EventBus, TaskScheduler
├── log/Logger.h/.cpp           # 日志系统
├── config/ConfigManager.h/.cpp # 配置管理
├── database/DatabaseManager.h/.cpp  # SQLite 数据库
├── plugins/                    # IPlugin, PluginManager, PluginLoader, PluginWidget
├── ui/                         # MainWindow, ThemeManager, StatusBarManager
├── terminal/                   # SSH/Telnet 终端模块 (20 个文件)
├── ping/PingWidget.h/.cpp
├── traceroute/TracerouteWidget.h/.cpp
├── network/                    # 11 个网络工具 (DeviceCenter, IPScanner, PortScanner, DNS, Syslog, Flow, PacketCapture, ProtocolAnalyzer, ARP, DHCP, WOL)
├── mac/MacToolkitWidget.h/.cpp
├── snmp/SNMPWidget.h/.cpp + SNMPMonitorWidget.h/.cpp
├── monitor/MonitorCenterWidget.h/.cpp
├── ftp/SFTPWidget.h/.cpp + TFTPWidget.h/.cpp
├── serial/SerialWidget.h/.cpp
├── http/HttpWidget.h/.cpp
├── ipcalc/IpCalculatorWidget.h/.cpp
├── planner/PlannerWidget.h/.cpp
├── mib/MibWidget.h/.cpp
├── alarm/AlarmCenterWidget.h/.cpp
├── dns/DnsToolkitWidget.h/.cpp
├── syslog/SyslogCenterWidget.h/.cpp
├── configcenter/ConfigCenterWidget.h/.cpp
├── report/ReportWidget.h/.cpp
├── topology/TopologyWidget.h/.cpp
├── inspection/InspectionWidget.h/.cpp
├── discovery/DiscoveryWidget.h/.cpp
├── aaa/AAAWidget.h/.cpp
├── security/SecurityWidget.h/.cpp
├── traffic/TrafficWidget.h/.cpp
├── asset/AssetWidget.h/.cpp
├── dashboard/DashboardWidget.h/.cpp
├── vpn/VPNWidget.h/.cpp
├── automation/AutomationWidget.h/.cpp
├── settings/SettingsWidget.h/.cpp
├── ha/HAWidget.h/.cpp
├── performance/PerformanceWidget.h/.cpp
├── qos/QoSWidget.h/.cpp
├── wireless/WirelessWidget.h/.cpp
├── ipv6/IPv6Widget.h/.cpp
├── flow/FlowCollectorWidget.h/.cpp
└── installer/Installer.h/.cpp + DeployConfig.h/.cpp
```

## 主题系统

`ThemeManager` 是全局单例，支持暗色/亮色切换：

- **枚举**: `AppTheme::Dark` / `AppTheme::Light`
- **切换入口**: 菜单栏 (Ctrl+T)、工具栏按钮、设置页下拉框
- **静态颜色方法**: `bgPrimary()`, `bgSecondary()`, `textPrimary()`, `accentColor()`, `borderColor()` 等 — 根据当前主题自动返回对应颜色
- **静态字体**: `uiFont()` (11px PingFang SC), `monoFont()` (10px JetBrains Mono), `titleFont()` (12px Medium)
- **信号**: `themeChanged(AppTheme theme)` — 主题切换时发出
- **样式表**: `styleSheet()` 返回覆盖 35+ Qt 控件的完整动态样式表

## 代码规范

### 命名约定
- 类名: PascalCase (`IPScannerWidget`, `SSHManager`)
- 文件名: 与类名一致 (`IPScannerWidget.h`, `IPScannerWidget.cpp`)
- 成员变量: `m_` 前缀 (`m_menuTree`, `m_workArea`)
- 静态单例: `instance()` 方法
- 禁止拷贝: 单例类删除拷贝构造和赋值运算符

### C++ 标准
- 统一 C++20
- 使用 `std::filesystem`, `std::thread`, `std::chrono`, `std::optional`, `std::variant`, `std::shared_ptr`, `std::unique_ptr`
- 所有头文件使用 `#pragma once`

### 设计原则
1. **稳定优先**: 模块独立，一个模块崩溃不影响整体
2. **启动快**: <2 秒
3. **内存小**: 空载 <150MB，运行 SSH <250MB
4. **离线运行**: 全部本地，不依赖服务器
5. **插件化**: 可扩展

### 日志规范
使用 `Logger::instance()` 单例：
```cpp
Logger::instance().info("ModuleName", "message");
Logger::instance().warning("ModuleName", "message");
Logger::instance().error("ModuleName", "message");
Logger::instance().debug("ModuleName", "message");
```
第一个参数是模块名，第二个是消息内容。

### 事件通信
模块间通过 `AppCore::instance().eventBus()` 通信：
```cpp
// 发布
AppCore::instance().eventBus()->publish("event.type", {{"key", value}});
// 订阅
AppCore::instance().eventBus()->subscribe("event.type", this, &MyWidget::onEvent);
```

## 终端模块

SSH 终端是最复杂的模块，包含以下组件：

| 类 | 职责 |
|---|---|
| `SSHManager` | SSH 会话管理器 |
| `SSHSession` | 单个 SSH 会话 |
| `SSHConnection` | 底层 SSH 连接 |
| `SSHThread` | SSH IO 线程 |
| `SSHAuth` | 认证（密码/密钥） |
| `SSHChannel` | SSH 通道 |
| `SSHKeyManager` | SSH 密钥管理 |
| `SSHConfig` | SSH 配置结构体 |
| `SSHConnectDialog` | 连接对话框 |
| `TerminalWidget` | 终端显示控件 |
| `TerminalBuffer` | 终端缓冲区 |
| `TerminalCursor` | 光标管理 |
| `TerminalColor` | 终端颜色 |
| `TerminalRenderer` | 终端渲染器 |
| `TerminalCell` | 单元格数据结构 |
| `AnsiParser` | VT100/ANSI 转义序列解析 |
| `TerminalSearch` | 终端搜索 |
| `TerminalTheme` | 终端配色方案 |
| `SelectionManager` | 文本选择 |
| `ClipboardManager` | 剪贴板管理 |
| `CommandHistory` | 命令历史 |
| `KeepAlive` | 心跳保活 |
| `SessionManager` | 会话管理 |
| `TelnetWidget` | Telnet 客户端 |

## 依赖项

### Qt6 模块
- `Qt6::Widgets` — GUI
- `Qt6::Sql` — SQLite
- `Qt6::Network` — TCP/UDP/HTTP
- `Qt6::SerialPort` — 串口通信

### 外部库
- `libssh2` — SSH 通信，通过 Homebrew 安装 (`/opt/homebrew/lib/libssh2.dylib`)

## 注意事项

- `FlowRecord` 结构体已在 `FlowWidget` 和 `FlowCollectorWidget` 中存在冲突，后者使用 `FlowRec` 避免冲突
- 8 个占位菜单项尚未实现：Whois, FTP, SCP, 配置Diff, 配置生成, MAC查询, 日志, DHCP重复
- 所有 Widget 作为 QWidget 子类实现，通过 `MainWindow::m_workArea` (QTabWidget) 以标签页形式展示
- MainWindow 的菜单树包含 59 个菜单项，49 个已实现双击处理，8 个占位
- macOS 构建产物为 `.app` bundle
- 编译时使用 `CMAKE_AUTOMOC`/`AUTORCC`/`AUTOUIC` 自动处理 Qt 元对象