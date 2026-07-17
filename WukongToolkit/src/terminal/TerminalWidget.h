#pragma once

#include <QWidget>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>

class TerminalBuffer;
class TerminalCursor;
class TerminalColor;
class TerminalRenderer;
class AnsiParser;
class SelectionManager;
class ClipboardManager;
class TerminalSearch;

class TerminalWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    ~TerminalWidget() override;

    void appendOutput(const QByteArray& data);
    void clearTerminal();
    void setConnected(bool connected);

    void setEncoding(const QString& encoding);
    QString encoding() const;

    void setFont(const QFont& font);

signals:
    void dataReady(const QByteArray& data);
    void copyRequested();
    void pasteRequested();
    void clearRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void handleSpecialKey(QKeyEvent* event);
    void sendText(const QString& text);
    QPoint cellPosFromPixel(const QPoint& pixel) const;

    // Core components
    TerminalBuffer* m_buffer;
    TerminalCursor* m_cursor;
    TerminalColor* m_color;
    TerminalRenderer* m_renderer;
    AnsiParser* m_parser;
    SelectionManager* m_selection;
    ClipboardManager* m_clipboard;
    TerminalSearch* m_search;

    QString m_encoding = "UTF-8";
    bool m_connected = false;
    bool m_selecting = false;
};