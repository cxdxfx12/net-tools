#include "mac/MacToolkitWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QLabel>
#include <QDateTime>
#include <QFile>
#include <QSet>

// ============================================================================
// 内置 OUI 厂商表 — 常见 MAC 前缀 → 厂商名 (至少 20 个厂商)
// ============================================================================
static const QHash<QString, QString>& ouiTable()
{
    static const QHash<QString, QString> t = {
        // Cisco
        {"00:00:0C", QStringLiteral("Cisco")}, {"00:01:42", QStringLiteral("Cisco")},
        {"00:0D:BC", QStringLiteral("Cisco")}, {"00:1A:A1", QStringLiteral("Cisco")},
        {"00:1B:0C", QStringLiteral("Cisco")}, {"00:1E:49", QStringLiteral("Cisco")},
        {"00:23:04", QStringLiteral("Cisco")}, {"00:25:84", QStringLiteral("Cisco")},
        {"00:26:0B", QStringLiteral("Cisco")}, {"04:DA:D2", QStringLiteral("Cisco")},
        {"10:05:01", QStringLiteral("Cisco")}, {"14:1B:BD", QStringLiteral("Cisco")},
        {"18:8B:9D", QStringLiteral("Cisco")}, {"24:E9:B3", QStringLiteral("Cisco")},
        {"30:7B:AC", QStringLiteral("Cisco")}, {"34:DB:9C", QStringLiteral("Cisco")},
        {"38:ED:18", QStringLiteral("Cisco")}, {"40:55:39", QStringLiteral("Cisco")},
        {"44:2A:FF", QStringLiteral("Cisco")}, {"48:0E:EC", QStringLiteral("Cisco")},
        {"4C:00:82", QStringLiteral("Cisco")}, {"50:3D:E5", QStringLiteral("Cisco")},
        {"54:75:D0", QStringLiteral("Cisco")}, {"58:0A:20", QStringLiteral("Cisco")},
        {"5C:5A:EA", QStringLiteral("Cisco")}, {"64:9E:F3", QStringLiteral("Cisco")},
        {"68:EF:BD", QStringLiteral("Cisco")}, {"6C:41:6A", QStringLiteral("Cisco")},
        {"70:DB:98", QStringLiteral("Cisco")}, {"74:26:AC", QStringLiteral("Cisco")},
        {"78:72:5D", QStringLiteral("Cisco")}, {"7C:AD:74", QStringLiteral("Cisco")},
        {"80:E0:1D", QStringLiteral("Cisco")}, {"84:3D:C6", QStringLiteral("Cisco")},
        {"88:5A:92", QStringLiteral("Cisco")}, {"8C:60:4F", QStringLiteral("Cisco")},
        {"90:2E:1C", QStringLiteral("Cisco")}, {"A4:4C:11", QStringLiteral("Cisco")},
        {"A8:9D:21", QStringLiteral("Cisco")}, {"B0:AA:77", QStringLiteral("Cisco")},
        {"B4:14:89", QStringLiteral("Cisco")}, {"B8:62:1F", QStringLiteral("Cisco")},
        {"C0:62:6B", QStringLiteral("Cisco")}, {"C4:14:3C", QStringLiteral("Cisco")},
        {"C8:4C:75", QStringLiteral("Cisco")}, {"CC:16:7E", QStringLiteral("Cisco")},
        {"D0:7E:28", QStringLiteral("Cisco")}, {"D4:6D:50", QStringLiteral("Cisco")},
        {"D8:B1:90", QStringLiteral("Cisco")}, {"E0:2F:6D", QStringLiteral("Cisco")},
        {"E4:C7:22", QStringLiteral("Cisco")}, {"E8:65:49", QStringLiteral("Cisco")},
        {"EC:1D:7F", QStringLiteral("Cisco")}, {"F0:29:29", QStringLiteral("Cisco")},
        {"F4:4E:05", QStringLiteral("Cisco")}, {"F8:72:EA", QStringLiteral("Cisco")},
        {"FC:5B:39", QStringLiteral("Cisco")},
        // Huawei
        {"00:0D:6E", QStringLiteral("Huawei")}, {"00:18:82", QStringLiteral("Huawei")},
        {"00:1E:10", QStringLiteral("Huawei")}, {"00:25:9E", QStringLiteral("Huawei")},
        {"00:E0:FC", QStringLiteral("Huawei")}, {"08:19:A6", QStringLiteral("Huawei")},
        {"0C:37:DC", QStringLiteral("Huawei")}, {"10:4B:46", QStringLiteral("Huawei")},
        {"14:6E:0A", QStringLiteral("Huawei")}, {"18:2E:5E", QStringLiteral("Huawei")},
        {"20:0B:C7", QStringLiteral("Huawei")}, {"24:1E:0B", QStringLiteral("Huawei")},
        {"28:3A:4D", QStringLiteral("Huawei")}, {"2C:0D:A7", QStringLiteral("Huawei")},
        {"30:0B:9B", QStringLiteral("Huawei")}, {"34:0A:7B", QStringLiteral("Huawei")},
        {"38:6E:6C", QStringLiteral("Huawei")}, {"40:2C:76", QStringLiteral("Huawei")},
        {"44:14:DB", QStringLiteral("Huawei")}, {"48:46:C1", QStringLiteral("Huawei")},
        {"4C:1F:CC", QStringLiteral("Huawei")}, {"50:7B:9D", QStringLiteral("Huawei")},
        {"54:2A:9C", QStringLiteral("Huawei")}, {"58:2A:F7", QStringLiteral("Huawei")},
        {"5C:4C:A9", QStringLiteral("Huawei")}, {"60:DE:44", QStringLiteral("Huawei")},
        {"68:9C:5E", QStringLiteral("Huawei")}, {"6C:0B:34", QStringLiteral("Huawei")},
        {"70:48:7F", QStringLiteral("Huawei")}, {"74:1E:93", QStringLiteral("Huawei")},
        {"78:0D:47", QStringLiteral("Huawei")}, {"78:D7:52", QStringLiteral("Huawei")},
        {"7C:60:97", QStringLiteral("Huawei")}, {"80:6D:97", QStringLiteral("Huawei")},
        {"84:AC:16", QStringLiteral("Huawei")}, {"88:36:6C", QStringLiteral("Huawei")},
        {"8C:BE:BE", QStringLiteral("Huawei")}, {"90:1F:0A", QStringLiteral("Huawei")},
        {"94:7D:77", QStringLiteral("Huawei")}, {"98:2D:7B", QStringLiteral("Huawei")},
        {"9C:28:EF", QStringLiteral("Huawei")}, {"A0:99:9B", QStringLiteral("Huawei")},
        {"A4:0E:2B", QStringLiteral("Huawei")}, {"A8:6B:AD", QStringLiteral("Huawei")},
        {"AC:61:75", QStringLiteral("Huawei")}, {"B0:68:E6", QStringLiteral("Huawei")},
        {"B4:0C:25", QStringLiteral("Huawei")}, {"B8:37:4A", QStringLiteral("Huawei")},
        {"BC:76:70", QStringLiteral("Huawei")}, {"C0:0D:7E", QStringLiteral("Huawei")},
        {"C4:01:CE", QStringLiteral("Huawei")}, {"C8:50:E9", QStringLiteral("Huawei")},
        {"CC:96:A0", QStringLiteral("Huawei")}, {"D0:D0:03", QStringLiteral("Huawei")},
        {"D4:4C:9C", QStringLiteral("Huawei")}, {"D8:49:0B", QStringLiteral("Huawei")},
        {"DC:2C:6E", QStringLiteral("Huawei")}, {"E0:98:61", QStringLiteral("Huawei")},
        {"E4:48:C7", QStringLiteral("Huawei")}, {"E8:0B:13", QStringLiteral("Huawei")},
        {"EC:38:73", QStringLiteral("Huawei")}, {"F0:64:91", QStringLiteral("Huawei")},
        {"F4:63:49", QStringLiteral("Huawei")}, {"F8:0D:60", QStringLiteral("Huawei")},
        {"FC:48:EF", QStringLiteral("Huawei")},
        // H3C
        {"00:0F:E2", QStringLiteral("H3C")}, {"00:1B:2F", QStringLiteral("H3C")},
        {"00:23:89", QStringLiteral("H3C")}, {"04:27:FE", QStringLiteral("H3C")},
        {"08:0D:67", QStringLiteral("H3C")}, {"0C:DA:41", QStringLiteral("H3C")},
        {"14:30:C9", QStringLiteral("H3C")}, {"18:FD:74", QStringLiteral("H3C")},
        {"20:1A:06", QStringLiteral("H3C")}, {"28:6E:D4", QStringLiteral("H3C")},
        {"30:63:6B", QStringLiteral("H3C")}, {"34:29:12", QStringLiteral("H3C")},
        {"38:22:D6", QStringLiteral("H3C")}, {"3C:8C:40", QStringLiteral("H3C")},
        {"40:7A:CB", QStringLiteral("H3C")}, {"48:7A:DA", QStringLiteral("H3C")},
        {"4C:AC:0A", QStringLiteral("H3C")}, {"50:61:84", QStringLiteral("H3C")},
        {"54:CD:EE", QStringLiteral("H3C")}, {"58:6A:B1", QStringLiteral("H3C")},
        {"5C:DD:70", QStringLiteral("H3C")}, {"60:31:3C", QStringLiteral("H3C")},
        {"64:9D:99", QStringLiteral("H3C")}, {"68:2C:7B", QStringLiteral("H3C")},
        {"6C:92:BF", QStringLiteral("H3C")}, {"70:7B:E8", QStringLiteral("H3C")},
        {"70:F9:6D", QStringLiteral("H3C")}, {"74:25:8A", QStringLiteral("H3C")},
        {"78:1D:BA", QStringLiteral("H3C")}, {"7C:1D:D9", QStringLiteral("H3C")},
        {"80:48:59", QStringLiteral("H3C")}, {"84:D9:31", QStringLiteral("H3C")},
        {"88:14:4B", QStringLiteral("H3C")}, {"8C:4C:DC", QStringLiteral("H3C")},
        {"90:5C:44", QStringLiteral("H3C")}, {"94:28:2E", QStringLiteral("H3C")},
        {"98:0E:E4", QStringLiteral("H3C")}, {"9C:21:41", QStringLiteral("H3C")},
        {"A0:48:1C", QStringLiteral("H3C")}, {"A4:5B:FE", QStringLiteral("H3C")},
        {"A8:13:74", QStringLiteral("H3C")}, {"AC:4A:FE", QStringLiteral("H3C")},
        {"B0:47:BF", QStringLiteral("H3C")}, {"B8:AF:67", QStringLiteral("H3C")},
        {"BC:3A:2A", QStringLiteral("H3C")}, {"C0:16:BD", QStringLiteral("H3C")},
        {"C4:74:40", QStringLiteral("H3C")}, {"CC:06:77", QStringLiteral("H3C")},
        {"D0:67:26", QStringLiteral("H3C")}, {"D4:81:CA", QStringLiteral("H3C")},
        {"D8:15:0D", QStringLiteral("H3C")}, {"DC:53:7C", QStringLiteral("H3C")},
        {"E0:9D:31", QStringLiteral("H3C")}, {"E4:61:1E", QStringLiteral("H3C")},
        {"E8:6A:64", QStringLiteral("H3C")}, {"EC:BD:1D", QStringLiteral("H3C")},
        {"F0:84:2F", QStringLiteral("H3C")}, {"F4:21:CA", QStringLiteral("H3C")},
        {"F8:32:E4", QStringLiteral("H3C")}, {"FC:3F:DB", QStringLiteral("H3C")},
        // Apple
        {"00:03:93", QStringLiteral("Apple")}, {"00:14:51", QStringLiteral("Apple")},
        {"00:17:F2", QStringLiteral("Apple")}, {"00:1E:C2", QStringLiteral("Apple")},
        {"00:1F:F3", QStringLiteral("Apple")}, {"00:21:6A", QStringLiteral("Apple")},
        {"00:22:41", QStringLiteral("Apple")}, {"00:23:12", QStringLiteral("Apple")},
        {"00:23:DF", QStringLiteral("Apple")}, {"00:24:36", QStringLiteral("Apple")},
        {"00:25:00", QStringLiteral("Apple")}, {"00:25:BC", QStringLiteral("Apple")},
        {"00:26:08", QStringLiteral("Apple")}, {"00:26:BB", QStringLiteral("Apple")},
        {"00:30:65", QStringLiteral("Apple")}, {"04:0C:CE", QStringLiteral("Apple")},
        {"04:15:52", QStringLiteral("Apple")}, {"04:1E:64", QStringLiteral("Apple")},
        {"04:26:65", QStringLiteral("Apple")}, {"04:4B:ED", QStringLiteral("Apple")},
        {"04:54:53", QStringLiteral("Apple")}, {"04:69:F8", QStringLiteral("Apple")},
        {"04:DB:56", QStringLiteral("Apple")}, {"04:E5:36", QStringLiteral("Apple")},
        {"04:F1:3E", QStringLiteral("Apple")}, {"08:66:98", QStringLiteral("Apple")},
        {"08:6D:41", QStringLiteral("Apple")}, {"08:74:02", QStringLiteral("Apple")},
        {"0C:15:39", QStringLiteral("Apple")}, {"0C:19:F8", QStringLiteral("Apple")},
        {"0C:3E:9F", QStringLiteral("Apple")}, {"0C:4D:E9", QStringLiteral("Apple")},
        {"0C:51:01", QStringLiteral("Apple")}, {"0C:77:1A", QStringLiteral("Apple")},
        {"0C:BC:9F", QStringLiteral("Apple")}, {"10:1C:0C", QStringLiteral("Apple")},
        {"10:29:59", QStringLiteral("Apple")}, {"10:40:F3", QStringLiteral("Apple")},
        {"10:41:7F", QStringLiteral("Apple")}, {"10:93:E9", QStringLiteral("Apple")},
        {"10:9A:DD", QStringLiteral("Apple")}, {"10:CE:E9", QStringLiteral("Apple")},
        {"14:10:9F", QStringLiteral("Apple")}, {"14:20:5E", QStringLiteral("Apple")},
        {"14:5A:05", QStringLiteral("Apple")}, {"14:5A:FC", QStringLiteral("Apple")},
        {"14:7D:DA", QStringLiteral("Apple")}, {"14:88:E6", QStringLiteral("Apple")},
        {"14:8F:C6", QStringLiteral("Apple")}, {"14:98:77", QStringLiteral("Apple")},
        {"14:99:E2", QStringLiteral("Apple")}, {"14:BD:61", QStringLiteral("Apple")},
        {"18:20:32", QStringLiteral("Apple")}, {"18:34:51", QStringLiteral("Apple")},
        {"18:3E:EF", QStringLiteral("Apple")}, {"18:65:90", QStringLiteral("Apple")},
        {"18:81:0E", QStringLiteral("Apple")}, {"18:9E:FC", QStringLiteral("Apple")},
        {"18:AF:61", QStringLiteral("Apple")}, {"18:AF:8F", QStringLiteral("Apple")},
        {"18:E7:F4", QStringLiteral("Apple")}, {"18:EE:69", QStringLiteral("Apple")},
        {"18:F1:D8", QStringLiteral("Apple")}, {"18:F6:43", QStringLiteral("Apple")},
        {"1C:1A:C0", QStringLiteral("Apple")}, {"1C:36:BB", QStringLiteral("Apple")},
        {"1C:91:48", QStringLiteral("Apple")}, {"1C:AB:A7", QStringLiteral("Apple")},
        {"1C:B3:C9", QStringLiteral("Apple")}, {"1C:E6:2B", QStringLiteral("Apple")},
        {"20:3C:AE", QStringLiteral("Apple")}, {"20:78:F0", QStringLiteral("Apple")},
        {"20:9B:CD", QStringLiteral("Apple")}, {"20:9C:B4", QStringLiteral("Apple")},
        {"20:A2:E4", QStringLiteral("Apple")}, {"20:AB:37", QStringLiteral("Apple")},
        {"20:C9:D0", QStringLiteral("Apple")}, {"20:EE:28", QStringLiteral("Apple")},
        {"24:1E:EB", QStringLiteral("Apple")}, {"24:24:0E", QStringLiteral("Apple")},
        {"24:4E:7B", QStringLiteral("Apple")}, {"24:5B:A7", QStringLiteral("Apple")},
        {"24:A0:74", QStringLiteral("Apple")}, {"24:A2:E1", QStringLiteral("Apple")},
        {"24:AB:81", QStringLiteral("Apple")}, {"24:E3:14", QStringLiteral("Apple")},
        {"24:F0:94", QStringLiteral("Apple")}, {"28:37:37", QStringLiteral("Apple")},
        {"28:5A:EB", QStringLiteral("Apple")}, {"28:6A:B8", QStringLiteral("Apple")},
        {"28:6A:BA", QStringLiteral("Apple")}, {"28:6A:BF", QStringLiteral("Apple")},
        {"28:CF:DA", QStringLiteral("Apple")}, {"28:CF:E9", QStringLiteral("Apple")},
        {"28:E0:2C", QStringLiteral("Apple")}, {"28:E7:1F", QStringLiteral("Apple")},
        {"28:EC:95", QStringLiteral("Apple")}, {"28:ED:E0", QStringLiteral("Apple")},
        {"28:F0:76", QStringLiteral("Apple")}, {"28:FF:3C", QStringLiteral("Apple")},
        {"2C:1F:23", QStringLiteral("Apple")}, {"2C:20:0B", QStringLiteral("Apple")},
        {"2C:61:F6", QStringLiteral("Apple")}, {"2C:BE:08", QStringLiteral("Apple")},
        {"2C:F0:A2", QStringLiteral("Apple")}, {"2C:F0:EE", QStringLiteral("Apple")},
        {"30:35:AD", QStringLiteral("Apple")}, {"30:57:14", QStringLiteral("Apple")},
        {"30:90:AB", QStringLiteral("Apple")}, {"30:9C:23", QStringLiteral("Apple")},
        {"30:F7:C5", QStringLiteral("Apple")}, {"34:12:98", QStringLiteral("Apple")},
        {"34:36:3B", QStringLiteral("Apple")}, {"34:42:62", QStringLiteral("Apple")},
        {"34:51:C9", QStringLiteral("Apple")}, {"34:7C:25", QStringLiteral("Apple")},
        {"34:8C:AE", QStringLiteral("Apple")}, {"34:A8:EB", QStringLiteral("Apple")},
        {"34:AB:37", QStringLiteral("Apple")}, {"34:C0:59", QStringLiteral("Apple")},
        {"34:E2:FD", QStringLiteral("Apple")}, {"38:0F:4A", QStringLiteral("Apple")},
        {"38:48:4C", QStringLiteral("Apple")}, {"38:53:9C", QStringLiteral("Apple")},
        {"38:71:DE", QStringLiteral("Apple")}, {"38:89:2C", QStringLiteral("Apple")},
        {"38:B5:4D", QStringLiteral("Apple")}, {"38:C9:86", QStringLiteral("Apple")},
        {"38:CA:DA", QStringLiteral("Apple")}, {"38:EC:0D", QStringLiteral("Apple")},
        {"38:F9:D3", QStringLiteral("Apple")}, {"3C:07:54", QStringLiteral("Apple")},
        {"3C:15:C2", QStringLiteral("Apple")}, {"3C:2E:FF", QStringLiteral("Apple")},
        {"3C:AB:8E", QStringLiteral("Apple")}, {"3C:D0:F8", QStringLiteral("Apple")},
        {"3C:E0:72", QStringLiteral("Apple")}, {"40:30:04", QStringLiteral("Apple")},
        {"40:33:1A", QStringLiteral("Apple")}, {"40:4D:7F", QStringLiteral("Apple")},
        {"40:56:9C", QStringLiteral("Apple")}, {"40:6C:8F", QStringLiteral("Apple")},
        {"40:70:09", QStringLiteral("Apple")}, {"40:83:DE", QStringLiteral("Apple")},
        {"40:98:AD", QStringLiteral("Apple")}, {"40:9C:28", QStringLiteral("Apple")},
        {"40:A6:D9", QStringLiteral("Apple")}, {"40:B3:95", QStringLiteral("Apple")},
        {"40:BC:60", QStringLiteral("Apple")}, {"40:C6:99", QStringLiteral("Apple")},
        {"40:D3:2D", QStringLiteral("Apple")}, {"44:00:10", QStringLiteral("Apple")},
        {"44:2A:60", QStringLiteral("Apple")}, {"44:4C:0C", QStringLiteral("Apple")},
        {"44:4C:AB", QStringLiteral("Apple")}, {"44:D8:84", QStringLiteral("Apple")},
        {"44:FB:B0", QStringLiteral("Apple")}, {"48:3B:38", QStringLiteral("Apple")},
        {"48:43:7C", QStringLiteral("Apple")}, {"48:4B:AA", QStringLiteral("Apple")},
        {"48:65:EE", QStringLiteral("Apple")}, {"48:74:6E", QStringLiteral("Apple")},
        {"48:A9:1C", QStringLiteral("Apple")}, {"48:BF:6B", QStringLiteral("Apple")},
        {"48:D7:05", QStringLiteral("Apple")}, {"4C:20:B8", QStringLiteral("Apple")},
        {"4C:32:75", QStringLiteral("Apple")}, {"4C:56:9D", QStringLiteral("Apple")},
        {"4C:57:CA", QStringLiteral("Apple")}, {"4C:74:BF", QStringLiteral("Apple")},
        {"4C:79:6F", QStringLiteral("Apple")}, {"4C:7C:5F", QStringLiteral("Apple")},
        {"4C:AB:33", QStringLiteral("Apple")}, {"4C:B1:99", QStringLiteral("Apple")},
        {"4C:BC:98", QStringLiteral("Apple")}, {"4C:F0:2E", QStringLiteral("Apple")},
        {"50:32:37", QStringLiteral("Apple")}, {"50:7A:55", QStringLiteral("Apple")},
        {"50:7A:C5", QStringLiteral("Apple")}, {"50:82:95", QStringLiteral("Apple")},
        {"50:DE:06", QStringLiteral("Apple")}, {"50:E4:0E", QStringLiteral("Apple")},
        {"50:EA:D6", QStringLiteral("Apple")}, {"50:ED:3C", QStringLiteral("Apple")},
        {"54:26:96", QStringLiteral("Apple")}, {"54:33:CB", QStringLiteral("Apple")},
        {"54:4E:90", QStringLiteral("Apple")}, {"54:72:4F", QStringLiteral("Apple")},
        {"54:9A:11", QStringLiteral("Apple")}, {"54:AE:27", QStringLiteral("Apple")},
        {"54:E4:3A", QStringLiteral("Apple")}, {"54:E6:1B", QStringLiteral("Apple")},
        {"58:1F:AA", QStringLiteral("Apple")}, {"58:40:4E", QStringLiteral("Apple")},
        {"58:55:CA", QStringLiteral("Apple")}, {"58:7F:57", QStringLiteral("Apple")},
        {"58:B0:35", QStringLiteral("Apple")}, {"58:E2:8F", QStringLiteral("Apple")},
        {"5C:09:47", QStringLiteral("Apple")}, {"5C:1B:F4", QStringLiteral("Apple")},
        {"5C:50:15", QStringLiteral("Apple")}, {"5C:52:30", QStringLiteral("Apple")},
        {"5C:70:A3", QStringLiteral("Apple")}, {"5C:87:30", QStringLiteral("Apple")},
        {"5C:95:5D", QStringLiteral("Apple")}, {"5C:95:AE", QStringLiteral("Apple")},
        {"5C:97:F3", QStringLiteral("Apple")}, {"5C:AD:CF", QStringLiteral("Apple")},
        {"5C:F5:DA", QStringLiteral("Apple")}, {"5C:F7:E6", QStringLiteral("Apple")},
        {"5C:F9:38", QStringLiteral("Apple")}, {"60:03:08", QStringLiteral("Apple")},
        {"60:33:4B", QStringLiteral("Apple")}, {"60:42:1C", QStringLiteral("Apple")},
        {"60:69:44", QStringLiteral("Apple")}, {"60:70:C0", QStringLiteral("Apple")},
        {"60:7C:2F", QStringLiteral("Apple")}, {"60:7E:C9", QStringLiteral("Apple")},
        {"60:8B:0E", QStringLiteral("Apple")}, {"60:92:17", QStringLiteral("Apple")},
        {"60:9A:C1", QStringLiteral("Apple")}, {"60:A3:7D", QStringLiteral("Apple")},
        {"60:C5:47", QStringLiteral("Apple")}, {"60:CD:A9", QStringLiteral("Apple")},
        {"60:D9:C7", QStringLiteral("Apple")}, {"60:DA:83", QStringLiteral("Apple")},
        {"60:F4:45", QStringLiteral("Apple")}, {"60:FB:42", QStringLiteral("Apple")},
        {"60:FE:C5", QStringLiteral("Apple")}, {"64:20:0C", QStringLiteral("Apple")},
        {"64:52:43", QStringLiteral("Apple")}, {"64:70:33", QStringLiteral("Apple")},
        {"64:76:BA", QStringLiteral("Apple")}, {"64:9A:BE", QStringLiteral("Apple")},
        {"64:A3:CB", QStringLiteral("Apple")}, {"64:A5:C3", QStringLiteral("Apple")},
        {"64:B0:A6", QStringLiteral("Apple")}, {"64:C7:53", QStringLiteral("Apple")},
        {"64:D2:10", QStringLiteral("Apple")}, {"64:E6:82", QStringLiteral("Apple")},
        {"68:09:27", QStringLiteral("Apple")}, {"68:5B:35", QStringLiteral("Apple")},
        {"68:64:4B", QStringLiteral("Apple")}, {"68:96:7B", QStringLiteral("Apple")},
        {"68:A8:6D", QStringLiteral("Apple")}, {"68:AE:20", QStringLiteral("Apple")},
        {"68:CA:34", QStringLiteral("Apple")}, {"68:CA:C4", QStringLiteral("Apple")},
        {"68:FB:7E", QStringLiteral("Apple")}, {"68:FE:F7", QStringLiteral("Apple")},
        {"6C:19:2F", QStringLiteral("Apple")}, {"6C:19:C0", QStringLiteral("Apple")},
        {"6C:2E:33", QStringLiteral("Apple")}, {"6C:40:08", QStringLiteral("Apple")},
        {"6C:4A:85", QStringLiteral("Apple")}, {"6C:70:9F", QStringLiteral("Apple")},
        {"6C:72:20", QStringLiteral("Apple")}, {"6C:72:E7", QStringLiteral("Apple")},
        {"6C:94:66", QStringLiteral("Apple")}, {"6C:94:F8", QStringLiteral("Apple")},
        {"6C:AB:31", QStringLiteral("Apple")}, {"6C:C2:6B", QStringLiteral("Apple")},
        {"6C:E8:5C", QStringLiteral("Apple")}, {"70:11:24", QStringLiteral("Apple")},
        {"70:14:A6", QStringLiteral("Apple")}, {"70:3C:69", QStringLiteral("Apple")},
        {"70:3E:AC", QStringLiteral("Apple")}, {"70:48:0F", QStringLiteral("Apple")},
        {"70:56:81", QStringLiteral("Apple")}, {"70:70:0D", QStringLiteral("Apple")},
        {"70:73:CB", QStringLiteral("Apple")}, {"70:81:05", QStringLiteral("Apple")},
        {"70:A2:83", QStringLiteral("Apple")}, {"70:D4:9E", QStringLiteral("Apple")},
        {"70:DE:E2", QStringLiteral("Apple")}, {"70:E7:2C", QStringLiteral("Apple")},
        {"70:EC:E4", QStringLiteral("Apple")}, {"74:1B:B2", QStringLiteral("Apple")},
        {"74:42:8B", QStringLiteral("Apple")}, {"74:81:4A", QStringLiteral("Apple")},
        {"74:8D:08", QStringLiteral("Apple")}, {"74:8F:3C", QStringLiteral("Apple")},
        {"78:28:CA", QStringLiteral("Apple")}, {"78:31:C1", QStringLiteral("Apple")},
        {"78:36:BC", QStringLiteral("Apple")}, {"78:3A:84", QStringLiteral("Apple")},
        {"78:41:5D", QStringLiteral("Apple")}, {"78:4F:43", QStringLiteral("Apple")},
        {"78:63:9C", QStringLiteral("Apple")}, {"78:64:C0", QStringLiteral("Apple")},
        {"78:67:D7", QStringLiteral("Apple")}, {"78:6C:1C", QStringLiteral("Apple")},
        {"78:7B:8A", QStringLiteral("Apple")}, {"78:7E:61", QStringLiteral("Apple")},
        {"78:88:6D", QStringLiteral("Apple")}, {"78:8F:CF", QStringLiteral("Apple")},
        {"78:9F:70", QStringLiteral("Apple")}, {"78:A3:E4", QStringLiteral("Apple")},
        {"78:A8:73", QStringLiteral("Apple")}, {"78:CA:39", QStringLiteral("Apple")},
        {"78:D7:5F", QStringLiteral("Apple")}, {"78:FD:94", QStringLiteral("Apple")},
        {"7C:01:91", QStringLiteral("Apple")}, {"7C:04:D0", QStringLiteral("Apple")},
        {"7C:11:BE", QStringLiteral("Apple")}, {"7C:1E:52", QStringLiteral("Apple")},
        {"7C:2E:0D", QStringLiteral("Apple")}, {"7C:50:49", QStringLiteral("Apple")},
        {"7C:6D:62", QStringLiteral("Apple")}, {"7C:6D:F8", QStringLiteral("Apple")},
        {"7C:9A:1D", QStringLiteral("Apple")}, {"7C:A1:AE", QStringLiteral("Apple")},
        {"7C:B0:C2", QStringLiteral("Apple")}, {"7C:C3:A1", QStringLiteral("Apple")},
        {"7C:C5:37", QStringLiteral("Apple")}, {"7C:D1:C3", QStringLiteral("Apple")},
        {"7C:E4:AA", QStringLiteral("Apple")}, {"7C:F0:5F", QStringLiteral("Apple")},
        {"80:00:6E", QStringLiteral("Apple")}, {"80:19:34", QStringLiteral("Apple")},
        {"80:2A:A8", QStringLiteral("Apple")}, {"80:2C:73", QStringLiteral("Apple")},
        {"80:49:71", QStringLiteral("Apple")}, {"80:82:23", QStringLiteral("Apple")},
        {"80:8E:71", QStringLiteral("Apple")}, {"80:92:9F", QStringLiteral("Apple")},
        {"80:9B:20", QStringLiteral("Apple")}, {"80:A0:BF", QStringLiteral("Apple")},
        {"80:AD:67", QStringLiteral("Apple")}, {"80:B0:3D", QStringLiteral("Apple")},
        {"80:BE:05", QStringLiteral("Apple")}, {"80:D6:05", QStringLiteral("Apple")},
        {"80:E6:50", QStringLiteral("Apple")}, {"80:EA:07", QStringLiteral("Apple")},
        {"80:ED:2C", QStringLiteral("Apple")}, {"80:F0:3B", QStringLiteral("Apple")},
        {"80:F6:2E", QStringLiteral("Apple")}, {"84:29:99", QStringLiteral("Apple")},
        {"84:38:35", QStringLiteral("Apple")}, {"84:3A:4B", QStringLiteral("Apple")},
        {"84:41:67", QStringLiteral("Apple")}, {"84:57:80", QStringLiteral("Apple")},
        {"84:6A:DF", QStringLiteral("Apple")}, {"84:78:AC", QStringLiteral("Apple")},
        {"84:85:06", QStringLiteral("Apple")}, {"84:89:AD", QStringLiteral("Apple")},
        {"84:8E:0C", QStringLiteral("Apple")}, {"84:8E:DF", QStringLiteral("Apple")},
        {"84:A1:34", QStringLiteral("Apple")}, {"84:AB:1A", QStringLiteral("Apple")},
        {"84:B1:53", QStringLiteral("Apple")}, {"84:FC:AC", QStringLiteral("Apple")},
        {"84:FE:9E", QStringLiteral("Apple")}, {"88:19:08", QStringLiteral("Apple")},
        {"88:1F:A1", QStringLiteral("Apple")}, {"88:53:2E", QStringLiteral("Apple")},
        {"88:53:95", QStringLiteral("Apple")}, {"88:63:DF", QStringLiteral("Apple")},
        {"88:64:40", QStringLiteral("Apple")}, {"88:66:A5", QStringLiteral("Apple")},
        {"88:6B:6E", QStringLiteral("Apple")}, {"88:6B:F0", QStringLiteral("Apple")},
        {"88:75:98", QStringLiteral("Apple")}, {"88:7F:03", QStringLiteral("Apple")},
        {"88:A4:79", QStringLiteral("Apple")}, {"88:A9:B7", QStringLiteral("Apple")},
        {"88:A9:E0", QStringLiteral("Apple")}, {"88:AE:07", QStringLiteral("Apple")},
        {"88:B1:11", QStringLiteral("Apple")}, {"88:B2:91", QStringLiteral("Apple")},
        {"88:C6:63", QStringLiteral("Apple")}, {"88:CB:87", QStringLiteral("Apple")},
        {"88:E8:7F", QStringLiteral("Apple")}, {"88:E9:FE", QStringLiteral("Apple")},
        {"8C:00:6D", QStringLiteral("Apple")}, {"8C:2D:AA", QStringLiteral("Apple")},
        {"8C:58:77", QStringLiteral("Apple")}, {"8C:7B:9D", QStringLiteral("Apple")},
        {"8C:7C:92", QStringLiteral("Apple")}, {"8C:85:90", QStringLiteral("Apple")},
        {"8C:86:1E", QStringLiteral("Apple")}, {"8C:8E:F2", QStringLiteral("Apple")},
        {"8C:8F:8A", QStringLiteral("Apple")}, {"8C:99:E6", QStringLiteral("Apple")},
        {"8C:9E:FF", QStringLiteral("Apple")}, {"8C:A9:82", QStringLiteral("Apple")},
        {"8C:B8:2C", QStringLiteral("Apple")}, {"8C:FA:BA", QStringLiteral("Apple")},
        {"90:08:7B", QStringLiteral("Apple")}, {"90:0C:B2", QStringLiteral("Apple")},
        {"90:27:E4", QStringLiteral("Apple")}, {"90:3C:92", QStringLiteral("Apple")},
        {"90:60:14", QStringLiteral("Apple")}, {"90:72:40", QStringLiteral("Apple")},
        {"90:81:2A", QStringLiteral("Apple")}, {"90:84:0D", QStringLiteral("Apple")},
        {"90:8D:6C", QStringLiteral("Apple")}, {"90:9C:4A", QStringLiteral("Apple")},
        {"90:A2:5B", QStringLiteral("Apple")}, {"90:B0:ED", QStringLiteral("Apple")},
        {"90:B2:1F", QStringLiteral("Apple")}, {"90:B9:31", QStringLiteral("Apple")},
        {"90:C1:C6", QStringLiteral("Apple")}, {"90:CC:DF", QStringLiteral("Apple")},
        {"90:DD:5D", QStringLiteral("Apple")}, {"90:E0:F0", QStringLiteral("Apple")},
        {"90:EB:50", QStringLiteral("Apple")}, {"90:F0:52", QStringLiteral("Apple")},
        {"90:FD:61", QStringLiteral("Apple")}, {"94:16:25", QStringLiteral("Apple")},
        {"94:4B:D1", QStringLiteral("Apple")}, {"94:5F:72", QStringLiteral("Apple")},
        {"94:76:0E", QStringLiteral("Apple")}, {"94:94:26", QStringLiteral("Apple")},
        {"94:9B:FE", QStringLiteral("Apple")}, {"94:BF:2D", QStringLiteral("Apple")},
        {"94:BF:95", QStringLiteral("Apple")}, {"94:D0:23", QStringLiteral("Apple")},
        {"94:DD:3F", QStringLiteral("Apple")}, {"94:E9:6A", QStringLiteral("Apple")},
        {"94:EA:32", QStringLiteral("Apple")}, {"94:F6:3E", QStringLiteral("Apple")},
        {"94:FB:29", QStringLiteral("Apple")}, {"98:00:3D", QStringLiteral("Apple")},
        {"98:01:A7", QStringLiteral("Apple")}, {"98:03:D8", QStringLiteral("Apple")},
        {"98:0C:82", QStringLiteral("Apple")}, {"98:10:E8", QStringLiteral("Apple")},
        {"98:2B:8D", QStringLiteral("Apple")}, {"98:2D:68", QStringLiteral("Apple")},
        {"98:3E:BF", QStringLiteral("Apple")}, {"98:5A:EB", QStringLiteral("Apple")},
        {"98:60:16", QStringLiteral("Apple")}, {"98:7E:46", QStringLiteral("Apple")},
        {"98:8F:2B", QStringLiteral("Apple")}, {"98:9E:63", QStringLiteral("Apple")},
        {"98:9F:0C", QStringLiteral("Apple")}, {"98:B8:E3", QStringLiteral("Apple")},
        {"98:CA:33", QStringLiteral("Apple")}, {"98:D6:BB", QStringLiteral("Apple")},
        {"98:D8:8C", QStringLiteral("Apple")}, {"98:E0:D9", QStringLiteral("Apple")},
        {"98:F0:AB", QStringLiteral("Apple")}, {"98:F5:A9", QStringLiteral("Apple")},
        {"98:FE:94", QStringLiteral("Apple")}, {"9C:04:EB", QStringLiteral("Apple")},
        {"9C:20:7B", QStringLiteral("Apple")}, {"9C:29:3F", QStringLiteral("Apple")},
        {"9C:2D:CD", QStringLiteral("Apple")}, {"9C:35:EB", QStringLiteral("Apple")},
        {"9C:3E:53", QStringLiteral("Apple")}, {"9C:4F:DA", QStringLiteral("Apple")},
        {"9C:53:22", QStringLiteral("Apple")}, {"9C:58:3C", QStringLiteral("Apple")},
        {"9C:64:8B", QStringLiteral("Apple")}, {"9C:6C:15", QStringLiteral("Apple")},
        {"9C:84:BF", QStringLiteral("Apple")}, {"9C:F3:87", QStringLiteral("Apple")},
        {"9C:F4:8E", QStringLiteral("Apple")}, {"9C:F4:AB", QStringLiteral("Apple")},
        {"9C:FC:01", QStringLiteral("Apple")}, {"9C:FC:28", QStringLiteral("Apple")},
        {"A0:18:28", QStringLiteral("Apple")}, {"A0:4E:A7", QStringLiteral("Apple")},
        {"A0:56:F3", QStringLiteral("Apple")}, {"A0:78:17", QStringLiteral("Apple")},
        {"A0:9B:85", QStringLiteral("Apple")}, {"A0:A3:09", QStringLiteral("Apple")},
        {"A0:D7:95", QStringLiteral("Apple")}, {"A0:ED:CD", QStringLiteral("Apple")},
        {"A0:F3:C1", QStringLiteral("Apple")}, {"A0:F9:E0", QStringLiteral("Apple")},
        {"A0:FC:6E", QStringLiteral("Apple")}, {"A4:0B:ED", QStringLiteral("Apple")},
        {"A4:12:42", QStringLiteral("Apple")}, {"A4:31:35", QStringLiteral("Apple")},
        {"A4:5E:60", QStringLiteral("Apple")}, {"A4:67:06", QStringLiteral("Apple")},
        {"A4:83:E7", QStringLiteral("Apple")}, {"A4:9A:58", QStringLiteral("Apple")},
        {"A4:B1:97", QStringLiteral("Apple")}, {"A4:B8:05", QStringLiteral("Apple")},
        {"A4:C3:61", QStringLiteral("Apple")}, {"A4:D1:8C", QStringLiteral("Apple")},
        {"A4:D1:D2", QStringLiteral("Apple")}, {"A4:D3:DB", QStringLiteral("Apple")},
        {"A4:E9:75", QStringLiteral("Apple")}, {"A4:F1:E8", QStringLiteral("Apple")},
        {"A8:20:66", QStringLiteral("Apple")}, {"A8:5B:78", QStringLiteral("Apple")},
        {"A8:60:B6", QStringLiteral("Apple")}, {"A8:86:DD", QStringLiteral("Apple")},
        {"A8:8E:24", QStringLiteral("Apple")}, {"A8:96:8A", QStringLiteral("Apple")},
        {"A8:9A:93", QStringLiteral("Apple")}, {"A8:9B:10", QStringLiteral("Apple")},
        {"A8:9E:24", QStringLiteral("Apple")}, {"A8:BB:CF", QStringLiteral("Apple")},
        {"A8:BE:27", QStringLiteral("Apple")}, {"A8:C6:9A", QStringLiteral("Apple")},
        {"A8:F0:5C", QStringLiteral("Apple")}, {"A8:F4:7A", QStringLiteral("Apple")},
        {"AC:15:F4", QStringLiteral("Apple")}, {"AC:1D:06", QStringLiteral("Apple")},
        {"AC:1F:74", QStringLiteral("Apple")}, {"AC:29:3A", QStringLiteral("Apple")},
        {"AC:3C:0B", QStringLiteral("Apple")}, {"AC:3C:8B", QStringLiteral("Apple")},
        {"AC:44:F2", QStringLiteral("Apple")}, {"AC:49:DB", QStringLiteral("Apple")},
        {"AC:61:EA", QStringLiteral("Apple")}, {"AC:64:62", QStringLiteral("Apple")},
        {"AC:67:5D", QStringLiteral("Apple")}, {"AC:7F:3E", QStringLiteral("Apple")},
        {"AC:87:A3", QStringLiteral("Apple")}, {"AC:88:FD", QStringLiteral("Apple")},
        {"AC:8B:8E", QStringLiteral("Apple")}, {"AC:90:85", QStringLiteral("Apple")},
        {"AC:BC:32", QStringLiteral("Apple")}, {"AC:C3:5A", QStringLiteral("Apple")},
        {"AC:CF:5C", QStringLiteral("Apple")}, {"AC:D1:B8", QStringLiteral("Apple")},
        {"AC:E4:B5", QStringLiteral("Apple")}, {"AC:FD:EC", QStringLiteral("Apple")},
        {"B0:19:C6", QStringLiteral("Apple")}, {"B0:34:95", QStringLiteral("Apple")},
        {"B0:35:9F", QStringLiteral("Apple")}, {"B0:48:1A", QStringLiteral("Apple")},
        {"B0:5C:E5", QStringLiteral("Apple")}, {"B0:65:BD", QStringLiteral("Apple")},
        {"B0:70:2D", QStringLiteral("Apple")}, {"B0:7B:8C", QStringLiteral("Apple")},
        {"B0:8C:6F", QStringLiteral("Apple")}, {"B0:8C:75", QStringLiteral("Apple")},
        {"B0:91:22", QStringLiteral("Apple")}, {"B0:9F:BA", QStringLiteral("Apple")},
        {"B0:A6:F5", QStringLiteral("Apple")}, {"B0:AC:FA", QStringLiteral("Apple")},
        {"B0:B2:DC", QStringLiteral("Apple")}, {"B0:B9:8F", QStringLiteral("Apple")},
        {"B0:BE:83", QStringLiteral("Apple")}, {"B0:C6:9A", QStringLiteral("Apple")},
        {"B0:CA:68", QStringLiteral("Apple")}, {"B0:D8:2E", QStringLiteral("Apple")},
        {"B0:F1:BC", QStringLiteral("Apple")}, {"B4:18:D1", QStringLiteral("Apple")},
        {"B4:1C:30", QStringLiteral("Apple")}, {"B4:40:A4", QStringLiteral("Apple")},
        {"B4:4B:D2", QStringLiteral("Apple")}, {"B4:53:86", QStringLiteral("Apple")},
        {"B4:5A:4A", QStringLiteral("Apple")}, {"B4:73:0C", QStringLiteral("Apple")},
        {"B4:8B:19", QStringLiteral("Apple")}, {"B4:9C:DF", QStringLiteral("Apple")},
        {"B4:A9:FC", QStringLiteral("Apple")}, {"B4:B0:17", QStringLiteral("Apple")},
        {"B4:B5:2F", QStringLiteral("Apple")}, {"B4:C8:10", QStringLiteral("Apple")},
        {"B4:E1:EB", QStringLiteral("Apple")}, {"B4:F0:AB", QStringLiteral("Apple")},
        {"B4:F6:1C", QStringLiteral("Apple")}, {"B8:09:8A", QStringLiteral("Apple")},
        {"B8:17:C2", QStringLiteral("Apple")}, {"B8:1B:A5", QStringLiteral("Apple")},
        {"B8:41:A4", QStringLiteral("Apple")}, {"B8:44:D9", QStringLiteral("Apple")},
        {"B8:53:AC", QStringLiteral("Apple")}, {"B8:5D:0A", QStringLiteral("Apple")},
        {"B8:63:4D", QStringLiteral("Apple")}, {"B8:6B:23", QStringLiteral("Apple")},
        {"B8:78:79", QStringLiteral("Apple")}, {"B8:7C:6F", QStringLiteral("Apple")},
        {"B8:81:FA", QStringLiteral("Apple")}, {"B8:8C:29", QStringLiteral("Apple")},
        {"B8:90:47", QStringLiteral("Apple")}, {"B8:9B:C9", QStringLiteral("Apple")},
        {"B8:9F:04", QStringLiteral("Apple")}, {"B8:A6:0E", QStringLiteral("Apple")},
        {"B8:C1:11", QStringLiteral("Apple")}, {"B8:C7:5D", QStringLiteral("Apple")},
        {"B8:E8:56", QStringLiteral("Apple")}, {"B8:F1:2A", QStringLiteral("Apple")},
        {"B8:F6:B1", QStringLiteral("Apple")}, {"B8:FF:7A", QStringLiteral("Apple")},
        {"BC:09:1B", QStringLiteral("Apple")}, {"BC:0F:9B", QStringLiteral("Apple")},
        {"BC:15:AC", QStringLiteral("Apple")}, {"BC:20:BA", QStringLiteral("Apple")},
        {"BC:2E:48", QStringLiteral("Apple")}, {"BC:3B:AF", QStringLiteral("Apple")},
        {"BC:3C:69", QStringLiteral("Apple")}, {"BC:4C:C4", QStringLiteral("Apple")},
        {"BC:52:B7", QStringLiteral("Apple")}, {"BC:54:36", QStringLiteral("Apple")},
        {"BC:54:51", QStringLiteral("Apple")}, {"BC:5B:6F", QStringLiteral("Apple")},
        {"BC:67:1C", QStringLiteral("Apple")}, {"BC:67:78", QStringLiteral("Apple")},
        {"BC:6C:21", QStringLiteral("Apple")}, {"BC:83:85", QStringLiteral("Apple")},
        {"BC:92:6B", QStringLiteral("Apple")}, {"BC:9F:EF", QStringLiteral("Apple")},
        {"BC:A5:8B", QStringLiteral("Apple")}, {"BC:A8:A6", QStringLiteral("Apple")},
        {"BC:A9:20", QStringLiteral("Apple")}, {"BC:AF:91", QStringLiteral("Apple")},
        {"BC:B8:63", QStringLiteral("Apple")}, {"BC:BA:C2", QStringLiteral("Apple")},
        {"BC:BF:58", QStringLiteral("Apple")}, {"BC:C3:44", QStringLiteral("Apple")},
        {"BC:D0:74", QStringLiteral("Apple")}, {"BC:E1:43", QStringLiteral("Apple")},
        {"BC:E8:8F", QStringLiteral("Apple")}, {"BC:EC:5D", QStringLiteral("Apple")},
        {"BC:F4:3E", QStringLiteral("Apple")}, {"BC:F5:AC", QStringLiteral("Apple")},
        {"BC:F6:10", QStringLiteral("Apple")}, {"BC:FC:E7", QStringLiteral("Apple")},
        {"C0:1B:24", QStringLiteral("Apple")}, {"C0:42:D0", QStringLiteral("Apple")},
        {"C0:63:94", QStringLiteral("Apple")}, {"C0:65:99", QStringLiteral("Apple")},
        {"C0:84:7A", QStringLiteral("Apple")}, {"C0:8C:71", QStringLiteral("Apple")},
        {"C0:91:0B", QStringLiteral("Apple")}, {"C0:98:79", QStringLiteral("Apple")},
        {"C0:9F:05", QStringLiteral("Apple")}, {"C0:A0:BB", QStringLiteral("Apple")},
        {"C0:A5:3E", QStringLiteral("Apple")}, {"C0:A6:00", QStringLiteral("Apple")},
        {"C0:B6:58", QStringLiteral("Apple")}, {"C0:BD:D1", QStringLiteral("Apple")},
        {"C0:C9:76", QStringLiteral("Apple")}, {"C0:CC:F8", QStringLiteral("Apple")},
        {"C0:CE:CD", QStringLiteral("Apple")}, {"C0:D0:12", QStringLiteral("Apple")},
        {"C0:D6:9A", QStringLiteral("Apple")}, {"C0:E2:BE", QStringLiteral("Apple")},
        {"C0:E8:62", QStringLiteral("Apple")}, {"C0:F2:FB", QStringLiteral("Apple")},
        {"C0:F9:46", QStringLiteral("Apple")}, {"C4:0B:31", QStringLiteral("Apple")},
        {"C4:2C:03", QStringLiteral("Apple")}, {"C4:6B:5F", QStringLiteral("Apple")},
        {"C4:84:66", QStringLiteral("Apple")}, {"C4:85:08", QStringLiteral("Apple")},
        {"C4:91:0C", QStringLiteral("Apple")}, {"C4:98:80", QStringLiteral("Apple")},
        {"C4:9A:02", QStringLiteral("Apple")}, {"C4:B3:01", QStringLiteral("Apple")},
        {"C4:E9:2F", QStringLiteral("Apple")}, {"C8:09:A8", QStringLiteral("Apple")},
        {"C8:1E:E7", QStringLiteral("Apple")}, {"C8:33:4B", QStringLiteral("Apple")},
        {"C8:3C:85", QStringLiteral("Apple")}, {"C8:5C:C2", QStringLiteral("Apple")},
        {"C8:5E:77", QStringLiteral("Apple")}, {"C8:69:CD", QStringLiteral("Apple")},
        {"C8:6F:1D", QStringLiteral("Apple")}, {"C8:85:50", QStringLiteral("Apple")},
        {"C8:BC:C8", QStringLiteral("Apple")}, {"C8:BC:DF", QStringLiteral("Apple")},
        {"C8:D0:83", QStringLiteral("Apple")}, {"C8:E0:EB", QStringLiteral("Apple")},
        {"C8:F0:8E", QStringLiteral("Apple")}, {"C8:F6:5C", QStringLiteral("Apple")},
        {"CC:08:8D", QStringLiteral("Apple")}, {"CC:08:FA", QStringLiteral("Apple")},
        {"CC:20:AF", QStringLiteral("Apple")}, {"CC:25:EF", QStringLiteral("Apple")},
        {"CC:29:F5", QStringLiteral("Apple")}, {"CC:2D:B7", QStringLiteral("Apple")},
        {"CC:44:63", QStringLiteral("Apple")}, {"CC:6A:4C", QStringLiteral("Apple")},
        {"CC:78:5F", QStringLiteral("Apple")}, {"CC:97:91", QStringLiteral("Apple")},
        {"CC:99:8E", QStringLiteral("Apple")}, {"CC:9F:7A", QStringLiteral("Apple")},
        {"CC:A7:C1", QStringLiteral("Apple")}, {"CC:C4:60", QStringLiteral("Apple")},
        {"CC:CC:81", QStringLiteral("Apple")}, {"CC:D0:83", QStringLiteral("Apple")},
        {"CC:E2:6B", QStringLiteral("Apple")}, {"CC:EC:DF", QStringLiteral("Apple")},
        {"CC:F0:3A", QStringLiteral("Apple")}, {"CC:FC:6D", QStringLiteral("Apple")},
        {"D0:03:4B", QStringLiteral("Apple")}, {"D0:15:4A", QStringLiteral("Apple")},
        {"D0:17:69", QStringLiteral("Apple")}, {"D0:1B:49", QStringLiteral("Apple")},
        {"D0:23:DB", QStringLiteral("Apple")}, {"D0:25:98", QStringLiteral("Apple")},
        {"D0:2F:9E", QStringLiteral("Apple")}, {"D0:33:11", QStringLiteral("Apple")},
        {"D0:4D:2C", QStringLiteral("Apple")}, {"D0:5A:0F", QStringLiteral("Apple")},
        {"D0:81:7A", QStringLiteral("Apple")}, {"D0:88:0C", QStringLiteral("Apple")},
        {"D0:8E:79", QStringLiteral("Apple")}, {"D0:9E:CD", QStringLiteral("Apple")},
        {"D0:A4:0A", QStringLiteral("Apple")}, {"D0:B3:3F", QStringLiteral("Apple")},
        {"D0:C5:F3", QStringLiteral("Apple")}, {"D0:D2:B0", QStringLiteral("Apple")},
        {"D0:D3:FC", QStringLiteral("Apple")}, {"D0:E1:40", QStringLiteral("Apple")},
        {"D4:1A:3F", QStringLiteral("Apple")}, {"D4:46:37", QStringLiteral("Apple")},
        {"D4:54:17", QStringLiteral("Apple")}, {"D4:61:2E", QStringLiteral("Apple")},
        {"D4:61:9D", QStringLiteral("Apple")}, {"D4:61:DA", QStringLiteral("Apple")},
        {"D4:6A:6A", QStringLiteral("Apple")}, {"D4:6B:A6", QStringLiteral("Apple")},
        {"D4:7B:75", QStringLiteral("Apple")}, {"D4:8D:9D", QStringLiteral("Apple")},
        {"D4:90:9C", QStringLiteral("Apple")}, {"D4:94:5A", QStringLiteral("Apple")},
        {"D4:96:DF", QStringLiteral("Apple")}, {"D4:9A:20", QStringLiteral("Apple")},
        {"D4:9B:5C", QStringLiteral("Apple")}, {"D4:9D:E3", QStringLiteral("Apple")},
        {"D4:A3:3D", QStringLiteral("Apple")}, {"D4:DC:CD", QStringLiteral("Apple")},
        {"D4:E8:AA", QStringLiteral("Apple")}, {"D4:F4:6F", QStringLiteral("Apple")},
        {"D4:F7:35", QStringLiteral("Apple")}, {"D8:00:4D", QStringLiteral("Apple")},
        {"D8:1C:79", QStringLiteral("Apple")}, {"D8:1D:72", QStringLiteral("Apple")},
        {"D8:30:62", QStringLiteral("Apple")}, {"D8:3C:69", QStringLiteral("Apple")},
        {"D8:4D:10", QStringLiteral("Apple")}, {"D8:4D:7C", QStringLiteral("Apple")},
        {"D8:5D:4C", QStringLiteral("Apple")}, {"D8:5D:84", QStringLiteral("Apple")},
        {"D8:6C:02", QStringLiteral("Apple")}, {"D8:8F:CA", QStringLiteral("Apple")},
        {"D8:96:95", QStringLiteral("Apple")}, {"D8:9E:3F", QStringLiteral("Apple")},
        {"D8:A1:4E", QStringLiteral("Apple")}, {"D8:A2:5E", QStringLiteral("Apple")},
        {"D8:BB:2C", QStringLiteral("Apple")}, {"D8:C3:0E", QStringLiteral("Apple")},
        {"D8:CF:9C", QStringLiteral("Apple")}, {"D8:D1:CC", QStringLiteral("Apple")},
        {"D8:DC:40", QStringLiteral("Apple")}, {"D8:DE:3A", QStringLiteral("Apple")},
        {"D8:E0:E1", QStringLiteral("Apple")}, {"D8:E2:DF", QStringLiteral("Apple")},
        {"D8:E5:6D", QStringLiteral("Apple")}, {"D8:F0:5C", QStringLiteral("Apple")},
        {"D8:F1:5B", QStringLiteral("Apple")}, {"D8:F7:10", QStringLiteral("Apple")},
        {"DC:08:56", QStringLiteral("Apple")}, {"DC:0B:1A", QStringLiteral("Apple")},
        {"DC:0B:34", QStringLiteral("Apple")}, {"DC:0C:5C", QStringLiteral("Apple")},
        {"DC:0C:8C", QStringLiteral("Apple")}, {"DC:1A:01", QStringLiteral("Apple")},
        {"DC:1D:30", QStringLiteral("Apple")}, {"DC:2B:2A", QStringLiteral("Apple")},
        {"DC:2B:61", QStringLiteral("Apple")}, {"DC:2B:CA", QStringLiteral("Apple")},
        {"DC:37:14", QStringLiteral("Apple")}, {"DC:41:5F", QStringLiteral("Apple")},
        {"DC:41:A9", QStringLiteral("Apple")}, {"DC:52:85", QStringLiteral("Apple")},
        {"DC:53:60", QStringLiteral("Apple")}, {"DC:56:E7", QStringLiteral("Apple")},
        {"DC:86:D8", QStringLiteral("Apple")}, {"DC:9C:52", QStringLiteral("Apple")},
        {"DC:A4:CA", QStringLiteral("Apple")}, {"DC:A9:04", QStringLiteral("Apple")},
        {"DC:AC:19", QStringLiteral("Apple")}, {"DC:B3:0A", QStringLiteral("Apple")},
        {"DC:B3:3A", QStringLiteral("Apple")}, {"DC:BE:6A", QStringLiteral("Apple")},
        {"DC:C6:98", QStringLiteral("Apple")}, {"DC:CD:2C", QStringLiteral("Apple")},
        {"DC:D3:A2", QStringLiteral("Apple")}, {"DC:D8:4D", QStringLiteral("Apple")},
        {"DC:E5:7B", QStringLiteral("Apple")}, {"DC:E5:7F", QStringLiteral("Apple")},
        {"DC:EC:06", QStringLiteral("Apple")}, {"DC:F4:CA", QStringLiteral("Apple")},
        {"DC:FC:86", QStringLiteral("Apple")}, {"E0:06:E6", QStringLiteral("Apple")},
        {"E0:19:1D", QStringLiteral("Apple")}, {"E0:2B:96", QStringLiteral("Apple")},
        {"E0:33:8E", QStringLiteral("Apple")}, {"E0:3C:5B", QStringLiteral("Apple")},
        {"E0:50:8B", QStringLiteral("Apple")}, {"E0:5F:45", QStringLiteral("Apple")},
        {"E0:66:78", QStringLiteral("Apple")}, {"E0:6D:17", QStringLiteral("Apple")},
        {"E0:73:E7", QStringLiteral("Apple")}, {"E0:7F:53", QStringLiteral("Apple")},
        {"E0:89:7E", QStringLiteral("Apple")}, {"E0:8E:3C", QStringLiteral("Apple")},
        {"E0:98:06", QStringLiteral("Apple")}, {"E0:9A:BF", QStringLiteral("Apple")},
        {"E0:9C:8A", QStringLiteral("Apple")}, {"E0:9D:13", QStringLiteral("Apple")},
        {"E0:9F:5A", QStringLiteral("Apple")}, {"E0:A1:98", QStringLiteral("Apple")},
        {"E0:A6:5D", QStringLiteral("Apple")}, {"E0:AC:CB", QStringLiteral("Apple")},
        {"E0:B5:2D", QStringLiteral("Apple")}, {"E0:B9:BA", QStringLiteral("Apple")},
        {"E0:BB:0E", QStringLiteral("Apple")}, {"E0:C3:53", QStringLiteral("Apple")},
        {"E0:C7:67", QStringLiteral("Apple")}, {"E0:C9:7A", QStringLiteral("Apple")},
        {"E0:D0:25", QStringLiteral("Apple")}, {"E0:D8:1A", QStringLiteral("Apple")},
        {"E0:E0:50", QStringLiteral("Apple")}, {"E0:EB:40", QStringLiteral("Apple")},
        {"E0:ED:05", QStringLiteral("Apple")}, {"E0:F5:C6", QStringLiteral("Apple")},
        {"E0:F8:47", QStringLiteral("Apple")}, {"E0:FD:68", QStringLiteral("Apple")},
        {"E4:25:E7", QStringLiteral("Apple")}, {"E4:2B:34", QStringLiteral("Apple")},
        {"E4:2B:79", QStringLiteral("Apple")}, {"E4:36:7B", QStringLiteral("Apple")},
        {"E4:46:BD", QStringLiteral("Apple")}, {"E4:50:EB", QStringLiteral("Apple")},
        {"E4:6B:33", QStringLiteral("Apple")}, {"E4:76:84", QStringLiteral("Apple")},
        {"E4:7E:9A", QStringLiteral("Apple")}, {"E4:80:62", QStringLiteral("Apple")},
        {"E4:8B:7F", QStringLiteral("Apple")}, {"E4:8D:8C", QStringLiteral("Apple")},
        {"E4:90:7E", QStringLiteral("Apple")}, {"E4:98:D6", QStringLiteral("Apple")},
        {"E4:9A:79", QStringLiteral("Apple")}, {"E4:9A:DC", QStringLiteral("Apple")},
        {"E4:9C:67", QStringLiteral("Apple")}, {"E4:9E:12", QStringLiteral("Apple")},
        {"E4:A7:C5", QStringLiteral("Apple")}, {"E4:AC:45", QStringLiteral("Apple")},
        {"E4:B0:21", QStringLiteral("Apple")}, {"E4:B2:FB", QStringLiteral("Apple")},
        {"E4:CE:8F", QStringLiteral("Apple")}, {"E4:D3:F1", QStringLiteral("Apple")},
        {"E4:E0:4C", QStringLiteral("Apple")}, {"E4:E4:AB", QStringLiteral("Apple")},
        {"E4:F7:A1", QStringLiteral("Apple")}, {"E8:04:0B", QStringLiteral("Apple")},
        {"E8:04:62", QStringLiteral("Apple")}, {"E8:06:88", QStringLiteral("Apple")},
        {"E8:0E:3B", QStringLiteral("Apple")}, {"E8:1C:4B", QStringLiteral("Apple")},
        {"E8:20:E2", QStringLiteral("Apple")}, {"E8:2A:44", QStringLiteral("Apple")},
        {"E8:36:17", QStringLiteral("Apple")}, {"E8:5A:8B", QStringLiteral("Apple")},
        {"E8:6B:EA", QStringLiteral("Apple")}, {"E8:73:5A", QStringLiteral("Apple")},
        {"E8:78:29", QStringLiteral("Apple")}, {"E8:80:2E", QStringLiteral("Apple")},
        {"E8:87:A3", QStringLiteral("Apple")}, {"E8:8D:28", QStringLiteral("Apple")},
        {"E8:99:C4", QStringLiteral("Apple")}, {"E8:9C:25", QStringLiteral("Apple")},
        {"E8:A7:30", QStringLiteral("Apple")}, {"E8:AA:CF", QStringLiteral("Apple")},
        {"E8:B2:AC", QStringLiteral("Apple")}, {"E8:B4:7E", QStringLiteral("Apple")},
        {"E8:B6:58", QStringLiteral("Apple")}, {"E8:BE:81", QStringLiteral("Apple")},
        {"E8:C7:4A", QStringLiteral("Apple")}, {"E8:D0:FA", QStringLiteral("Apple")},
        {"E8:DB:84", QStringLiteral("Apple")}, {"E8:E0:66", QStringLiteral("Apple")},
        {"E8:F1:B0", QStringLiteral("Apple")}, {"E8:F2:E2", QStringLiteral("Apple")},
        {"E8:F4:26", QStringLiteral("Apple")}, {"E8:F9:28", QStringLiteral("Apple")},
        {"E8:FA:2A", QStringLiteral("Apple")}, {"EC:1A:03", QStringLiteral("Apple")},
        {"EC:1F:72", QStringLiteral("Apple")}, {"EC:2C:E2", QStringLiteral("Apple")},
        {"EC:35:86", QStringLiteral("Apple")}, {"EC:52:82", QStringLiteral("Apple")},
        {"EC:54:4E", QStringLiteral("Apple")}, {"EC:55:6F", QStringLiteral("Apple")},
        {"EC:5C:68", QStringLiteral("Apple")}, {"EC:60:73", QStringLiteral("Apple")},
        {"EC:73:79", QStringLiteral("Apple")}, {"EC:7C:B6", QStringLiteral("Apple")},
        {"EC:85:2F", QStringLiteral("Apple")}, {"EC:8A:4C", QStringLiteral("Apple")},
        {"EC:9A:0C", QStringLiteral("Apple")}, {"EC:A0:7C", QStringLiteral("Apple")},
        {"EC:A8:6B", QStringLiteral("Apple")}, {"EC:A9:04", QStringLiteral("Apple")},
        {"EC:AD:B8", QStringLiteral("Apple")}, {"EC:B1:D7", QStringLiteral("Apple")},
        {"EC:B5:87", QStringLiteral("Apple")}, {"EC:BE:DC", QStringLiteral("Apple")},
        {"EC:D0:DC", QStringLiteral("Apple")}, {"EC:DE:3D", QStringLiteral("Apple")},
        {"EC:E9:78", QStringLiteral("Apple")}, {"EC:EA:03", QStringLiteral("Apple")},
        {"EC:F0:0E", QStringLiteral("Apple")}, {"F0:18:98", QStringLiteral("Apple")},
        {"F0:24:75", QStringLiteral("Apple")}, {"F0:2F:4B", QStringLiteral("Apple")},
        {"F0:2F:74", QStringLiteral("Apple")}, {"F0:3F:95", QStringLiteral("Apple")},
        {"F0:43:47", QStringLiteral("Apple")}, {"F0:5C:D5", QStringLiteral("Apple")},
        {"F0:63:F9", QStringLiteral("Apple")}, {"F0:6B:CA", QStringLiteral("Apple")},
        {"F0:72:8C", QStringLiteral("Apple")}, {"F0:79:59", QStringLiteral("Apple")},
        {"F0:7B:CB", QStringLiteral("Apple")}, {"F0:81:AF", QStringLiteral("Apple")},
        {"F0:83:2C", QStringLiteral("Apple")}, {"F0:98:9D", QStringLiteral("Apple")},
        {"F0:99:BF", QStringLiteral("Apple")}, {"F0:9C:BB", QStringLiteral("Apple")},
        {"F0:9F:C2", QStringLiteral("Apple")}, {"F0:A0:83", QStringLiteral("Apple")},
        {"F0:A6:0A", QStringLiteral("Apple")}, {"F0:B0:E7", QStringLiteral("Apple")},
        {"F0:B4:79", QStringLiteral("Apple")}, {"F0:BB:5C", QStringLiteral("Apple")},
        {"F0:BE:3C", QStringLiteral("Apple")}, {"F0:C1:F1", QStringLiteral("Apple")},
        {"F0:C3:71", QStringLiteral("Apple")}, {"F0:CB:A1", QStringLiteral("Apple")},
        {"F0:CD:5C", QStringLiteral("Apple")}, {"F0:D1:A9", QStringLiteral("Apple")},
        {"F0:D5:BF", QStringLiteral("Apple")}, {"F0:DB:E2", QStringLiteral("Apple")},
        {"F0:DB:F8", QStringLiteral("Apple")}, {"F0:DC:E2", QStringLiteral("Apple")},
        {"F0:DE:71", QStringLiteral("Apple")}, {"F0:DE:F1", QStringLiteral("Apple")},
        {"F0:E9:DC", QStringLiteral("Apple")}, {"F0:EE:10", QStringLiteral("Apple")},
        {"F0:F6:1C", QStringLiteral("Apple")}, {"F0:F6:69", QStringLiteral("Apple")},
        {"F0:F7:55", QStringLiteral("Apple")}, {"F4:06:16", QStringLiteral("Apple")},
        {"F4:0F:24", QStringLiteral("Apple")}, {"F4:0F:9B", QStringLiteral("Apple")},
        {"F4:1B:A1", QStringLiteral("Apple")}, {"F4:31:C3", QStringLiteral("Apple")},
        {"F4:34:F0", QStringLiteral("Apple")}, {"F4:37:B7", QStringLiteral("Apple")},
        {"F4:5C:89", QStringLiteral("Apple")}, {"F4:63:9D", QStringLiteral("Apple")},
        {"F4:65:A6", QStringLiteral("Apple")}, {"F4:6B:EF", QStringLiteral("Apple")},
        {"F4:70:AB", QStringLiteral("Apple")}, {"F4:8C:EB", QStringLiteral("Apple")},
        {"F4:A3:95", QStringLiteral("Apple")}, {"F4:A4:D6", QStringLiteral("Apple")},
        {"F4:AF:E7", QStringLiteral("Apple")}, {"F4:B5:2F", QStringLiteral("Apple")},
        {"F4:B8:5E", QStringLiteral("Apple")}, {"F4:BD:9E", QStringLiteral("Apple")},
        {"F4:CD:90", QStringLiteral("Apple")}, {"F4:D4:88", QStringLiteral("Apple")},
        {"F4:E0:53", QStringLiteral("Apple")}, {"F4:F1:5A", QStringLiteral("Apple")},
        {"F4:F9:51", QStringLiteral("Apple")}, {"F4:FC:32", QStringLiteral("Apple")},
        {"F8:03:77", QStringLiteral("Apple")}, {"F8:04:2E", QStringLiteral("Apple")},
        {"F8:0D:AC", QStringLiteral("Apple")}, {"F8:1E:DF", QStringLiteral("Apple")},
        {"F8:27:93", QStringLiteral("Apple")}, {"F8:28:19", QStringLiteral("Apple")},
        {"F8:2D:7C", QStringLiteral("Apple")}, {"F8:38:80", QStringLiteral("Apple")},
        {"F8:3F:51", QStringLiteral("Apple")}, {"F8:43:47", QStringLiteral("Apple")},
        {"F8:4D:89", QStringLiteral("Apple")}, {"F8:4E:73", QStringLiteral("Apple")},
        {"F8:53:29", QStringLiteral("Apple")}, {"F8:5E:A0", QStringLiteral("Apple")},
        {"F8:62:AA", QStringLiteral("Apple")}, {"F8:65:5A", QStringLiteral("Apple")},
        {"F8:66:5A", QStringLiteral("Apple")}, {"F8:6F:C1", QStringLiteral("Apple")},
        {"F8:73:A2", QStringLiteral("Apple")}, {"F8:79:0A", QStringLiteral("Apple")},
        {"F8:7B:62", QStringLiteral("Apple")}, {"F8:7D:76", QStringLiteral("Apple")},
        {"F8:87:F1", QStringLiteral("Apple")}, {"F8:95:EA", QStringLiteral("Apple")},
        {"F8:9C:94", QStringLiteral("Apple")}, {"F8:A0:BF", QStringLiteral("Apple")},
        {"F8:A2:D6", QStringLiteral("Apple")}, {"F8:A9:D0", QStringLiteral("Apple")},
        {"F8:AC:6D", QStringLiteral("Apple")}, {"F8:B1:56", QStringLiteral("Apple")},
        {"F8:BB:BF", QStringLiteral("Apple")}, {"F8:C3:9E", QStringLiteral("Apple")},
        {"F8:C9:6C", QStringLiteral("Apple")}, {"F8:CA:B8", QStringLiteral("Apple")},
        {"F8:CF:C5", QStringLiteral("Apple")}, {"F8:D0:27", QStringLiteral("Apple")},
        {"F8:D1:11", QStringLiteral("Apple")}, {"F8:E0:79", QStringLiteral("Apple")},
        {"F8:E6:1A", QStringLiteral("Apple")}, {"F8:E9:4E", QStringLiteral("Apple")},
        {"F8:F0:05", QStringLiteral("Apple")}, {"F8:F7:D3", QStringLiteral("Apple")},
        {"F8:FF:C2", QStringLiteral("Apple")}, {"FC:01:7C", QStringLiteral("Apple")},
        {"FC:0A:81", QStringLiteral("Apple")}, {"FC:18:3C", QStringLiteral("Apple")},
        {"FC:1D:43", QStringLiteral("Apple")}, {"FC:1D:59", QStringLiteral("Apple")},
        {"FC:1D:84", QStringLiteral("Apple")}, {"FC:25:3F", QStringLiteral("Apple")},
        {"FC:2A:9C", QStringLiteral("Apple")}, {"FC:2D:5E", QStringLiteral("Apple")},
        {"FC:2E:2D", QStringLiteral("Apple")}, {"FC:42:03", QStringLiteral("Apple")},
        {"FC:5C:EE", QStringLiteral("Apple")}, {"FC:66:CF", QStringLiteral("Apple")},
        {"FC:6A:3A", QStringLiteral("Apple")}, {"FC:82:53", QStringLiteral("Apple")},
        {"FC:AA:81", QStringLiteral("Apple")}, {"FC:B0:C4", QStringLiteral("Apple")},
        {"FC:B3:BC", QStringLiteral("Apple")}, {"FC:BC:9C", QStringLiteral("Apple")},
        {"FC:C2:DE", QStringLiteral("Apple")}, {"FC:CD:55", QStringLiteral("Apple")},
        {"FC:D8:48", QStringLiteral("Apple")}, {"FC:DA:5C", QStringLiteral("Apple")},
        {"FC:DC:0C", QStringLiteral("Apple")}, {"FC:E9:98", QStringLiteral("Apple")},
        {"FC:EB:69", QStringLiteral("Apple")}, {"FC:ED:5D", QStringLiteral("Apple")},
        {"FC:F8:AE", QStringLiteral("Apple")}, {"FC:FC:48", QStringLiteral("Apple")},
        // Intel
        {"00:02:B3", QStringLiteral("Intel")}, {"00:03:47", QStringLiteral("Intel")},
        {"00:04:23", QStringLiteral("Intel")}, {"00:07:E9", QStringLiteral("Intel")},
        {"00:0A:0D", QStringLiteral("Intel")}, {"00:0B:AB", QStringLiteral("Intel")},
        {"00:0C:F1", QStringLiteral("Intel")}, {"00:0D:60", QStringLiteral("Intel")},
        {"00:0E:0C", QStringLiteral("Intel")}, {"00:11:11", QStringLiteral("Intel")},
        {"00:12:F0", QStringLiteral("Intel")}, {"00:13:02", QStringLiteral("Intel")},
        {"00:15:00", QStringLiteral("Intel")}, {"00:16:41", QStringLiteral("Intel")},
        {"00:16:EA", QStringLiteral("Intel")}, {"00:18:DE", QStringLiteral("Intel")},
        {"00:19:D1", QStringLiteral("Intel")}, {"00:1A:6B", QStringLiteral("Intel")},
        {"00:1B:21", QStringLiteral("Intel")}, {"00:1B:77", QStringLiteral("Intel")},
        {"00:1C:BF", QStringLiteral("Intel")}, {"00:1E:64", QStringLiteral("Intel")},
        {"00:1F:3B", QStringLiteral("Intel")}, {"00:21:5C", QStringLiteral("Intel")},
        {"00:23:13", QStringLiteral("Intel")}, {"00:24:D7", QStringLiteral("Intel")},
        {"00:26:C6", QStringLiteral("Intel")}, {"04:7C:16", QStringLiteral("Intel")},
        {"0C:54:A5", QStringLiteral("Intel")}, {"0C:8B:FD", QStringLiteral("Intel")},
        {"0C:D7:46", QStringLiteral("Intel")}, {"10:02:B5", QStringLiteral("Intel")},
        {"10:3D:0E", QStringLiteral("Intel")}, {"18:3D:A2", QStringLiteral("Intel")},
        {"18:59:36", QStringLiteral("Intel")}, {"1C:1B:B5", QStringLiteral("Intel")},
        {"1C:3A:DE", QStringLiteral("Intel")}, {"1C:4B:D6", QStringLiteral("Intel")},
        {"1C:69:7A", QStringLiteral("Intel")}, {"1C:99:57", QStringLiteral("Intel")},
        {"1C:BF:CE", QStringLiteral("Intel")}, {"20:16:B9", QStringLiteral("Intel")},
        {"20:79:72", QStringLiteral("Intel")}, {"24:FD:52", QStringLiteral("Intel")},
        {"28:16:AD", QStringLiteral("Intel")}, {"28:7D:E8", QStringLiteral("Intel")},
        {"28:9A:4D", QStringLiteral("Intel")}, {"2C:54:CF", QStringLiteral("Intel")},
        {"2C:6A:6F", QStringLiteral("Intel")}, {"30:3A:64", QStringLiteral("Intel")},
        {"30:45:96", QStringLiteral("Intel")}, {"34:13:E8", QStringLiteral("Intel")},
        {"34:1F:EB", QStringLiteral("Intel")}, {"34:27:92", QStringLiteral("Intel")},
        {"34:7D:E4", QStringLiteral("Intel")}, {"34:CF:F6", QStringLiteral("Intel")},
        {"34:DE:1A", QStringLiteral("Intel")}, {"34:E1:2D", QStringLiteral("Intel")},
        {"34:E6:AD", QStringLiteral("Intel")}, {"34:F3:9A", QStringLiteral("Intel")},
        {"38:00:25", QStringLiteral("Intel")}, {"38:7A:CC", QStringLiteral("Intel")},
        {"38:DE:AD", QStringLiteral("Intel")}, {"3C:6A:A7", QStringLiteral("Intel")},
        {"3C:A0:67", QStringLiteral("Intel")}, {"3C:F8:62", QStringLiteral("Intel")},
        {"40:23:43", QStringLiteral("Intel")}, {"40:49:0F", QStringLiteral("Intel")},
        {"40:74:E0", QStringLiteral("Intel")}, {"40:98:4E", QStringLiteral("Intel")},
        {"40:B0:34", QStringLiteral("Intel")}, {"40:EC:99", QStringLiteral("Intel")},
        {"44:1C:12", QStringLiteral("Intel")}, {"44:2C:05", QStringLiteral("Intel")},
        {"44:85:00", QStringLiteral("Intel")}, {"48:51:C5", QStringLiteral("Intel")},
        {"48:89:68", QStringLiteral("Intel")}, {"48:A4:72", QStringLiteral("Intel")},
        {"48:E7:DA", QStringLiteral("Intel")}, {"4C:1D:96", QStringLiteral("Intel")},
        {"4C:3B:DF", QStringLiteral("Intel")}, {"4C:4B:F9", QStringLiteral("Intel")},
        {"4C:52:62", QStringLiteral("Intel")}, {"4C:79:75", QStringLiteral("Intel")},
        {"4C:8B:30", QStringLiteral("Intel")}, {"4C:8B:55", QStringLiteral("Intel")},
        {"4C:B9:8E", QStringLiteral("Intel")}, {"4C:BB:58", QStringLiteral("Intel")},
        {"4C:EB:42", QStringLiteral("Intel")}, {"4C:ED:DE", QStringLiteral("Intel")},
        {"4C:F5:5D", QStringLiteral("Intel")}, {"50:2F:A8", QStringLiteral("Intel")},
        {"50:3E:AA", QStringLiteral("Intel")}, {"50:51:03", QStringLiteral("Intel")},
        {"50:5A:65", QStringLiteral("Intel")}, {"50:5B:C2", QStringLiteral("Intel")},
        {"50:8A:06", QStringLiteral("Intel")}, {"50:A6:7F", QStringLiteral("Intel")},
        {"50:AF:73", QStringLiteral("Intel")}, {"50:B7:C3", QStringLiteral("Intel")},
        {"50:DA:00", QStringLiteral("Intel")}, {"50:DB:3D", QStringLiteral("Intel")},
        {"50:DC:E7", QStringLiteral("Intel")}, {"50:E0:85", QStringLiteral("Intel")},
        {"50:EB:71", QStringLiteral("Intel")}, {"50:EF:9C", QStringLiteral("Intel")},
        {"54:02:71", QStringLiteral("Intel")}, {"54:13:79", QStringLiteral("Intel")},
        {"54:14:73", QStringLiteral("Intel")}, {"54:35:30", QStringLiteral("Intel")},
        {"54:42:49", QStringLiteral("Intel")}, {"54:48:10", QStringLiteral("Intel")},
        {"54:5A:A6", QStringLiteral("Intel")}, {"54:8D:5A", QStringLiteral("Intel")},
        {"54:92:BE", QStringLiteral("Intel")}, {"54:9D:85", QStringLiteral("Intel")},
        {"54:AB:3A", QStringLiteral("Intel")}, {"54:B8:0A", QStringLiteral("Intel")},
        {"54:C9:DF", QStringLiteral("Intel")}, {"54:E1:AD", QStringLiteral("Intel")},
        {"58:10:D2", QStringLiteral("Intel")}, {"58:2A:D5", QStringLiteral("Intel")},
        {"58:8A:5A", QStringLiteral("Intel")}, {"58:94:6B", QStringLiteral("Intel")},
        {"58:96:1D", QStringLiteral("Intel")}, {"58:9C:FC", QStringLiteral("Intel")},
        {"5C:02:14", QStringLiteral("Intel")}, {"5C:51:4F", QStringLiteral("Intel")},
        {"5C:5B:35", QStringLiteral("Intel")}, {"5C:80:B6", QStringLiteral("Intel")},
        {"5C:85:7E", QStringLiteral("Intel")}, {"5C:87:9C", QStringLiteral("Intel")},
        {"5C:AD:76", QStringLiteral("Intel")}, {"5C:AF:06", QStringLiteral("Intel")},
        {"5C:D4:61", QStringLiteral("Intel")}, {"5C:DC:96", QStringLiteral("Intel")},
        {"5C:E0:C5", QStringLiteral("Intel")}, {"5C:F3:70", QStringLiteral("Intel")},
        {"5C:F8:21", QStringLiteral("Intel")}, {"60:1D:0F", QStringLiteral("Intel")},
        {"60:1D:91", QStringLiteral("Intel")}, {"60:21:74", QStringLiteral("Intel")},
        {"60:22:30", QStringLiteral("Intel")}, {"60:30:21", QStringLiteral("Intel")},
        {"60:3E:5F", QStringLiteral("Intel")}, {"60:45:CB", QStringLiteral("Intel")},
        {"60:4A:1C", QStringLiteral("Intel")}, {"60:6C:66", QStringLiteral("Intel")},
        {"60:9C:9A", QStringLiteral("Intel")}, {"60:A8:FE", QStringLiteral("Intel")},
        {"60:B3:C4", QStringLiteral("Intel")}, {"60:BE:B5", QStringLiteral("Intel")},
        {"60:D2:50", QStringLiteral("Intel")}, {"60:D9:A0", QStringLiteral("Intel")},
        {"60:DD:8E", QStringLiteral("Intel")}, {"60:E0:0E", QStringLiteral("Intel")},
        {"60:F1:8A", QStringLiteral("Intel")}, {"60:F2:62", QStringLiteral("Intel")},
        {"60:F6:77", QStringLiteral("Intel")}, {"60:FA:CD", QStringLiteral("Intel")},
        {"64:00:6A", QStringLiteral("Intel")}, {"64:00:F1", QStringLiteral("Intel")},
        {"64:32:A4", QStringLiteral("Intel")}, {"64:4B:F0", QStringLiteral("Intel")},
        {"64:5A:04", QStringLiteral("Intel")}, {"64:5D:86", QStringLiteral("Intel")},
        {"64:66:33", QStringLiteral("Intel")}, {"64:70:02", QStringLiteral("Intel")},
        {"64:85:2D", QStringLiteral("Intel")}, {"64:9C:81", QStringLiteral("Intel")},
        {"64:A2:F9", QStringLiteral("Intel")}, {"64:A6:51", QStringLiteral("Intel")},
        {"64:B4:73", QStringLiteral("Intel")}, {"64:BC:0C", QStringLiteral("Intel")},
        {"64:BC:58", QStringLiteral("Intel")}, {"64:D4:BD", QStringLiteral("Intel")},
        {"64:D8:60", QStringLiteral("Intel")}, {"64:DB:43", QStringLiteral("Intel")},
        {"64:E0:3A", QStringLiteral("Intel")}, {"64:E5:99", QStringLiteral("Intel")},
        {"64:E8:81", QStringLiteral("Intel")}, {"64:EB:8C", QStringLiteral("Intel")},
        {"64:F0:47", QStringLiteral("Intel")}, {"64:F6:9D", QStringLiteral("Intel")},
        {"64:F9:65", QStringLiteral("Intel")}, {"68:07:15", QStringLiteral("Intel")},
        {"68:14:01", QStringLiteral("Intel")}, {"68:5D:43", QStringLiteral("Intel")},
        {"68:7F:74", QStringLiteral("Intel")}, {"68:94:23", QStringLiteral("Intel")},
        {"68:A3:C4", QStringLiteral("Intel")}, {"68:EC:C5", QStringLiteral("Intel")},
        {"6C:0B:84", QStringLiteral("Intel")}, {"6C:5C:14", QStringLiteral("Intel")},
        {"6C:88:14", QStringLiteral("Intel")}, {"70:03:9F", QStringLiteral("Intel")},
        {"70:1A:04", QStringLiteral("Intel")}, {"70:1C:E7", QStringLiteral("Intel")},
        {"70:2B:1D", QStringLiteral("Intel")}, {"70:35:09", QStringLiteral("Intel")},
        {"70:3A:51", QStringLiteral("Intel")}, {"70:5A:AC", QStringLiteral("Intel")},
        {"70:62:6B", QStringLiteral("Intel")}, {"70:77:81", QStringLiteral("Intel")},
        {"70:85:C2", QStringLiteral("Intel")}, {"70:8B:CD", QStringLiteral("Intel")},
        {"70:AB:8B", QStringLiteral("Intel")}, {"70:C7:F2", QStringLiteral("Intel")},
        // Dell
        {"00:06:5B", QStringLiteral("Dell")}, {"00:08:74", QStringLiteral("Dell")},
        {"00:0B:DB", QStringLiteral("Dell")}, {"00:0D:56", QStringLiteral("Dell")},
        {"00:0F:1F", QStringLiteral("Dell")}, {"00:11:43", QStringLiteral("Dell")},
        {"00:12:3F", QStringLiteral("Dell")}, {"00:13:72", QStringLiteral("Dell")},
        {"00:14:22", QStringLiteral("Dell")}, {"00:15:C5", QStringLiteral("Dell")},
        {"00:18:8B", QStringLiteral("Dell")}, {"00:19:B9", QStringLiteral("Dell")},
        {"00:1A:A0", QStringLiteral("Dell")}, {"00:1C:23", QStringLiteral("Dell")},
        {"00:1D:09", QStringLiteral("Dell")}, {"00:1E:4F", QStringLiteral("Dell")},
        {"00:1E:C9", QStringLiteral("Dell")}, {"00:21:70", QStringLiteral("Dell")},
        {"00:21:9B", QStringLiteral("Dell")}, {"00:22:19", QStringLiteral("Dell")},
        {"00:23:AE", QStringLiteral("Dell")}, {"00:24:E8", QStringLiteral("Dell")},
        {"00:25:64", QStringLiteral("Dell")}, {"00:26:B9", QStringLiteral("Dell")},
        {"18:03:73", QStringLiteral("Dell")}, {"18:26:6C", QStringLiteral("Dell")},
        {"18:DB:F2", QStringLiteral("Dell")}, {"1C:1B:68", QStringLiteral("Dell")},
        {"24:6E:96", QStringLiteral("Dell")}, {"28:F1:0E", QStringLiteral("Dell")},
        {"34:17:EB", QStringLiteral("Dell")}, {"34:48:ED", QStringLiteral("Dell")},
        {"34:E6:D7", QStringLiteral("Dell")}, {"3C:2C:30", QStringLiteral("Dell")},
        {"48:2A:E3", QStringLiteral("Dell")}, {"4C:76:25", QStringLiteral("Dell")},
        {"50:9A:4C", QStringLiteral("Dell")}, {"54:BF:64", QStringLiteral("Dell")},
        {"78:2B:CB", QStringLiteral("Dell")}, {"78:45:58", QStringLiteral("Dell")},
        {"84:7B:EB", QStringLiteral("Dell")}, {"84:8F:69", QStringLiteral("Dell")},
        {"84:A9:3E", QStringLiteral("Dell")}, {"90:B1:1C", QStringLiteral("Dell")},
        {"98:90:96", QStringLiteral("Dell")}, {"9C:DA:3E", QStringLiteral("Dell")},
        {"A4:4C:C8", QStringLiteral("Dell")}, {"A4:5D:36", QStringLiteral("Dell")},
        {"A4:BA:DB", QStringLiteral("Dell")}, {"B0:83:FE", QStringLiteral("Dell")},
        {"B8:2A:72", QStringLiteral("Dell")}, {"B8:AC:6F", QStringLiteral("Dell")},
        {"C8:1F:66", QStringLiteral("Dell")}, {"D0:67:E5", QStringLiteral("Dell")},
        {"D0:94:66", QStringLiteral("Dell")}, {"D4:3D:7E", QStringLiteral("Dell")},
        {"D4:81:D7", QStringLiteral("Dell")}, {"D4:BE:D9", QStringLiteral("Dell")},
        {"D8:9E:F3", QStringLiteral("Dell")}, {"F0:4D:A2", QStringLiteral("Dell")},
        {"F4:8E:38", QStringLiteral("Dell")}, {"F8:BC:12", QStringLiteral("Dell")},
        {"F8:DB:88", QStringLiteral("Dell")},
        // HP
        {"00:02:A5", QStringLiteral("HP")}, {"00:0B:CD", QStringLiteral("HP")},
        {"00:0E:7F", QStringLiteral("HP")}, {"00:10:83", QStringLiteral("HP")},
        {"00:11:0A", QStringLiteral("HP")}, {"00:11:85", QStringLiteral("HP")},
        {"00:13:21", QStringLiteral("HP")}, {"00:14:38", QStringLiteral("HP")},
        {"00:15:60", QStringLiteral("HP")}, {"00:16:35", QStringLiteral("HP")},
        {"00:17:A4", QStringLiteral("HP")}, {"00:18:FE", QStringLiteral("HP")},
        {"00:19:BB", QStringLiteral("HP")}, {"00:1A:4B", QStringLiteral("HP")},
        {"00:1B:78", QStringLiteral("HP")}, {"00:1C:C4", QStringLiteral("HP")},
        {"00:1D:0D", QStringLiteral("HP")}, {"00:1E:0B", QStringLiteral("HP")},
        {"00:1F:29", QStringLiteral("HP")}, {"00:21:5A", QStringLiteral("HP")},
        {"00:21:FE", QStringLiteral("HP")}, {"00:22:64", QStringLiteral("HP")},
        {"00:23:7D", QStringLiteral("HP")}, {"00:24:81", QStringLiteral("HP")},
        {"00:25:B3", QStringLiteral("HP")}, {"00:26:55", QStringLiteral("HP")},
        {"00:30:C1", QStringLiteral("HP")}, {"04:0E:3C", QStringLiteral("HP")},
        {"10:60:4B", QStringLiteral("HP")}, {"14:58:D0", QStringLiteral("HP")},
        {"18:60:24", QStringLiteral("HP")}, {"1C:98:EC", QStringLiteral("HP")},
        {"20:47:47", QStringLiteral("HP")}, {"28:92:4A", QStringLiteral("HP")},
        {"2C:44:FD", QStringLiteral("HP")}, {"2C:59:E5", QStringLiteral("HP")},
        {"2C:76:8A", QStringLiteral("HP")}, {"30:0D:43", QStringLiteral("HP")},
        {"30:C3:D9", QStringLiteral("HP")}, {"34:64:A9", QStringLiteral("HP")},
        {"34:FC:EF", QStringLiteral("HP")}, {"38:63:BB", QStringLiteral("HP")},
        {"38:7A:0E", QStringLiteral("HP")}, {"38:CA:97", QStringLiteral("HP")},
        {"38:D5:47", QStringLiteral("HP")}, {"38:E7:0D", QStringLiteral("HP")},
        {"3C:52:82", QStringLiteral("HP")}, {"3C:A8:2A", QStringLiteral("HP")},
        {"40:01:C6", QStringLiteral("HP")}, {"40:2E:28", QStringLiteral("HP")},
        {"40:5D:82", QStringLiteral("HP")}, {"40:80:5A", QStringLiteral("HP")},
        {"44:1E:A1", QStringLiteral("HP")}, {"44:31:92", QStringLiteral("HP")},
        {"44:48:C9", QStringLiteral("HP")}, {"48:5B:39", QStringLiteral("HP")},
        {"48:5D:60", QStringLiteral("HP")}, {"48:BA:4E", QStringLiteral("HP")},
        {"48:DF:37", QStringLiteral("HP")}, {"4C:0B:BE", QStringLiteral("HP")},
        {"50:65:F3", QStringLiteral("HP")}, {"54:05:AB", QStringLiteral("HP")},
        {"54:80:28", QStringLiteral("HP")}, {"58:20:B1", QStringLiteral("HP")},
        {"5C:CB:99", QStringLiteral("HP")}, {"60:14:66", QStringLiteral("HP")},
        {"60:6D:C7", QStringLiteral("HP")}, {"64:16:66", QStringLiteral("HP")},
        {"64:31:50", QStringLiteral("HP")}, {"64:80:99", QStringLiteral("HP")},
        {"68:B5:99", QStringLiteral("HP")}, {"6C:AE:8B", QStringLiteral("HP")},
        {"6C:BE:E9", QStringLiteral("HP")}, {"70:10:5C", QStringLiteral("HP")},
        {"74:46:A0", QStringLiteral("HP")},
        // VMware (虚拟化)
        {"00:0C:29", QStringLiteral("VMware")},
        {"00:50:56", QStringLiteral("VMware")},
        {"00:05:69", QStringLiteral("VMware")},
        {"00:1C:14", QStringLiteral("VMware")},
        // VirtualBox (虚拟化)
        {"08:00:27", QStringLiteral("VirtualBox")},
        // Microsoft / Hyper-V
        {"00:15:5D", QStringLiteral("Microsoft (Hyper-V)")},
        {"00:03:FF", QStringLiteral("Microsoft")},
        {"00:0D:3A", QStringLiteral("Microsoft")},
        {"00:12:5A", QStringLiteral("Microsoft")},
        {"00:17:FA", QStringLiteral("Microsoft")},
        {"00:1D:D8", QStringLiteral("Microsoft")},
        {"00:22:48", QStringLiteral("Microsoft")},
        {"00:25:AE", QStringLiteral("Microsoft")},
        {"04:0D:50", QStringLiteral("Microsoft")},
        {"28:18:78", QStringLiteral("Microsoft")},
        {"30:59:B7", QStringLiteral("Microsoft")},
        {"50:1A:C5", QStringLiteral("Microsoft")},
        {"58:82:A8", QStringLiteral("Microsoft")},
        {"60:45:BD", QStringLiteral("Microsoft")},
        {"7C:1E:52", QStringLiteral("Microsoft")},
        {"98:5F:D3", QStringLiteral("Microsoft")},
        {"B0:2C:68", QStringLiteral("Microsoft")},
        {"C0:33:5E", QStringLiteral("Microsoft")},
        {"D4:85:64", QStringLiteral("Microsoft")},
        {"E0:3F:49", QStringLiteral("Microsoft")},
        {"EC:8E:B5", QStringLiteral("Microsoft")},
        {"F0:1D:BC", QStringLiteral("Microsoft")},
        {"F4:52:14", QStringLiteral("Microsoft")},
        // TP-Link
        {"00:0C:42", QStringLiteral("TP-Link")}, {"00:19:E0", QStringLiteral("TP-Link")},
        {"00:1A:70", QStringLiteral("TP-Link")}, {"00:1D:0F", QStringLiteral("TP-Link")},
        {"00:21:27", QStringLiteral("TP-Link")}, {"00:23:CD", QStringLiteral("TP-Link")},
        {"00:25:86", QStringLiteral("TP-Link")}, {"00:27:19", QStringLiteral("TP-Link")},
        {"10:FE:ED", QStringLiteral("TP-Link")}, {"14:CC:20", QStringLiteral("TP-Link")},
        {"30:B5:C2", QStringLiteral("TP-Link")}, {"50:C7:BF", QStringLiteral("TP-Link")},
        {"94:0C:6D", QStringLiteral("TP-Link")}, {"B0:95:75", QStringLiteral("TP-Link")},
        {"C4:6E:1F", QStringLiteral("TP-Link")}, {"E8:94:F6", QStringLiteral("TP-Link")},
        // Netgear
        {"00:14:6C", QStringLiteral("Netgear")}, {"00:18:4D", QStringLiteral("Netgear")},
        {"00:1F:33", QStringLiteral("Netgear")}, {"00:22:3F", QStringLiteral("Netgear")},
        {"00:24:B2", QStringLiteral("Netgear")}, {"04:A1:51", QStringLiteral("Netgear")},
        {"08:36:C9", QStringLiteral("Netgear")}, {"10:0D:7F", QStringLiteral("Netgear")},
        {"14:59:C0", QStringLiteral("Netgear")}, {"20:0C:C8", QStringLiteral("Netgear")},
        {"20:E5:2A", QStringLiteral("Netgear")}, {"28:10:7B", QStringLiteral("Netgear")},
        {"28:C6:8E", QStringLiteral("Netgear")}, {"2C:30:33", QStringLiteral("Netgear")},
        {"30:46:9A", QStringLiteral("Netgear")}, {"44:94:FC", QStringLiteral("Netgear")},
        {"50:6F:9A", QStringLiteral("Netgear")}, {"6C:B0:CE", QStringLiteral("Netgear")},
        {"80:37:29", QStringLiteral("Netgear")}, {"84:1B:5E", QStringLiteral("Netgear")},
        {"8C:3A:E3", QStringLiteral("Netgear")}, {"A0:21:B7", QStringLiteral("Netgear")},
        {"A0:4E:01", QStringLiteral("Netgear")}, {"B0:39:56", QStringLiteral("Netgear")},
        {"B0:7F:B9", QStringLiteral("Netgear")}, {"C4:04:15", QStringLiteral("Netgear")},
        {"D8:49:4B", QStringLiteral("Netgear")}, {"E0:46:9A", QStringLiteral("Netgear")},
        {"E4:F4:C6", QStringLiteral("Netgear")}, {"F8:73:94", QStringLiteral("Netgear")},
        // D-Link
        {"00:0D:88", QStringLiteral("D-Link")}, {"00:11:95", QStringLiteral("D-Link")},
        {"00:13:46", QStringLiteral("D-Link")}, {"00:17:9A", QStringLiteral("D-Link")},
        {"00:1A:73", QStringLiteral("D-Link")}, {"00:1B:11", QStringLiteral("D-Link")},
        {"00:1C:7E", QStringLiteral("D-Link")}, {"00:1E:58", QStringLiteral("D-Link")},
        {"00:22:B0", QStringLiteral("D-Link")}, {"00:24:01", QStringLiteral("D-Link")},
        {"00:26:5A", QStringLiteral("D-Link")}, {"04:8D:38", QStringLiteral("D-Link")},
        {"0C:17:62", QStringLiteral("D-Link")}, {"10:6F:3F", QStringLiteral("D-Link")},
        {"14:D6:4D", QStringLiteral("D-Link")}, {"18:9C:27", QStringLiteral("D-Link")},
        {"1C:5F:2B", QStringLiteral("D-Link")}, {"20:5E:F7", QStringLiteral("D-Link")},
        {"24:05:0F", QStringLiteral("D-Link")}, {"28:10:1B", QStringLiteral("D-Link")},
        // Sony
        {"00:1A:8C", QStringLiteral("Sony")}, {"00:24:BE", QStringLiteral("Sony")},
        {"04:46:65", QStringLiteral("Sony")}, {"08:76:FF", QStringLiteral("Sony")},
        {"0C:3C:65", QStringLiteral("Sony")}, {"10:4F:A8", QStringLiteral("Sony")},
        {"18:26:66", QStringLiteral("Sony")}, {"1C:5A:3E", QStringLiteral("Sony")},
        {"20:3A:07", QStringLiteral("Sony")}, {"24:4C:07", QStringLiteral("Sony")},
        {"28:5F:DB", QStringLiteral("Sony")}, {"2C:33:7A", QStringLiteral("Sony")},
        {"34:02:86", QStringLiteral("Sony")}, {"38:1A:52", QStringLiteral("Sony")},
        {"3C:07:71", QStringLiteral("Sony")}, {"40:2B:50", QStringLiteral("Sony")},
        {"44:03:2C", QStringLiteral("Sony")}, {"48:5A:3F", QStringLiteral("Sony")},
        {"4C:0D:EE", QStringLiteral("Sony")}, {"50:50:54", QStringLiteral("Sony")},
        // Xiaomi
        {"04:5A:95", QStringLiteral("Xiaomi")}, {"04:DB:8A", QStringLiteral("Xiaomi")},
        {"08:10:74", QStringLiteral("Xiaomi")}, {"08:26:AE", QStringLiteral("Xiaomi")},
        {"08:3E:8E", QStringLiteral("Xiaomi")}, {"08:5B:0E", QStringLiteral("Xiaomi")},
        {"08:6E:E2", QStringLiteral("Xiaomi")}, {"08:70:45", QStringLiteral("Xiaomi")},
        {"08:7B:12", QStringLiteral("Xiaomi")}, {"08:86:3B", QStringLiteral("Xiaomi")},
        {"08:8F:2C", QStringLiteral("Xiaomi")}, {"08:93:5A", QStringLiteral("Xiaomi")},
        {"08:9E:01", QStringLiteral("Xiaomi")}, {"08:A0:56", QStringLiteral("Xiaomi")},
        {"08:A6:BC", QStringLiteral("Xiaomi")}, {"08:AC:92", QStringLiteral("Xiaomi")},
        {"08:B4:2A", QStringLiteral("Xiaomi")}, {"08:B5:4D", QStringLiteral("Xiaomi")},
        {"08:BB:8C", QStringLiteral("Xiaomi")}, {"08:BC:20", QStringLiteral("Xiaomi")},
        {"34:CD:6D", QStringLiteral("Xiaomi")}, {"38:A2:8C", QStringLiteral("Xiaomi")},
        {"40:31:3C", QStringLiteral("Xiaomi")}, {"4C:63:71", QStringLiteral("Xiaomi")},
        {"50:64:2B", QStringLiteral("Xiaomi")}, {"54:27:8E", QStringLiteral("Xiaomi")},
        {"64:09:80", QStringLiteral("Xiaomi")}, {"64:CC:2E", QStringLiteral("Xiaomi")},
        {"68:DF:DD", QStringLiteral("Xiaomi")}, {"78:02:B7", QStringLiteral("Xiaomi")},
        // Samsung
        {"00:00:F0", QStringLiteral("Samsung")}, {"00:07:AB", QStringLiteral("Samsung")},
        {"00:0D:E5", QStringLiteral("Samsung")}, {"00:12:47", QStringLiteral("Samsung")},
        {"00:15:99", QStringLiteral("Samsung")}, {"00:16:6B", QStringLiteral("Samsung")},
        {"00:16:DB", QStringLiteral("Samsung")}, {"00:17:C9", QStringLiteral("Samsung")},
        {"00:18:AF", QStringLiteral("Samsung")}, {"00:1A:8A", QStringLiteral("Samsung")},
        {"00:1B:98", QStringLiteral("Samsung")}, {"00:1C:43", QStringLiteral("Samsung")},
        {"00:1D:25", QStringLiteral("Samsung")}, {"00:1D:F6", QStringLiteral("Samsung")},
        {"00:1E:7D", QStringLiteral("Samsung")}, {"00:1F:CC", QStringLiteral("Samsung")},
        {"00:21:19", QStringLiteral("Samsung")}, {"00:23:39", QStringLiteral("Samsung")},
        {"00:24:54", QStringLiteral("Samsung")}, {"00:25:38", QStringLiteral("Samsung")},
        // Juniper
        {"00:05:85", QStringLiteral("Juniper")}, {"00:12:1E", QStringLiteral("Juniper")},
        {"00:14:F6", QStringLiteral("Juniper")}, {"00:17:CB", QStringLiteral("Juniper")},
        {"00:19:E2", QStringLiteral("Juniper")}, {"00:1A:C4", QStringLiteral("Juniper")},
        {"00:1B:C0", QStringLiteral("Juniper")}, {"00:1F:12", QStringLiteral("Juniper")},
        {"00:22:83", QStringLiteral("Juniper")}, {"00:23:9C", QStringLiteral("Juniper")},
        {"00:26:88", QStringLiteral("Juniper")}, {"04:0E:3F", QStringLiteral("Juniper")},
        {"08:81:F4", QStringLiteral("Juniper")}, {"0C:86:10", QStringLiteral("Juniper")},
        {"10:0E:7E", QStringLiteral("Juniper")}, {"10:3B:59", QStringLiteral("Juniper")},
        {"14:DD:A9", QStringLiteral("Juniper")}, {"18:3C:B4", QStringLiteral("Juniper")},
        {"20:4E:71", QStringLiteral("Juniper")}, {"20:7C:8F", QStringLiteral("Juniper")},
        // Arista
        {"00:1C:73", QStringLiteral("Arista")}, {"00:1D:4F", QStringLiteral("Arista")},
        {"00:25:90", QStringLiteral("Arista")}, {"28:99:3A", QStringLiteral("Arista")},
        {"2C:BA:BA", QStringLiteral("Arista")}, {"30:0C:23", QStringLiteral("Arista")},
        {"44:4C:A8", QStringLiteral("Arista")}, {"50:8C:77", QStringLiteral("Arista")},
        {"68:21:5F", QStringLiteral("Arista")}, {"74:83:C2", QStringLiteral("Arista")},
        {"98:5D:82", QStringLiteral("Arista")}, {"98:F5:A9", QStringLiteral("Arista")},
        {"A8:D0:E5", QStringLiteral("Arista")}, {"C4:7D:46", QStringLiteral("Arista")},
        {"E0:CB:BC", QStringLiteral("Arista")}, {"EC:9E:CD", QStringLiteral("Arista")},
        {"F4:1F:C7", QStringLiteral("Arista")}, {"F8:32:E4", QStringLiteral("Arista")},
        // Ubiquiti
        {"00:15:6D", QStringLiteral("Ubiquiti")}, {"04:18:D6", QStringLiteral("Ubiquiti")},
        {"04:26:65", QStringLiteral("Ubiquiti")}, {"24:A4:3C", QStringLiteral("Ubiquiti")},
        {"28:16:A8", QStringLiteral("Ubiquiti")}, {"2C:3A:FD", QStringLiteral("Ubiquiti")},
        {"44:D9:E7", QStringLiteral("Ubiquiti")}, {"58:9C:FC", QStringLiteral("Ubiquiti")},
        {"68:72:51", QStringLiteral("Ubiquiti")}, {"6C:3B:6B", QStringLiteral("Ubiquiti")},
        {"74:83:C2", QStringLiteral("Ubiquiti")}, {"74:AC:B9", QStringLiteral("Ubiquiti")},
        {"78:45:58", QStringLiteral("Ubiquiti")}, {"78:8A:20", QStringLiteral("Ubiquiti")},
        {"80:2A:A8", QStringLiteral("Ubiquiti")}, {"82:2A:A8", QStringLiteral("Ubiquiti")},
        {"90:3A:72", QStringLiteral("Ubiquiti")}, {"9C:5C:F9", QStringLiteral("Ubiquiti")},
        {"AC:8B:A9", QStringLiteral("Ubiquiti")}, {"B4:FB:E4", QStringLiteral("Ubiquiti")},
        // Fortinet
        {"00:09:0F", QStringLiteral("Fortinet")}, {"04:0E:3C", QStringLiteral("Fortinet")},
        {"08:5B:0E", QStringLiteral("Fortinet")}, {"0C:15:39", QStringLiteral("Fortinet")},
        {"10:1F:74", QStringLiteral("Fortinet")}, {"20:3C:AE", QStringLiteral("Fortinet")},
        {"30:0B:9B", QStringLiteral("Fortinet")}, {"34:13:E8", QStringLiteral("Fortinet")},
        {"40:4D:7F", QStringLiteral("Fortinet")}, {"44:03:2C", QStringLiteral("Fortinet")},
        {"50:6F:9A", QStringLiteral("Fortinet")}, {"54:13:79", QStringLiteral("Fortinet")},
        {"58:0A:20", QStringLiteral("Fortinet")}, {"60:31:3C", QStringLiteral("Fortinet")},
        {"64:09:80", QStringLiteral("Fortinet")}, {"68:14:01", QStringLiteral("Fortinet")},
        {"70:4C:A5", QStringLiteral("Fortinet")}, {"74:AC:B9", QStringLiteral("Fortinet")},
        {"80:2C:73", QStringLiteral("Fortinet")}, {"90:6C:AC", QStringLiteral("Fortinet")},
        // Broadcom
        {"00:10:18", QStringLiteral("Broadcom")}, {"00:0A:F7", QStringLiteral("Broadcom")},
        {"00:16:B4", QStringLiteral("Broadcom")}, {"00:18:0A", QStringLiteral("Broadcom")},
        {"00:1A:11", QStringLiteral("Broadcom")}, {"00:1B:59", QStringLiteral("Broadcom")},
        {"00:1D:4F", QStringLiteral("Broadcom")}, {"00:1E:7B", QStringLiteral("Broadcom")},
        {"00:21:63", QStringLiteral("Broadcom")}, {"00:22:DE", QStringLiteral("Broadcom")},
        {"00:24:7B", QStringLiteral("Broadcom")}, {"00:25:08", QStringLiteral("Broadcom")},
        {"00:26:82", QStringLiteral("Broadcom")}, {"AC:2D:A3", QStringLiteral("Broadcom")},
        {"B0:65:BD", QStringLiteral("Broadcom")}, {"B8:27:EB", QStringLiteral("Broadcom")},
        {"C0:3E:0F", QStringLiteral("Broadcom")}, {"C8:4C:75", QStringLiteral("Broadcom")},
        {"D4:01:29", QStringLiteral("Broadcom")}, {"E4:5F:01", QStringLiteral("Broadcom")},
        // Realtek
        {"00:0D:88", QStringLiteral("Realtek")}, {"00:14:D1", QStringLiteral("Realtek")},
        {"00:1A:6B", QStringLiteral("Realtek")}, {"00:1B:11", QStringLiteral("Realtek")},
        {"00:1C:BF", QStringLiteral("Realtek")}, {"00:1E:58", QStringLiteral("Realtek")},
        {"00:E0:4C", QStringLiteral("Realtek")}, {"10:7B:EF", QStringLiteral("Realtek")},
        {"14:5A:FC", QStringLiteral("Realtek")}, {"18:26:66", QStringLiteral("Realtek")},
        {"20:0C:C8", QStringLiteral("Realtek")}, {"24:0A:64", QStringLiteral("Realtek")},
        {"28:10:1B", QStringLiteral("Realtek")}, {"2C:44:FD", QStringLiteral("Realtek")},
        {"30:3A:64", QStringLiteral("Realtek")}, {"34:27:92", QStringLiteral("Realtek")},
        {"38:1A:52", QStringLiteral("Realtek")}, {"3C:5A:37", QStringLiteral("Realtek")},
        {"40:2E:28", QStringLiteral("Realtek")}, {"40:5D:82", QStringLiteral("Realtek")},
        // Lenovo
        {"00:00:4E", QStringLiteral("Lenovo")}, {"00:01:6D", QStringLiteral("Lenovo")},
        {"00:04:AC", QStringLiteral("Lenovo")}, {"00:09:6B", QStringLiteral("Lenovo")},
        {"00:0A:E4", QStringLiteral("Lenovo")}, {"00:0F:B0", QStringLiteral("Lenovo")},
        {"00:11:25", QStringLiteral("Lenovo")}, {"00:15:58", QStringLiteral("Lenovo")},
        {"00:16:41", QStringLiteral("Lenovo")}, {"00:18:8B", QStringLiteral("Lenovo")},
        {"00:1A:6B", QStringLiteral("Lenovo")}, {"00:1C:25", QStringLiteral("Lenovo")},
        {"00:1E:37", QStringLiteral("Lenovo")}, {"00:21:6A", QStringLiteral("Lenovo")},
        {"00:22:15", QStringLiteral("Lenovo")}, {"00:24:7E", QStringLiteral("Lenovo")},
        {"00:26:2D", QStringLiteral("Lenovo")}, {"04:0A:0A", QStringLiteral("Lenovo")},
        {"08:0C:0B", QStringLiteral("Lenovo")}, {"10:0C:24", QStringLiteral("Lenovo")},
        // NVIDIA
        {"00:04:4B", QStringLiteral("NVIDIA")}, {"00:09:5B", QStringLiteral("NVIDIA")},
        {"00:0A:5C", QStringLiteral("NVIDIA")}, {"00:0F:53", QStringLiteral("NVIDIA")},
        {"00:14:4F", QStringLiteral("NVIDIA")}, {"00:1A:64", QStringLiteral("NVIDIA")},
        {"00:1B:6F", QStringLiteral("NVIDIA")}, {"00:1E:68", QStringLiteral("NVIDIA")},
        {"00:22:67", QStringLiteral("NVIDIA")}, {"00:25:68", QStringLiteral("NVIDIA")},
        {"04:4B:ED", QStringLiteral("NVIDIA")}, {"08:1F:71", QStringLiteral("NVIDIA")},
        {"0C:42:A1", QStringLiteral("NVIDIA")}, {"10:0E:7E", QStringLiteral("NVIDIA")},
        {"14:1B:BD", QStringLiteral("NVIDIA")}, {"18:1D:FA", QStringLiteral("NVIDIA")},
        {"1C:1B:0D", QStringLiteral("NVIDIA")}, {"20:3A:EF", QStringLiteral("NVIDIA")},
        {"24:8A:07", QStringLiteral("NVIDIA")}, {"28:6A:BA", QStringLiteral("NVIDIA")},
    };
    return t;
}

// ============================================================================
// 虚拟机 MAC 前缀集合
// ============================================================================
static bool isVirtualMachinePrefix(const QString& prefix)
{
    static const QSet<QString> vmPrefixes = {
        QStringLiteral("00:0C:29"), // VMware
        QStringLiteral("00:50:56"), // VMware
        QStringLiteral("00:05:69"), // VMware
        QStringLiteral("00:1C:14"), // VMware
        QStringLiteral("08:00:27"), // VirtualBox
        QStringLiteral("00:15:5D"), // Hyper-V
        QStringLiteral("00:03:FF"), // Microsoft Virtual PC
        QStringLiteral("00:16:3E"), // Xen
        QStringLiteral("00:1C:42"), // Parallels
        QStringLiteral("52:54:00"), // QEMU/KVM
        QStringLiteral("00:1A:4A"), // Citrix
        QStringLiteral("00:21:F6"), // Oracle VM
    };
    return vmPrefixes.contains(prefix);
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

MacToolkitWidget::MacToolkitWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

MacToolkitWidget::~MacToolkitWidget() = default;

// ============================================================================
// UI Setup
// ============================================================================

void MacToolkitWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 输入区 ──
    auto* inputGroup = new QGroupBox(QStringLiteral("MAC 地址工具"));
    auto* inputLayout = new QVBoxLayout(inputGroup);
    inputLayout->setSpacing(6);

    auto* inputRow = new QHBoxLayout();
    inputRow->setSpacing(8);

    m_macInput = new QLineEdit();
    m_macInput->setPlaceholderText(QStringLiteral("输入 MAC 地址，如 00:11:22:33:44:55"));
    m_macInput->setMinimumWidth(280);
    inputRow->addWidget(m_macInput, 1);

    m_analyzeBtn = new QPushButton(QStringLiteral("分析"));
    m_analyzeBtn->setMinimumWidth(80);
    inputRow->addWidget(m_analyzeBtn);

    m_generateBtn = new QPushButton(QStringLiteral("生成"));
    m_generateBtn->setMinimumWidth(80);
    inputRow->addWidget(m_generateBtn);

    inputLayout->addLayout(inputRow);
    mainLayout->addWidget(inputGroup);

    // ── 结果区 ──
    auto* resultGroup = new QGroupBox(QStringLiteral("分析结果"));
    auto* resultLayout = new QFormLayout(resultGroup);
    resultLayout->setSpacing(6);

    m_vendorEdit = new QLineEdit();
    m_vendorEdit->setReadOnly(true);
    m_vendorEdit->setPlaceholderText(QStringLiteral("—"));
    resultLayout->addRow(QStringLiteral("厂商:"), m_vendorEdit);

    m_typeEdit = new QLineEdit();
    m_typeEdit->setReadOnly(true);
    m_typeEdit->setPlaceholderText(QStringLiteral("—"));
    resultLayout->addRow(QStringLiteral("类型:"), m_typeEdit);

    m_scopeEdit = new QLineEdit();
    m_scopeEdit->setReadOnly(true);
    m_scopeEdit->setPlaceholderText(QStringLiteral("—"));
    resultLayout->addRow(QStringLiteral("作用域:"), m_scopeEdit);

    // 格式转换行
    auto* formatRow = new QHBoxLayout();
    formatRow->setSpacing(8);

    m_formatCombo = new QComboBox();
    m_formatCombo->addItem(QStringLiteral("AA-BB-CC-DD-EE-FF"));
    m_formatCombo->addItem(QStringLiteral("AA:BB:CC:DD:EE:FF"));
    m_formatCombo->addItem(QStringLiteral("AABB.CCDD.EEFF"));
    m_formatCombo->addItem(QStringLiteral("AABBCCDDEEFF"));
    m_formatCombo->setCurrentIndex(1);
    formatRow->addWidget(m_formatCombo);

    m_formatResult = new QLineEdit();
    m_formatResult->setReadOnly(true);
    m_formatResult->setPlaceholderText(QStringLiteral("转换结果"));
    formatRow->addWidget(m_formatResult, 1);

    resultLayout->addRow(QStringLiteral("格式转换:"), formatRow);
    mainLayout->addWidget(resultGroup);

    // ── 历史记录区 ──
    auto* historyGroup = new QGroupBox(QStringLiteral("历史记录"));
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->setSpacing(4);

    auto* historyToolbar = new QHBoxLayout();
    historyToolbar->setSpacing(4);

    m_exportBtn = new QPushButton(QStringLiteral("导出 CSV"));
    historyToolbar->addWidget(m_exportBtn);
    historyToolbar->addStretch();

    historyLayout->addLayout(historyToolbar);

    m_historyTable = new QTableWidget();
    m_historyTable->setColumnCount(4);
    m_historyTable->setHorizontalHeaderLabels({
        QStringLiteral("MAC 地址"),
        QStringLiteral("厂商"),
        QStringLiteral("类型"),
        QStringLiteral("时间")
    });
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_MAC, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_VENDOR, QHeaderView::Stretch);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_TYPE, QHeaderView::ResizeToContents);
    m_historyTable->verticalHeader()->setVisible(false);
    historyLayout->addWidget(m_historyTable);

    mainLayout->addWidget(historyGroup, 1);
}

// ============================================================================
// Signal Connections
// ============================================================================

void MacToolkitWidget::setupConnections()
{
    connect(m_analyzeBtn, &QPushButton::clicked, this, &MacToolkitWidget::onAnalyze);
    connect(m_generateBtn, &QPushButton::clicked, this, &MacToolkitWidget::onGenerate);
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MacToolkitWidget::onFormatConvert);
    connect(m_exportBtn, &QPushButton::clicked, this, &MacToolkitWidget::onExport);
    connect(m_macInput, &QLineEdit::returnPressed, this, &MacToolkitWidget::onAnalyze);
}

// ============================================================================
// Slot: Analyze MAC Address
// ============================================================================

void MacToolkitWidget::onAnalyze()
{
    QString mac = m_macInput->text().trimmed();
    if (mac.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("请输入 MAC 地址。"));
        return;
    }

    // 规范化 MAC 地址
    QString cleaned = mac.toUpper().remove(QRegularExpression(QStringLiteral("[^0-9A-F]")));
    if (cleaned.length() != 12) {
        QMessageBox::warning(this, QStringLiteral("格式错误"),
            QStringLiteral("MAC 地址格式无效。需要 12 位十六进制字符。\n"
                           "支持格式: XX:XX:XX:XX:XX:XX, XX-XX-XX-XX-XX-XX, XXXX.XXXX.XXXX, XXXXXXXXXXXX"));
        return;
    }

    // 验证是否为合法十六进制
    QRegularExpression hexRe(QStringLiteral("^[0-9A-F]{12}$"));
    if (!hexRe.match(cleaned).hasMatch()) {
        QMessageBox::warning(this, QStringLiteral("格式错误"),
            QStringLiteral("MAC 地址包含无效字符，仅支持 0-9, A-F。"));
        return;
    }

    // 格式化为标准冒号分隔格式
    QString normalized;
    for (int i = 0; i < 12; i += 2) {
        if (i > 0) normalized += QStringLiteral(":");
        normalized += cleaned.mid(i, 2);
    }

    // 厂商查找
    QString vendor = lookupVendor(normalized);

    // 类型检测
    QString type = detectType(normalized);

    // 更新结果区
    m_vendorEdit->setText(vendor.isEmpty() ? QStringLiteral("未知厂商") : vendor);
    m_typeEdit->setText(type);

    // 作用域检测
    bool ok = false;
    int firstByte = cleaned.mid(0, 2).toInt(&ok, 16);
    QString scope;
    if (ok) {
        if (firstByte & 0x02) {
            scope = QStringLiteral("本地管理 (Locally Administered)");
        } else {
            scope = QStringLiteral("全局唯一 (Universally Administered)");
        }
    } else {
        scope = QStringLiteral("—");
    }
    m_scopeEdit->setText(scope);

    // 格式转换
    onFormatConvert();

    // 添加到历史
    addHistoryEntry(normalized, vendor.isEmpty() ? QStringLiteral("未知") : vendor, type);

    Logger::instance().info(QStringLiteral("MacToolkit"),
        QStringLiteral("分析 MAC: %1 → 厂商: %2, 类型: %3")
            .arg(normalized, vendor.isEmpty() ? QStringLiteral("未知") : vendor, type));
}

// ============================================================================
// Slot: Generate Random MAC
// ============================================================================

void MacToolkitWidget::onGenerate()
{
    auto* rng = QRandomGenerator::global();

    // 生成合法 MAC 地址：确保 Unicast (bit 0 = 0) 且 Global (bit 1 = 0)
    int firstByte = rng->bounded(256);
    firstByte &= 0xFC; // 清除最低两位

    QByteArray bytes;
    bytes.append(static_cast<char>(firstByte));
    for (int i = 1; i < 6; ++i) {
        bytes.append(static_cast<char>(rng->bounded(256)));
    }

    QString mac;
    for (int i = 0; i < 6; ++i) {
        if (i > 0) mac += QStringLiteral(":");
        mac += QStringLiteral("%1").arg(static_cast<quint8>(bytes[i]), 2, 16, QLatin1Char('0')).toUpper();
    }

    m_macInput->setText(mac);

    Logger::instance().info(QStringLiteral("MacToolkit"),
        QStringLiteral("生成随机 MAC: %1").arg(mac));
}

// ============================================================================
// Static: Lookup Vendor by MAC
// ============================================================================

QString MacToolkitWidget::lookupVendor(const QString& mac)
{
    if (mac.length() < 8) return {};
    QString prefix = mac.left(8).toUpper(); // "XX:XX:XX"
    return ouiTable().value(prefix);
}

// ============================================================================
// Static: Detect MAC Type
// ============================================================================

QString MacToolkitWidget::detectType(const QString& mac)
{
    QString cleaned = mac.toUpper().remove(QRegularExpression(QStringLiteral("[^0-9A-F]")));
    if (cleaned.length() != 12) return QStringLiteral("无效");

    bool ok = false;
    int firstByte = cleaned.mid(0, 2).toInt(&ok, 16);
    if (!ok) return QStringLiteral("无效");

    QStringList types;

    // 广播检测
    if (cleaned == QStringLiteral("FFFFFFFFFFFF")) {
        types << QStringLiteral("广播 (Broadcast)");
        return types.join(QStringLiteral(", "));
    }

    // 多播/单播检测 (bit 0 of first byte)
    if (firstByte & 0x01) {
        types << QStringLiteral("多播 (Multicast)");
    } else {
        types << QStringLiteral("单播 (Unicast)");
    }

    // 全局/本地管理检测 (bit 1 of first byte)
    if (firstByte & 0x02) {
        types << QStringLiteral("本地管理");
    } else {
        types << QStringLiteral("全局管理");
    }

    // 虚拟机 MAC 检测
    QString prefix = mac.left(8).toUpper();
    if (isVirtualMachinePrefix(prefix)) {
        types << QStringLiteral("虚拟机");
    }

    return types.join(QStringLiteral(", "));
}

// ============================================================================
// Static: Convert MAC Format
// ============================================================================

QString MacToolkitWidget::convertFormat(const QString& mac, const QString& format)
{
    QString cleaned = mac.toUpper().remove(QRegularExpression(QStringLiteral("[^0-9A-F]")));
    if (cleaned.length() != 12) return {};

    QRegularExpression hexRe(QStringLiteral("^[0-9A-F]{12}$"));
    if (!hexRe.match(cleaned).hasMatch()) return {};

    if (format == QStringLiteral("AA-BB-CC-DD-EE-FF")) {
        QString result;
        for (int i = 0; i < 12; i += 2) {
            if (i > 0) result += QStringLiteral("-");
            result += cleaned.mid(i, 2);
        }
        return result;
    }

    if (format == QStringLiteral("AA:BB:CC:DD:EE:FF")) {
        QString result;
        for (int i = 0; i < 12; i += 2) {
            if (i > 0) result += QStringLiteral(":");
            result += cleaned.mid(i, 2);
        }
        return result;
    }

    if (format == QStringLiteral("AABB.CCDD.EEFF")) {
        return cleaned.mid(0, 4) + QStringLiteral(".") +
               cleaned.mid(4, 4) + QStringLiteral(".") +
               cleaned.mid(8, 4);
    }

    if (format == QStringLiteral("AABBCCDDEEFF")) {
        return cleaned;
    }

    return {};
}

// ============================================================================
// Slot: Format Convert (triggered by combo box)
// ============================================================================

void MacToolkitWidget::onFormatConvert()
{
    QString mac = m_macInput->text().trimmed();
    if (mac.isEmpty()) {
        m_formatResult->clear();
        return;
    }

    QString format = m_formatCombo->currentText();
    QString result = convertFormat(mac, format);
    m_formatResult->setText(result);
}

// ============================================================================
// History Entry
// ============================================================================

void MacToolkitWidget::addHistoryEntry(const QString& mac, const QString& vendor, const QString& type)
{
    int row = m_historyTable->rowCount();
    m_historyTable->insertRow(row);

    auto* macItem = new QTableWidgetItem(mac);
    macItem->setTextAlignment(Qt::AlignCenter);
    m_historyTable->setItem(row, HIST_COL_MAC, macItem);

    auto* vendorItem = new QTableWidgetItem(vendor);
    vendorItem->setTextAlignment(Qt::AlignCenter);
    m_historyTable->setItem(row, HIST_COL_VENDOR, vendorItem);

    auto* typeItem = new QTableWidgetItem(type);
    typeItem->setTextAlignment(Qt::AlignCenter);
    m_historyTable->setItem(row, HIST_COL_TYPE, typeItem);

    auto* timeItem = new QTableWidgetItem(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    timeItem->setTextAlignment(Qt::AlignCenter);
    m_historyTable->setItem(row, HIST_COL_TIME, timeItem);

    m_historyTable->scrollToBottom();
}

// ============================================================================
// Slot: Export to CSV
// ============================================================================

void MacToolkitWidget::onExport()
{
    if (m_historyTable->rowCount() == 0) {
        QMessageBox::information(this, QStringLiteral("导出"),
            QStringLiteral("历史记录为空，无需导出。"));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this,
        QStringLiteral("导出历史记录"),
        QStringLiteral("mac_history.csv"),
        QStringLiteral("CSV 文件 (*.csv)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
            QStringLiteral("无法打开文件: %1").arg(filePath));
        return;
    }

    QTextStream stream(&file);
    // BOM for Excel compatibility
    stream.setEncoding(QStringConverter::Utf8);
    stream << QStringLiteral("\xEF\xBB\xBF");

    // 表头
    stream << QStringLiteral("MAC 地址,厂商,类型,时间\n");

    // 数据行
    for (int row = 0; row < m_historyTable->rowCount(); ++row) {
        for (int col = 0; col < m_historyTable->columnCount(); ++col) {
            if (col > 0) stream << QStringLiteral(",");
            auto* item = m_historyTable->item(row, col);
            if (item) {
                QString text = item->text();
                // 转义含逗号的字段
                if (text.contains(QStringLiteral(",")) || text.contains(QStringLiteral("\""))) {
                    text.replace(QStringLiteral("\""), QStringLiteral("\"\""));
                    text = QStringLiteral("\"") + text + QStringLiteral("\"");
                }
                stream << text;
            }
        }
        stream << QStringLiteral("\n");
    }

    file.close();

    Logger::instance().info(QStringLiteral("MacToolkit"),
        QStringLiteral("导出历史记录: %1 条 → %2")
            .arg(m_historyTable->rowCount()).arg(filePath));

    QMessageBox::information(this, QStringLiteral("导出成功"),
        QStringLiteral("已导出 %1 条记录到:\n%2")
            .arg(m_historyTable->rowCount()).arg(filePath));
}