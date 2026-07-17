cmake_minimum_required(VERSION 3.20)
project(WukongToolkit VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network SerialPort)

# libssh2
set(LIBSSH2_INCLUDE_DIR /opt/homebrew/include)
set(LIBSSH2_LIBRARY /opt/homebrew/lib/libssh2.dylib)

set(SOURCES
    src/main.cpp
    # core
    src/core/AppCore.cpp
    src/core/EventBus.cpp
    src/core/TaskScheduler.cpp
    # app
    src/app/Application.cpp
    # log
    src/log/Logger.cpp
    # config
    src/config/ConfigManager.cpp
    # installer
    src/installer/Installer.cpp
    src/installer/DeployConfig.cpp
    # database
    src/database/DatabaseManager.cpp
    # plugins
    src/plugins/PluginManager.cpp
    src/plugins/PluginLoader.cpp
    src/plugins/PluginWidget.cpp
    # ui
    src/ui/MainWindow.cpp
    src/ui/ThemeManager.cpp
    src/ui/StatusBarManager.cpp
    # terminal
    src/terminal/SSHManager.cpp
    src/terminal/SSHSession.cpp
    src/terminal/SSHThread.cpp
    src/terminal/TerminalWidget.cpp
    src/terminal/CommandHistory.cpp
    src/terminal/KeepAlive.cpp
    src/terminal/SessionManager.cpp
    src/terminal/SSHConnectDialog.cpp
    src/terminal/TerminalBuffer.cpp
    src/terminal/TerminalCursor.cpp
    src/terminal/TerminalColor.cpp
    src/terminal/TerminalRenderer.cpp
    src/terminal/AnsiParser.cpp
    src/terminal/SelectionManager.cpp
    src/terminal/ClipboardManager.cpp
    src/terminal/TerminalTheme.cpp
    src/terminal/TerminalSearch.cpp
    src/terminal/SSHConnection.cpp
    src/terminal/SSHAuth.cpp
    src/terminal/SSHChannel.cpp
    src/terminal/SSHKeyManager.cpp
    src/terminal/TelnetWidget.cpp
    # ping
    src/ping/PingWidget.cpp
    # traceroute
    src/traceroute/TracerouteWidget.cpp
    # network
    src/network/DeviceCenter.cpp
    src/network/IPScannerWidget.cpp
    src/network/PortScannerWidget.cpp
    src/network/DNSWidget.cpp
    src/network/SyslogWidget.cpp
    src/network/FlowWidget.cpp
    src/network/PacketCaptureWidget.cpp
    src/network/ProtocolAnalyzerWidget.cpp
    src/network/ARPWidget.cpp
    src/network/DHCPWidget.cpp
    src/network/WOLWidget.cpp
    # mac
    src/mac/MacToolkitWidget.cpp
    # snmp
    src/snmp/SNMPWidget.cpp
    src/snmp/SNMPMonitorWidget.cpp
    # monitor
    src/monitor/MonitorCenterWidget.cpp
    # ftp
    src/ftp/SFTPWidget.cpp
    src/ftp/TFTPWidget.cpp
    # serial
    src/serial/SerialWidget.cpp
    # http
    src/http/HttpWidget.cpp
    # ipcalc
    src/ipcalc/IpCalculatorWidget.cpp
    # planner
    src/planner/PlannerWidget.cpp
    # mib
    src/mib/MibWidget.cpp
    # alarm
    src/alarm/AlarmCenterWidget.cpp
    # dns
    src/dns/DnsToolkitWidget.cpp
    # syslog
    src/syslog/SyslogCenterWidget.cpp
    # configcenter
    src/configcenter/ConfigCenterWidget.cpp
    # report
    src/report/ReportWidget.cpp
    # topology
    src/topology/TopologyWidget.cpp
    # inspection
    src/inspection/InspectionWidget.cpp
    # discovery
    src/discovery/DiscoveryWidget.cpp
    # aaa
    src/aaa/AAAWidget.cpp
    # security
    src/security/SecurityWidget.cpp
    # traffic
    src/traffic/TrafficWidget.cpp
    # asset
    src/asset/AssetWidget.cpp
    # dashboard
    src/dashboard/DashboardWidget.cpp
    # vpn
    src/vpn/VPNWidget.cpp
    # automation
    src/automation/AutomationWidget.cpp
    # settings
    src/settings/SettingsWidget.cpp
    # ha
    src/ha/HAWidget.cpp
    # performance
    src/performance/PerformanceWidget.cpp
    # qos
    src/qos/QoSWidget.cpp
    # wireless
    src/wireless/WirelessWidget.cpp
    # ipv6
    src/ipv6/IPv6Widget.cpp
    # flow
    src/flow/FlowCollectorWidget.cpp
)

set(HEADERS
    # core
    src/core/AppCore.h
    src/core/EventBus.h
    src/core/TaskScheduler.h
    # app
    src/app/Application.h
    # log
    src/log/Logger.h
    # config
    src/config/ConfigManager.h
    # installer
    src/installer/Installer.h
    src/installer/DeployConfig.h
    # database
    src/database/DatabaseManager.h
    # plugins
    src/plugins/IPlugin.h
    src/plugins/PluginManager.h
    src/plugins/PluginLoader.h
    src/plugins/PluginWidget.h
    # ui
    src/ui/MainWindow.h
    src/ui/ThemeManager.h
    src/ui/StatusBarManager.h
    # terminal
    src/terminal/SSHConfig.h
    src/terminal/SSHManager.h
    src/terminal/SSHSession.h
    src/terminal/SSHThread.h
    src/terminal/TerminalWidget.h
    src/terminal/CommandHistory.h
    src/terminal/KeepAlive.h
    src/terminal/SessionManager.h
    src/terminal/SSHConnectDialog.h
    src/terminal/TerminalCell.h
    src/terminal/TerminalBuffer.h
    src/terminal/TerminalCursor.h
    src/terminal/TerminalColor.h
    src/terminal/TerminalRenderer.h
    src/terminal/AnsiParser.h
    src/terminal/SelectionManager.h
    src/terminal/ClipboardManager.h
    src/terminal/TerminalTheme.h
    src/terminal/TerminalSearch.h
    src/terminal/SSHConnection.h
    src/terminal/SSHAuth.h
    src/terminal/SSHChannel.h
    src/terminal/SSHKeyManager.h
    src/terminal/TelnetWidget.h
    # ping
    src/ping/PingWidget.h
    # traceroute
    src/traceroute/TracerouteWidget.h
    # network
    src/network/DeviceCenter.h
    src/network/IPScannerWidget.h
    src/network/PortScannerWidget.h
    src/network/DNSWidget.h
    src/network/SyslogWidget.h
    src/network/FlowWidget.h
    src/network/PacketCaptureWidget.h
    src/network/ProtocolAnalyzerWidget.h
    src/network/ARPWidget.h
    src/network/DHCPWidget.h
    src/network/WOLWidget.h
    # mac
    src/mac/MacToolkitWidget.h
    # snmp
    src/snmp/SNMPWidget.h
    src/snmp/SNMPMonitorWidget.h
    # monitor
    src/monitor/MonitorCenterWidget.h
    # ftp
    src/ftp/SFTPWidget.h
    src/ftp/TFTPWidget.h
    # serial
    src/serial/SerialWidget.h
    # http
    src/http/HttpWidget.h
    # ipcalc
    src/ipcalc/IpCalculatorWidget.h
    # planner
    src/planner/PlannerWidget.h
    # alarm
    src/alarm/AlarmCenterWidget.h
    # mib
    src/mib/MibWidget.h
    # dns
    src/dns/DnsToolkitWidget.h
    # syslog
    src/syslog/SyslogCenterWidget.h
    # configcenter
    src/configcenter/ConfigCenterWidget.h
    # report
    src/report/ReportWidget.h
    # inspection
    src/inspection/InspectionWidget.h
    # topology
    src/topology/TopologyWidget.h
    # discovery
    src/discovery/DiscoveryWidget.h
    # aaa
    src/aaa/AAAWidget.h
    # security
    src/security/SecurityWidget.h
    # traffic
    src/traffic/TrafficWidget.h
    # vpn
    src/vpn/VPNWidget.h
    # asset
    src/asset/AssetWidget.h
    # dashboard
    src/dashboard/DashboardWidget.h
    # settings
    src/settings/SettingsWidget.h
    # automation
    src/automation/AutomationWidget.h
    # ha
    src/ha/HAWidget.h
    # performance
    src/performance/PerformanceWidget.h
    # qos
    src/qos/QoSWidget.h
    # wireless
    src/wireless/WirelessWidget.h
    # ipv6
    src/ipv6/IPv6Widget.h
    # flow
    src/flow/FlowCollectorWidget.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${LIBSSH2_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::SerialPort
    ${LIBSSH2_LIBRARY}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)