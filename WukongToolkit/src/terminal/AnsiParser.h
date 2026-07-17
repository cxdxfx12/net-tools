#pragma once

#include <QByteArray>
#include <QVector>
#include <QString>
#include <QColor>
#include <functional>

class TerminalBuffer;
class TerminalCursor;
class TerminalColor;

enum class AnsiAction
{
    None,
    Print,
    NewLine,
    CarriageReturn,
    Tab,
    Backspace,
    Bell,
    CursorUp,
    CursorDown,
    CursorForward,
    CursorBackward,
    CursorPosition,
    CursorHome,
    CursorSave,
    CursorRestore,
    CursorShow,
    CursorHide,
    EraseDisplay,
    EraseLine,
    ScrollUp,
    ScrollDown,
    SetForeground,
    SetBackground,
    SetAttribute,
    ResetAttributes,
    ReverseVideo,
    DeviceStatusReport,
};

class AnsiParser
{
public:
    AnsiParser(TerminalBuffer* buffer, TerminalCursor* cursor, TerminalColor* color);

    void parse(const QByteArray& data);

    using OutputCallback = std::function<void(const QByteArray&)>;
    void setOutputCallback(OutputCallback callback) { m_outputCallback = std::move(callback); }

private:
    void processByte(char byte);
    void processEscape();
    void processCSI(char finalByte);
    void processOSC();
    void executeAction(AnsiAction action, const QVector<int>& params = {});

    void handleSGR(const QVector<int>& params);
    void handleCursorPosition(const QVector<int>& params);
    void handleEraseDisplay(const QVector<int>& params);
    void handleEraseLine(const QVector<int>& params);
    void handleDeviceStatusReport(const QVector<int>& params);

    void printChar(char ch);
    void advanceCursor();
    void newLine();

    TerminalBuffer* m_buffer;
    TerminalCursor* m_cursor;
    TerminalColor* m_color;

    // Parser state
    enum class State { Normal, Escape, CSI, OSC, OSCString };
    State m_state = State::Normal;
    QByteArray m_params;
    QByteArray m_oscString;

    // Current text attributes
    struct TextAttributes {
        QColor fg;
        QColor bg;
        bool bold = false;
        bool underline = false;
        bool reverse = false;
        bool blink = false;
    };
    TextAttributes m_attrs;

    OutputCallback m_outputCallback;
};