#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QProgressBar>
#include <QProcess>
#include <QVector>
#include <QPair>

struct SNMPResult
{
    QString oid;
    QString value;
    QString type;
};

class SNMPWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SNMPWidget(QWidget* parent = nullptr);
    ~SNMPWidget() override;

private slots:
    void onSend();
    void onStop();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onExportCSV();
    void onClear();
    void onMibTreeItemClicked(QTreeWidgetItem* item, int column);
    void onMibShortcutClicked();

private:
    void setupUI();
    void setupConnections();
    void buildMibTree();
    void addMibNode(QTreeWidgetItem* parent, const QString& name, const QString& oid);
    void parseSnmpLine(const QString& line);
    void appendResult(const QString& oid, const QString& value, const QString& type);
    QString buildCommand() const;
    QStringList buildArgs() const;

    // UI elements
    QLineEdit* m_targetEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_communityEdit;
    QComboBox* m_versionCombo;
    QLineEdit* m_oidEdit;
    QComboBox* m_operationCombo;
    QPushButton* m_sendBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_exportBtn;
    QPushButton* m_clearBtn;
    QPlainTextEdit* m_resultsArea;
    QTreeWidget* m_mibTree;
    QProgressBar* m_progressBar;

    // Process
    QProcess* m_process;
    bool m_running;
    QVector<SNMPResult> m_results;
    QString m_currentCommand;
    QString m_currentOid;
};