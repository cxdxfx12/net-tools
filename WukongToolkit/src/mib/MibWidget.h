#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QProcess>
#include <QHash>
#include <QVector>
#include <QFileInfo>

struct MibOidInfo
{
    QString name;
    QString oid;
    QString type;
    QString access;
    QString description;
};

class MibWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MibWidget(QWidget* parent = nullptr);
    ~MibWidget() override;

private slots:
    void onImportMib();
    void onDeleteMib();
    void onSearchOid();
    void onAddFavorite();
    void onOidTreeItemClicked(QTreeWidgetItem* item, int column);
    void onSnmpGet();
    void onSnmpWalk();
    void onExport();

private:
    void setupUI();
    void setupConnections();
    void loadMibFile(const QString& filePath);
    void parseMibLine(const QString& line);
    QString resolveOidDescription(const QString& oid);
    void addOidToTree(const QString& name, const QString& oid,
                      const QString& type, const QString& access,
                      const QString& description);
    void buildBuiltinOidTree();
    void setupBuiltinOidDatabase();
    QTreeWidgetItem* findParentNode(QTreeWidgetItem* root, const QString& oid);
    void searchTreeItems(QTreeWidgetItem* root, const QString& keyword,
                         QList<QTreeWidgetItem*>& results);
    void collectLeafOids(QTreeWidgetItem* root, QVector<MibOidInfo>& results);

    // UI components
    QListWidget* m_mibFileList;
    QPushButton* m_importMibBtn;
    QPushButton* m_deleteMibBtn;
    QLineEdit* m_oidSearchEdit;
    QTreeWidget* m_oidTree;
    QPushButton* m_searchBtn;
    QPushButton* m_favoriteBtn;
    QPlainTextEdit* m_detailArea;
    QLineEdit* m_deviceIpEdit;
    QLineEdit* m_communityEdit;
    QPushButton* m_getBtn;
    QPushButton* m_walkBtn;
    QPlainTextEdit* m_snmpResultArea;
    QPushButton* m_exportBtn;

    // Data
    QHash<QString, MibOidInfo> m_oidDatabase;
    QStringList m_loadedMibFiles;
    QProcess* m_snmpProcess;
};