#include "network/IPScannerWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QTextStream>
#include <QHostInfo>
#include <QRegularExpression>
#include <QTimer>
#include <QProcess>
#include <QMetaObject>
#include <QDesktopServices>
#include <QUrl>

// ============================================================================
// 内置 OUI 厂商表 — 常见 MAC 前缀 → 厂商名 (前三个字节大写，冒号分隔)
// ============================================================================
static const QHash<QString, QString>& ouiTable()
{
    static const QHash<QString, QString> t = {
        // Cisco
        {"00:00:0C","Cisco"},{"00:01:42","Cisco"},{"00:0D:BC","Cisco"},{"00:1A:A1","Cisco"},
        {"00:1B:0C","Cisco"},{"00:1E:49","Cisco"},{"00:23:04","Cisco"},{"00:25:84","Cisco"},
        {"00:26:0B","Cisco"},{"04:DA:D2","Cisco"},{"10:05:01","Cisco"},{"14:1B:BD","Cisco"},
        {"18:8B:9D","Cisco"},{"1C:1D:86","Cisco"},{"24:E9:B3","Cisco"},{"2C:5A:05","Cisco"},
        {"30:7B:AC","Cisco"},{"34:DB:9C","Cisco"},{"38:ED:18","Cisco"},{"40:55:39","Cisco"},
        {"44:2A:FF","Cisco"},{"48:0E:EC","Cisco"},{"4C:00:82","Cisco"},{"50:3D:E5","Cisco"},
        {"54:75:D0","Cisco"},{"58:0A:20","Cisco"},{"5C:5A:EA","Cisco"},{"64:9E:F3","Cisco"},
        {"68:EF:BD","Cisco"},{"6C:41:6A","Cisco"},{"70:DB:98","Cisco"},{"74:26:AC","Cisco"},
        {"78:72:5D","Cisco"},{"7C:AD:74","Cisco"},{"80:E0:1D","Cisco"},{"84:3D:C6","Cisco"},
        {"88:5A:92","Cisco"},{"8C:60:4F","Cisco"},{"90:2E:1C","Cisco"},{"A4:4C:11","Cisco"},
        {"A8:9D:21","Cisco"},{"B0:AA:77","Cisco"},{"B4:14:89","Cisco"},{"B8:62:1F","Cisco"},
        {"C0:62:6B","Cisco"},{"C4:14:3C","Cisco"},{"C8:4C:75","Cisco"},{"CC:16:7E","Cisco"},
        {"D0:7E:28","Cisco"},{"D4:6D:50","Cisco"},{"D8:B1:90","Cisco"},{"E0:2F:6D","Cisco"},
        {"E4:C7:22","Cisco"},{"E8:65:49","Cisco"},{"EC:1D:7F","Cisco"},{"F0:29:29","Cisco"},
        {"F4:4E:05","Cisco"},{"F8:72:EA","Cisco"},{"FC:5B:39","Cisco"},
        // Huawei
        {"00:0D:6E","Huawei"},{"00:18:82","Huawei"},{"00:1E:10","Huawei"},{"00:25:9E","Huawei"},
        {"00:E0:FC","Huawei"},{"08:19:A6","Huawei"},{"0C:37:DC","Huawei"},{"10:4B:46","Huawei"},
        {"14:6E:0A","Huawei"},{"18:2E:5E","Huawei"},{"20:0B:C7","Huawei"},{"24:1E:0B","Huawei"},
        {"28:3A:4D","Huawei"},{"2C:0D:A7","Huawei"},{"30:0B:9B","Huawei"},{"34:0A:7B","Huawei"},
        {"38:6E:6C","Huawei"},{"40:2C:76","Huawei"},{"44:14:DB","Huawei"},{"48:46:C1","Huawei"},
        {"4C:1F:CC","Huawei"},{"50:7B:9D","Huawei"},{"54:2A:9C","Huawei"},{"58:2A:F7","Huawei"},
        {"5C:4C:A9","Huawei"},{"60:DE:44","Huawei"},{"68:9C:5E","Huawei"},{"6C:0B:34","Huawei"},
        {"70:48:7F","Huawei"},{"74:1E:93","Huawei"},{"78:0D:47","Huawei"},{"78:D7:52","Huawei"},
        {"7C:60:97","Huawei"},{"80:6D:97","Huawei"},{"84:AC:16","Huawei"},{"88:36:6C","Huawei"},
        {"8C:BE:BE","Huawei"},{"90:1F:0A","Huawei"},{"94:7D:77","Huawei"},{"98:2D:7B","Huawei"},
        {"9C:28:EF","Huawei"},{"A0:99:9B","Huawei"},{"A4:0E:2B","Huawei"},{"A8:6B:AD","Huawei"},
        {"AC:61:75","Huawei"},{"B0:68:E6","Huawei"},{"B4:0C:25","Huawei"},{"B8:37:4A","Huawei"},
        {"BC:76:70","Huawei"},{"C0:0D:7E","Huawei"},{"C4:01:CE","Huawei"},{"C8:50:E9","Huawei"},
        {"CC:96:A0","Huawei"},{"D0:D0:03","Huawei"},{"D4:4C:9C","Huawei"},{"D8:49:0B","Huawei"},
        {"DC:2C:6E","Huawei"},{"E0:98:61","Huawei"},{"E4:48:C7","Huawei"},{"E8:0B:13","Huawei"},
        {"EC:38:73","Huawei"},{"F0:64:91","Huawei"},{"F4:63:49","Huawei"},{"F8:0D:60","Huawei"},
        {"FC:48:EF","Huawei"},
        // H3C
        {"00:0F:E2","H3C"},{"00:1B:2F","H3C"},{"00:23:89","H3C"},{"04:27:FE","H3C"},
        {"08:0D:67","H3C"},{"0C:DA:41","H3C"},{"14:30:C9","H3C"},{"18:FD:74","H3C"},
        {"20:1A:06","H3C"},{"28:6E:D4","H3C"},{"30:63:6B","H3C"},{"34:29:12","H3C"},
        {"38:22:D6","H3C"},{"3C:8C:40","H3C"},{"40:7A:CB","H3C"},{"48:7A:DA","H3C"},
        {"4C:AC:0A","H3C"},{"50:61:84","H3C"},{"54:CD:EE","H3C"},{"58:6A:B1","H3C"},
        {"5C:DD:70","H3C"},{"60:31:3C","H3C"},{"64:9D:99","H3C"},{"68:2C:7B","H3C"},
        {"6C:92:BF","H3C"},{"70:7B:E8","H3C"},{"70:F9:6D","H3C"},{"74:25:8A","H3C"},
        {"78:1D:BA","H3C"},{"7C:1D:D9","H3C"},{"80:48:59","H3C"},{"84:D9:31","H3C"},
        {"88:14:4B","H3C"},{"8C:4C:DC","H3C"},{"90:5C:44","H3C"},{"94:28:2E","H3C"},
        {"98:0E:E4","H3C"},{"9C:21:41","H3C"},{"A0:48:1C","H3C"},{"A4:5B:FE","H3C"},
        {"A8:13:74","H3C"},{"AC:4A:FE","H3C"},{"B0:47:BF","H3C"},{"B8:AF:67","H3C"},
        {"BC:3A:2A","H3C"},{"C0:16:BD","H3C"},{"C4:74:40","H3C"},{"CC:06:77","H3C"},
        {"D0:67:26","H3C"},{"D4:81:CA","H3C"},{"D8:15:0D","H3C"},{"DC:53:7C","H3C"},
        {"E0:9D:31","H3C"},{"E4:61:1E","H3C"},{"E8:6A:64","H3C"},{"EC:BD:1D","H3C"},
        {"F0:84:2F","H3C"},{"F4:21:CA","H3C"},{"F8:32:E4","H3C"},{"FC:3F:DB","H3C"},
        // Apple
        {"00:03:93","Apple"},{"00:14:51","Apple"},{"00:17:F2","Apple"},{"00:1E:C2","Apple"},
        {"00:1F:F3","Apple"},{"00:21:6A","Apple"},{"00:22:41","Apple"},{"00:23:12","Apple"},
        {"00:23:DF","Apple"},{"00:24:36","Apple"},{"00:25:00","Apple"},{"00:25:BC","Apple"},
        {"00:26:08","Apple"},{"00:26:BB","Apple"},{"00:30:65","Apple"},{"04:0C:CE","Apple"},
        {"04:15:52","Apple"},{"04:1E:64","Apple"},{"04:26:65","Apple"},{"04:4B:ED","Apple"},
        {"04:54:53","Apple"},{"04:69:F8","Apple"},{"04:DB:56","Apple"},{"04:E5:36","Apple"},
        {"04:F1:3E","Apple"},{"08:66:98","Apple"},{"08:6D:41","Apple"},{"08:74:02","Apple"},
        {"0C:15:39","Apple"},{"0C:19:F8","Apple"},{"0C:3E:9F","Apple"},{"0C:4D:E9","Apple"},
        {"0C:51:01","Apple"},{"0C:77:1A","Apple"},{"0C:BC:9F","Apple"},{"10:1C:0C","Apple"},
        {"10:29:59","Apple"},{"10:40:F3","Apple"},{"10:41:7F","Apple"},{"10:93:E9","Apple"},
        {"10:9A:DD","Apple"},{"10:CE:E9","Apple"},{"14:10:9F","Apple"},{"14:20:5E","Apple"},
        {"14:5A:05","Apple"},{"14:5A:FC","Apple"},{"14:7D:DA","Apple"},{"14:88:E6","Apple"},
        {"14:8F:C6","Apple"},{"14:98:77","Apple"},{"14:99:E2","Apple"},{"14:BD:61","Apple"},
        {"18:20:32","Apple"},{"18:34:51","Apple"},{"18:3E:EF","Apple"},{"18:65:90","Apple"},
        {"18:81:0E","Apple"},{"18:9E:FC","Apple"},{"18:AF:61","Apple"},{"18:AF:8F","Apple"},
        {"18:E7:F4","Apple"},{"18:EE:69","Apple"},{"18:F1:D8","Apple"},{"18:F6:43","Apple"},
        {"1C:1A:C0","Apple"},{"1C:36:BB","Apple"},{"1C:91:48","Apple"},{"1C:AB:A7","Apple"},
        {"1C:B3:C9","Apple"},{"1C:E6:2B","Apple"},{"20:3C:AE","Apple"},{"20:78:F0","Apple"},
        {"20:9B:CD","Apple"},{"20:9C:B4","Apple"},{"20:A2:E4","Apple"},{"20:AB:37","Apple"},
        {"20:C9:D0","Apple"},{"20:EE:28","Apple"},{"24:1E:EB","Apple"},{"24:24:0E","Apple"},
        {"24:4E:7B","Apple"},{"24:5B:A7","Apple"},{"24:A0:74","Apple"},{"24:A2:E1","Apple"},
        {"24:AB:81","Apple"},{"24:E3:14","Apple"},{"24:F0:94","Apple"},{"28:37:37","Apple"},
        {"28:5A:EB","Apple"},{"28:6A:B8","Apple"},{"28:6A:BA","Apple"},{"28:6A:BF","Apple"},
        {"28:CF:DA","Apple"},{"28:CF:E9","Apple"},{"28:E0:2C","Apple"},{"28:E7:1F","Apple"},
        {"28:EC:95","Apple"},{"28:ED:E0","Apple"},{"28:F0:76","Apple"},{"28:FF:3C","Apple"},
        {"2C:1F:23","Apple"},{"2C:20:0B","Apple"},{"2C:61:F6","Apple"},{"2C:BE:08","Apple"},
        {"2C:F0:A2","Apple"},{"2C:F0:EE","Apple"},{"30:35:AD","Apple"},{"30:57:14","Apple"},
        {"30:90:AB","Apple"},{"30:9C:23","Apple"},{"30:F7:C5","Apple"},{"34:12:98","Apple"},
        {"34:36:3B","Apple"},{"34:42:62","Apple"},{"34:51:C9","Apple"},{"34:7C:25","Apple"},
        {"34:8C:AE","Apple"},{"34:A8:EB","Apple"},{"34:AB:37","Apple"},{"34:C0:59","Apple"},
        {"34:E2:FD","Apple"},{"38:0F:4A","Apple"},{"38:48:4C","Apple"},{"38:53:9C","Apple"},
        {"38:71:DE","Apple"},{"38:89:2C","Apple"},{"38:B5:4D","Apple"},{"38:C9:86","Apple"},
        {"38:CA:DA","Apple"},{"38:EC:0D","Apple"},{"38:F9:D3","Apple"},{"3C:07:54","Apple"},
        {"3C:15:C2","Apple"},{"3C:2E:FF","Apple"},{"3C:AB:8E","Apple"},{"3C:D0:F8","Apple"},
        {"3C:E0:72","Apple"},{"40:30:04","Apple"},{"40:33:1A","Apple"},{"40:4D:7F","Apple"},
        {"40:56:9C","Apple"},{"40:6C:8F","Apple"},{"40:70:09","Apple"},{"40:83:DE","Apple"},
        {"40:98:AD","Apple"},{"40:9C:28","Apple"},{"40:A6:D9","Apple"},{"40:B3:95","Apple"},
        {"40:BC:60","Apple"},{"40:C6:99","Apple"},{"40:D3:2D","Apple"},{"44:00:10","Apple"},
        {"44:2A:60","Apple"},{"44:4C:0C","Apple"},{"44:4C:AB","Apple"},{"44:D8:84","Apple"},
        {"44:FB:B0","Apple"},{"48:3B:38","Apple"},{"48:43:7C","Apple"},{"48:4B:AA","Apple"},
        {"48:65:EE","Apple"},{"48:74:6E","Apple"},{"48:A9:1C","Apple"},{"48:BF:6B","Apple"},
        {"48:D7:05","Apple"},{"4C:20:B8","Apple"},{"4C:32:75","Apple"},{"4C:56:9D","Apple"},
        {"4C:57:CA","Apple"},{"4C:74:BF","Apple"},{"4C:79:6F","Apple"},{"4C:7C:5F","Apple"},
        {"4C:AB:33","Apple"},{"4C:B1:99","Apple"},{"4C:BC:98","Apple"},{"4C:F0:2E","Apple"},
        {"50:32:37","Apple"},{"50:7A:55","Apple"},{"50:7A:C5","Apple"},{"50:82:95","Apple"},
        {"50:DE:06","Apple"},{"50:E4:0E","Apple"},{"50:EA:D6","Apple"},{"50:ED:3C","Apple"},
        {"54:26:96","Apple"},{"54:33:CB","Apple"},{"54:4E:90","Apple"},{"54:72:4F","Apple"},
        {"54:9A:11","Apple"},{"54:AE:27","Apple"},{"54:E4:3A","Apple"},{"54:E6:1B","Apple"},
        {"58:1F:AA","Apple"},{"58:40:4E","Apple"},{"58:55:CA","Apple"},{"58:7F:57","Apple"},
        {"58:B0:35","Apple"},{"58:E2:8F","Apple"},{"5C:09:47","Apple"},{"5C:1B:F4","Apple"},
        {"5C:50:15","Apple"},{"5C:52:30","Apple"},{"5C:70:A3","Apple"},{"5C:87:30","Apple"},
        {"5C:95:5D","Apple"},{"5C:95:AE","Apple"},{"5C:97:F3","Apple"},{"5C:AD:CF","Apple"},
        {"5C:F5:DA","Apple"},{"5C:F7:E6","Apple"},{"5C:F9:38","Apple"},{"60:03:08","Apple"},
        {"60:33:4B","Apple"},{"60:42:1C","Apple"},{"60:69:44","Apple"},{"60:70:C0","Apple"},
        {"60:7C:2F","Apple"},{"60:7E:C9","Apple"},{"60:8B:0E","Apple"},{"60:92:17","Apple"},
        {"60:9A:C1","Apple"},{"60:A3:7D","Apple"},{"60:C5:47","Apple"},{"60:CD:A9","Apple"},
        {"60:D9:C7","Apple"},{"60:DA:83","Apple"},{"60:F4:45","Apple"},{"60:FB:42","Apple"},
        {"60:FE:C5","Apple"},{"64:20:0C","Apple"},{"64:52:43","Apple"},{"64:70:33","Apple"},
        {"64:76:BA","Apple"},{"64:9A:BE","Apple"},{"64:A3:CB","Apple"},{"64:A5:C3","Apple"},
        {"64:B0:A6","Apple"},{"64:C7:53","Apple"},{"64:D2:10","Apple"},{"64:E6:82","Apple"},
        {"68:09:27","Apple"},{"68:5B:35","Apple"},{"68:64:4B","Apple"},{"68:96:7B","Apple"},
        {"68:A8:6D","Apple"},{"68:AE:20","Apple"},{"68:CA:34","Apple"},{"68:CA:C4","Apple"},
        {"68:FB:7E","Apple"},{"68:FE:F7","Apple"},{"6C:19:2F","Apple"},{"6C:19:C0","Apple"},
        {"6C:2E:33","Apple"},{"6C:40:08","Apple"},{"6C:4A:85","Apple"},{"6C:70:9F","Apple"},
        {"6C:72:20","Apple"},{"6C:72:E7","Apple"},{"6C:94:66","Apple"},{"6C:94:F8","Apple"},
        {"6C:AB:31","Apple"},{"6C:C2:6B","Apple"},{"6C:E8:5C","Apple"},{"70:11:24","Apple"},
        {"70:14:A6","Apple"},{"70:3C:69","Apple"},{"70:3E:AC","Apple"},{"70:48:0F","Apple"},
        {"70:56:81","Apple"},{"70:70:0D","Apple"},{"70:73:CB","Apple"},{"70:81:05","Apple"},
        {"70:A2:83","Apple"},{"70:D4:9E","Apple"},{"70:DE:E2","Apple"},{"70:E7:2C","Apple"},
        {"70:EC:E4","Apple"},{"74:1B:B2","Apple"},{"74:42:8B","Apple"},{"74:81:4A","Apple"},
        {"74:8D:08","Apple"},{"74:8F:3C","Apple"},{"78:28:CA","Apple"},{"78:31:C1","Apple"},
        {"78:36:BC","Apple"},{"78:3A:84","Apple"},{"78:41:5D","Apple"},{"78:4F:43","Apple"},
        {"78:63:9C","Apple"},{"78:64:C0","Apple"},{"78:67:D7","Apple"},{"78:6C:1C","Apple"},
        {"78:7B:8A","Apple"},{"78:7E:61","Apple"},{"78:88:6D","Apple"},{"78:8F:CF","Apple"},
        {"78:9F:70","Apple"},{"78:A3:E4","Apple"},{"78:A8:73","Apple"},{"78:CA:39","Apple"},
        {"78:D7:5F","Apple"},{"78:FD:94","Apple"},{"7C:01:91","Apple"},{"7C:04:D0","Apple"},
        {"7C:11:BE","Apple"},{"7C:1E:52","Apple"},{"7C:2E:0D","Apple"},{"7C:50:49","Apple"},
        {"7C:6D:62","Apple"},{"7C:6D:F8","Apple"},{"7C:9A:1D","Apple"},{"7C:A1:AE","Apple"},
        {"7C:B0:C2","Apple"},{"7C:C3:A1","Apple"},{"7C:C5:37","Apple"},{"7C:D1:C3","Apple"},
        {"7C:E4:AA","Apple"},{"7C:F0:5F","Apple"},{"80:00:6E","Apple"},{"80:19:34","Apple"},
        {"80:2A:A8","Apple"},{"80:2C:73","Apple"},{"80:49:71","Apple"},{"80:82:23","Apple"},
        {"80:8E:71","Apple"},{"80:92:9F","Apple"},{"80:9B:20","Apple"},{"80:A0:BF","Apple"},
        {"80:AD:67","Apple"},{"80:B0:3D","Apple"},{"80:BE:05","Apple"},{"80:D6:05","Apple"},
        {"80:E6:50","Apple"},{"80:EA:07","Apple"},{"80:ED:2C","Apple"},{"80:F0:3B","Apple"},
        {"80:F6:2E","Apple"},{"84:29:99","Apple"},{"84:38:35","Apple"},{"84:3A:4B","Apple"},
        {"84:41:67","Apple"},{"84:57:80","Apple"},{"84:6A:DF","Apple"},{"84:78:AC","Apple"},
        {"84:85:06","Apple"},{"84:89:AD","Apple"},{"84:8E:0C","Apple"},{"84:8E:DF","Apple"},
        {"84:A1:34","Apple"},{"84:AB:1A","Apple"},{"84:B1:53","Apple"},{"84:FC:AC","Apple"},
        {"84:FE:9E","Apple"},{"88:19:08","Apple"},{"88:1F:A1","Apple"},{"88:53:2E","Apple"},
        {"88:53:95","Apple"},{"88:63:DF","Apple"},{"88:64:40","Apple"},{"88:66:A5","Apple"},
        {"88:6B:6E","Apple"},{"88:6B:F0","Apple"},{"88:75:98","Apple"},{"88:7F:03","Apple"},
        {"88:A4:79","Apple"},{"88:A9:B7","Apple"},{"88:A9:E0","Apple"},{"88:AE:07","Apple"},
        {"88:B1:11","Apple"},{"88:B2:91","Apple"},{"88:C6:63","Apple"},{"88:CB:87","Apple"},
        {"88:E8:7F","Apple"},{"88:E9:FE","Apple"},{"8C:00:6D","Apple"},{"8C:2D:AA","Apple"},
        {"8C:58:77","Apple"},{"8C:7B:9D","Apple"},{"8C:7C:92","Apple"},{"8C:85:90","Apple"},
        {"8C:86:1E","Apple"},{"8C:8E:F2","Apple"},{"8C:8F:8A","Apple"},{"8C:99:E6","Apple"},
        {"8C:9E:FF","Apple"},{"8C:A9:82","Apple"},{"8C:B8:2C","Apple"},{"8C:FA:BA","Apple"},
        {"90:08:7B","Apple"},{"90:0C:B2","Apple"},{"90:27:E4","Apple"},{"90:3C:92","Apple"},
        {"90:60:14","Apple"},{"90:72:40","Apple"},{"90:81:2A","Apple"},{"90:84:0D","Apple"},
        {"90:8D:6C","Apple"},{"90:9C:4A","Apple"},{"90:A2:5B","Apple"},{"90:B0:ED","Apple"},
        {"90:B2:1F","Apple"},{"90:B9:31","Apple"},{"90:C1:C6","Apple"},{"90:CC:DF","Apple"},
        {"90:DD:5D","Apple"},{"90:E0:F0","Apple"},{"90:EB:50","Apple"},{"90:F0:52","Apple"},
        {"90:FD:61","Apple"},{"94:16:25","Apple"},{"94:4B:D1","Apple"},{"94:5F:72","Apple"},
        {"94:76:0E","Apple"},{"94:94:26","Apple"},{"94:9B:FE","Apple"},{"94:BF:2D","Apple"},
        {"94:BF:95","Apple"},{"94:D0:23","Apple"},{"94:DD:3F","Apple"},{"94:E9:6A","Apple"},
        {"94:EA:32","Apple"},{"94:F6:3E","Apple"},{"94:FB:29","Apple"},{"98:00:3D","Apple"},
        {"98:01:A7","Apple"},{"98:03:D8","Apple"},{"98:0C:82","Apple"},{"98:10:E8","Apple"},
        {"98:2B:8D","Apple"},{"98:2D:68","Apple"},{"98:3E:BF","Apple"},{"98:5A:EB","Apple"},
        {"98:60:16","Apple"},{"98:7E:46","Apple"},{"98:8F:2B","Apple"},{"98:9E:63","Apple"},
        {"98:9F:0C","Apple"},{"98:B8:E3","Apple"},{"98:CA:33","Apple"},{"98:D6:BB","Apple"},
        {"98:D8:8C","Apple"},{"98:E0:D9","Apple"},{"98:F0:AB","Apple"},{"98:F5:A9","Apple"},
        {"98:FE:94","Apple"},{"9C:04:EB","Apple"},{"9C:20:7B","Apple"},{"9C:29:3F","Apple"},
        {"9C:2D:CD","Apple"},{"9C:35:EB","Apple"},{"9C:3E:53","Apple"},{"9C:4F:DA","Apple"},
        {"9C:53:22","Apple"},{"9C:58:3C","Apple"},{"9C:64:8B","Apple"},{"9C:6C:15","Apple"},
        {"9C:84:BF","Apple"},{"9C:F3:87","Apple"},{"9C:F4:8E","Apple"},{"9C:F4:AB","Apple"},
        {"9C:FC:01","Apple"},{"9C:FC:28","Apple"},{"A0:18:28","Apple"},{"A0:4E:A7","Apple"},
        {"A0:56:F3","Apple"},{"A0:78:17","Apple"},{"A0:9B:85","Apple"},{"A0:A3:09","Apple"},
        {"A0:D7:95","Apple"},{"A0:ED:CD","Apple"},{"A0:F3:C1","Apple"},{"A0:F9:E0","Apple"},
        {"A0:FC:6E","Apple"},{"A4:0B:ED","Apple"},{"A4:12:42","Apple"},{"A4:31:35","Apple"},
        {"A4:5E:60","Apple"},{"A4:67:06","Apple"},{"A4:83:E7","Apple"},{"A4:9A:58","Apple"},
        {"A4:B1:97","Apple"},{"A4:B8:05","Apple"},{"A4:C3:61","Apple"},{"A4:D1:8C","Apple"},
        {"A4:D1:D2","Apple"},{"A4:D3:DB","Apple"},{"A4:E9:75","Apple"},{"A4:F1:E8","Apple"},
        {"A8:20:66","Apple"},{"A8:5B:78","Apple"},{"A8:60:B6","Apple"},{"A8:86:DD","Apple"},
        {"A8:8E:24","Apple"},{"A8:96:8A","Apple"},{"A8:9A:93","Apple"},{"A8:9B:10","Apple"},
        {"A8:9E:24","Apple"},{"A8:BB:CF","Apple"},{"A8:BE:27","Apple"},{"A8:C6:9A","Apple"},
        {"A8:F0:5C","Apple"},{"A8:F4:7A","Apple"},{"AC:15:F4","Apple"},{"AC:1D:06","Apple"},
        {"AC:1F:74","Apple"},{"AC:29:3A","Apple"},{"AC:3C:0B","Apple"},{"AC:3C:8B","Apple"},
        {"AC:44:F2","Apple"},{"AC:49:DB","Apple"},{"AC:61:EA","Apple"},{"AC:64:62","Apple"},
        {"AC:67:5D","Apple"},{"AC:7F:3E","Apple"},{"AC:87:A3","Apple"},{"AC:88:FD","Apple"},
        {"AC:8B:8E","Apple"},{"AC:90:85","Apple"},{"AC:BC:32","Apple"},{"AC:C3:5A","Apple"},
        {"AC:CF:5C","Apple"},{"AC:D1:B8","Apple"},{"AC:E4:B5","Apple"},{"AC:FD:EC","Apple"},
        {"B0:19:C6","Apple"},{"B0:34:95","Apple"},{"B0:35:9F","Apple"},{"B0:48:1A","Apple"},
        {"B0:5C:E5","Apple"},{"B0:65:BD","Apple"},{"B0:70:2D","Apple"},{"B0:7B:8C","Apple"},
        {"B0:8C:6F","Apple"},{"B0:8C:75","Apple"},{"B0:91:22","Apple"},{"B0:9F:BA","Apple"},
        {"B0:A6:F5","Apple"},{"B0:AC:FA","Apple"},{"B0:B2:DC","Apple"},{"B0:B9:8F","Apple"},
        {"B0:BE:83","Apple"},{"B0:C6:9A","Apple"},{"B0:CA:68","Apple"},{"B0:D8:2E","Apple"},
        {"B0:F1:BC","Apple"},{"B4:18:D1","Apple"},{"B4:1C:30","Apple"},{"B4:40:A4","Apple"},
        {"B4:4B:D2","Apple"},{"B4:53:86","Apple"},{"B4:5A:4A","Apple"},{"B4:73:0C","Apple"},
        {"B4:8B:19","Apple"},{"B4:9C:DF","Apple"},{"B4:A9:FC","Apple"},{"B4:B0:17","Apple"},
        {"B4:B5:2F","Apple"},{"B4:C8:10","Apple"},{"B4:E1:EB","Apple"},{"B4:F0:AB","Apple"},
        {"B4:F6:1C","Apple"},{"B8:09:8A","Apple"},{"B8:17:C2","Apple"},{"B8:1B:A5","Apple"},
        {"B8:41:A4","Apple"},{"B8:44:D9","Apple"},{"B8:53:AC","Apple"},{"B8:5D:0A","Apple"},
        {"B8:63:4D","Apple"},{"B8:6B:23","Apple"},{"B8:78:79","Apple"},{"B8:7C:6F","Apple"},
        {"B8:81:FA","Apple"},{"B8:8C:29","Apple"},{"B8:90:47","Apple"},{"B8:9B:C9","Apple"},
        {"B8:9F:04","Apple"},{"B8:A6:0E","Apple"},{"B8:C1:11","Apple"},{"B8:C7:5D","Apple"},
        {"B8:E8:56","Apple"},{"B8:F1:2A","Apple"},{"B8:F6:B1","Apple"},{"B8:FF:7A","Apple"},
        {"BC:09:1B","Apple"},{"BC:0F:9B","Apple"},{"BC:15:AC","Apple"},{"BC:20:BA","Apple"},
        {"BC:2E:48","Apple"},{"BC:3B:AF","Apple"},{"BC:3C:69","Apple"},{"BC:4C:C4","Apple"},
        {"BC:52:B7","Apple"},{"BC:54:36","Apple"},{"BC:54:51","Apple"},{"BC:5B:6F","Apple"},
        {"BC:67:1C","Apple"},{"BC:67:78","Apple"},{"BC:6C:21","Apple"},{"BC:83:85","Apple"},
        {"BC:92:6B","Apple"},{"BC:9F:EF","Apple"},{"BC:A5:8B","Apple"},{"BC:A8:A6","Apple"},
        {"BC:A9:20","Apple"},{"BC:AF:91","Apple"},{"BC:B8:63","Apple"},{"BC:BA:C2","Apple"},
        {"BC:BF:58","Apple"},{"BC:C3:44","Apple"},{"BC:D0:74","Apple"},{"BC:E1:43","Apple"},
        {"BC:E8:8F","Apple"},{"BC:EC:5D","Apple"},{"BC:F4:3E","Apple"},{"BC:F5:AC","Apple"},
        {"BC:F6:10","Apple"},{"BC:FC:E7","Apple"},{"C0:1B:24","Apple"},{"C0:42:D0","Apple"},
        {"C0:63:94","Apple"},{"C0:65:99","Apple"},{"C0:84:7A","Apple"},{"C0:8C:71","Apple"},
        {"C0:91:0B","Apple"},{"C0:98:79","Apple"},{"C0:9F:05","Apple"},{"C0:A0:BB","Apple"},
        {"C0:A5:3E","Apple"},{"C0:A6:00","Apple"},{"C0:B6:58","Apple"},{"C0:BD:D1","Apple"},
        {"C0:C9:76","Apple"},{"C0:CC:F8","Apple"},{"C0:CE:CD","Apple"},{"C0:D0:12","Apple"},
        {"C0:D6:9A","Apple"},{"C0:E2:BE","Apple"},{"C0:E8:62","Apple"},{"C0:F2:FB","Apple"},
        {"C0:F9:46","Apple"},{"C4:0B:31","Apple"},{"C4:2C:03","Apple"},{"C4:6B:5F","Apple"},
        {"C4:84:66","Apple"},{"C4:85:08","Apple"},{"C4:91:0C","Apple"},{"C4:98:80","Apple"},
        {"C4:9A:02","Apple"},{"C4:B3:01","Apple"},{"C4:E9:2F","Apple"},{"C8:09:A8","Apple"},
        {"C8:1E:E7","Apple"},{"C8:33:4B","Apple"},{"C8:3C:85","Apple"},{"C8:5C:C2","Apple"},
        {"C8:5E:77","Apple"},{"C8:69:CD","Apple"},{"C8:6F:1D","Apple"},{"C8:85:50","Apple"},
        {"C8:BC:C8","Apple"},{"C8:BC:DF","Apple"},{"C8:D0:83","Apple"},{"C8:E0:EB","Apple"},
        {"C8:F0:8E","Apple"},{"C8:F6:5C","Apple"},{"CC:08:8D","Apple"},{"CC:08:FA","Apple"},
        {"CC:20:AF","Apple"},{"CC:25:EF","Apple"},{"CC:29:F5","Apple"},{"CC:2D:B7","Apple"},
        {"CC:44:63","Apple"},{"CC:6A:4C","Apple"},{"CC:78:5F","Apple"},{"CC:97:91","Apple"},
        {"CC:99:8E","Apple"},{"CC:9F:7A","Apple"},{"CC:A7:C1","Apple"},{"CC:C4:60","Apple"},
        {"CC:CC:81","Apple"},{"CC:D0:83","Apple"},{"CC:E2:6B","Apple"},{"CC:EC:DF","Apple"},
        {"CC:F0:3A","Apple"},{"CC:FC:6D","Apple"},{"D0:03:4B","Apple"},{"D0:15:4A","Apple"},
        {"D0:17:69","Apple"},{"D0:1B:49","Apple"},{"D0:23:DB","Apple"},{"D0:25:98","Apple"},
        {"D0:2F:9E","Apple"},{"D0:33:11","Apple"},{"D0:4D:2C","Apple"},{"D0:5A:0F","Apple"},
        {"D0:81:7A","Apple"},{"D0:88:0C","Apple"},{"D0:8E:79","Apple"},{"D0:9E:CD","Apple"},
        {"D0:A4:0A","Apple"},{"D0:B3:3F","Apple"},{"D0:C5:F3","Apple"},{"D0:D2:B0","Apple"},
        {"D0:D3:FC","Apple"},{"D0:E1:40","Apple"},{"D4:1A:3F","Apple"},{"D4:46:37","Apple"},
        {"D4:54:17","Apple"},{"D4:61:2E","Apple"},{"D4:61:9D","Apple"},{"D4:61:DA","Apple"},
        {"D4:6A:6A","Apple"},{"D4:6B:A6","Apple"},{"D4:7B:75","Apple"},{"D4:8D:9D","Apple"},
        {"D4:90:9C","Apple"},{"D4:94:5A","Apple"},{"D4:96:DF","Apple"},{"D4:9A:20","Apple"},
        {"D4:9B:5C","Apple"},{"D4:9D:E3","Apple"},{"D4:A3:3D","Apple"},{"D4:DC:CD","Apple"},
        {"D4:E8:AA","Apple"},{"D4:F4:6F","Apple"},{"D4:F7:35","Apple"},{"D8:00:4D","Apple"},
        {"D8:1C:79","Apple"},{"D8:1D:72","Apple"},{"D8:30:62","Apple"},{"D8:3C:69","Apple"},
        {"D8:4D:10","Apple"},{"D8:4D:7C","Apple"},{"D8:5D:4C","Apple"},{"D8:5D:84","Apple"},
        {"D8:6C:02","Apple"},{"D8:8F:CA","Apple"},{"D8:96:95","Apple"},{"D8:9E:3F","Apple"},
        {"D8:A1:4E","Apple"},{"D8:A2:5E","Apple"},{"D8:BB:2C","Apple"},{"D8:C3:0E","Apple"},
        {"D8:CF:9C","Apple"},{"D8:D1:CC","Apple"},{"D8:DC:40","Apple"},{"D8:DE:3A","Apple"},
        {"D8:E0:E1","Apple"},{"D8:E2:DF","Apple"},{"D8:E5:6D","Apple"},{"D8:F0:5C","Apple"},
        {"D8:F1:5B","Apple"},{"D8:F7:10","Apple"},{"DC:08:56","Apple"},{"DC:0B:1A","Apple"},
        {"DC:0B:34","Apple"},{"DC:0C:5C","Apple"},{"DC:0C:8C","Apple"},{"DC:1A:01","Apple"},
        {"DC:1D:30","Apple"},{"DC:2B:2A","Apple"},{"DC:2B:61","Apple"},{"DC:2B:CA","Apple"},
        {"DC:37:14","Apple"},{"DC:41:5F","Apple"},{"DC:41:A9","Apple"},{"DC:52:85","Apple"},
        {"DC:53:60","Apple"},{"DC:56:E7","Apple"},{"DC:86:D8","Apple"},{"DC:9C:52","Apple"},
        {"DC:A4:CA","Apple"},{"DC:A9:04","Apple"},{"DC:AC:19","Apple"},{"DC:B3:0A","Apple"},
        {"DC:B3:3A","Apple"},{"DC:BE:6A","Apple"},{"DC:C6:98","Apple"},{"DC:CD:2C","Apple"},
        {"DC:D3:A2","Apple"},{"DC:D8:4D","Apple"},{"DC:E5:7B","Apple"},{"DC:E5:7F","Apple"},
        {"DC:EC:06","Apple"},{"DC:F4:CA","Apple"},{"DC:FC:86","Apple"},{"E0:06:E6","Apple"},
        {"E0:19:1D","Apple"},{"E0:2B:96","Apple"},{"E0:33:8E","Apple"},{"E0:3C:5B","Apple"},
        {"E0:50:8B","Apple"},{"E0:5F:45","Apple"},{"E0:66:78","Apple"},{"E0:6D:17","Apple"},
        {"E0:73:E7","Apple"},{"E0:7F:53","Apple"},{"E0:89:7E","Apple"},{"E0:8E:3C","Apple"},
        {"E0:98:06","Apple"},{"E0:9A:BF","Apple"},{"E0:9C:8A","Apple"},{"E0:9D:13","Apple"},
        {"E0:9F:5A","Apple"},{"E0:A1:98","Apple"},{"E0:A6:5D","Apple"},{"E0:AC:CB","Apple"},
        {"E0:B5:2D","Apple"},{"E0:B9:BA","Apple"},{"E0:BB:0E","Apple"},{"E0:C3:53","Apple"},
        {"E0:C7:67","Apple"},{"E0:C9:7A","Apple"},{"E0:D0:25","Apple"},{"E0:D8:1A","Apple"},
        {"E0:E0:50","Apple"},{"E0:EB:40","Apple"},{"E0:ED:05","Apple"},{"E0:F5:C6","Apple"},
        {"E0:F8:47","Apple"},{"E0:FD:68","Apple"},{"E4:25:E7","Apple"},{"E4:2B:34","Apple"},
        {"E4:2B:79","Apple"},{"E4:36:7B","Apple"},{"E4:46:BD","Apple"},{"E4:50:EB","Apple"},
        {"E4:6B:33","Apple"},{"E4:76:84","Apple"},{"E4:7E:9A","Apple"},{"E4:80:62","Apple"},
        {"E4:8B:7F","Apple"},{"E4:8D:8C","Apple"},{"E4:90:7E","Apple"},{"E4:98:D6","Apple"},
        {"E4:9A:79","Apple"},{"E4:9A:DC","Apple"},{"E4:9C:67","Apple"},{"E4:9E:12","Apple"},
        {"E4:A7:C5","Apple"},{"E4:AC:45","Apple"},{"E4:B0:21","Apple"},{"E4:B2:FB","Apple"},
        {"E4:CE:8F","Apple"},{"E4:D3:F1","Apple"},{"E4:E0:4C","Apple"},{"E4:E4:AB","Apple"},
        {"E4:F7:A1","Apple"},{"E8:04:0B","Apple"},{"E8:04:62","Apple"},{"E8:06:88","Apple"},
        {"E8:0E:3B","Apple"},{"E8:1C:4B","Apple"},{"E8:20:E2","Apple"},{"E8:2A:44","Apple"},
        {"E8:36:17","Apple"},{"E8:5A:8B","Apple"},{"E8:6B:EA","Apple"},{"E8:73:5A","Apple"},
        {"E8:78:29","Apple"},{"E8:80:2E","Apple"},{"E8:87:A3","Apple"},{"E8:8D:28","Apple"},
        {"E8:99:C4","Apple"},{"E8:9C:25","Apple"},{"E8:A7:30","Apple"},{"E8:AA:CF","Apple"},
        {"E8:B2:AC","Apple"},{"E8:B4:7E","Apple"},{"E8:B6:58","Apple"},{"E8:BE:81","Apple"},
        {"E8:C7:4A","Apple"},{"E8:D0:FA","Apple"},{"E8:DB:84","Apple"},{"E8:E0:66","Apple"},
        {"E8:F1:B0","Apple"},{"E8:F2:E2","Apple"},{"E8:F4:26","Apple"},{"E8:F9:28","Apple"},
        {"E8:FA:2A","Apple"},{"EC:1A:03","Apple"},{"EC:1F:72","Apple"},{"EC:2C:E2","Apple"},
        {"EC:35:86","Apple"},{"EC:52:82","Apple"},{"EC:54:4E","Apple"},{"EC:55:6F","Apple"},
        {"EC:5C:68","Apple"},{"EC:60:73","Apple"},{"EC:73:79","Apple"},{"EC:7C:B6","Apple"},
        {"EC:85:2F","Apple"},{"EC:8A:4C","Apple"},{"EC:9A:0C","Apple"},{"EC:A0:7C","Apple"},
        {"EC:A8:6B","Apple"},{"EC:A9:04","Apple"},{"EC:AD:B8","Apple"},{"EC:B1:D7","Apple"},
        {"EC:B5:87","Apple"},{"EC:BE:DC","Apple"},{"EC:D0:DC","Apple"},{"EC:DE:3D","Apple"},
        {"EC:E9:78","Apple"},{"EC:EA:03","Apple"},{"EC:F0:0E","Apple"},{"F0:18:98","Apple"},
        {"F0:24:75","Apple"},{"F0:2F:4B","Apple"},{"F0:2F:74","Apple"},{"F0:3F:95","Apple"},
        {"F0:43:47","Apple"},{"F0:5C:D5","Apple"},{"F0:63:F9","Apple"},{"F0:6B:CA","Apple"},
        {"F0:72:8C","Apple"},{"F0:79:59","Apple"},{"F0:7B:CB","Apple"},{"F0:81:AF","Apple"},
        {"F0:83:2C","Apple"},{"F0:98:9D","Apple"},{"F0:99:BF","Apple"},{"F0:9C:BB","Apple"},
        {"F0:9F:C2","Apple"},{"F0:A0:83","Apple"},{"F0:A6:0A","Apple"},{"F0:B0:E7","Apple"},
        {"F0:B4:79","Apple"},{"F0:BB:5C","Apple"},{"F0:BE:3C","Apple"},{"F0:C1:F1","Apple"},
        {"F0:C3:71","Apple"},{"F0:CB:A1","Apple"},{"F0:CD:5C","Apple"},{"F0:D1:A9","Apple"},
        {"F0:D5:BF","Apple"},{"F0:DB:E2","Apple"},{"F0:DB:F8","Apple"},{"F0:DC:E2","Apple"},
        {"F0:DE:71","Apple"},{"F0:DE:F1","Apple"},{"F0:E9:DC","Apple"},{"F0:EE:10","Apple"},
        {"F0:F6:1C","Apple"},{"F0:F6:69","Apple"},{"F0:F7:55","Apple"},{"F4:06:16","Apple"},
        {"F4:0F:24","Apple"},{"F4:0F:9B","Apple"},{"F4:1B:A1","Apple"},{"F4:31:C3","Apple"},
        {"F4:34:F0","Apple"},{"F4:37:B7","Apple"},{"F4:5C:89","Apple"},{"F4:63:9D","Apple"},
        {"F4:65:A6","Apple"},{"F4:6B:EF","Apple"},{"F4:70:AB","Apple"},{"F4:8C:EB","Apple"},
        {"F4:A3:95","Apple"},{"F4:A4:D6","Apple"},{"F4:AF:E7","Apple"},{"F4:B5:2F","Apple"},
        {"F4:B8:5E","Apple"},{"F4:BD:9E","Apple"},{"F4:CD:90","Apple"},{"F4:D4:88","Apple"},
        {"F4:E0:53","Apple"},{"F4:F1:5A","Apple"},{"F4:F9:51","Apple"},{"F4:FC:32","Apple"},
        {"F8:03:77","Apple"},{"F8:04:2E","Apple"},{"F8:0D:AC","Apple"},{"F8:1E:DF","Apple"},
        {"F8:27:93","Apple"},{"F8:28:19","Apple"},{"F8:2D:7C","Apple"},{"F8:38:80","Apple"},
        {"F8:3F:51","Apple"},{"F8:43:47","Apple"},{"F8:4D:89","Apple"},{"F8:4E:73","Apple"},
        {"F8:53:29","Apple"},{"F8:5E:A0","Apple"},{"F8:62:AA","Apple"},{"F8:65:5A","Apple"},
        {"F8:66:5A","Apple"},{"F8:6F:C1","Apple"},{"F8:73:A2","Apple"},{"F8:79:0A","Apple"},
        {"F8:7B:62","Apple"},{"F8:7D:76","Apple"},{"F8:87:F1","Apple"},{"F8:95:EA","Apple"},
        {"F8:9C:94","Apple"},{"F8:A0:BF","Apple"},{"F8:A2:D6","Apple"},{"F8:A9:D0","Apple"},
        {"F8:AC:6D","Apple"},{"F8:B1:56","Apple"},{"F8:BB:BF","Apple"},{"F8:C3:9E","Apple"},
        {"F8:C9:6C","Apple"},{"F8:CA:B8","Apple"},{"F8:CF:C5","Apple"},{"F8:D0:27","Apple"},
        {"F8:D1:11","Apple"},{"F8:E0:79","Apple"},{"F8:E6:1A","Apple"},{"F8:E9:4E","Apple"},
        {"F8:F0:05","Apple"},{"F8:F7:D3","Apple"},{"F8:FF:C2","Apple"},{"FC:01:7C","Apple"},
        {"FC:0A:81","Apple"},{"FC:18:3C","Apple"},{"FC:1D:43","Apple"},{"FC:1D:59","Apple"},
        {"FC:1D:84","Apple"},{"FC:25:3F","Apple"},{"FC:2A:9C","Apple"},{"FC:2D:5E","Apple"},
        {"FC:2E:2D","Apple"},{"FC:42:03","Apple"},{"FC:5C:EE","Apple"},{"FC:66:CF","Apple"},
        {"FC:6A:3A","Apple"},{"FC:82:53","Apple"},{"FC:AA:81","Apple"},{"FC:B0:C4","Apple"},
        {"FC:B3:BC","Apple"},{"FC:BC:9C","Apple"},{"FC:C2:DE","Apple"},{"FC:CD:55","Apple"},
        {"FC:D8:48","Apple"},{"FC:DA:5C","Apple"},{"FC:DC:0C","Apple"},{"FC:E9:98","Apple"},
        {"FC:EB:69","Apple"},{"FC:ED:5D","Apple"},{"FC:F8:AE","Apple"},{"FC:FC:48","Apple"},
        // Intel
        {"00:02:B3","Intel"},{"00:03:47","Intel"},{"00:04:23","Intel"},{"00:07:E9","Intel"},
        {"00:0A:0D","Intel"},{"00:0B:AB","Intel"},{"00:0C:F1","Intel"},{"00:0D:60","Intel"},
        {"00:0E:0C","Intel"},{"00:11:11","Intel"},{"00:12:F0","Intel"},{"00:13:02","Intel"},
        {"00:15:00","Intel"},{"00:16:41","Intel"},{"00:16:EA","Intel"},{"00:18:DE","Intel"},
        {"00:19:D1","Intel"},{"00:1A:6B","Intel"},{"00:1B:21","Intel"},{"00:1B:77","Intel"},
        {"00:1C:BF","Intel"},{"00:1E:64","Intel"},{"00:1F:3B","Intel"},{"00:21:5C","Intel"},
        {"00:23:13","Intel"},{"00:24:D7","Intel"},{"00:26:C6","Intel"},{"04:7C:16","Intel"},
        {"0C:54:A5","Intel"},{"0C:8B:FD","Intel"},{"0C:D7:46","Intel"},{"10:02:B5","Intel"},
        {"10:3D:0E","Intel"},{"18:3D:A2","Intel"},{"18:59:36","Intel"},{"1C:1B:B5","Intel"},
        {"1C:3A:DE","Intel"},{"1C:4B:D6","Intel"},{"1C:69:7A","Intel"},{"1C:99:57","Intel"},
        {"1C:BF:CE","Intel"},{"20:16:B9","Intel"},{"20:79:72","Intel"},{"24:FD:52","Intel"},
        {"28:16:AD","Intel"},{"28:7D:E8","Intel"},{"28:9A:4D","Intel"},{"2C:54:CF","Intel"},
        {"2C:6A:6F","Intel"},{"30:3A:64","Intel"},{"30:45:96","Intel"},{"34:13:E8","Intel"},
        {"34:1F:EB","Intel"},{"34:27:92","Intel"},{"34:7D:E4","Intel"},{"34:CF:F6","Intel"},
        {"34:DE:1A","Intel"},{"34:E1:2D","Intel"},{"34:E6:AD","Intel"},{"34:F3:9A","Intel"},
        {"38:00:25","Intel"},{"38:7A:CC","Intel"},{"38:DE:AD","Intel"},{"3C:6A:A7","Intel"},
        {"3C:A0:67","Intel"},{"3C:F8:62","Intel"},{"40:23:43","Intel"},{"40:49:0F","Intel"},
        {"40:74:E0","Intel"},{"40:98:4E","Intel"},{"40:B0:34","Intel"},{"40:EC:99","Intel"},
        {"44:1C:12","Intel"},{"44:2C:05","Intel"},{"44:85:00","Intel"},{"48:51:C5","Intel"},
        {"48:89:68","Intel"},{"48:A4:72","Intel"},{"48:E7:DA","Intel"},{"4C:1D:96","Intel"},
        {"4C:3B:DF","Intel"},{"4C:4B:F9","Intel"},{"4C:52:62","Intel"},{"4C:79:75","Intel"},
        {"4C:8B:30","Intel"},{"4C:8B:55","Intel"},{"4C:B9:8E","Intel"},{"4C:BB:58","Intel"},
        {"4C:EB:42","Intel"},{"4C:ED:DE","Intel"},{"4C:F5:5D","Intel"},{"50:2F:A8","Intel"},
        {"50:3E:AA","Intel"},{"50:51:03","Intel"},{"50:5A:65","Intel"},{"50:5B:C2","Intel"},
        {"50:8A:06","Intel"},{"50:A6:7F","Intel"},{"50:AF:73","Intel"},{"50:B7:C3","Intel"},
        {"50:DA:00","Intel"},{"50:DB:3D","Intel"},{"50:DC:E7","Intel"},{"50:E0:85","Intel"},
        {"50:EB:71","Intel"},{"50:EF:9C","Intel"},{"54:02:71","Intel"},{"54:13:79","Intel"},
        {"54:14:73","Intel"},{"54:35:30","Intel"},{"54:42:49","Intel"},{"54:48:10","Intel"},
        {"54:5A:A6","Intel"},{"54:8D:5A","Intel"},{"54:92:BE","Intel"},{"54:9D:85","Intel"},
        {"54:AB:3A","Intel"},{"54:B8:0A","Intel"},{"54:C9:DF","Intel"},{"54:E1:AD","Intel"},
        {"58:10:D2","Intel"},{"58:2A:D5","Intel"},{"58:8A:5A","Intel"},{"58:94:6B","Intel"},
        {"58:96:1D","Intel"},{"58:9C:FC","Intel"},{"5C:02:14","Intel"},{"5C:51:4F","Intel"},
        {"5C:5B:35","Intel"},{"5C:80:B6","Intel"},{"5C:85:7E","Intel"},{"5C:87:9C","Intel"},
        {"5C:AD:76","Intel"},{"5C:AF:06","Intel"},{"5C:D4:61","Intel"},{"5C:DC:96","Intel"},
        {"5C:E0:C5","Intel"},{"5C:F3:70","Intel"},{"5C:F8:21","Intel"},{"60:1D:0F","Intel"},
        {"60:1D:91","Intel"},{"60:21:74","Intel"},{"60:22:30","Intel"},{"60:30:21","Intel"},
        {"60:3E:5F","Intel"},{"60:45:CB","Intel"},{"60:4A:1C","Intel"},{"60:6C:66","Intel"},
        {"60:9C:9A","Intel"},{"60:A8:FE","Intel"},{"60:B3:C4","Intel"},{"60:BE:B5","Intel"},
        {"60:D2:50","Intel"},{"60:D9:A0","Intel"},{"60:DD:8E","Intel"},{"60:E0:0E","Intel"},
        {"60:F1:8A","Intel"},{"60:F2:62","Intel"},{"60:F6:77","Intel"},{"60:FA:CD","Intel"},
        {"64:00:6A","Intel"},{"64:00:F1","Intel"},{"64:32:A4","Intel"},{"64:4B:F0","Intel"},
        {"64:5A:04","Intel"},{"64:5D:86","Intel"},{"64:66:33","Intel"},{"64:70:02","Intel"},
        {"64:85:2D","Intel"},{"64:9C:81","Intel"},{"64:A2:F9","Intel"},{"64:A6:51","Intel"},
        {"64:B4:73","Intel"},{"64:BC:0C","Intel"},{"64:BC:58","Intel"},{"64:D4:BD","Intel"},
        {"64:D8:60","Intel"},{"64:DB:43","Intel"},{"64:E0:3A","Intel"},{"64:E5:99","Intel"},
        {"64:E8:81","Intel"},{"64:EB:8C","Intel"},{"64:F0:47","Intel"},{"64:F6:9D","Intel"},
        {"64:F9:65","Intel"},{"68:07:15","Intel"},{"68:14:01","Intel"},{"68:5D:43","Intel"},
        {"68:7F:74","Intel"},{"68:94:23","Intel"},{"68:A3:C4","Intel"},{"68:EC:C5","Intel"},
        {"6C:0B:84","Intel"},{"6C:5C:14","Intel"},{"6C:88:14","Intel"},{"70:03:9F","Intel"},
        {"70:1A:04","Intel"},{"70:1C:E7","Intel"},{"70:2B:1D","Intel"},{"70:35:09","Intel"},
        {"70:3A:51","Intel"},{"70:5A:AC","Intel"},{"70:62:6B","Intel"},{"70:77:81","Intel"},
        {"70:85:C2","Intel"},{"70:8B:CD","Intel"},{"70:AB:8B","Intel"},{"70:C7:F2","Intel"},
        // Dell
        {"00:06:5B","Dell"},{"00:08:74","Dell"},{"00:0B:DB","Dell"},{"00:0D:56","Dell"},
        {"00:0F:1F","Dell"},{"00:11:43","Dell"},{"00:12:3F","Dell"},{"00:13:72","Dell"},
        {"00:14:22","Dell"},{"00:15:C5","Dell"},{"00:18:8B","Dell"},{"00:19:B9","Dell"},
        {"00:1A:A0","Dell"},{"00:1C:23","Dell"},{"00:1D:09","Dell"},{"00:1E:4F","Dell"},
        {"00:1E:C9","Dell"},{"00:21:70","Dell"},{"00:21:9B","Dell"},{"00:22:19","Dell"},
        {"00:23:AE","Dell"},{"00:24:E8","Dell"},{"00:25:64","Dell"},{"00:26:B9","Dell"},
        {"18:03:73","Dell"},{"18:26:6C","Dell"},{"18:DB:F2","Dell"},{"1C:1B:68","Dell"},
        {"24:6E:96","Dell"},{"28:F1:0E","Dell"},{"34:17:EB","Dell"},{"34:48:ED","Dell"},
        {"34:E6:D7","Dell"},{"3C:2C:30","Dell"},{"48:2A:E3","Dell"},{"4C:76:25","Dell"},
        {"50:9A:4C","Dell"},{"54:BF:64","Dell"},{"78:2B:CB","Dell"},{"78:45:58","Dell"},
        {"84:7B:EB","Dell"},{"84:8F:69","Dell"},{"84:A9:3E","Dell"},{"90:B1:1C","Dell"},
        {"98:90:96","Dell"},{"9C:DA:3E","Dell"},{"A4:4C:C8","Dell"},{"A4:5D:36","Dell"},
        {"A4:BA:DB","Dell"},{"B0:83:FE","Dell"},{"B8:2A:72","Dell"},{"B8:AC:6F","Dell"},
        {"C8:1F:66","Dell"},{"D0:67:E5","Dell"},{"D0:94:66","Dell"},{"D4:3D:7E","Dell"},
        {"D4:81:D7","Dell"},{"D4:BE:D9","Dell"},{"D8:9E:F3","Dell"},{"F0:4D:A2","Dell"},
        {"F4:8E:38","Dell"},{"F8:BC:12","Dell"},{"F8:DB:88","Dell"},
        // HP
        {"00:02:A5","HP"},{"00:0B:CD","HP"},{"00:0E:7F","HP"},{"00:10:83","HP"},
        {"00:11:0A","HP"},{"00:11:85","HP"},{"00:13:21","HP"},{"00:14:38","HP"},
        {"00:15:60","HP"},{"00:16:35","HP"},{"00:17:A4","HP"},{"00:18:FE","HP"},
        {"00:19:BB","HP"},{"00:1A:4B","HP"},{"00:1B:78","HP"},{"00:1C:C4","HP"},
        {"00:1D:0D","HP"},{"00:1E:0B","HP"},{"00:1F:29","HP"},{"00:21:5A","HP"},
        {"00:21:FE","HP"},{"00:22:64","HP"},{"00:23:7D","HP"},{"00:24:81","HP"},
        {"00:25:B3","HP"},{"00:26:55","HP"},{"00:30:C1","HP"},{"04:0E:3C","HP"},
        {"10:60:4B","HP"},{"14:58:D0","HP"},{"18:60:24","HP"},{"1C:98:EC","HP"},
        {"20:47:47","HP"},{"28:92:4A","HP"},{"2C:44:FD","HP"},{"2C:59:E5","HP"},
        {"2C:76:8A","HP"},{"30:0D:43","HP"},{"30:C3:D9","HP"},{"34:64:A9","HP"},
        {"34:FC:EF","HP"},{"38:63:BB","HP"},{"38:7A:0E","HP"},{"38:CA:97","HP"},
        {"38:D5:47","HP"},{"38:E7:0D","HP"},{"3C:52:82","HP"},{"3C:A8:2A","HP"},
        {"40:01:C6","HP"},{"40:2E:28","HP"},{"40:5D:82","HP"},{"40:80:5A","HP"},
        {"44:1E:A1","HP"},{"44:31:92","HP"},{"44:48:C9","HP"},{"48:5B:39","HP"},
        {"48:5D:60","HP"},{"48:BA:4E","HP"},{"48:DF:37","HP"},{"4C:0B:BE","HP"},
        {"50:65:F3","HP"},{"54:05:AB","HP"},{"54:80:28","HP"},{"58:20:B1","HP"},
        {"5C:CB:99","HP"},{"60:14:66","HP"},{"60:6D:C7","HP"},{"64:16:66","HP"},
        {"64:31:50","HP"},{"64:80:99","HP"},{"68:B5:99","HP"},{"6C:AE:8B","HP"},
        {"6C:BE:E9","HP"},{"70:10:5C","HP"},{"74:46:A0","HP"},
        // 其他常见厂商
        {"00:0C:29","VMware"},{"00:50:56","VMware"},{"00:05:69","VMware"},{"00:1C:14","VMware"},
        {"00:0C:42","TP-Link"},{"00:19:E0","TP-Link"},{"00:1A:70","TP-Link"},{"00:1D:0F","TP-Link"},
        {"00:21:27","TP-Link"},{"00:23:CD","TP-Link"},{"00:25:86","TP-Link"},{"00:27:19","TP-Link"},
        {"10:FE:ED","TP-Link"},{"14:CC:20","TP-Link"},{"30:B5:C2","TP-Link"},{"50:C7:BF","TP-Link"},
        {"94:0C:6D","TP-Link"},{"B0:95:75","TP-Link"},{"C4:6E:1F","TP-Link"},{"E8:94:F6","TP-Link"},
        {"00:14:6C","Netgear"},{"00:18:4D","Netgear"},{"00:1F:33","Netgear"},{"00:22:3F","Netgear"},
        {"00:24:B2","Netgear"},{"04:A1:51","Netgear"},{"08:36:C9","Netgear"},{"10:0D:7F","Netgear"},
        {"14:59:C0","Netgear"},{"20:0C:C8","Netgear"},{"20:E5:2A","Netgear"},{"28:10:7B","Netgear"},
        {"28:C6:8E","Netgear"},{"2C:30:33","Netgear"},{"30:46:9A","Netgear"},{"44:94:FC","Netgear"},
        {"50:6F:9A","Netgear"},{"6C:B0:CE","Netgear"},{"80:37:29","Netgear"},{"84:1B:5E","Netgear"},
        {"8C:3A:E3","Netgear"},{"A0:21:B7","Netgear"},{"A0:4E:01","Netgear"},{"B0:39:56","Netgear"},
        {"B0:7F:B9","Netgear"},{"C4:04:15","Netgear"},{"D8:49:4B","Netgear"},{"E0:46:9A","Netgear"},
        {"E4:F4:C6","Netgear"},{"F8:73:94","Netgear"},
        {"00:0D:88","D-Link"},{"00:11:95","D-Link"},{"00:13:46","D-Link"},{"00:17:9A","D-Link"},
        {"00:1A:73","D-Link"},{"00:1B:11","D-Link"},{"00:1C:7E","D-Link"},{"00:1E:58","D-Link"},
        {"00:22:B0","D-Link"},{"00:24:01","D-Link"},{"00:26:5A","D-Link"},{"04:8D:38","D-Link"},
        {"0C:17:62","D-Link"},{"10:6F:3F","D-Link"},{"14:D6:4D","D-Link"},{"18:9C:27","D-Link"},
        {"1C:5F:2B","D-Link"},{"20:5E:F7","D-Link"},{"24:05:0F","D-Link"},{"28:10:1B","D-Link"},
        {"2C:00:33","D-Link"},{"30:05:5C","D-Link"},{"34:08:04","D-Link"},{"38:63:F6","D-Link"},
        {"3C:5A:37","D-Link"},{"40:16:9F","D-Link"},{"44:D9:E7","D-Link"},{"48:22:54","D-Link"},
        {"4C:63:EB","D-Link"},{"50:8F:4C","D-Link"},{"54:2A:1B","D-Link"},{"58:0E:C6","D-Link"},
        {"5C:35:3B","D-Link"},{"60:02:92","D-Link"},{"64:0F:28","D-Link"},{"68:1A:B2","D-Link"},
        {"6C:19:8F","D-Link"},{"70:8B:78","D-Link"},{"74:4D:28","D-Link"},{"78:54:2E","D-Link"},
        {"7C:D9:5C","D-Link"},{"80:3F:5D","D-Link"},{"84:25:19","D-Link"},{"88:9B:39","D-Link"},
        {"90:8F:0B","D-Link"},{"94:62:69","D-Link"},{"9C:5D:12","D-Link"},{"A0:39:F7","D-Link"},
        {"A4:2B:8C","D-Link"},{"A8:6D:AA","D-Link"},{"AC:5D:10","D-Link"},{"B0:38:29","D-Link"},
        {"B4:21:8A","D-Link"},{"B8:6C:E8","D-Link"},{"BC:62:0E","D-Link"},{"C0:56:27","D-Link"},
        {"C4:12:F5","D-Link"},{"C8:3A:35","D-Link"},{"CC:1F:C9","D-Link"},{"D0:7A:B5","D-Link"},
        {"D4:AE:52","D-Link"},{"DC:9B:9C","D-Link"},{"E0:0C:7F","D-Link"},{"E4:6F:13","D-Link"},
        {"E8:CC:18","D-Link"},{"EC:1A:59","D-Link"},{"F0:7D:68","D-Link"},{"F8:1A:67","D-Link"},
        {"FC:75:16","D-Link"},
        {"00:1A:8C","Sony"},{"00:24:BE","Sony"},{"04:46:65","Sony"},{"08:76:FF","Sony"},
        {"0C:3C:65","Sony"},{"10:4F:A8","Sony"},{"18:26:66","Sony"},{"1C:5A:3E","Sony"},
        {"20:3A:07","Sony"},{"24:4C:07","Sony"},{"28:5F:DB","Sony"},{"2C:33:7A","Sony"},
        {"34:02:86","Sony"},{"38:1A:52","Sony"},{"3C:07:71","Sony"},{"40:2B:50","Sony"},
        {"44:03:2C","Sony"},{"48:5A:3F","Sony"},{"4C:0D:EE","Sony"},{"50:50:54","Sony"},
        {"54:42:3A","Sony"},{"58:53:18","Sony"},{"5C:84:7A","Sony"},{"60:38:E0","Sony"},
        {"64:BE:63","Sony"},{"68:05:71","Sony"},{"6C:AD:3F","Sony"},{"70:9E:29","Sony"},
        {"74:34:28","Sony"},{"78:84:3C","Sony"},{"80:91:33","Sony"},{"84:0D:8E","Sony"},
        {"88:BF:D5","Sony"},{"8C:93:4E","Sony"},{"90:6F:18","Sony"},{"94:AE:2F","Sony"},
        {"98:52:3D","Sony"},{"9C:5C:8E","Sony"},{"A4:0B:C0","Sony"},{"A8:2B:B9","Sony"},
        {"AC:9B:0A","Sony"},{"B0:0D:77","Sony"},{"B4:5D:22","Sony"},{"B8:27:79","Sony"},
        {"BC:AE:C5","Sony"},{"C0:5B:A3","Sony"},{"C4:6E:7F","Sony"},{"C8:63:14","Sony"},
        {"D0:27:88","Sony"},{"D4:1F:0C","Sony"},{"D8:0C:FF","Sony"},{"DC:8B:94","Sony"},
        {"E0:51:63","Sony"},{"E4:7D:BD","Sony"},{"EC:0E:C4","Sony"},{"F0:47:26","Sony"},
        {"F4:0B:93","Sony"},{"F8:46:1C","Sony"},{"FC:25:06","Sony"},
        // Xiaomi
        {"04:5A:95","Xiaomi"},{"04:DB:8A","Xiaomi"},{"08:10:74","Xiaomi"},{"08:26:AE","Xiaomi"},
        {"08:3E:8E","Xiaomi"},{"08:5B:0E","Xiaomi"},{"08:6E:E2","Xiaomi"},{"08:70:45","Xiaomi"},
        {"08:7B:12","Xiaomi"},{"08:86:3B","Xiaomi"},{"08:8F:2C","Xiaomi"},{"08:93:5A","Xiaomi"},
        {"08:9E:01","Xiaomi"},{"08:A0:56","Xiaomi"},{"08:A6:BC","Xiaomi"},{"08:AC:92","Xiaomi"},
        {"08:B4:2A","Xiaomi"},{"08:B5:4D","Xiaomi"},{"08:BB:8C","Xiaomi"},{"08:BC:20","Xiaomi"},
        {"08:BF:5D","Xiaomi"},{"08:C5:E1","Xiaomi"},{"08:C8:5C","Xiaomi"},{"08:CE:5A","Xiaomi"},
        {"08:D1:7B","Xiaomi"},{"08:D2:9A","Xiaomi"},{"08:D4:0C","Xiaomi"},{"08:D4:6E","Xiaomi"},
        {"08:D8:33","Xiaomi"},{"08:DB:3F","Xiaomi"},{"08:DF:1F","Xiaomi"},{"08:E0:5F","Xiaomi"},
        {"08:E2:5E","Xiaomi"},{"08:E4:24","Xiaomi"},{"08:E5:DA","Xiaomi"},{"08:E8:4F","Xiaomi"},
        {"08:E9:3C","Xiaomi"},{"08:EA:40","Xiaomi"},{"08:EA:44","Xiaomi"},{"08:EB:ED","Xiaomi"},
        {"08:EC:A9","Xiaomi"},{"08:EE:8A","Xiaomi"},{"08:EF:3B","Xiaomi"},{"08:F0:2E","Xiaomi"},
        {"08:F1:1C","Xiaomi"},{"08:F1:B7","Xiaomi"},{"08:F2:BA","Xiaomi"},{"08:F2:E2","Xiaomi"},
        {"08:F4:6E","Xiaomi"},{"08:F4:AB","Xiaomi"},{"08:F7:28","Xiaomi"},{"08:F8:BC","Xiaomi"},
        {"08:F9:3B","Xiaomi"},{"08:FA:E0","Xiaomi"},{"08:FC:52","Xiaomi"},{"08:FC:88","Xiaomi"},
        {"08:FD:0E","Xiaomi"},{"08:FE:ED","Xiaomi"},{"0C:1D:AF","Xiaomi"},{"0C:2E:2B","Xiaomi"},
        {"0C:4D:2B","Xiaomi"},{"0C:86:10","Xiaomi"},{"0C:8D:CA","Xiaomi"},{"0C:98:38","Xiaomi"},
        {"0C:9B:13","Xiaomi"},{"0C:9D:92","Xiaomi"},{"0C:A0:0C","Xiaomi"},{"0C:A5:4F","Xiaomi"},
        {"0C:A8:94","Xiaomi"},{"0C:B5:2D","Xiaomi"},{"0C:BC:BA","Xiaomi"},{"0C:CB:8D","Xiaomi"},
        {"0C:CE:8E","Xiaomi"},{"0C:D0:3D","Xiaomi"},{"0C:D6:96","Xiaomi"},{"0C:DC:7E","Xiaomi"},
        {"0C:E0:E4","Xiaomi"},{"0C:EF:03","Xiaomi"},{"0C:F0:B4","Xiaomi"},{"0C:F4:D5","Xiaomi"},
        {"0C:F8:93","Xiaomi"},{"0C:FA:0E","Xiaomi"},{"0C:FC:83","Xiaomi"},{"34:CD:6D","Xiaomi"},
        {"38:A2:8C","Xiaomi"},{"40:31:3C","Xiaomi"},{"4C:63:71","Xiaomi"},{"50:64:2B","Xiaomi"},
        {"54:27:8E","Xiaomi"},{"64:09:80","Xiaomi"},{"64:CC:2E","Xiaomi"},{"68:DF:DD","Xiaomi"},
        {"78:02:B7","Xiaomi"},{"78:11:DC","Xiaomi"},{"78:92:9C","Xiaomi"},{"7C:49:EB","Xiaomi"},
        {"8C:8D:28","Xiaomi"},{"8C:CE:4E","Xiaomi"},{"90:48:6C","Xiaomi"},{"94:65:9C","Xiaomi"},
        {"9C:99:1E","Xiaomi"},{"A4:31:F1","Xiaomi"},{"A4:77:58","Xiaomi"},{"A4:9B:4F","Xiaomi"},
        {"A4:CC:32","Xiaomi"},{"AC:64:DD","Xiaomi"},{"AC:7B:A1","Xiaomi"},{"AC:83:F3","Xiaomi"},
        {"AC:9E:17","Xiaomi"},{"AC:C1:EE","Xiaomi"},{"AC:F7:F3","Xiaomi"},{"B0:25:AA","Xiaomi"},
        {"B0:41:1D","Xiaomi"},{"B0:55:08","Xiaomi"},{"B0:7D:64","Xiaomi"},{"B0:8B:CF","Xiaomi"},
        {"B0:B2:8F","Xiaomi"},{"B0:C0:90","Xiaomi"},{"B0:CA:6F","Xiaomi"},{"B0:CF:4D","Xiaomi"},
        {"B0:D5:9D","Xiaomi"},{"B0:D7:C5","Xiaomi"},{"B0:DF:3A","Xiaomi"},{"B0:E1:7E","Xiaomi"},
        {"B0:E2:35","Xiaomi"},{"B0:E5:12","Xiaomi"},{"B0:E7:54","Xiaomi"},{"B0:E9:FE","Xiaomi"},
        {"B0:EB:91","Xiaomi"},{"B0:EC:8F","Xiaomi"},{"B0:ED:BD","Xiaomi"},{"B0:EE:7B","Xiaomi"},
        {"B0:EF:4A","Xiaomi"},{"B0:F0:25","Xiaomi"},{"B0:F1:5A","Xiaomi"},{"B0:F2:8D","Xiaomi"},
        {"B0:F3:9A","Xiaomi"},{"B0:F4:5B","Xiaomi"},{"B0:F5:2E","Xiaomi"},{"B0:F6:3C","Xiaomi"},
        {"B0:F7:49","Xiaomi"},{"B0:F8:7A","Xiaomi"},{"B0:F9:1B","Xiaomi"},{"B0:FA:6E","Xiaomi"},
        {"B0:FB:4C","Xiaomi"},{"B0:FC:0D","Xiaomi"},{"B0:FD:2A","Xiaomi"},{"B0:FE:9B","Xiaomi"},
        {"B0:FF:7F","Xiaomi"},{"C0:EE:FB","Xiaomi"},{"C4:0B:CB","Xiaomi"},{"C4:0E:45","Xiaomi"},
        {"C4:1E:CE","Xiaomi"},{"C4:2F:90","Xiaomi"},{"C4:3A:EA","Xiaomi"},{"C4:3E:1F","Xiaomi"},
        {"C4:43:8F","Xiaomi"},{"C4:46:19","Xiaomi"},{"C4:47:3E","Xiaomi"},{"C4:48:5C","Xiaomi"},
        {"C4:49:7A","Xiaomi"},{"C8:2B:96","Xiaomi"},{"C8:3A:6B","Xiaomi"},{"C8:47:8C","Xiaomi"},
        {"CC:9E:A2","Xiaomi"},{"D0:03:DF","Xiaomi"},{"D0:2B:20","Xiaomi"},{"D0:4E:03","Xiaomi"},
        {"D0:5A:65","Xiaomi"},{"D0:6F:4A","Xiaomi"},{"D0:7E:35","Xiaomi"},{"D0:88:3C","Xiaomi"},
        {"D0:91:7A","Xiaomi"},{"D0:97:7B","Xiaomi"},{"D0:9B:05","Xiaomi"},{"D0:A0:37","Xiaomi"},
        {"D0:A2:6A","Xiaomi"},
    };
    return t;
}

// ============================================================================
// IPScanWorker 实现
// ============================================================================
IPScanWorker::IPScanWorker(const QString& ip, int timeoutMs, QObject* receiver,
                           QAtomicInt* scanningFlag, QAtomicInt* progressCounter)
    : m_ip(ip)
    , m_timeoutMs(timeoutMs)
    , m_receiver(receiver)
    , m_scanning(scanningFlag)
    , m_progress(progressCounter)
{
    setAutoDelete(true);
}

QString IPScanWorker::pingIP()
{
    QProcess proc;
    QStringList args;
    args << "-c" << "1" << "-W" << QString::number(m_timeoutMs) << m_ip;
    proc.start("ping", args);
    if (!proc.waitForFinished(m_timeoutMs + 2000)) {
        proc.kill();
        return {};
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    if (proc.exitCode() == 0) {
        // 提取响应时间
        QRegularExpression re("time[=<]\\s*([\\d.]+)\\s*ms");
        auto match = re.match(output);
        if (match.hasMatch()) {
            return match.captured(1);
        }
        return "0"; // ping 成功但无法解析时间
    }
    return {};
}

QString IPScanWorker::resolveMAC()
{
    QProcess proc;
    // macOS: arp -a 输出格式: "? (192.168.1.1) at xx:xx:xx:xx:xx:xx on en0 ifscope [ethernet]"
    proc.start("arp", {"-a", m_ip});
    if (!proc.waitForFinished(3000)) {
        proc.kill();
        return {};
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QRegularExpression re("at\\s+([0-9a-fA-F:]{17})");
    auto match = re.match(output);
    if (match.hasMatch()) {
        return match.captured(1).toUpper();
    }

    // Linux: arp -n 格式: "192.168.1.1 ether xx:xx:xx:xx:xx:xx"
    proc.start("arp", {"-n", m_ip});
    if (!proc.waitForFinished(3000)) {
        proc.kill();
        return {};
    }
    output = QString::fromUtf8(proc.readAllStandardOutput());
    QRegularExpression re2("([0-9a-fA-F:]{17})");
    auto match2 = re2.match(output);
    if (match2.hasMatch()) {
        return match2.captured(1).toUpper();
    }

    return {};
}

QString IPScanWorker::resolveHostname()
{
    QHostInfo info = QHostInfo::fromName(m_ip);
    if (info.error() == QHostInfo::NoError && !info.hostName().isEmpty()
        && info.hostName() != m_ip) {
        return info.hostName();
    }
    return {};
}

void IPScanWorker::run()
{
    if (m_scanning->loadRelaxed() == 0) return;

    QString responseTime = pingIP();
    QString status = responseTime.isEmpty() ? "无响应" : "在线";
    QString mac = responseTime.isEmpty() ? QString() : resolveMAC();
    QString hostname = responseTime.isEmpty() ? QString() : resolveHostname();
    QString vendor = mac.isEmpty() ? QString() : IPScannerWidget::lookupOUI(mac);

    int rtMs = responseTime.isEmpty() ? -1 : static_cast<int>(responseTime.toFloat() * 100) / 100;

    QMetaObject::invokeMethod(m_receiver, "onResultReceived", Qt::QueuedConnection,
                              Q_ARG(QString, m_ip),
                              Q_ARG(QString, mac),
                              Q_ARG(QString, hostname),
                              Q_ARG(QString, vendor),
                              Q_ARG(int, rtMs),
                              Q_ARG(QString, status));

    m_progress->fetchAndAddRelaxed(1);
}

// ============================================================================
// IPScannerWidget 实现
// ============================================================================
IPScannerWidget::IPScannerWidget(QWidget* parent)
    : QWidget(parent)
    , m_threadPool(new QThreadPool(this))
    , m_scanning(0)
    , m_scannedCount(0)
    , m_totalCount(0)
    , m_foundCount(0)
{
    m_threadPool->setMaxThreadCount(32);
    setupUI();
    setupConnections();
}

IPScannerWidget::~IPScannerWidget()
{
    m_scanning.storeRelaxed(0);
    m_threadPool->clear();
    m_threadPool->waitForDone(5000);
}

void IPScannerWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // --- 扫描配置 ---
    auto* configGroup = new QGroupBox("扫描配置");
    auto* configForm = new QFormLayout(configGroup);
    configForm->setSpacing(6);

    m_ipRangeEdit = new QLineEdit();
    m_ipRangeEdit->setPlaceholderText("例如: 192.168.1.0/24 或 192.168.1.1-192.168.1.254");
    configForm->addRow("IP 范围:", m_ipRangeEdit);

    m_scanMethodCombo = new QComboBox();
    m_scanMethodCombo->addItem("ICMP Ping");
    m_scanMethodCombo->addItem("ARP 扫描");
    configForm->addRow("扫描方式:", m_scanMethodCombo);

    m_threadCountSpin = new QSpinBox();
    m_threadCountSpin->setRange(1, 256);
    m_threadCountSpin->setValue(32);
    m_threadCountSpin->setSuffix(" 线程");
    configForm->addRow("线程数:", m_threadCountSpin);

    m_timeoutSpin = new QSpinBox();
    m_timeoutSpin->setRange(100, 10000);
    m_timeoutSpin->setValue(1000);
    m_timeoutSpin->setSingleStep(100);
    m_timeoutSpin->setSuffix(" ms");
    configForm->addRow("超时:", m_timeoutSpin);

    mainLayout->addWidget(configGroup);

    // --- 按钮栏 ---
    auto* btnLayout = new QHBoxLayout();
    m_startBtn = new QPushButton("开始扫描");
    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setEnabled(false);
    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setEnabled(false);

    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_stopBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(btnLayout);

    // --- 进度条 ---
    auto* progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("就绪");
    progressLayout->addWidget(m_progressBar);
    mainLayout->addLayout(progressLayout);

    // --- 结果表 ---
    m_resultTable = new QTableWidget();
    m_resultTable->setColumnCount(6);
    QStringList headers = {"IP 地址", "MAC 地址", "主机名", "厂商(OUI)", "响应时间", "状态"};
    m_resultTable->setHorizontalHeaderLabels(headers);
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_resultTable->verticalHeader()->setVisible(false);

    // 设置列宽
    m_resultTable->setColumnWidth(0, 150);
    m_resultTable->setColumnWidth(1, 140);
    m_resultTable->setColumnWidth(2, 200);
    m_resultTable->setColumnWidth(3, 150);
    m_resultTable->setColumnWidth(4, 100);
    m_resultTable->setColumnWidth(5, 80);

    mainLayout->addWidget(m_resultTable);
}

void IPScannerWidget::setupConnections()
{
    connect(m_startBtn, &QPushButton::clicked, this, &IPScannerWidget::onStartScan);
    connect(m_stopBtn, &QPushButton::clicked, this, &IPScannerWidget::onStopScan);
    connect(m_exportBtn, &QPushButton::clicked, this, &IPScannerWidget::onExportCSV);
    connect(m_resultTable, &QTableWidget::customContextMenuRequested,
            this, &IPScannerWidget::onContextMenu);
}

// ============================================================================
// IP 范围解析
// ============================================================================
void IPScannerWidget::parseIPRange(const QString& input, QStringList& outIps)
{
    outIps.clear();
    QString trimmed = input.trimmed();

    // 格式1: CIDR 192.168.1.0/24
    QRegularExpression cidrRe("^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(\\d{1,2})$");
    auto cidrMatch = cidrRe.match(trimmed);
    if (cidrMatch.hasMatch()) {
        QString base = cidrMatch.captured(1);
        int bits = cidrMatch.captured(2).toInt();
        if (bits < 0 || bits > 32) return;

        QStringList parts = base.split('.');
        if (parts.size() != 4) return;

        quint32 ip = 0;
        for (int i = 0; i < 4; ++i) {
            int octet = parts[i].toInt();
            if (octet < 0 || octet > 255) return;
            ip = (ip << 8) | static_cast<quint32>(octet);
        }

        quint32 mask = (bits == 0) ? 0 : (~0u << (32 - bits));
        quint32 network = ip & mask;
        quint32 broadcast = network | (~mask);
        // 跳过网络地址和广播地址
        quint32 start = network + 1;
        quint32 end = (bits >= 31) ? broadcast : broadcast - 1;
        if (start > end) { start = network; end = broadcast; }

        for (quint32 i = start; i <= end; ++i) {
            outIps.append(QString("%1.%2.%3.%4")
                              .arg((i >> 24) & 0xFF)
                              .arg((i >> 16) & 0xFF)
                              .arg((i >> 8) & 0xFF)
                              .arg(i & 0xFF));
        }
        return;
    }

    // 格式2: 范围 192.168.1.1-192.168.1.254
    QRegularExpression rangeRe("^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})\\s*-\\s*(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})$");
    auto rangeMatch = rangeRe.match(trimmed);
    if (rangeMatch.hasMatch()) {
        QString startStr = rangeMatch.captured(1);
        QString endStr = rangeMatch.captured(2);

        auto ipToUint = [](const QString& s) -> quint32 {
            QStringList p = s.split('.');
            if (p.size() != 4) return 0;
            quint32 v = 0;
            for (int i = 0; i < 4; ++i) v = (v << 8) | static_cast<quint32>(p[i].toInt());
            return v;
        };

        quint32 start = ipToUint(startStr);
        quint32 end = ipToUint(endStr);
        if (start > end) std::swap(start, end);

        for (quint32 i = start; i <= end; ++i) {
            outIps.append(QString("%1.%2.%3.%4")
                              .arg((i >> 24) & 0xFF)
                              .arg((i >> 16) & 0xFF)
                              .arg((i >> 8) & 0xFF)
                              .arg(i & 0xFF));
        }
        return;
    }

    // 格式3: 单个 IP
    QRegularExpression singleRe("^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})$");
    auto singleMatch = singleRe.match(trimmed);
    if (singleMatch.hasMatch()) {
        outIps.append(trimmed);
        return;
    }
}

// ============================================================================
// 扫描控制
// ============================================================================
void IPScannerWidget::onStartScan()
{
    QStringList ips;
    parseIPRange(m_ipRangeEdit->text(), ips);
    if (ips.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "无法解析 IP 范围，请检查输入格式。\n"
                             "支持格式: 192.168.1.0/24, 192.168.1.1-192.168.1.254, 或单个 IP");
        return;
    }

    clearResults();
    m_scanning.storeRelaxed(1);
    m_scannedCount.storeRelaxed(0);
    m_totalCount = ips.size();
    m_foundCount = 0;

    m_threadPool->setMaxThreadCount(m_threadCountSpin->value());

    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_exportBtn->setEnabled(false);
    m_progressBar->setRange(0, m_totalCount);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("扫描中... %v / %m");

    int timeout = m_timeoutSpin->value();

    Logger::instance().info("IPScanner", QString("开始扫描 %1 个 IP，%2 线程，超时 %3 ms")
                            .arg(m_totalCount).arg(m_threadCountSpin->value()).arg(timeout));

    // 提交所有任务
    for (const QString& ip : ips) {
        auto* worker = new IPScanWorker(ip, timeout, this, &m_scanning, &m_scannedCount);
        m_threadPool->start(worker);
    }

    // 定时器检查扫描完成
    auto* timer = new QTimer(this);
    timer->setObjectName("scanCompleteTimer");
    connect(timer, &QTimer::timeout, this, [this, timer]() {
        int scanned = m_scannedCount.loadRelaxed();
        m_progressBar->setValue(scanned);
        if (scanned >= m_totalCount) {
            timer->stop();
            timer->deleteLater();
            m_scanning.storeRelaxed(0);
            m_startBtn->setEnabled(true);
            m_stopBtn->setEnabled(false);
            m_exportBtn->setEnabled(m_foundCount > 0);
            m_progressBar->setFormat(QString("完成 — 发现 %1 个设备").arg(m_foundCount));
            Logger::instance().info("IPScanner",
                                    QString("扫描完成: %1/%2 个在线").arg(m_foundCount).arg(m_totalCount));
            emit scanFinished(m_foundCount);
        }
    });
    timer->start(200);
}

void IPScannerWidget::onStopScan()
{
    m_scanning.storeRelaxed(0);
    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_exportBtn->setEnabled(m_foundCount > 0);
    m_progressBar->setFormat(QString("已停止 — 发现 %1 个设备").arg(m_foundCount));
    Logger::instance().info("IPScanner", "扫描已手动停止");
}

// ============================================================================
// 结果处理
// ============================================================================
void IPScannerWidget::onResultReceived(QString ip, QString mac, QString hostname,
                                        QString vendor, int responseTime, QString status)
{
    if (status == "在线") {
        QMutexLocker lock(&m_resultMutex);
        m_foundCount++;
        addResultRow(ip, mac, hostname, vendor, responseTime, status);
        emit deviceFound(ip, mac, hostname);
    }
}

void IPScannerWidget::addResultRow(const QString& ip, const QString& mac,
                                    const QString& hostname, const QString& vendor,
                                    int responseTime, const QString& status)
{
    int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);

    m_resultTable->setItem(row, 0, new QTableWidgetItem(ip));
    m_resultTable->setItem(row, 1, new QTableWidgetItem(mac.isEmpty() ? "-" : mac));
    m_resultTable->setItem(row, 2, new QTableWidgetItem(hostname.isEmpty() ? "-" : hostname));
    m_resultTable->setItem(row, 3, new QTableWidgetItem(vendor.isEmpty() ? "-" : vendor));

    QString rtStr = (responseTime >= 0) ? QString("%1 ms").arg(responseTime) : "-";
    m_resultTable->setItem(row, 4, new QTableWidgetItem(rtStr));

    auto* statusItem = new QTableWidgetItem(status);
    if (status == "在线") {
        statusItem->setForeground(QColor(0, 150, 0));
    } else {
        statusItem->setForeground(QColor(180, 0, 0));
    }
    m_resultTable->setItem(row, 5, statusItem);

    m_resultTable->scrollToBottom();
}

void IPScannerWidget::clearResults()
{
    m_resultTable->setRowCount(0);
    m_foundCount = 0;
}

// ============================================================================
// OUI 查找
// ============================================================================
QString IPScannerWidget::lookupOUI(const QString& mac)
{
    if (mac.length() < 8) return {};
    QString prefix = mac.left(8).toUpper(); // "XX:XX:XX"
    return ouiTable().value(prefix);
}

// ============================================================================
// ARP 表解析
// ============================================================================
QString IPScannerWidget::parseArpTable(const QString& ip)
{
    QProcess proc;
    proc.start("arp", {"-a", ip});
    if (!proc.waitForFinished(3000)) return {};
    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QRegularExpression re("at\\s+([0-9a-fA-F:]{17})");
    auto m = re.match(output);
    return m.hasMatch() ? m.captured(1).toUpper() : QString();
}

// ============================================================================
// 导出 CSV
// ============================================================================
void IPScannerWidget::onExportCSV()
{
    if (m_resultTable->rowCount() == 0) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出 CSV", "ip_scan_result.csv",
                                                     "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    QTextStream out(&file);
    // BOM for Excel UTF-8
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF";

    // 表头
    out << "IP 地址,MAC 地址,主机名,厂商(OUI),响应时间,状态\n";

    for (int row = 0; row < m_resultTable->rowCount(); ++row) {
        QStringList cols;
        for (int col = 0; col < m_resultTable->columnCount(); ++col) {
            auto* item = m_resultTable->item(row, col);
            QString val = item ? item->text() : "-";
            // 如果值包含逗号则加引号
            if (val.contains(',')) val = "\"" + val + "\"";
            cols << val;
        }
        out << cols.join(',') << "\n";
    }

    file.close();
    Logger::instance().info("IPScanner",
                            QString("结果已导出到: %1 (%2 条记录)").arg(filePath).arg(m_resultTable->rowCount()));
    QMessageBox::information(this, "导出成功",
                              QString("已导出 %1 条记录到:\n%2").arg(m_resultTable->rowCount()).arg(filePath));
}

// ============================================================================
// 右键菜单
// ============================================================================
void IPScannerWidget::onContextMenu(const QPoint& pos)
{
    QTableWidgetItem* item = m_resultTable->itemAt(pos);
    if (!item) return;

    int row = item->row();
    QString ip = m_resultTable->item(row, 0)->text();
    QString mac = m_resultTable->item(row, 1)->text();

    QMenu menu(this);
    QAction* copyIpAction = menu.addAction("复制 IP 地址");
    QAction* copyMacAction = menu.addAction("复制 MAC 地址");
    menu.addSeparator();
    QAction* sshAction = menu.addAction("SSH 连接到该主机");

    copyMacAction->setEnabled(mac != "-");

    QAction* chosen = menu.exec(m_resultTable->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == copyIpAction) {
        QApplication::clipboard()->setText(ip);
        Logger::instance().info("IPScanner", "已复制 IP: " + ip);
    } else if (chosen == copyMacAction) {
        QApplication::clipboard()->setText(mac);
        Logger::instance().info("IPScanner", "已复制 MAC: " + mac);
    } else if (chosen == sshAction) {
        // 发出信号让外部处理 SSH 连接
        // 同时尝试通过系统默认终端打开
        QString sshUrl = QString("ssh://%1").arg(ip);
        QDesktopServices::openUrl(QUrl(sshUrl));
        Logger::instance().info("IPScanner", "SSH 连接到: " + ip);
        emit deviceFound(ip, mac, QString());
    }
}