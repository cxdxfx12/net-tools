#include "terminal/AnsiParser.h"
#include "terminal/TerminalBuffer.h"
#include "terminal/TerminalCursor.h"
#include "terminal/TerminalColor.h"
#include <QDebug>

AnsiParser::AnsiParser(TerminalBuffer* buffer, TerminalCursor* cursor, TerminalColor* color)
    : m_buffer(buffer)
    , m_cursor(cursor)
    , m_color(color)
{
    m_attrs.fg = m_color->defaultFg();
    m_attrs.bg = m_color->defaultBg();
}

void AnsiParser::parse(const QByteArray& data)
{
    for (int i = 0; i < data.size(); ++i) {
        processByte(data[i]);
    }
}

void AnsiParser::processByte(char byte)
{
    switch (m_state) {
    case State::Normal:
        if (byte == '\x1b') {
            m_state = State::Escape;
        } else if (byte == '\r') {
            executeAction(AnsiAction::CarriageReturn);
        } else if (byte == '\n') {
            executeAction(AnsiAction::NewLine);
        } else if (byte == '\t') {
            executeAction(AnsiAction::Tab);
        } else if (byte == '\b') {
            executeAction(AnsiAction::Backspace);
        } else if (byte == '\a') {
            executeAction(AnsiAction::Bell);
        } else if (byte >= 0x20 || byte < 0) {
            printChar(byte);
        }
        break;

    case State::Escape:
        if (byte == '[') {
            m_state = State::CSI;
            m_params.clear();
        } else if (byte == ']') {
            m_state = State::OSC;
            m_oscString.clear();
        } else if (byte == '7') {
            executeAction(AnsiAction::CursorSave);
            m_state = State::Normal;
        } else if (byte == '8') {
            executeAction(AnsiAction::CursorRestore);
            m_state = State::Normal;
        } else if (byte == 'D') {
            executeAction(AnsiAction::ScrollDown);
            m_state = State::Normal;
        } else if (byte == 'M') {
            executeAction(AnsiAction::ScrollUp);
            m_state = State::Normal;
        } else if (byte == 'c') {
            executeAction(AnsiAction::ResetAttributes);
            m_state = State::Normal;
        } else {
            m_state = State::Normal;
        }
        break;

    case State::CSI:
        if (byte >= 0x40 && byte <= 0x7E) {
            processCSI(byte);
            m_state = State::Normal;
        } else if (byte >= 0x20 && byte <= 0x3F) {
            m_params.append(byte);
        }
        break;

    case State::OSC:
        if (byte == '\a' || (byte == '\\' && !m_oscString.isEmpty() && m_oscString.back() == '\x1b')) {
            processOSC();
            m_state = State::Normal;
        } else {
            m_oscString.append(byte);
        }
        break;

    default:
        m_state = State::Normal;
        break;
    }
}

void AnsiParser::processCSI(char finalByte)
{
    // Parse numeric parameters
    QVector<int> params;
    QList<QByteArray> parts = m_params.split(';');
    for (const auto& part : parts) {
        bool ok = false;
        int val = part.toInt(&ok);
        params.append(ok ? val : 0);
    }
    if (params.isEmpty()) {
        params.append(0);
    }

    switch (finalByte) {
    case 'A': executeAction(AnsiAction::CursorUp, {params[0] > 0 ? params[0] : 1}); break;
    case 'B': executeAction(AnsiAction::CursorDown, {params[0] > 0 ? params[0] : 1}); break;
    case 'C': executeAction(AnsiAction::CursorForward, {params[0] > 0 ? params[0] : 1}); break;
    case 'D': executeAction(AnsiAction::CursorBackward, {params[0] > 0 ? params[0] : 1}); break;
    case 'H':
    case 'f': handleCursorPosition(params); break;
    case 'J': handleEraseDisplay(params); break;
    case 'K': handleEraseLine(params); break;
    case 'm': handleSGR(params); break;
    case 'n': handleDeviceStatusReport(params); break;
    case 's': executeAction(AnsiAction::CursorSave); break;
    case 'u': executeAction(AnsiAction::CursorRestore); break;
    case 'h':
        if (params.size() >= 1 && params[0] == 25) executeAction(AnsiAction::CursorShow);
        break;
    case 'l':
        if (params.size() >= 1 && params[0] == 25) executeAction(AnsiAction::CursorHide);
        break;
    default: break;
    }
}

void AnsiParser::processOSC()
{
    // OSC sequences - typically title changes, etc.
    // OSC 0;title BEL or OSC 2;title BEL
    if (m_oscString.startsWith("0;") || m_oscString.startsWith("2;")) {
        // Window title change - not handled in first version
    }
}

void AnsiParser::handleCursorPosition(const QVector<int>& params)
{
    int row = params.size() > 0 ? std::max(0, params[0] - 1) : 0;
    int col = params.size() > 1 ? std::max(0, params[1] - 1) : 0;
    row = std::min(row, m_buffer->rows() - 1);
    col = std::min(col, m_buffer->columns() - 1);
    m_cursor->setPos(col, row);
    m_buffer->setCursorPos(col, row);
}

void AnsiParser::handleEraseDisplay(const QVector<int>& params)
{
    int mode = params.size() > 0 ? params[0] : 0;
    switch (mode) {
    case 0: // Erase from cursor to end
        m_buffer->eraseBelow(m_cursor->col(), m_cursor->row());
        break;
    case 1: // Erase from start to cursor
        m_buffer->eraseAbove(m_cursor->col(), m_cursor->row());
        break;
    case 2: // Erase entire display
    case 3:
        m_buffer->clear();
        m_cursor->setPos(0, 0);
        m_buffer->setCursorPos(0, 0);
        break;
    }
}

void AnsiParser::handleEraseLine(const QVector<int>& params)
{
    int mode = params.size() > 0 ? params[0] : 0;
    switch (mode) {
    case 0: // Erase from cursor to end of line
        m_buffer->eraseFromCursor(m_cursor->col(), m_cursor->row());
        break;
    case 1: // Erase from start of line to cursor
        m_buffer->eraseToCursor(m_cursor->col(), m_cursor->row());
        break;
    case 2: // Erase entire line
        m_buffer->eraseLine(m_cursor->row());
        break;
    }
}

void AnsiParser::handleDeviceStatusReport(const QVector<int>& params)
{
    if (params.size() > 0 && params[0] == 6 && m_outputCallback) {
        // Report cursor position
        QString report = QString("\x1b[%1;%2R")
                             .arg(m_cursor->row() + 1)
                             .arg(m_cursor->col() + 1);
        m_outputCallback(report.toUtf8());
    }
}

void AnsiParser::handleSGR(const QVector<int>& params)
{
    if (params.isEmpty() || params[0] == 0) {
        // Reset all attributes
        m_attrs.fg = m_color->defaultFg();
        m_attrs.bg = m_color->defaultBg();
        m_attrs.bold = false;
        m_attrs.underline = false;
        m_attrs.reverse = false;
        m_attrs.blink = false;
        return;
    }

    for (int i = 0; i < params.size(); ++i) {
        int p = params[i];
        switch (p) {
        case 0: // Reset
            m_attrs.fg = m_color->defaultFg();
            m_attrs.bg = m_color->defaultBg();
            m_attrs.bold = false;
            m_attrs.underline = false;
            m_attrs.reverse = false;
            m_attrs.blink = false;
            break;
        case 1: m_attrs.bold = true; break;
        case 4: m_attrs.underline = true; break;
        case 5: m_attrs.blink = true; break;
        case 7: m_attrs.reverse = true; break;
        case 22: m_attrs.bold = false; break;
        case 24: m_attrs.underline = false; break;
        case 25: m_attrs.blink = false; break;
        case 27: m_attrs.reverse = false; break;
        case 30: m_attrs.fg = m_color->ansiColor(0); break;
        case 31: m_attrs.fg = m_color->ansiColor(1); break;
        case 32: m_attrs.fg = m_color->ansiColor(2); break;
        case 33: m_attrs.fg = m_color->ansiColor(3); break;
        case 34: m_attrs.fg = m_color->ansiColor(4); break;
        case 35: m_attrs.fg = m_color->ansiColor(5); break;
        case 36: m_attrs.fg = m_color->ansiColor(6); break;
        case 37: m_attrs.fg = m_color->ansiColor(7); break;
        case 38:
            if (i + 2 < params.size() && params[i + 1] == 5) {
                m_attrs.fg = m_color->color256(params[i + 2]);
                i += 2;
            }
            break;
        case 39: m_attrs.fg = m_color->defaultFg(); break;
        case 40: m_attrs.bg = m_color->ansiColor(0); break;
        case 41: m_attrs.bg = m_color->ansiColor(1); break;
        case 42: m_attrs.bg = m_color->ansiColor(2); break;
        case 43: m_attrs.bg = m_color->ansiColor(3); break;
        case 44: m_attrs.bg = m_color->ansiColor(4); break;
        case 45: m_attrs.bg = m_color->ansiColor(5); break;
        case 46: m_attrs.bg = m_color->ansiColor(6); break;
        case 47: m_attrs.bg = m_color->ansiColor(7); break;
        case 48:
            if (i + 2 < params.size() && params[i + 1] == 5) {
                m_attrs.bg = m_color->color256(params[i + 2]);
                i += 2;
            }
            break;
        case 49: m_attrs.bg = m_color->defaultBg(); break;
        case 90: m_attrs.fg = m_color->ansiColor(8); break;
        case 91: m_attrs.fg = m_color->ansiColor(9); break;
        case 92: m_attrs.fg = m_color->ansiColor(10); break;
        case 93: m_attrs.fg = m_color->ansiColor(11); break;
        case 94: m_attrs.fg = m_color->ansiColor(12); break;
        case 95: m_attrs.fg = m_color->ansiColor(13); break;
        case 96: m_attrs.fg = m_color->ansiColor(14); break;
        case 97: m_attrs.fg = m_color->ansiColor(15); break;
        case 100: m_attrs.bg = m_color->ansiColor(8); break;
        case 101: m_attrs.bg = m_color->ansiColor(9); break;
        case 102: m_attrs.bg = m_color->ansiColor(10); break;
        case 103: m_attrs.bg = m_color->ansiColor(11); break;
        case 104: m_attrs.bg = m_color->ansiColor(12); break;
        case 105: m_attrs.bg = m_color->ansiColor(13); break;
        case 106: m_attrs.bg = m_color->ansiColor(14); break;
        case 107: m_attrs.bg = m_color->ansiColor(15); break;
        default: break;
        }
    }
}

void AnsiParser::executeAction(AnsiAction action, const QVector<int>& params)
{
    switch (action) {
    case AnsiAction::CursorUp:
        m_cursor->move(0, -std::max(1, params.value(0, 1)));
        m_buffer->setCursorPos(m_cursor->col(), m_cursor->row());
        break;
    case AnsiAction::CursorDown:
        m_cursor->move(0, std::max(1, params.value(0, 1)));
        m_buffer->setCursorPos(m_cursor->col(), m_cursor->row());
        break;
    case AnsiAction::CursorForward:
        m_cursor->move(std::max(1, params.value(0, 1)), 0);
        m_buffer->setCursorPos(m_cursor->col(), m_cursor->row());
        break;
    case AnsiAction::CursorBackward:
        m_cursor->move(-std::max(1, params.value(0, 1)), 0);
        m_buffer->setCursorPos(m_cursor->col(), m_cursor->row());
        break;
    case AnsiAction::CursorHome:
        m_cursor->setPos(0, 0);
        m_buffer->setCursorPos(0, 0);
        break;
    case AnsiAction::CursorSave:
        m_cursor->save();
        break;
    case AnsiAction::CursorRestore:
        m_cursor->restore();
        m_buffer->setCursorPos(m_cursor->col(), m_cursor->row());
        break;
    case AnsiAction::CursorShow:
        m_cursor->setVisible(true);
        break;
    case AnsiAction::CursorHide:
        m_cursor->setVisible(false);
        break;
    case AnsiAction::CarriageReturn:
        m_cursor->setPos(0, m_cursor->row());
        m_buffer->setCursorPos(0, m_cursor->row());
        break;
    case AnsiAction::NewLine:
        newLine();
        break;
    case AnsiAction::Tab:
        for (int i = 0; i < 4; ++i) advanceCursor();
        break;
    case AnsiAction::Backspace:
        if (m_cursor->col() > 0) {
            m_cursor->move(-1, 0);
        }
        m_buffer->setCursorPos(m_cursor->col(), m_cursor->row());
        break;
    case AnsiAction::ScrollUp:
        m_buffer->scrollUp(params.value(0, 1));
        break;
    case AnsiAction::ScrollDown:
        m_buffer->scrollDown(params.value(0, 1));
        break;
    case AnsiAction::ResetAttributes:
        m_buffer->clear();
        m_cursor->setPos(0, 0);
        m_attrs.fg = m_color->defaultFg();
        m_attrs.bg = m_color->defaultBg();
        m_attrs.bold = false;
        m_attrs.underline = false;
        m_attrs.reverse = false;
        m_attrs.blink = false;
        break;
    default: break;
    }
}

void AnsiParser::printChar(char ch)
{
    int col = m_cursor->col();
    int row = m_cursor->row();

    if (col >= m_buffer->columns()) {
        col = 0;
        row++;
        if (row >= m_buffer->rows()) {
            m_buffer->scrollUp();
            row = m_buffer->rows() - 1;
        }
    }

    TerminalCell cell;
    cell.ch = QChar::fromLatin1(ch);
    cell.fg = m_attrs.reverse ? m_attrs.bg : m_attrs.fg;
    cell.bg = m_attrs.reverse ? m_attrs.fg : m_attrs.bg;
    cell.bold = m_attrs.bold;
    cell.underline = m_attrs.underline;
    cell.blink = m_attrs.blink;
    cell.reverse = m_attrs.reverse;

    m_buffer->setCell(col, row, cell);
    advanceCursor();
}

void AnsiParser::advanceCursor()
{
    int col = m_cursor->col() + 1;
    int row = m_cursor->row();

    if (col >= m_buffer->columns()) {
        col = 0;
        row++;
        if (row >= m_buffer->rows()) {
            m_buffer->scrollUp();
            row = m_buffer->rows() - 1;
        }
    }

    m_cursor->setPos(col, row);
    m_buffer->setCursorPos(col, row);
}

void AnsiParser::newLine()
{
    int row = m_cursor->row() + 1;
    if (row >= m_buffer->rows()) {
        m_buffer->scrollUp();
        row = m_buffer->rows() - 1;
    }
    m_cursor->setPos(m_cursor->col(), row);
    m_buffer->setCursorPos(m_cursor->col(), row);
}