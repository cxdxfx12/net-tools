#pragma once

#include <QObject>
#include <QTimer>

class TerminalCursor : public QObject
{
    Q_OBJECT

public:
    explicit TerminalCursor(QObject* parent = nullptr);

    int row() const { return m_row; }
    int col() const { return m_col; }
    void setPos(int col, int row);
    void move(int deltaCol, int deltaRow);

    bool visible() const { return m_visible; }
    void setVisible(bool visible);

    bool blinkEnabled() const { return m_blinkEnabled; }
    void setBlinkEnabled(bool enabled);

    bool blinkOn() const { return m_blinkOn; }

    // Saved cursor state (for ESC 7 / ESC 8)
    void save();
    void restore();

    int savedRow() const { return m_savedRow; }
    int savedCol() const { return m_savedCol; }

private slots:
    void toggleBlink();

private:
    int m_col = 0;
    int m_row = 0;
    bool m_visible = true;
    bool m_blinkEnabled = true;
    bool m_blinkOn = true;

    int m_savedCol = 0;
    int m_savedRow = 0;

    QTimer* m_blinkTimer;
};