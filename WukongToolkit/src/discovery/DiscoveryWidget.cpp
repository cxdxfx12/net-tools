#include "discovery/DiscoveryWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QProgressBar>
#include <QSpinBox>
#include <QCheckBox>
#include <QRegularExpression>
#include <QHostAddress>
#include <QHostInfo>
#include <QFile>
#include <QDateTime>

// ============================================================================
// 内置 OUI 厂商表 — 常见网络设备厂商 MAC 前缀
// ============================================================================
static const QHash<QString, QString>& ouiTable()
{
    static const QHash<QString, QString> t = {
        // Cisco
        {"00:00:0C",QStringLiteral("Cisco")},{"00:01:42",QStringLiteral("Cisco")},{"00:0D:BC",QStringLiteral("Cisco")},
        {"00:1A:A1",QStringLiteral("Cisco")},{"00:1B:0C",QStringLiteral("Cisco")},{"00:1E:49",QStringLiteral("Cisco")},
        {"00:23:04",QStringLiteral("Cisco")},{"00:25:84",QStringLiteral("Cisco")},{"00:26:0B",QStringLiteral("Cisco")},
        {"04:DA:D2",QStringLiteral("Cisco")},{"10:05:01",QStringLiteral("Cisco")},{"14:1B:BD",QStringLiteral("Cisco")},
        {"18:8B:9D",QStringLiteral("Cisco")},{"24:E9:B3",QStringLiteral("Cisco")},{"2C:5A:05",QStringLiteral("Cisco")},
        {"30:7B:AC",QStringLiteral("Cisco")},{"34:DB:9C",QStringLiteral("Cisco")},{"38:ED:18",QStringLiteral("Cisco")},
        {"40:55:39",QStringLiteral("Cisco")},{"44:2A:FF",QStringLiteral("Cisco")},{"48:0E:EC",QStringLiteral("Cisco")},
        {"4C:00:82",QStringLiteral("Cisco")},{"50:3D:E5",QStringLiteral("Cisco")},{"54:75:D0",QStringLiteral("Cisco")},
        {"58:0A:20",QStringLiteral("Cisco")},{"5C:5A:EA",QStringLiteral("Cisco")},{"64:9E:F3",QStringLiteral("Cisco")},
        {"68:EF:BD",QStringLiteral("Cisco")},{"6C:41:6A",QStringLiteral("Cisco")},{"70:DB:98",QStringLiteral("Cisco")},
        {"74:26:AC",QStringLiteral("Cisco")},{"78:72:5D",QStringLiteral("Cisco")},{"7C:AD:74",QStringLiteral("Cisco")},
        {"80:E0:1D",QStringLiteral("Cisco")},{"84:3D:C6",QStringLiteral("Cisco")},{"88:5A:92",QStringLiteral("Cisco")},
        {"8C:60:4F",QStringLiteral("Cisco")},{"90:2E:1C",QStringLiteral("Cisco")},{"A4:4C:11",QStringLiteral("Cisco")},
        {"A8:9D:21",QStringLiteral("Cisco")},{"B0:AA:77",QStringLiteral("Cisco")},{"B4:14:89",QStringLiteral("Cisco")},
        {"B8:62:1F",QStringLiteral("Cisco")},{"C0:62:6B",QStringLiteral("Cisco")},{"C4:14:3C",QStringLiteral("Cisco")},
        {"C8:4C:75",QStringLiteral("Cisco")},{"CC:16:7E",QStringLiteral("Cisco")},{"D0:7E:28",QStringLiteral("Cisco")},
        {"D4:6D:50",QStringLiteral("Cisco")},{"D8:B1:90",QStringLiteral("Cisco")},{"E0:2F:6D",QStringLiteral("Cisco")},
        {"E4:C7:22",QStringLiteral("Cisco")},{"E8:65:49",QStringLiteral("Cisco")},{"EC:1D:7F",QStringLiteral("Cisco")},
        {"F0:29:29",QStringLiteral("Cisco")},{"F4:4E:05",QStringLiteral("Cisco")},{"F8:72:EA",QStringLiteral("Cisco")},
        {"FC:5B:39",QStringLiteral("Cisco")},
        // Huawei
        {"00:0D:6E",QStringLiteral("Huawei")},{"00:18:82",QStringLiteral("Huawei")},{"00:1E:10",QStringLiteral("Huawei")},
        {"00:25:9E",QStringLiteral("Huawei")},{"00:E0:FC",QStringLiteral("Huawei")},{"08:19:A6",QStringLiteral("Huawei")},
        {"0C:37:DC",QStringLiteral("Huawei")},{"10:4B:46",QStringLiteral("Huawei")},{"14:6E:0A",QStringLiteral("Huawei")},
        {"18:2E:5E",QStringLiteral("Huawei")},{"20:0B:C7",QStringLiteral("Huawei")},{"24:1E:0B",QStringLiteral("Huawei")},
        {"28:3A:4D",QStringLiteral("Huawei")},{"2C:0D:A7",QStringLiteral("Huawei")},{"30:0B:9B",QStringLiteral("Huawei")},
        {"34:0A:7B",QStringLiteral("Huawei")},{"38:6E:6C",QStringLiteral("Huawei")},{"40:2C:76",QStringLiteral("Huawei")},
        {"44:14:DB",QStringLiteral("Huawei")},{"48:46:C1",QStringLiteral("Huawei")},{"4C:1F:CC",QStringLiteral("Huawei")},
        {"50:7B:9D",QStringLiteral("Huawei")},{"54:2A:9C",QStringLiteral("Huawei")},{"58:2A:F7",QStringLiteral("Huawei")},
        {"5C:4C:A9",QStringLiteral("Huawei")},{"60:DE:44",QStringLiteral("Huawei")},{"68:9C:5E",QStringLiteral("Huawei")},
        {"6C:0B:34",QStringLiteral("Huawei")},{"70:48:7F",QStringLiteral("Huawei")},{"74:1E:93",QStringLiteral("Huawei")},
        {"78:0D:47",QStringLiteral("Huawei")},{"78:D7:52",QStringLiteral("Huawei")},{"7C:60:97",QStringLiteral("Huawei")},
        {"80:6D:97",QStringLiteral("Huawei")},{"84:AC:16",QStringLiteral("Huawei")},{"88:36:6C",QStringLiteral("Huawei")},
        {"8C:BE:BE",QStringLiteral("Huawei")},{"90:1F:0A",QStringLiteral("Huawei")},{"94:7D:77",QStringLiteral("Huawei")},
        {"98:2D:7B",QStringLiteral("Huawei")},{"9C:28:EF",QStringLiteral("Huawei")},{"A0:99:9B",QStringLiteral("Huawei")},
        {"A4:0E:2B",QStringLiteral("Huawei")},{"A8:6B:AD",QStringLiteral("Huawei")},{"AC:61:75",QStringLiteral("Huawei")},
        {"B0:68:E6",QStringLiteral("Huawei")},{"B4:0C:25",QStringLiteral("Huawei")},{"B8:37:4A",QStringLiteral("Huawei")},
        {"BC:76:70",QStringLiteral("Huawei")},{"C0:0D:7E",QStringLiteral("Huawei")},{"C4:01:CE",QStringLiteral("Huawei")},
        {"C8:50:E9",QStringLiteral("Huawei")},{"CC:96:A0",QStringLiteral("Huawei")},{"D0:D0:03",QStringLiteral("Huawei")},
        {"D4:4C:9C",QStringLiteral("Huawei")},{"D8:49:0B",QStringLiteral("Huawei")},{"DC:2C:6E",QStringLiteral("Huawei")},
        {"E0:98:61",QStringLiteral("Huawei")},{"E4:48:C7",QStringLiteral("Huawei")},{"E8:0B:13",QStringLiteral("Huawei")},
        {"EC:38:73",QStringLiteral("Huawei")},{"F0:64:91",QStringLiteral("Huawei")},{"F4:63:49",QStringLiteral("Huawei")},
        {"F8:0D:60",QStringLiteral("Huawei")},{"FC:48:EF",QStringLiteral("Huawei")},
        // H3C
        {"00:0F:E2",QStringLiteral("H3C")},{"00:1B:2F",QStringLiteral("H3C")},{"00:23:89",QStringLiteral("H3C")},
        {"04:27:FE",QStringLiteral("H3C")},{"08:0D:67",QStringLiteral("H3C")},{"0C:DA:41",QStringLiteral("H3C")},
        {"14:30:C9",QStringLiteral("H3C")},{"18:FD:74",QStringLiteral("H3C")},{"20:1A:06",QStringLiteral("H3C")},
        {"28:6E:D4",QStringLiteral("H3C")},{"30:63:6B",QStringLiteral("H3C")},{"34:29:12",QStringLiteral("H3C")},
        {"38:22:D6",QStringLiteral("H3C")},{"3C:8C:40",QStringLiteral("H3C")},{"40:7A:CB",QStringLiteral("H3C")},
        {"48:7A:DA",QStringLiteral("H3C")},{"4C:AC:0A",QStringLiteral("H3C")},{"50:61:84",QStringLiteral("H3C")},
        {"54:CD:EE",QStringLiteral("H3C")},{"58:6A:B1",QStringLiteral("H3C")},{"5C:DD:70",QStringLiteral("H3C")},
        {"60:31:3C",QStringLiteral("H3C")},{"64:9D:99",QStringLiteral("H3C")},{"68:2C:7B",QStringLiteral("H3C")},
        {"6C:92:BF",QStringLiteral("H3C")},{"70:7B:E8",QStringLiteral("H3C")},{"70:F9:6D",QStringLiteral("H3C")},
        {"74:25:8A",QStringLiteral("H3C")},{"78:1D:BA",QStringLiteral("H3C")},{"7C:1D:D9",QStringLiteral("H3C")},
        {"80:48:59",QStringLiteral("H3C")},{"84:D9:31",QStringLiteral("H3C")},{"88:14:4B",QStringLiteral("H3C")},
        {"8C:4C:DC",QStringLiteral("H3C")},{"90:5C:44",QStringLiteral("H3C")},{"94:28:2E",QStringLiteral("H3C")},
        {"98:0E:E4",QStringLiteral("H3C")},{"9C:21:41",QStringLiteral("H3C")},{"A0:48:1C",QStringLiteral("H3C")},
        {"A4:5B:FE",QStringLiteral("H3C")},{"A8:13:74",QStringLiteral("H3C")},{"AC:4A:FE",QStringLiteral("H3C")},
        {"B0:47:BF",QStringLiteral("H3C")},{"B8:AF:67",QStringLiteral("H3C")},{"BC:3A:2A",QStringLiteral("H3C")},
        {"C0:16:BD",QStringLiteral("H3C")},{"C4:74:40",QStringLiteral("H3C")},{"CC:06:77",QStringLiteral("H3C")},
        {"D0:67:26",QStringLiteral("H3C")},{"D4:81:CA",QStringLiteral("H3C")},{"D8:15:0D",QStringLiteral("H3C")},
        {"DC:53:7C",QStringLiteral("H3C")},{"E0:9D:31",QStringLiteral("H3C")},{"E4:61:1E",QStringLiteral("H3C")},
        {"E8:6A:64",QStringLiteral("H3C")},{"EC:BD:1D",QStringLiteral("H3C")},{"F0:84:2F",QStringLiteral("H3C")},
        {"F4:21:CA",QStringLiteral("H3C")},{"F8:32:E4",QStringLiteral("H3C")},{"FC:3F:DB",QStringLiteral("H3C")},
        // Juniper
        {"00:05:85",QStringLiteral("Juniper")},{"00:12:1E",QStringLiteral("Juniper")},{"00:14:F6",QStringLiteral("Juniper")},
        {"00:17:CB",QStringLiteral("Juniper")},{"00:19:E2",QStringLiteral("Juniper")},{"00:1C:73",QStringLiteral("Juniper")},
        {"00:1F:12",QStringLiteral("Juniper")},{"00:22:83",QStringLiteral("Juniper")},{"00:23:9C",QStringLiteral("Juniper")},
        {"00:26:88",QStringLiteral("Juniper")},{"04:0E:56",QStringLiteral("Juniper")},{"08:81:F4",QStringLiteral("Juniper")},
        {"0C:86:10",QStringLiteral("Juniper")},{"10:0E:7E",QStringLiteral("Juniper")},{"20:4E:71",QStringLiteral("Juniper")},
        {"28:8A:1C",QStringLiteral("Juniper")},{"28:C0:DA",QStringLiteral("Juniper")},{"2C:21:72",QStringLiteral("Juniper")},
        {"2C:6B:F5",QStringLiteral("Juniper")},{"30:7C:5E",QStringLiteral("Juniper")},{"3C:8A:B0",QStringLiteral("Juniper")},
        {"40:71:83",QStringLiteral("Juniper")},{"44:95:8A",QStringLiteral("Juniper")},{"4C:96:14",QStringLiteral("Juniper")},
        {"54:4B:8C",QStringLiteral("Juniper")},{"5C:45:27",QStringLiteral("Juniper")},{"5C:5E:AB",QStringLiteral("Juniper")},
        {"64:64:9B",QStringLiteral("Juniper")},{"64:87:D7",QStringLiteral("Juniper")},{"84:18:88",QStringLiteral("Juniper")},
        {"88:E0:F3",QStringLiteral("Juniper")},{"8C:49:62",QStringLiteral("Juniper")},{"A0:36:FA",QStringLiteral("Juniper")},
        {"A8:D0:E5",QStringLiteral("Juniper")},{"B0:A8:6E",QStringLiteral("Juniper")},{"B0:C6:9A",QStringLiteral("Juniper")},
        {"C8:5B:76",QStringLiteral("Juniper")},{"CC:E1:7F",QStringLiteral("Juniper")},{"D0:07:CA",QStringLiteral("Juniper")},
        {"E0:89:7E",QStringLiteral("Juniper")},{"E4:55:78",QStringLiteral("Juniper")},{"F0:1C:2D",QStringLiteral("Juniper")},
        {"F4:CC:55",QStringLiteral("Juniper")},{"F8:BC:41",QStringLiteral("Juniper")},
        // Arista
        {"00:1C:73",QStringLiteral("Arista")},{"00:1D:4F",QStringLiteral("Arista")},{"00:23:AC",QStringLiteral("Arista")},
        {"08:00:27",QStringLiteral("Arista")},{"28:99:3A",QStringLiteral("Arista")},{"2C:DD:0C",QStringLiteral("Arista")},
        {"44:4C:A8",QStringLiteral("Arista")},{"50:8C:B1",QStringLiteral("Arista")},{"74:83:C2",QStringLiteral("Arista")},
        {"98:5D:AD",QStringLiteral("Arista")},{"A4:BA:76",QStringLiteral("Arista")},{"C4:7E:8E",QStringLiteral("Arista")},
        {"D4:6E:0E",QStringLiteral("Arista")},{"E8:65:BF",QStringLiteral("Arista")},{"FC:BD:67",QStringLiteral("Arista")},
        // VMware
        {"00:0C:29",QStringLiteral("VMware")},{"00:50:56",QStringLiteral("VMware")},{"00:05:69",QStringLiteral("VMware")},
        {"00:1C:14",QStringLiteral("VMware")},
        // Dell
        {"00:06:5B",QStringLiteral("Dell")},{"00:08:74",QStringLiteral("Dell")},{"00:0B:DB",QStringLiteral("Dell")},
        {"00:0D:56",QStringLiteral("Dell")},{"00:0F:1F",QStringLiteral("Dell")},{"00:11:43",QStringLiteral("Dell")},
        {"00:12:3F",QStringLiteral("Dell")},{"00:13:72",QStringLiteral("Dell")},{"00:14:22",QStringLiteral("Dell")},
        {"00:15:C5",QStringLiteral("Dell")},{"00:18:8B",QStringLiteral("Dell")},{"00:19:B9",QStringLiteral("Dell")},
        {"00:1A:A0",QStringLiteral("Dell")},{"00:1C:23",QStringLiteral("Dell")},{"00:1D:09",QStringLiteral("Dell")},
        {"00:1E:4F",QStringLiteral("Dell")},{"00:1E:C9",QStringLiteral("Dell")},{"00:21:70",QStringLiteral("Dell")},
        {"00:21:9B",QStringLiteral("Dell")},{"00:22:19",QStringLiteral("Dell")},{"00:23:AE",QStringLiteral("Dell")},
        {"00:24:E8",QStringLiteral("Dell")},{"00:25:64",QStringLiteral("Dell")},{"00:26:B9",QStringLiteral("Dell")},
        {"18:03:73",QStringLiteral("Dell")},{"18:26:6C",QStringLiteral("Dell")},{"18:DB:F2",QStringLiteral("Dell")},
        {"1C:1B:68",QStringLiteral("Dell")},{"24:6E:96",QStringLiteral("Dell")},{"28:F1:0E",QStringLiteral("Dell")},
        {"34:17:EB",QStringLiteral("Dell")},{"34:48:ED",QStringLiteral("Dell")},{"34:E6:D7",QStringLiteral("Dell")},
        {"3C:2C:30",QStringLiteral("Dell")},{"48:2A:E3",QStringLiteral("Dell")},{"4C:76:25",QStringLiteral("Dell")},
        {"50:9A:4C",QStringLiteral("Dell")},{"54:BF:64",QStringLiteral("Dell")},{"78:2B:CB",QStringLiteral("Dell")},
        {"78:45:58",QStringLiteral("Dell")},{"84:7B:EB",QStringLiteral("Dell")},{"84:8F:69",QStringLiteral("Dell")},
        {"84:A9:3E",QStringLiteral("Dell")},{"90:B1:1C",QStringLiteral("Dell")},{"98:90:96",QStringLiteral("Dell")},
        {"9C:DA:3E",QStringLiteral("Dell")},{"A4:4C:C8",QStringLiteral("Dell")},{"A4:5D:36",QStringLiteral("Dell")},
        {"A4:BA:DB",QStringLiteral("Dell")},{"B0:83:FE",QStringLiteral("Dell")},{"B8:2A:72",QStringLiteral("Dell")},
        {"B8:AC:6F",QStringLiteral("Dell")},{"C8:1F:66",QStringLiteral("Dell")},{"D0:67:E5",QStringLiteral("Dell")},
        {"D0:94:66",QStringLiteral("Dell")},{"D4:3D:7E",QStringLiteral("Dell")},{"D4:81:D7",QStringLiteral("Dell")},
        {"D4:BE:D9",QStringLiteral("Dell")},{"D8:9E:F3",QStringLiteral("Dell")},{"F0:4D:A2",QStringLiteral("Dell")},
        {"F4:8E:38",QStringLiteral("Dell")},{"F8:BC:12",QStringLiteral("Dell")},{"F8:DB:88",QStringLiteral("Dell")},
        // HP
        {"00:02:A5",QStringLiteral("HP")},{"00:0B:CD",QStringLiteral("HP")},{"00:0E:7F",QStringLiteral("HP")},
        {"00:10:83",QStringLiteral("HP")},{"00:11:0A",QStringLiteral("HP")},{"00:11:85",QStringLiteral("HP")},
        {"00:13:21",QStringLiteral("HP")},{"00:14:38",QStringLiteral("HP")},{"00:15:60",QStringLiteral("HP")},
        {"00:16:35",QStringLiteral("HP")},{"00:17:A4",QStringLiteral("HP")},{"00:18:FE",QStringLiteral("HP")},
        {"00:19:BB",QStringLiteral("HP")},{"00:1A:4B",QStringLiteral("HP")},{"00:1B:78",QStringLiteral("HP")},
        {"00:1C:C4",QStringLiteral("HP")},{"00:1D:0D",QStringLiteral("HP")},{"00:1E:0B",QStringLiteral("HP")},
        {"00:1F:29",QStringLiteral("HP")},{"00:21:5A",QStringLiteral("HP")},{"00:21:FE",QStringLiteral("HP")},
        {"00:22:64",QStringLiteral("HP")},{"00:23:7D",QStringLiteral("HP")},{"00:24:81",QStringLiteral("HP")},
        {"00:25:B3",QStringLiteral("HP")},{"00:26:55",QStringLiteral("HP")},{"00:30:C1",QStringLiteral("HP")},
        {"04:0E:3C",QStringLiteral("HP")},{"10:60:4B",QStringLiteral("HP")},{"14:58:D0",QStringLiteral("HP")},
        {"18:60:24",QStringLiteral("HP")},{"1C:98:EC",QStringLiteral("HP")},{"20:47:47",QStringLiteral("HP")},
        {"28:92:4A",QStringLiteral("HP")},{"2C:44:FD",QStringLiteral("HP")},{"2C:59:E5",QStringLiteral("HP")},
        {"2C:76:8A",QStringLiteral("HP")},{"30:0D:43",QStringLiteral("HP")},{"30:C3:D9",QStringLiteral("HP")},
        {"34:64:A9",QStringLiteral("HP")},{"34:FC:EF",QStringLiteral("HP")},{"38:63:BB",QStringLiteral("HP")},
        {"38:7A:0E",QStringLiteral("HP")},{"38:CA:97",QStringLiteral("HP")},{"38:D5:47",QStringLiteral("HP")},
        {"38:E7:0D",QStringLiteral("HP")},{"3C:52:82",QStringLiteral("HP")},{"3C:A8:2A",QStringLiteral("HP")},
        {"40:01:C6",QStringLiteral("HP")},{"40:2E:28",QStringLiteral("HP")},{"40:5D:82",QStringLiteral("HP")},
        {"40:80:5A",QStringLiteral("HP")},{"44:1E:A1",QStringLiteral("HP")},{"44:31:92",QStringLiteral("HP")},
        {"44:48:C9",QStringLiteral("HP")},{"48:5B:39",QStringLiteral("HP")},{"48:5D:60",QStringLiteral("HP")},
        {"48:BA:4E",QStringLiteral("HP")},{"48:DF:37",QStringLiteral("HP")},{"4C:0B:BE",QStringLiteral("HP")},
        {"50:65:F3",QStringLiteral("HP")},{"54:05:AB",QStringLiteral("HP")},{"54:80:28",QStringLiteral("HP")},
        {"58:20:B1",QStringLiteral("HP")},{"5C:CB:99",QStringLiteral("HP")},{"60:14:66",QStringLiteral("HP")},
        {"60:6D:C7",QStringLiteral("HP")},{"64:16:66",QStringLiteral("HP")},{"64:31:50",QStringLiteral("HP")},
        {"64:80:99",QStringLiteral("HP")},{"68:B5:99",QStringLiteral("HP")},
        // Fortinet
        {"00:09:0F",QStringLiteral("Fortinet")},{"00:1C:58",QStringLiteral("Fortinet")},{"04:09:73",QStringLiteral("Fortinet")},
        {"08:5B:0E",QStringLiteral("Fortinet")},{"0C:45:BA",QStringLiteral("Fortinet")},{"10:A7:93",QStringLiteral("Fortinet")},
        {"14:1B:BD",QStringLiteral("Fortinet")},{"20:3C:AE",QStringLiteral("Fortinet")},{"28:5A:EB",QStringLiteral("Fortinet")},
        {"30:3A:64",QStringLiteral("Fortinet")},{"34:A8:EB",QStringLiteral("Fortinet")},{"38:ED:18",QStringLiteral("Fortinet")},
        {"40:55:39",QStringLiteral("Fortinet")},{"44:4C:A8",QStringLiteral("Fortinet")},{"48:DF:37",QStringLiteral("Fortinet")},
        {"4C:32:75",QStringLiteral("Fortinet")},{"50:0E:6D",QStringLiteral("Fortinet")},{"54:27:8E",QStringLiteral("Fortinet")},
        {"58:20:B1",QStringLiteral("Fortinet")},{"5C:5A:EA",QStringLiteral("Fortinet")},{"60:DE:44",QStringLiteral("Fortinet")},
        {"64:9E:F3",QStringLiteral("Fortinet")},{"68:9C:5E",QStringLiteral("Fortinet")},{"6C:41:6A",QStringLiteral("Fortinet")},
        {"70:48:7F",QStringLiteral("Fortinet")},{"74:26:AC",QStringLiteral("Fortinet")},{"78:11:DC",QStringLiteral("Fortinet")},
        {"80:6D:97",QStringLiteral("Fortinet")},{"84:AC:16",QStringLiteral("Fortinet")},{"88:36:6C",QStringLiteral("Fortinet")},
        {"8C:BE:BE",QStringLiteral("Fortinet")},{"90:1F:0A",QStringLiteral("Fortinet")},{"94:7D:77",QStringLiteral("Fortinet")},
        {"98:2D:7B",QStringLiteral("Fortinet")},{"9C:28:EF",QStringLiteral("Fortinet")},
        // Palo Alto Networks
        {"00:1B:17",QStringLiteral("Palo Alto")},{"00:1C:DF",QStringLiteral("Palo Alto")},{"00:1D:09",QStringLiteral("Palo Alto")},
        {"00:25:64",QStringLiteral("Palo Alto")},{"00:26:B9",QStringLiteral("Palo Alto")},{"04:15:52",QStringLiteral("Palo Alto")},
        {"08:27:9E",QStringLiteral("Palo Alto")},{"0C:15:39",QStringLiteral("Palo Alto")},{"10:1F:74",QStringLiteral("Palo Alto")},
        {"10:5A:F7",QStringLiteral("Palo Alto")},{"14:9E:EC",QStringLiteral("Palo Alto")},{"18:9C:27",QStringLiteral("Palo Alto")},
        {"1C:87:6C",QStringLiteral("Palo Alto")},{"20:3A:07",QStringLiteral("Palo Alto")},{"24:77:03",QStringLiteral("Palo Alto")},
        {"28:3A:4D",QStringLiteral("Palo Alto")},{"2C:33:7A",QStringLiteral("Palo Alto")},{"30:3A:64",QStringLiteral("Palo Alto")},
        {"34:12:98",QStringLiteral("Palo Alto")},{"38:00:25",QStringLiteral("Palo Alto")},{"3C:15:C2",QStringLiteral("Palo Alto")},
        {"40:16:9F",QStringLiteral("Palo Alto")},{"44:2A:60",QStringLiteral("Palo Alto")},{"48:5D:60",QStringLiteral("Palo Alto")},
        {"4C:57:CA",QStringLiteral("Palo Alto")},{"50:3D:E5",QStringLiteral("Palo Alto")},{"54:2A:9C",QStringLiteral("Palo Alto")},
        {"58:2A:F7",QStringLiteral("Palo Alto")},{"5C:4C:A9",QStringLiteral("Palo Alto")},{"60:14:66",QStringLiteral("Palo Alto")},
        {"64:00:6A",QStringLiteral("Palo Alto")},{"68:9C:5E",QStringLiteral("Palo Alto")},{"6C:0B:34",QStringLiteral("Palo Alto")},
        {"70:48:7F",QStringLiteral("Palo Alto")},{"74:26:AC",QStringLiteral("Palo Alto")},
        // Apple
        {"00:03:93",QStringLiteral("Apple")},{"00:14:51",QStringLiteral("Apple")},{"00:17:F2",QStringLiteral("Apple")},
        {"00:1E:C2",QStringLiteral("Apple")},{"00:1F:F3",QStringLiteral("Apple")},{"00:21:6A",QStringLiteral("Apple")},
        {"00:22:41",QStringLiteral("Apple")},{"00:23:12",QStringLiteral("Apple")},{"00:23:DF",QStringLiteral("Apple")},
        {"00:24:36",QStringLiteral("Apple")},{"00:25:00",QStringLiteral("Apple")},{"00:25:BC",QStringLiteral("Apple")},
        {"00:26:08",QStringLiteral("Apple")},{"00:26:BB",QStringLiteral("Apple")},{"00:30:65",QStringLiteral("Apple")},
        {"04:0C:CE",QStringLiteral("Apple")},{"04:15:52",QStringLiteral("Apple")},{"04:1E:64",QStringLiteral("Apple")},
        {"04:26:65",QStringLiteral("Apple")},{"04:4B:ED",QStringLiteral("Apple")},{"04:54:53",QStringLiteral("Apple")},
        {"04:69:F8",QStringLiteral("Apple")},{"04:DB:56",QStringLiteral("Apple")},{"04:E5:36",QStringLiteral("Apple")},
        {"04:F1:3E",QStringLiteral("Apple")},{"08:66:98",QStringLiteral("Apple")},{"08:6D:41",QStringLiteral("Apple")},
        {"08:74:02",QStringLiteral("Apple")},{"0C:15:39",QStringLiteral("Apple")},{"0C:19:F8",QStringLiteral("Apple")},
        {"0C:3E:9F",QStringLiteral("Apple")},{"0C:4D:E9",QStringLiteral("Apple")},{"0C:51:01",QStringLiteral("Apple")},
        {"0C:77:1A",QStringLiteral("Apple")},{"0C:BC:9F",QStringLiteral("Apple")},{"10:1C:0C",QStringLiteral("Apple")},
        {"10:29:59",QStringLiteral("Apple")},{"10:40:F3",QStringLiteral("Apple")},{"10:41:7F",QStringLiteral("Apple")},
        {"10:93:E9",QStringLiteral("Apple")},{"10:9A:DD",QStringLiteral("Apple")},{"10:CE:E9",QStringLiteral("Apple")},
        {"14:10:9F",QStringLiteral("Apple")},{"14:20:5E",QStringLiteral("Apple")},{"14:5A:05",QStringLiteral("Apple")},
        {"14:5A:FC",QStringLiteral("Apple")},{"14:7D:DA",QStringLiteral("Apple")},{"14:88:E6",QStringLiteral("Apple")},
        {"14:8F:C6",QStringLiteral("Apple")},{"14:98:77",QStringLiteral("Apple")},{"14:99:E2",QStringLiteral("Apple")},
        {"14:BD:61",QStringLiteral("Apple")},{"18:20:32",QStringLiteral("Apple")},{"18:34:51",QStringLiteral("Apple")},
        {"18:3E:EF",QStringLiteral("Apple")},{"18:65:90",QStringLiteral("Apple")},{"18:81:0E",QStringLiteral("Apple")},
        {"18:9E:FC",QStringLiteral("Apple")},{"18:AF:61",QStringLiteral("Apple")},{"18:AF:8F",QStringLiteral("Apple")},
        {"18:E7:F4",QStringLiteral("Apple")},{"18:EE:69",QStringLiteral("Apple")},{"18:F1:D8",QStringLiteral("Apple")},
        {"18:F6:43",QStringLiteral("Apple")},{"1C:1A:C0",QStringLiteral("Apple")},{"1C:36:BB",QStringLiteral("Apple")},
        {"1C:91:48",QStringLiteral("Apple")},{"1C:AB:A7",QStringLiteral("Apple")},
        // Intel
        {"00:02:B3",QStringLiteral("Intel")},{"00:03:47",QStringLiteral("Intel")},{"00:04:23",QStringLiteral("Intel")},
        {"00:07:E9",QStringLiteral("Intel")},{"00:0A:0D",QStringLiteral("Intel")},{"00:0B:AB",QStringLiteral("Intel")},
        {"00:0C:F1",QStringLiteral("Intel")},{"00:0D:60",QStringLiteral("Intel")},{"00:0E:0C",QStringLiteral("Intel")},
        {"00:11:11",QStringLiteral("Intel")},{"00:12:F0",QStringLiteral("Intel")},{"00:13:02",QStringLiteral("Intel")},
        {"00:15:00",QStringLiteral("Intel")},{"00:16:41",QStringLiteral("Intel")},{"00:16:EA",QStringLiteral("Intel")},
        {"00:18:DE",QStringLiteral("Intel")},{"00:19:D1",QStringLiteral("Intel")},{"00:1A:6B",QStringLiteral("Intel")},
        {"00:1B:21",QStringLiteral("Intel")},{"00:1B:77",QStringLiteral("Intel")},{"00:1C:BF",QStringLiteral("Intel")},
        {"00:1E:64",QStringLiteral("Intel")},{"00:1F:3B",QStringLiteral("Intel")},{"00:21:5C",QStringLiteral("Intel")},
        {"00:23:13",QStringLiteral("Intel")},{"00:24:D7",QStringLiteral("Intel")},{"00:26:C6",QStringLiteral("Intel")},
        {"04:7C:16",QStringLiteral("Intel")},{"0C:54:A5",QStringLiteral("Intel")},{"0C:8B:FD",QStringLiteral("Intel")},
        {"0C:D7:46",QStringLiteral("Intel")},{"10:02:B5",QStringLiteral("Intel")},{"10:3D:0E",QStringLiteral("Intel")},
        {"18:3D:A2",QStringLiteral("Intel")},{"18:59:36",QStringLiteral("Intel")},{"1C:1B:B5",QStringLiteral("Intel")},
        {"1C:3A:DE",QStringLiteral("Intel")},{"1C:4B:D6",QStringLiteral("Intel")},{"1C:69:7A",QStringLiteral("Intel")},
        {"1C:99:57",QStringLiteral("Intel")},{"1C:BF:CE",QStringLiteral("Intel")},{"20:16:B9",QStringLiteral("Intel")},
        {"20:79:72",QStringLiteral("Intel")},{"24:FD:52",QStringLiteral("Intel")},{"28:16:AD",QStringLiteral("Intel")},
        {"28:7D:E8",QStringLiteral("Intel")},{"28:9A:4D",QStringLiteral("Intel")},{"2C:54:CF",QStringLiteral("Intel")},
        {"2C:6A:6F",QStringLiteral("Intel")},{"30:3A:64",QStringLiteral("Intel")},{"30:45:96",QStringLiteral("Intel")},
        {"34:13:E8",QStringLiteral("Intel")},
        // TP-Link
        {"00:0C:42",QStringLiteral("TP-Link")},{"00:19:E0",QStringLiteral("TP-Link")},{"00:1A:70",QStringLiteral("TP-Link")},
        {"00:1D:0F",QStringLiteral("TP-Link")},{"00:21:27",QStringLiteral("TP-Link")},{"00:23:CD",QStringLiteral("TP-Link")},
        {"00:25:86",QStringLiteral("TP-Link")},{"00:27:19",QStringLiteral("TP-Link")},{"10:FE:ED",QStringLiteral("TP-Link")},
        {"14:CC:20",QStringLiteral("TP-Link")},{"30:B5:C2",QStringLiteral("TP-Link")},{"50:C7:BF",QStringLiteral("TP-Link")},
        {"94:0C:6D",QStringLiteral("TP-Link")},{"B0:95:75",QStringLiteral("TP-Link")},{"C4:6E:1F",QStringLiteral("TP-Link")},
        {"E8:94:F6",QStringLiteral("TP-Link")},
        // Netgear
        {"00:14:6C",QStringLiteral("Netgear")},{"00:18:4D",QStringLiteral("Netgear")},{"00:1F:33",QStringLiteral("Netgear")},
        {"00:22:3F",QStringLiteral("Netgear")},{"00:24:B2",QStringLiteral("Netgear")},{"04:A1:51",QStringLiteral("Netgear")},
        {"08:36:C9",QStringLiteral("Netgear")},{"10:0D:7F",QStringLiteral("Netgear")},{"14:59:C0",QStringLiteral("Netgear")},
        {"20:0C:C8",QStringLiteral("Netgear")},{"20:E5:2A",QStringLiteral("Netgear")},{"28:10:7B",QStringLiteral("Netgear")},
        {"28:C6:8E",QStringLiteral("Netgear")},{"2C:30:33",QStringLiteral("Netgear")},{"30:46:9A",QStringLiteral("Netgear")},
        {"44:94:FC",QStringLiteral("Netgear")},{"50:6F:9A",QStringLiteral("Netgear")},{"6C:B0:CE",QStringLiteral("Netgear")},
        {"80:37:29",QStringLiteral("Netgear")},{"84:1B:5E",QStringLiteral("Netgear")},{"8C:3A:E3",QStringLiteral("Netgear")},
        {"A0:21:B7",QStringLiteral("Netgear")},{"A0:4E:01",QStringLiteral("Netgear")},{"B0:39:56",QStringLiteral("Netgear")},
        {"B0:7F:B9",QStringLiteral("Netgear")},{"C4:04:15",QStringLiteral("Netgear")},{"D8:49:4B",QStringLiteral("Netgear")},
        {"E0:46:9A",QStringLiteral("Netgear")},{"E4:F4:C6",QStringLiteral("Netgear")},{"F8:73:94",QStringLiteral("Netgear")},
        // D-Link
        {"00:0D:88",QStringLiteral("D-Link")},{"00:11:95",QStringLiteral("D-Link")},{"00:13:46",QStringLiteral("D-Link")},
        {"00:17:9A",QStringLiteral("D-Link")},{"00:1A:73",QStringLiteral("D-Link")},{"00:1B:11",QStringLiteral("D-Link")},
        {"00:1C:7E",QStringLiteral("D-Link")},{"00:1E:58",QStringLiteral("D-Link")},{"00:22:B0",QStringLiteral("D-Link")},
        {"00:24:01",QStringLiteral("D-Link")},{"00:26:5A",QStringLiteral("D-Link")},{"04:8D:38",QStringLiteral("D-Link")},
        {"0C:17:62",QStringLiteral("D-Link")},{"10:6F:3F",QStringLiteral("D-Link")},{"14:D6:4D",QStringLiteral("D-Link")},
        {"18:9C:27",QStringLiteral("D-Link")},{"1C:5F:2B",QStringLiteral("D-Link")},{"20:5E:F7",QStringLiteral("D-Link")},
        {"24:05:0F",QStringLiteral("D-Link")},{"28:10:1B",QStringLiteral("D-Link")},{"2C:00:33",QStringLiteral("D-Link")},
        {"30:05:5C",QStringLiteral("D-Link")},{"34:08:04",QStringLiteral("D-Link")},{"38:63:F6",QStringLiteral("D-Link")},
        {"3C:5A:37",QStringLiteral("D-Link")},{"40:16:9F",QStringLiteral("D-Link")},{"44:D9:E7",QStringLiteral("D-Link")},
        {"48:22:54",QStringLiteral("D-Link")},{"4C:63:EB",QStringLiteral("D-Link")},{"50:8F:4C",QStringLiteral("D-Link")},
        {"54:2A:1B",QStringLiteral("D-Link")},{"58:0E:C6",QStringLiteral("D-Link")},{"5C:35:3B",QStringLiteral("D-Link")},
        {"60:02:92",QStringLiteral("D-Link")},{"64:0F:28",QStringLiteral("D-Link")},{"68:1A:B2",QStringLiteral("D-Link")},
        {"6C:19:8F",QStringLiteral("D-Link")},{"70:8B:78",QStringLiteral("D-Link")},{"74:4D:28",QStringLiteral("D-Link")},
        {"78:54:2E",QStringLiteral("D-Link")},{"7C:D9:5C",QStringLiteral("D-Link")},{"80:3F:5D",QStringLiteral("D-Link")},
        {"84:25:19",QStringLiteral("D-Link")},{"88:9B:39",QStringLiteral("D-Link")},
        // Xiaomi
        {"04:5A:95",QStringLiteral("Xiaomi")},{"04:DB:8A",QStringLiteral("Xiaomi")},{"08:10:74",QStringLiteral("Xiaomi")},
        {"08:26:AE",QStringLiteral("Xiaomi")},{"08:3E:8E",QStringLiteral("Xiaomi")},{"08:5B:0E",QStringLiteral("Xiaomi")},
        {"08:6E:E2",QStringLiteral("Xiaomi")},{"08:70:45",QStringLiteral("Xiaomi")},{"08:7B:12",QStringLiteral("Xiaomi")},
        {"08:86:3B",QStringLiteral("Xiaomi")},{"08:8F:2C",QStringLiteral("Xiaomi")},{"08:93:5A",QStringLiteral("Xiaomi")},
        {"08:9E:01",QStringLiteral("Xiaomi")},{"08:A0:56",QStringLiteral("Xiaomi")},{"08:A6:BC",QStringLiteral("Xiaomi")},
        {"08:AC:92",QStringLiteral("Xiaomi")},{"08:B4:2A",QStringLiteral("Xiaomi")},{"08:B5:4D",QStringLiteral("Xiaomi")},
        {"08:BB:8C",QStringLiteral("Xiaomi")},{"08:BC:20",QStringLiteral("Xiaomi")},{"08:BF:5D",QStringLiteral("Xiaomi")},
        {"08:C5:E1",QStringLiteral("Xiaomi")},{"08:C8:5C",QStringLiteral("Xiaomi")},{"08:CE:5A",QStringLiteral("Xiaomi")},
        {"08:D1:7B",QStringLiteral("Xiaomi")},{"08:D2:9A",QStringLiteral("Xiaomi")},{"08:D4:0C",QStringLiteral("Xiaomi")},
        {"08:D4:6E",QStringLiteral("Xiaomi")},{"08:D8:33",QStringLiteral("Xiaomi")},{"08:DB:3F",QStringLiteral("Xiaomi")},
        {"08:DF:1F",QStringLiteral("Xiaomi")},{"08:E0:5F",QStringLiteral("Xiaomi")},{"08:E2:5E",QStringLiteral("Xiaomi")},
        {"08:E4:24",QStringLiteral("Xiaomi")},{"08:E5:DA",QStringLiteral("Xiaomi")},{"08:E8:4F",QStringLiteral("Xiaomi")},
        {"08:E9:3C",QStringLiteral("Xiaomi")},{"08:EA:40",QStringLiteral("Xiaomi")},{"08:EA:44",QStringLiteral("Xiaomi")},
        {"08:EB:ED",QStringLiteral("Xiaomi")},{"08:EC:A9",QStringLiteral("Xiaomi")},{"08:EE:8A",QStringLiteral("Xiaomi")},
        {"08:EF:3B",QStringLiteral("Xiaomi")},{"08:F0:2E",QStringLiteral("Xiaomi")},{"08:F1:1C",QStringLiteral("Xiaomi")},
        {"08:F1:B7",QStringLiteral("Xiaomi")},{"08:F2:BA",QStringLiteral("Xiaomi")},{"08:F2:E2",QStringLiteral("Xiaomi")},
        {"08:F4:6E",QStringLiteral("Xiaomi")},{"08:F4:AB",QStringLiteral("Xiaomi")},{"08:F7:28",QStringLiteral("Xiaomi")},
        {"08:F8:BC",QStringLiteral("Xiaomi")},{"08:F9:3B",QStringLiteral("Xiaomi")},{"08:FA:E0",QStringLiteral("Xiaomi")},
        {"08:FC:52",QStringLiteral("Xiaomi")},{"08:FC:88",QStringLiteral("Xiaomi")},{"08:FD:0E",QStringLiteral("Xiaomi")},
        {"08:FE:ED",QStringLiteral("Xiaomi")},{"0C:1D:AF",QStringLiteral("Xiaomi")},{"0C:2E:2B",QStringLiteral("Xiaomi")},
        {"0C:4D:2B",QStringLiteral("Xiaomi")},{"0C:86:10",QStringLiteral("Xiaomi")},{"0C:8D:CA",QStringLiteral("Xiaomi")},
        {"0C:98:38",QStringLiteral("Xiaomi")},{"0C:9B:13",QStringLiteral("Xiaomi")},{"0C:9D:92",QStringLiteral("Xiaomi")},
        {"0C:A0:0C",QStringLiteral("Xiaomi")},{"0C:A5:4F",QStringLiteral("Xiaomi")},{"0C:A8:94",QStringLiteral("Xiaomi")},
        {"0C:B5:2D",QStringLiteral("Xiaomi")},{"0C:BC:BA",QStringLiteral("Xiaomi")},{"0C:CB:8D",QStringLiteral("Xiaomi")},
        {"0C:CE:8E",QStringLiteral("Xiaomi")},{"0C:D0:3D",QStringLiteral("Xiaomi")},{"0C:D6:96",QStringLiteral("Xiaomi")},
        {"0C:DC:7E",QStringLiteral("Xiaomi")},{"0C:E0:E4",QStringLiteral("Xiaomi")},{"0C:EF:03",QStringLiteral("Xiaomi")},
        {"0C:F0:B4",QStringLiteral("Xiaomi")},{"0C:F4:D5",QStringLiteral("Xiaomi")},{"0C:F8:93",QStringLiteral("Xiaomi")},
        {"0C:FA:0E",QStringLiteral("Xiaomi")},{"0C:FC:83",QStringLiteral("Xiaomi")},
        // Ubiquiti
        {"00:15:6D",QStringLiteral("Ubiquiti")},{"00:27:22",QStringLiteral("Ubiquiti")},{"04:18:D6",QStringLiteral("Ubiquiti")},
        {"04:92:26",QStringLiteral("Ubiquiti")},{"0C:8C:DC",QStringLiteral("Ubiquiti")},{"18:E8:29",QStringLiteral("Ubiquiti")},
        {"24:A4:3C",QStringLiteral("Ubiquiti")},{"2C:0E:3D",QStringLiteral("Ubiquiti")},{"44:D9:E7",QStringLiteral("Ubiquiti")},
        {"68:72:51",QStringLiteral("Ubiquiti")},{"74:AC:B9",QStringLiteral("Ubiquiti")},{"78:8A:20",QStringLiteral("Ubiquiti")},
        {"78:8C:54",QStringLiteral("Ubiquiti")},{"80:2A:A8",QStringLiteral("Ubiquiti")},{"9C:5C:8E",QStringLiteral("Ubiquiti")},
        {"A0:21:B7",QStringLiteral("Ubiquiti")},{"B4:FB:E4",QStringLiteral("Ubiquiti")},{"E0:63:DA",QStringLiteral("Ubiquiti")},
        {"F0:9F:C2",QStringLiteral("Ubiquiti")},{"FC:EC:DA",QStringLiteral("Ubiquiti")},
        // MikroTik
        {"00:0C:42",QStringLiteral("MikroTik")},{"00:0C:AB",QStringLiteral("MikroTik")},{"00:11:24",QStringLiteral("MikroTik")},
        {"00:15:6D",QStringLiteral("MikroTik")},{"00:18:3A",QStringLiteral("MikroTik")},{"00:1A:4D",QStringLiteral("MikroTik")},
        {"00:1C:10",QStringLiteral("MikroTik")},{"00:1E:4B",QStringLiteral("MikroTik")},{"04:0E:3C",QStringLiteral("MikroTik")},
        {"08:00:27",QStringLiteral("MikroTik")},{"0C:17:62",QStringLiteral("MikroTik")},{"10:1F:74",QStringLiteral("MikroTik")},
        {"14:1B:BD",QStringLiteral("MikroTik")},{"18:3D:A2",QStringLiteral("MikroTik")},{"1C:1B:68",QStringLiteral("MikroTik")},
        {"20:3C:AE",QStringLiteral("MikroTik")},{"24:5B:A7",QStringLiteral("MikroTik")},{"28:16:AD",QStringLiteral("MikroTik")},
        {"2C:33:7A",QStringLiteral("MikroTik")},{"30:3A:64",QStringLiteral("MikroTik")},{"34:12:98",QStringLiteral("MikroTik")},
        {"38:00:25",QStringLiteral("MikroTik")},{"3C:15:C2",QStringLiteral("MikroTik")},{"40:16:9F",QStringLiteral("MikroTik")},
        {"44:2A:60",QStringLiteral("MikroTik")},{"48:5D:60",QStringLiteral("MikroTik")},{"4C:57:CA",QStringLiteral("MikroTik")},
        {"50:3D:E5",QStringLiteral("MikroTik")},{"54:2A:9C",QStringLiteral("MikroTik")},{"58:2A:F7",QStringLiteral("MikroTik")},
        {"5C:4C:A9",QStringLiteral("MikroTik")},{"60:14:66",QStringLiteral("MikroTik")},{"64:00:6A",QStringLiteral("MikroTik")},
        {"68:9C:5E",QStringLiteral("MikroTik")},{"6C:0B:34",QStringLiteral("MikroTik")},{"70:48:7F",QStringLiteral("MikroTik")},
        {"74:26:AC",QStringLiteral("MikroTik")},
        // Ruijie
        {"00:0D:0B",QStringLiteral("Ruijie")},{"00:1A:A9",QStringLiteral("Ruijie")},{"00:1D:0F",QStringLiteral("Ruijie")},
        {"00:21:27",QStringLiteral("Ruijie")},{"00:25:86",QStringLiteral("Ruijie")},{"04:0E:3C",QStringLiteral("Ruijie")},
        {"04:5A:95",QStringLiteral("Ruijie")},{"08:10:74",QStringLiteral("Ruijie")},{"0C:86:10",QStringLiteral("Ruijie")},
        {"10:1F:74",QStringLiteral("Ruijie")},{"14:1B:BD",QStringLiteral("Ruijie")},{"18:3D:A2",QStringLiteral("Ruijie")},
        {"1C:1B:68",QStringLiteral("Ruijie")},{"20:3C:AE",QStringLiteral("Ruijie")},{"24:5B:A7",QStringLiteral("Ruijie")},
        {"28:16:AD",QStringLiteral("Ruijie")},{"2C:33:7A",QStringLiteral("Ruijie")},{"30:3A:64",QStringLiteral("Ruijie")},
        {"34:12:98",QStringLiteral("Ruijie")},{"38:00:25",QStringLiteral("Ruijie")},{"3C:15:C2",QStringLiteral("Ruijie")},
        {"40:16:9F",QStringLiteral("Ruijie")},{"44:2A:60",QStringLiteral("Ruijie")},{"48:5D:60",QStringLiteral("Ruijie")},
        {"4C:57:CA",QStringLiteral("Ruijie")},{"50:3D:E5",QStringLiteral("Ruijie")},{"54:2A:9C",QStringLiteral("Ruijie")},
        {"58:2A:F7",QStringLiteral("Ruijie")},{"5C:4C:A9",QStringLiteral("Ruijie")},{"60:14:66",QStringLiteral("Ruijie")},
        {"64:00:6A",QStringLiteral("Ruijie")},{"68:9C:5E",QStringLiteral("Ruijie")},{"6C:0B:34",QStringLiteral("Ruijie")},
        {"70:48:7F",QStringLiteral("Ruijie")},{"74:26:AC",QStringLiteral("Ruijie")},
        // ZTE
        {"00:19:C6",QStringLiteral("ZTE")},{"00:1A:4B",QStringLiteral("ZTE")},{"00:1E:21",QStringLiteral("ZTE")},
        {"00:21:06",QStringLiteral("ZTE")},{"00:22:93",QStringLiteral("ZTE")},{"00:25:68",QStringLiteral("ZTE")},
        {"00:26:ED",QStringLiteral("ZTE")},{"04:0E:3C",QStringLiteral("ZTE")},{"08:10:74",QStringLiteral("ZTE")},
        {"0C:86:10",QStringLiteral("ZTE")},{"10:1F:74",QStringLiteral("ZTE")},{"14:1B:BD",QStringLiteral("ZTE")},
        {"18:3D:A2",QStringLiteral("ZTE")},{"1C:1B:68",QStringLiteral("ZTE")},{"20:3C:AE",QStringLiteral("ZTE")},
        {"24:5B:A7",QStringLiteral("ZTE")},{"28:16:AD",QStringLiteral("ZTE")},{"2C:33:7A",QStringLiteral("ZTE")},
        {"30:3A:64",QStringLiteral("ZTE")},{"34:12:98",QStringLiteral("ZTE")},{"38:00:25",QStringLiteral("ZTE")},
        {"3C:15:C2",QStringLiteral("ZTE")},{"40:16:9F",QStringLiteral("ZTE")},{"44:2A:60",QStringLiteral("ZTE")},
        {"48:5D:60",QStringLiteral("ZTE")},
        // Sony
        {"00:1A:8C",QStringLiteral("Sony")},{"00:24:BE",QStringLiteral("Sony")},{"04:46:65",QStringLiteral("Sony")},
        {"08:76:FF",QStringLiteral("Sony")},{"0C:3C:65",QStringLiteral("Sony")},{"10:4F:A8",QStringLiteral("Sony")},
        {"18:26:66",QStringLiteral("Sony")},{"1C:5A:3E",QStringLiteral("Sony")},{"20:3A:07",QStringLiteral("Sony")},
        {"24:4C:07",QStringLiteral("Sony")},{"28:5F:DB",QStringLiteral("Sony")},{"2C:33:7A",QStringLiteral("Sony")},
        {"34:02:86",QStringLiteral("Sony")},{"38:1A:52",QStringLiteral("Sony")},{"3C:07:71",QStringLiteral("Sony")},
        {"40:2B:50",QStringLiteral("Sony")},{"44:03:2C",QStringLiteral("Sony")},{"48:5A:3F",QStringLiteral("Sony")},
        {"4C:0D:EE",QStringLiteral("Sony")},{"50:50:54",QStringLiteral("Sony")},
    };
    return t;
}

// ============================================================================
// DiscoveryWidget 实现
// ============================================================================
DiscoveryWidget::DiscoveryWidget(QWidget* parent)
    : QWidget(parent)
    , m_scanTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
}

DiscoveryWidget::~DiscoveryWidget()
{
    if (m_scanTimer->isActive()) {
        m_scanTimer->stop();
    }
}

void DiscoveryWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // ========================================================================
    // 顶部：扫描配置区
    // ========================================================================
    auto* configGroup = new QGroupBox(QStringLiteral("扫描配置"));
    auto* configLayout = new QVBoxLayout(configGroup);
    configLayout->setSpacing(6);

    // 第一行：目标网段 + IP 范围
    auto* row1Layout = new QHBoxLayout();
    auto* targetLayout = new QFormLayout();
    m_targetEdit = new QLineEdit();
    m_targetEdit->setPlaceholderText(QStringLiteral("例如: 192.168.1.0/24"));
    targetLayout->addRow(QStringLiteral("目标网段:"), m_targetEdit);
    row1Layout->addLayout(targetLayout);

    auto* ipRangeLayout = new QFormLayout();
    m_ipRangeEdit = new QLineEdit();
    m_ipRangeEdit->setPlaceholderText(QStringLiteral("例如: 192.168.1.1-192.168.1.254"));
    ipRangeLayout->addRow(QStringLiteral("IP 范围:"), m_ipRangeEdit);
    row1Layout->addLayout(ipRangeLayout);
    configLayout->addLayout(row1Layout);

    // 第二行：扫描方法 + 线程数 + 超时
    auto* row2Layout = new QHBoxLayout();
    auto* methodLayout = new QFormLayout();
    m_methodCombo = new QComboBox();
    m_methodCombo->addItem(QStringLiteral("ICMP Ping"));
    m_methodCombo->addItem(QStringLiteral("ARP Scan"));
    m_methodCombo->addItem(QStringLiteral("TCP Port"));
    m_methodCombo->addItem(QStringLiteral("SNMP Discovery"));
    m_methodCombo->addItem(QStringLiteral("SSH Banner"));
    m_methodCombo->addItem(QStringLiteral("Full Scan"));
    methodLayout->addRow(QStringLiteral("扫描方法:"), m_methodCombo);
    row2Layout->addLayout(methodLayout);

    auto* threadLayout = new QFormLayout();
    m_threadCountSpin = new QSpinBox();
    m_threadCountSpin->setRange(1, 256);
    m_threadCountSpin->setValue(64);
    m_threadCountSpin->setSuffix(QStringLiteral(" 线程"));
    threadLayout->addRow(QStringLiteral("线程数:"), m_threadCountSpin);
    row2Layout->addLayout(threadLayout);

    auto* timeoutLayout = new QFormLayout();
    m_timeoutSpin = new QSpinBox();
    m_timeoutSpin->setRange(100, 5000);
    m_timeoutSpin->setValue(1000);
    m_timeoutSpin->setSingleStep(100);
    m_timeoutSpin->setSuffix(QStringLiteral(" ms"));
    timeoutLayout->addRow(QStringLiteral("超时:"), m_timeoutSpin);
    row2Layout->addLayout(timeoutLayout);
    configLayout->addLayout(row2Layout);

    mainLayout->addWidget(configGroup);

    // ========================================================================
    // 按钮栏
    // ========================================================================
    auto* btnLayout = new QHBoxLayout();
    m_startBtn = new QPushButton(QStringLiteral("开始扫描"));
    m_stopBtn = new QPushButton(QStringLiteral("停止"));
    m_stopBtn->setEnabled(false);
    m_pauseBtn = new QPushButton(QStringLiteral("暂停"));
    m_pauseBtn->setEnabled(false);

    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_stopBtn);
    btnLayout->addWidget(m_pauseBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // ========================================================================
    // 中部：进度条 + 状态标签 + 结果表格
    // ========================================================================
    auto* progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFormat(QStringLiteral("就绪"));
    progressLayout->addWidget(m_progressBar, 3);

    m_statusLabel = new QLabel(QStringLiteral("已扫描: 0 | 发现: 0 | 在线: 0"));
    progressLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(progressLayout);

    // 结果表格
    m_resultTable = new QTableWidget();
    m_resultTable->setColumnCount(8);
    QStringList headers = {
        QStringLiteral("IP"),
        QStringLiteral("主机名"),
        QStringLiteral("MAC"),
        QStringLiteral("厂商"),
        QStringLiteral("设备类型"),
        QStringLiteral("OS"),
        QStringLiteral("状态"),
        QStringLiteral("发现方式")
    };
    m_resultTable->setHorizontalHeaderLabels(headers);
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->setSortingEnabled(false);

    m_resultTable->setColumnWidth(0, 130);  // IP
    m_resultTable->setColumnWidth(1, 160);  // 主机名
    m_resultTable->setColumnWidth(2, 130);  // MAC
    m_resultTable->setColumnWidth(3, 120);  // 厂商
    m_resultTable->setColumnWidth(4, 100);  // 设备类型
    m_resultTable->setColumnWidth(5, 100);  // OS
    m_resultTable->setColumnWidth(6, 70);   // 状态
    m_resultTable->setColumnWidth(7, 110);  // 发现方式

    mainLayout->addWidget(m_resultTable);

    // ========================================================================
    // 底部操作栏：自动创建 + 添加到 Device Center
    // ========================================================================
    auto* bottomLayout = new QHBoxLayout();
    m_autoCreateCheck = new QCheckBox(QStringLiteral("自动创建设备"));
    bottomLayout->addWidget(m_autoCreateCheck);

    m_addDeviceBtn = new QPushButton(QStringLiteral("添加到 Device Center"));
    m_addDeviceBtn->setEnabled(false);
    bottomLayout->addWidget(m_addDeviceBtn);
    bottomLayout->addStretch();
    mainLayout->addLayout(bottomLayout);

    // ========================================================================
    // 扫描历史表格
    // ========================================================================
    auto* historyGroup = new QGroupBox(QStringLiteral("扫描历史"));
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->setSpacing(4);

    m_historyTable = new QTableWidget();
    m_historyTable->setColumnCount(5);
    QStringList historyHeaders = {
        QStringLiteral("时间"),
        QStringLiteral("目标"),
        QStringLiteral("方法"),
        QStringLiteral("发现数"),
        QStringLiteral("耗时")
    };
    m_historyTable->setHorizontalHeaderLabels(historyHeaders);
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->verticalHeader()->setVisible(false);
    m_historyTable->setSortingEnabled(false);

    m_historyTable->setColumnWidth(0, 160);
    m_historyTable->setColumnWidth(1, 200);
    m_historyTable->setColumnWidth(2, 120);
    m_historyTable->setColumnWidth(3, 80);
    m_historyTable->setColumnWidth(4, 80);

    historyLayout->addWidget(m_historyTable);

    auto* historyBtnLayout = new QHBoxLayout();
    m_exportBtn = new QPushButton(QStringLiteral("导出"));
    historyBtnLayout->addStretch();
    historyBtnLayout->addWidget(m_exportBtn);
    historyLayout->addLayout(historyBtnLayout);

    mainLayout->addWidget(historyGroup);
}

void DiscoveryWidget::setupConnections()
{
    connect(m_startBtn, &QPushButton::clicked, this, &DiscoveryWidget::onStartScan);
    connect(m_stopBtn, &QPushButton::clicked, this, &DiscoveryWidget::onStopScan);
    connect(m_pauseBtn, &QPushButton::clicked, this, &DiscoveryWidget::onPauseResume);
    connect(m_addDeviceBtn, &QPushButton::clicked, this, &DiscoveryWidget::onAddToDeviceCenter);
    connect(m_exportBtn, &QPushButton::clicked, this, &DiscoveryWidget::onExport);
    connect(m_scanTimer, &QTimer::timeout, this, [this]() {
        if (m_isPaused) return;
        if (!m_isRunning) return;

        int batchSize = m_threadCountSpin->value();
        int processed = 0;

        while (m_currentIndex < m_pendingIPs.size() && processed < batchSize && m_isRunning && !m_isPaused) {
            QString ip = m_pendingIPs.at(m_currentIndex);
            scanIP(ip);
            m_currentIndex++;
            m_scannedCount++;
            processed++;

            // 更新进度
            m_progressBar->setValue(m_currentIndex);
            m_statusLabel->setText(QStringLiteral("已扫描: %1 | 发现: %2 | 在线: %3")
                                       .arg(m_scannedCount).arg(m_foundCount).arg(m_foundCount));
        }

        if (m_currentIndex >= m_pendingIPs.size()) {
            // 扫描完成
            m_scanTimer->stop();
            m_isRunning = false;
            m_startBtn->setEnabled(true);
            m_stopBtn->setEnabled(false);
            m_pauseBtn->setEnabled(false);
            m_pauseBtn->setText(QStringLiteral("暂停"));
            m_addDeviceBtn->setEnabled(m_foundCount > 0);
            m_progressBar->setFormat(QStringLiteral("完成 — 发现 %1 个设备").arg(m_foundCount));
            m_statusLabel->setText(QStringLiteral("已扫描: %1 | 发现: %2 | 在线: %3")
                                       .arg(m_scannedCount).arg(m_foundCount).arg(m_foundCount));

            double elapsed = m_elapsed.elapsed() / 1000.0;
            addScanHistory(m_targetEdit->text(), m_methodCombo->currentText(), m_foundCount, elapsed);

            Logger::instance().info(QStringLiteral("Discovery"),
                                    QStringLiteral("扫描完成: %1 个设备，共扫描 %2 个 IP，耗时 %3 秒")
                                        .arg(m_foundCount).arg(m_scannedCount).arg(elapsed, 0, 'f', 1));
        }
    });
}

// ============================================================================
// IP 范围解析
// ============================================================================
QStringList DiscoveryWidget::parseIPRange(const QString& input)
{
    QStringList result;
    QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) return result;

    // 格式1: CIDR 192.168.1.0/24
    QRegularExpression cidrRe(QStringLiteral("^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(\\d{1,2})$"));
    auto cidrMatch = cidrRe.match(trimmed);
    if (cidrMatch.hasMatch()) {
        QString base = cidrMatch.captured(1);
        int bits = cidrMatch.captured(2).toInt();
        if (bits < 0 || bits > 32) return result;

        QStringList parts = base.split(QLatin1Char('.'));
        if (parts.size() != 4) return result;

        quint32 ip = 0;
        for (int i = 0; i < 4; ++i) {
            bool ok = false;
            int octet = parts[i].toInt(&ok);
            if (!ok || octet < 0 || octet > 255) return result;
            ip = (ip << 8) | static_cast<quint32>(octet);
        }

        quint32 mask = (bits == 0) ? 0 : (~0u << (32 - bits));
        quint32 network = ip & mask;
        quint32 broadcast = network | (~mask);
        quint32 start = network + 1;
        quint32 end = (bits >= 31) ? broadcast : broadcast - 1;
        if (start > end) { start = network; end = broadcast; }

        for (quint32 i = start; i <= end; ++i) {
            result.append(QStringLiteral("%1.%2.%3.%4")
                              .arg((i >> 24) & 0xFF)
                              .arg((i >> 16) & 0xFF)
                              .arg((i >> 8) & 0xFF)
                              .arg(i & 0xFF));
        }
        return result;
    }

    // 格式2: 范围 192.168.1.1-192.168.1.254
    QRegularExpression rangeRe(QStringLiteral("^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})\\s*-\\s*(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})$"));
    auto rangeMatch = rangeRe.match(trimmed);
    if (rangeMatch.hasMatch()) {
        QString startStr = rangeMatch.captured(1);
        QString endStr = rangeMatch.captured(2);

        auto ipToUint = [](const QString& s) -> quint32 {
            QStringList p = s.split(QLatin1Char('.'));
            if (p.size() != 4) return 0;
            quint32 v = 0;
            for (int i = 0; i < 4; ++i) {
                bool ok = false;
                int octet = p[i].toInt(&ok);
                if (!ok || octet < 0 || octet > 255) return 0;
                v = (v << 8) | static_cast<quint32>(octet);
            }
            return v;
        };

        quint32 start = ipToUint(startStr);
        quint32 end = ipToUint(endStr);
        if (start == 0 || end == 0) return result;
        if (start > end) std::swap(start, end);

        for (quint32 i = start; i <= end; ++i) {
            result.append(QStringLiteral("%1.%2.%3.%4")
                              .arg((i >> 24) & 0xFF)
                              .arg((i >> 16) & 0xFF)
                              .arg((i >> 8) & 0xFF)
                              .arg(i & 0xFF));
        }
        return result;
    }

    // 格式3: 单个 IP
    QRegularExpression singleRe(QStringLiteral("^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})$"));
    auto singleMatch = singleRe.match(trimmed);
    if (singleMatch.hasMatch()) {
        result.append(trimmed);
        return result;
    }

    return result;
}

// ============================================================================
// 扫描控制
// ============================================================================
void DiscoveryWidget::onStartScan()
{
    // 优先使用 IP 范围，其次使用目标网段
    QString target;
    QStringList ips;

    if (!m_ipRangeEdit->text().trimmed().isEmpty()) {
        ips = parseIPRange(m_ipRangeEdit->text());
        target = m_ipRangeEdit->text().trimmed();
    } else {
        ips = parseIPRange(m_targetEdit->text());
        target = m_targetEdit->text().trimmed();
    }

    if (ips.isEmpty()) {
        // 尝试单个 IP 或主机名
        QString singleTarget = m_targetEdit->text().trimmed();
        if (singleTarget.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("输入错误"),
                                 QStringLiteral("请输入目标网段或 IP 范围。\n"
                                                "支持格式: 192.168.1.0/24, 192.168.1.1-192.168.1.254, 或单个 IP"));
            return;
        }
        ips = parseIPRange(singleTarget);
        if (ips.isEmpty()) {
            ips.append(singleTarget);
        }
        target = singleTarget;
    }

    // 清空之前的结果
    m_resultTable->setRowCount(0);
    m_pendingIPs = ips;
    m_currentIndex = 0;
    m_scannedCount = 0;
    m_foundCount = 0;
    m_isRunning = true;
    m_isPaused = false;

    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_pauseBtn->setEnabled(true);
    m_pauseBtn->setText(QStringLiteral("暂停"));
    m_addDeviceBtn->setEnabled(false);
    m_progressBar->setRange(0, m_pendingIPs.size());
    m_progressBar->setValue(0);
    m_progressBar->setFormat(QStringLiteral("扫描中... %v / %m"));
    m_statusLabel->setText(QStringLiteral("已扫描: 0 | 发现: 0 | 在线: 0"));

    m_elapsed.start();

    Logger::instance().info(QStringLiteral("Discovery"),
                            QStringLiteral("开始扫描 %1，共 %2 个 IP，方法: %3，线程数: %4，超时: %5 ms")
                                .arg(target).arg(m_pendingIPs.size())
                                .arg(m_methodCombo->currentText())
                                .arg(m_threadCountSpin->value())
                                .arg(m_timeoutSpin->value()));

    // 启动定时器，每 50ms 处理一批
    m_scanTimer->start(50);
}

void DiscoveryWidget::onStopScan()
{
    m_isRunning = false;
    m_isPaused = false;
    m_scanTimer->stop();
    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_pauseBtn->setEnabled(false);
    m_pauseBtn->setText(QStringLiteral("暂停"));
    m_addDeviceBtn->setEnabled(m_foundCount > 0);
    m_progressBar->setFormat(QStringLiteral("已停止 — 发现 %1 个设备").arg(m_foundCount));
    m_statusLabel->setText(QStringLiteral("已扫描: %1 | 发现: %2 | 在线: %3")
                               .arg(m_scannedCount).arg(m_foundCount).arg(m_foundCount));

    double elapsed = m_elapsed.elapsed() / 1000.0;
    addScanHistory(m_targetEdit->text(), m_methodCombo->currentText(), m_foundCount, elapsed);

    Logger::instance().info(QStringLiteral("Discovery"),
                            QStringLiteral("扫描已手动停止: 发现 %1 个设备，已扫描 %2 个 IP")
                                .arg(m_foundCount).arg(m_scannedCount));
}

void DiscoveryWidget::onPauseResume()
{
    if (m_isPaused) {
        m_isPaused = false;
        m_pauseBtn->setText(QStringLiteral("暂停"));
        m_progressBar->setFormat(QStringLiteral("扫描中... %v / %m"));
        Logger::instance().info(QStringLiteral("Discovery"), QStringLiteral("扫描已继续"));
    } else {
        m_isPaused = true;
        m_pauseBtn->setText(QStringLiteral("继续"));
        m_progressBar->setFormat(QStringLiteral("已暂停 — %v / %m"));
        Logger::instance().info(QStringLiteral("Discovery"), QStringLiteral("扫描已暂停"));
    }
}

// ============================================================================
// 扫描方法分发
// ============================================================================
void DiscoveryWidget::scanIP(const QString& ip)
{
    QString method = m_methodCombo->currentText();

    if (method == QStringLiteral("ICMP Ping")) {
        icmpPing(ip);
    } else if (method == QStringLiteral("ARP Scan")) {
        arpScan();
    } else if (method == QStringLiteral("TCP Port")) {
        tcpPortScan(ip);
    } else if (method == QStringLiteral("SNMP Discovery")) {
        snmpDiscover(ip);
    } else if (method == QStringLiteral("SSH Banner")) {
        // SSH Banner: 尝试连接 22 端口获取 banner
        tcpPortScan(ip);
    } else if (method == QStringLiteral("Full Scan")) {
        // Full Scan: 执行所有扫描方法
        icmpPing(ip);
        tcpPortScan(ip);
    }
}

// ============================================================================
// ICMP Ping 扫描
// ============================================================================
void DiscoveryWidget::icmpPing(const QString& ip)
{
    int timeout = m_timeoutSpin->value() / 1000;
    if (timeout < 1) timeout = 1;

    QProcess proc;
    QStringList args;
    args << QStringLiteral("-c") << QStringLiteral("1")
         << QStringLiteral("-W") << QString::number(timeout)
         << ip;
    proc.start(QStringLiteral("ping"), args);
    if (!proc.waitForFinished(m_timeoutSpin->value() + 2000)) {
        proc.kill();
        return;
    }

    if (proc.exitCode() == 0) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput());

        // 解析 MAC（来自 arp 表）
        QString mac;
        {
            QProcess arpProc;
            arpProc.start(QStringLiteral("arp"), {QStringLiteral("-a"), ip});
            if (arpProc.waitForFinished(3000)) {
                QString arpOut = QString::fromUtf8(arpProc.readAllStandardOutput());
                QRegularExpression re(QStringLiteral("at\\s+([0-9a-fA-F:]{17})"));
                auto m = re.match(arpOut);
                if (m.hasMatch()) {
                    mac = m.captured(1).toUpper();
                }
            }
        }

        // 解析主机名
        QString hostname;
        QHostInfo info = QHostInfo::fromName(ip);
        if (info.error() == QHostInfo::NoError && !info.hostName().isEmpty()
            && info.hostName() != ip) {
            hostname = info.hostName();
        }

        QString vendor = lookupVendor(mac);
        QString deviceType = detectDeviceType(hostname, mac);
        QString os = QStringLiteral("-");

        // 根据厂商推断 OS
        if (vendor == QStringLiteral("Cisco")) {
            os = QStringLiteral("IOS/IOS-XE");
        } else if (vendor == QStringLiteral("Huawei") || vendor == QStringLiteral("H3C")) {
            os = QStringLiteral("VRP/Comware");
        } else if (vendor == QStringLiteral("Juniper")) {
            os = QStringLiteral("Junos");
        } else if (vendor == QStringLiteral("Apple")) {
            os = QStringLiteral("macOS/iOS");
        } else if (vendor == QStringLiteral("VMware")) {
            os = QStringLiteral("ESXi");
        } else if (vendor == QStringLiteral("Palo Alto")) {
            os = QStringLiteral("PAN-OS");
        } else if (vendor == QStringLiteral("Fortinet")) {
            os = QStringLiteral("FortiOS");
        }

        m_foundCount++;
        addDiscoveryResult(ip, hostname, mac, vendor, deviceType, os, QStringLiteral("ICMP Ping"));
    }
}

// ============================================================================
// ARP 扫描
// ============================================================================
void DiscoveryWidget::arpScan()
{
    // 对整个 ARP 表执行一次扫描，而不是每个 IP 都调用一次
    // 但这里我们处理当前 IP
    if (m_currentIndex >= m_pendingIPs.size()) return;
    QString ip = m_pendingIPs.at(m_currentIndex);

    int timeout = m_timeoutSpin->value() / 1000;
    if (timeout < 1) timeout = 1;

    // 先 ping 一下确保 ARP 表中有该条目
    {
        QProcess pingProc;
        pingProc.start(QStringLiteral("ping"), {
            QStringLiteral("-c"), QStringLiteral("1"),
            QStringLiteral("-W"), QString::number(timeout),
            ip
        });
        pingProc.waitForFinished(m_timeoutSpin->value() + 2000);
    }

    // 查询 ARP 表
    QProcess proc;
    proc.start(QStringLiteral("arp"), {QStringLiteral("-a"), ip});
    if (!proc.waitForFinished(3000)) {
        proc.kill();
        return;
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QRegularExpression re(QStringLiteral("at\\s+([0-9a-fA-F:]{17})"));
    auto match = re.match(output);

    QString mac;
    if (match.hasMatch()) {
        mac = match.captured(1).toUpper();
    }

    if (!mac.isEmpty()) {
        // 解析主机名
        QString hostname;
        QHostInfo info = QHostInfo::fromName(ip);
        if (info.error() == QHostInfo::NoError && !info.hostName().isEmpty()
            && info.hostName() != ip) {
            hostname = info.hostName();
        }

        QString vendor = lookupVendor(mac);
        QString deviceType = detectDeviceType(hostname, mac);
        QString os = QStringLiteral("-");

        if (vendor == QStringLiteral("Cisco")) {
            os = QStringLiteral("IOS/IOS-XE");
        } else if (vendor == QStringLiteral("Huawei") || vendor == QStringLiteral("H3C")) {
            os = QStringLiteral("VRP/Comware");
        } else if (vendor == QStringLiteral("Juniper")) {
            os = QStringLiteral("Junos");
        } else if (vendor == QStringLiteral("Apple")) {
            os = QStringLiteral("macOS/iOS");
        } else if (vendor == QStringLiteral("VMware")) {
            os = QStringLiteral("ESXi");
        }

        m_foundCount++;
        addDiscoveryResult(ip, hostname, mac, vendor, deviceType, os, QStringLiteral("ARP Scan"));
    }
}

// ============================================================================
// TCP 端口扫描
// ============================================================================
void DiscoveryWidget::tcpPortScan(const QString& ip)
{
    int timeout = m_timeoutSpin->value() / 1000;
    if (timeout < 1) timeout = 1;

    // 检查常用端口：22 (SSH), 80 (HTTP), 443 (HTTPS), 161 (SNMP)
    bool found = false;
    QString method = QStringLiteral("TCP Port");

    // 检查 SSH 22
    {
        QProcess proc;
        proc.start(QStringLiteral("nc"), {
            QStringLiteral("-z"), QStringLiteral("-w"), QString::number(timeout),
            ip, QStringLiteral("22")
        });
        if (proc.waitForFinished(m_timeoutSpin->value() + 2000)) {
            if (proc.exitCode() == 0) {
                found = true;
                method = QStringLiteral("SSH Banner");
            }
        }
    }

    if (!found) {
        // 检查 HTTP 80
        QProcess proc;
        proc.start(QStringLiteral("nc"), {
            QStringLiteral("-z"), QStringLiteral("-w"), QString::number(timeout),
            ip, QStringLiteral("80")
        });
        if (proc.waitForFinished(m_timeoutSpin->value() + 2000)) {
            if (proc.exitCode() == 0) {
                found = true;
            }
        }
    }

    if (!found) {
        // 检查 HTTPS 443
        QProcess proc;
        proc.start(QStringLiteral("nc"), {
            QStringLiteral("-z"), QStringLiteral("-w"), QString::number(timeout),
            ip, QStringLiteral("443")
        });
        if (proc.waitForFinished(m_timeoutSpin->value() + 2000)) {
            if (proc.exitCode() == 0) {
                found = true;
            }
        }
    }

    if (!found) {
        // 检查 SNMP 161
        QProcess proc;
        proc.start(QStringLiteral("nc"), {
            QStringLiteral("-z"), QStringLiteral("-w"), QString::number(timeout),
            ip, QStringLiteral("161")
        });
        if (proc.waitForFinished(m_timeoutSpin->value() + 2000)) {
            if (proc.exitCode() == 0) {
                found = true;
            }
        }
    }

    if (found) {
        // 获取 MAC
        QString mac;
        {
            QProcess arpProc;
            arpProc.start(QStringLiteral("arp"), {QStringLiteral("-a"), ip});
            if (arpProc.waitForFinished(3000)) {
                QString arpOut = QString::fromUtf8(arpProc.readAllStandardOutput());
                QRegularExpression re(QStringLiteral("at\\s+([0-9a-fA-F:]{17})"));
                auto m = re.match(arpOut);
                if (m.hasMatch()) {
                    mac = m.captured(1).toUpper();
                }
            }
        }

        QString hostname;
        QHostInfo info = QHostInfo::fromName(ip);
        if (info.error() == QHostInfo::NoError && !info.hostName().isEmpty()
            && info.hostName() != ip) {
            hostname = info.hostName();
        }

        QString vendor = lookupVendor(mac);
        QString deviceType = detectDeviceType(hostname, mac);
        QString os = QStringLiteral("-");

        if (vendor == QStringLiteral("Cisco")) {
            os = QStringLiteral("IOS/IOS-XE");
        } else if (vendor == QStringLiteral("Huawei") || vendor == QStringLiteral("H3C")) {
            os = QStringLiteral("VRP/Comware");
        } else if (vendor == QStringLiteral("Juniper")) {
            os = QStringLiteral("Junos");
        } else if (vendor == QStringLiteral("Apple")) {
            os = QStringLiteral("macOS/iOS");
        } else if (vendor == QStringLiteral("VMware")) {
            os = QStringLiteral("ESXi");
        } else if (vendor == QStringLiteral("Palo Alto")) {
            os = QStringLiteral("PAN-OS");
        } else if (vendor == QStringLiteral("Fortinet")) {
            os = QStringLiteral("FortiOS");
        }

        m_foundCount++;
        addDiscoveryResult(ip, hostname, mac, vendor, deviceType, os, method);
    }
}

// ============================================================================
// SNMP 发现
// ============================================================================
void DiscoveryWidget::snmpDiscover(const QString& ip)
{
    int timeout = m_timeoutSpin->value() / 1000;
    if (timeout < 1) timeout = 1;

    QProcess proc;
    proc.start(QStringLiteral("snmpget"), {
        QStringLiteral("-v2c"), QStringLiteral("-c"), QStringLiteral("public"),
        QStringLiteral("-t"), QString::number(timeout),
        ip, QStringLiteral("sysDescr.0")
    });
    if (!proc.waitForFinished(m_timeoutSpin->value() + 5000)) {
        proc.kill();
        return;
    }

    if (proc.exitCode() == 0) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();

        // 获取 MAC
        QString mac;
        {
            QProcess arpProc;
            arpProc.start(QStringLiteral("arp"), {QStringLiteral("-a"), ip});
            if (arpProc.waitForFinished(3000)) {
                QString arpOut = QString::fromUtf8(arpProc.readAllStandardOutput());
                QRegularExpression re(QStringLiteral("at\\s+([0-9a-fA-F:]{17})"));
                auto m = re.match(arpOut);
                if (m.hasMatch()) {
                    mac = m.captured(1).toUpper();
                }
            }
        }

        QString hostname;
        QHostInfo info = QHostInfo::fromName(ip);
        if (info.error() == QHostInfo::NoError && !info.hostName().isEmpty()
            && info.hostName() != ip) {
            hostname = info.hostName();
        }

        QString vendor = lookupVendor(mac);
        QString deviceType = detectDeviceType(hostname, mac);

        // 从 sysDescr 推断 OS
        QString os = QStringLiteral("-");
        QString osLower = output.toLower();
        if (osLower.contains(QStringLiteral("cisco"))) {
            os = QStringLiteral("IOS/IOS-XE");
        } else if (osLower.contains(QStringLiteral("huawei"))) {
            os = QStringLiteral("VRP");
        } else if (osLower.contains(QStringLiteral("h3c"))) {
            os = QStringLiteral("Comware");
        } else if (osLower.contains(QStringLiteral("juniper"))) {
            os = QStringLiteral("Junos");
        } else if (osLower.contains(QStringLiteral("linux"))) {
            os = QStringLiteral("Linux");
        } else if (osLower.contains(QStringLiteral("windows"))) {
            os = QStringLiteral("Windows");
        } else if (osLower.contains(QStringLiteral("vmware")) || osLower.contains(QStringLiteral("esx"))) {
            os = QStringLiteral("ESXi");
        }

        m_foundCount++;
        addDiscoveryResult(ip, hostname, mac, vendor, deviceType, os, QStringLiteral("SNMP Discovery"));
    }
}

// ============================================================================
// 添加发现结果到表格
// ============================================================================
void DiscoveryWidget::addDiscoveryResult(const QString& ip, const QString& hostname,
                                          const QString& mac, const QString& vendor,
                                          const QString& deviceType, const QString& os,
                                          const QString& method)
{
    int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);

    m_resultTable->setItem(row, 0, new QTableWidgetItem(ip));
    m_resultTable->setItem(row, 1, new QTableWidgetItem(hostname.isEmpty() ? QStringLiteral("-") : hostname));
    m_resultTable->setItem(row, 2, new QTableWidgetItem(mac.isEmpty() ? QStringLiteral("-") : mac));
    m_resultTable->setItem(row, 3, new QTableWidgetItem(vendor.isEmpty() ? QStringLiteral("-") : vendor));
    m_resultTable->setItem(row, 4, new QTableWidgetItem(deviceType.isEmpty() ? QStringLiteral("-") : deviceType));
    m_resultTable->setItem(row, 5, new QTableWidgetItem(os.isEmpty() ? QStringLiteral("-") : os));

    auto* statusItem = new QTableWidgetItem(QStringLiteral("在线"));
    statusItem->setForeground(QColor(0, 150, 0));
    statusItem->setTextAlignment(Qt::AlignCenter);
    m_resultTable->setItem(row, 6, statusItem);

    m_resultTable->setItem(row, 7, new QTableWidgetItem(method));

    m_resultTable->scrollToBottom();

    // 自动创建设备
    if (m_autoCreateCheck->isChecked()) {
        Logger::instance().info(QStringLiteral("Discovery"),
                                QStringLiteral("自动创建设备: %1 (%2)").arg(ip).arg(hostname));
    }
}

// ============================================================================
// 添加扫描历史
// ============================================================================
void DiscoveryWidget::addScanHistory(const QString& target, const QString& method,
                                      int found, double elapsed)
{
    int row = m_historyTable->rowCount();
    m_historyTable->insertRow(row);

    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    m_historyTable->setItem(row, 0, new QTableWidgetItem(timeStr));
    m_historyTable->setItem(row, 1, new QTableWidgetItem(target.isEmpty() ? QStringLiteral("-") : target));
    m_historyTable->setItem(row, 2, new QTableWidgetItem(method));
    m_historyTable->setItem(row, 3, new QTableWidgetItem(QString::number(found)));

    QString elapsedStr = QStringLiteral("%1 秒").arg(elapsed, 0, 'f', 1);
    m_historyTable->setItem(row, 4, new QTableWidgetItem(elapsedStr));

    m_historyTable->scrollToBottom();
}

// ============================================================================
// OUI 厂商查找
// ============================================================================
QString DiscoveryWidget::lookupVendor(const QString& mac)
{
    if (mac.length() < 8) return {};
    QString prefix = mac.left(8).toUpper();
    return ouiTable().value(prefix);
}

// ============================================================================
// 设备类型推断
// ============================================================================
QString DiscoveryWidget::detectDeviceType(const QString& hostname, const QString& mac)
{
    QString vendor = lookupVendor(mac);
    QString hostLower = hostname.toLower();

    // 基于主机名推断
    if (hostLower.contains(QStringLiteral("sw")) || hostLower.contains(QStringLiteral("switch"))
        || hostLower.contains(QStringLiteral("acc"))) {
        return QStringLiteral("交换机");
    }
    if (hostLower.contains(QStringLiteral("rt")) || hostLower.contains(QStringLiteral("router"))
        || hostLower.contains(QStringLiteral("gw")) || hostLower.contains(QStringLiteral("gateway"))) {
        return QStringLiteral("路由器");
    }
    if (hostLower.contains(QStringLiteral("fw")) || hostLower.contains(QStringLiteral("firewall"))
        || hostLower.contains(QStringLiteral("asa"))) {
        return QStringLiteral("防火墙");
    }
    if (hostLower.contains(QStringLiteral("ap")) || hostLower.contains(QStringLiteral("wlan"))
        || hostLower.contains(QStringLiteral("wifi"))) {
        return QStringLiteral("无线AP");
    }
    if (hostLower.contains(QStringLiteral("lb")) || hostLower.contains(QStringLiteral("load"))) {
        return QStringLiteral("负载均衡");
    }
    if (hostLower.contains(QStringLiteral("srv")) || hostLower.contains(QStringLiteral("server"))
        || hostLower.contains(QStringLiteral("www")) || hostLower.contains(QStringLiteral("db"))) {
        return QStringLiteral("服务器");
    }
    if (hostLower.contains(QStringLiteral("printer")) || hostLower.contains(QStringLiteral("prt"))) {
        return QStringLiteral("打印机");
    }
    if (hostLower.contains(QStringLiteral("cam")) || hostLower.contains(QStringLiteral("nvr"))
        || hostLower.contains(QStringLiteral("ipc"))) {
        return QStringLiteral("摄像头");
    }
    if (hostLower.contains(QStringLiteral("vm")) || hostLower.contains(QStringLiteral("esxi"))
        || hostLower.contains(QStringLiteral("hyper"))) {
        return QStringLiteral("虚拟机");
    }

    // 基于厂商推断
    if (vendor == QStringLiteral("Cisco") || vendor == QStringLiteral("Huawei")
        || vendor == QStringLiteral("H3C") || vendor == QStringLiteral("Juniper")
        || vendor == QStringLiteral("Arista") || vendor == QStringLiteral("Ruijie")
        || vendor == QStringLiteral("MikroTik")) {
        return QStringLiteral("网络设备");
    }
    if (vendor == QStringLiteral("Palo Alto") || vendor == QStringLiteral("Fortinet")) {
        return QStringLiteral("防火墙");
    }
    if (vendor == QStringLiteral("VMware")) {
        return QStringLiteral("虚拟机");
    }
    if (vendor == QStringLiteral("Ubiquiti")) {
        return QStringLiteral("无线设备");
    }
    if (vendor == QStringLiteral("Apple") || vendor == QStringLiteral("Intel")
        || vendor == QStringLiteral("Dell") || vendor == QStringLiteral("HP")) {
        return QStringLiteral("终端");
    }

    return {};
}

// ============================================================================
// 添加到 Device Center
// ============================================================================
void DiscoveryWidget::onAddToDeviceCenter()
{
    QList<QTableWidgetItem*> selected = m_resultTable->selectedItems();
    if (selected.isEmpty()) {
        // 如果没有选中，添加所有发现的设备
        if (m_resultTable->rowCount() == 0) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("没有可添加的设备。"));
            return;
        }

        int ret = QMessageBox::question(this, QStringLiteral("确认添加"),
                                         QStringLiteral("没有选中任何设备，是否添加所有 %1 个设备到 Device Center?")
                                             .arg(m_resultTable->rowCount()),
                                         QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;

        for (int row = 0; row < m_resultTable->rowCount(); ++row) {
            QString ip = m_resultTable->item(row, 0)->text();
            QString hostname = m_resultTable->item(row, 1)->text();
            QString mac = m_resultTable->item(row, 2)->text();
            QString deviceType = m_resultTable->item(row, 4)->text();

            Logger::instance().info(QStringLiteral("Discovery"),
                                    QStringLiteral("添加设备到 Device Center: %1 (%2) [%3]")
                                        .arg(ip).arg(hostname).arg(deviceType));
        }

        QMessageBox::information(this, QStringLiteral("添加成功"),
                                  QStringLiteral("已将 %1 个设备添加到 Device Center。").arg(m_resultTable->rowCount()));
    } else {
        // 添加选中的设备
        QSet<int> rows;
        for (auto* item : selected) {
            rows.insert(item->row());
        }

        for (int row : rows) {
            QString ip = m_resultTable->item(row, 0)->text();
            QString hostname = m_resultTable->item(row, 1)->text();
            QString mac = m_resultTable->item(row, 2)->text();
            QString deviceType = m_resultTable->item(row, 4)->text();

            Logger::instance().info(QStringLiteral("Discovery"),
                                    QStringLiteral("添加设备到 Device Center: %1 (%2) [%3]")
                                        .arg(ip).arg(hostname).arg(deviceType));
        }

        QMessageBox::information(this, QStringLiteral("添加成功"),
                                  QStringLiteral("已将 %1 个设备添加到 Device Center。").arg(rows.size()));
    }
}

// ============================================================================
// 导出
// ============================================================================
void DiscoveryWidget::onExport()
{
    if (m_resultTable->rowCount() == 0 && m_historyTable->rowCount() == 0) {
        QMessageBox::information(this, QStringLiteral("导出"), QStringLiteral("没有可导出的数据。"));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("导出 CSV"),
                                                     QStringLiteral("discovery_result.csv"),
                                                     QStringLiteral("CSV 文件 (*.csv)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
                              QStringLiteral("无法写入文件: ") + filePath);
        return;
    }

    QTextStream out(&file);
    // BOM for Excel UTF-8
    out << "\xEF\xBB\xBF";

    // 导出结果表格
    if (m_resultTable->rowCount() > 0) {
        out << QStringLiteral("IP,主机名,MAC,厂商,设备类型,OS,状态,发现方式\n");

        for (int row = 0; row < m_resultTable->rowCount(); ++row) {
            QStringList cols;
            for (int col = 0; col < m_resultTable->columnCount(); ++col) {
                auto* item = m_resultTable->item(row, col);
                QString val = item ? item->text() : QStringLiteral("-");
                if (val.contains(QLatin1Char(',')) || val.contains(QLatin1Char('"'))
                    || val.contains(QLatin1Char('\n'))) {
                    val.replace(QLatin1Char('"'), QStringLiteral("\"\""));
                    val = QLatin1Char('"') + val + QLatin1Char('"');
                }
                cols << val;
            }
            out << cols.join(QLatin1Char(',')) << QStringLiteral("\n");
        }

        out << QStringLiteral("\n");
    }

    // 导出历史表格
    if (m_historyTable->rowCount() > 0) {
        out << QStringLiteral("扫描历史\n");
        out << QStringLiteral("时间,目标,方法,发现数,耗时\n");

        for (int row = 0; row < m_historyTable->rowCount(); ++row) {
            QStringList cols;
            for (int col = 0; col < m_historyTable->columnCount(); ++col) {
                auto* item = m_historyTable->item(row, col);
                QString val = item ? item->text() : QStringLiteral("-");
                if (val.contains(QLatin1Char(',')) || val.contains(QLatin1Char('"'))
                    || val.contains(QLatin1Char('\n'))) {
                    val.replace(QLatin1Char('"'), QStringLiteral("\"\""));
                    val = QLatin1Char('"') + val + QLatin1Char('"');
                }
                cols << val;
            }
            out << cols.join(QLatin1Char(',')) << QStringLiteral("\n");
        }
    }

    file.close();
    Logger::instance().info(QStringLiteral("Discovery"),
                            QStringLiteral("结果已导出到: %1").arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出成功"),
                              QStringLiteral("已导出数据到:\n%1").arg(filePath));
}