#include "terminal/TerminalWidget.h"
#include "terminal/TerminalBuffer.h"
#include "terminal/TerminalCursor.h"
#include "terminal/TerminalColor.h"
#include "terminal/TerminalRenderer.h"
#include "terminal/AnsiParser.h"
#include "terminal/SelectionManager.h"
#include "terminal/ClipboardManager.h"
#include "terminal/TerminalSearch.h"

#include <QPainter>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QApplication>
#include <QScrollBar>
#include <QResizeEvent>

TerminalWidget::TerminalWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setCursor(Qt::IBeamCursor);
    setMinimumSize(200, 100);

    // Initialize core components
    m_buffer = new TerminalBuffer(80, 24);
    m_cursor = new TerminalCursor(this);
    m_color = new TerminalColor();
    m_renderer = new TerminalRenderer(m_buffer, m_cursor, m_color, this);
    m_selection = new SelectionManager(this);
    m_selection->setBuffer(m_buffer);
    m_clipboard = new ClipboardManager(this);
    m_search = new TerminalSearch(this);
    m_search->setBuffer(m_buffer);

    m_parser = new AnsiParser(m_buffer, m_cursor, m_color);
    m_parser->setOutputCallback([this](const QByteArray& data) {
        emit dataReady(data);
    });

    // Use a timer to trigger cursor blink updates
    auto* blinkTimer = new QTimer(this);
    blinkTimer->setInterval(500);
    connect(blinkTimer, &QTimer::timeout, this, [this]() {
        update();
    });
    blinkTimer->start();
}

TerminalWidget::~TerminalWidget() = default;

void TerminalWidget::appendOutput(const QByteArray& data)
{
    m_parser->parse(data);
    update();
}

void TerminalWidget::clearTerminal()
{
    m_buffer->clear();
    m_cursor->setPos(0, 0);
    m_search->clearSearch();
    update();
}

void TerminalWidget::setConnected(bool connected)
{
    m_connected = connected;
    if (!connected) {
        m_cursor->setBlinkEnabled(false);
    } else {
        m_cursor->setBlinkEnabled(true);
    }
    update();
}

void TerminalWidget::setEncoding(const QString& encoding)
{
    m_encoding = encoding;
}

QString TerminalWidget::encoding() const
{
    return m_encoding;
}

void TerminalWidget::setFont(const QFont& font)
{
    m_renderer->setFont(font);
    update();
}

// ─── Paint ────────────────────────────────────────────────────────────

void TerminalWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    m_renderer->render(painter, rect(), m_buffer->scrollOffset());

    // Draw selection highlight
    if (m_selection->hasSelection()) {
        QSize cellSize = m_renderer->cellSize();
        QPoint start = m_selection->startPos();
        QPoint end = m_selection->endPos();

        int startRow = std::min(start.y(), end.y());
        int endRow = std::max(start.y(), end.y());

        painter.setBrush(QColor(64, 158, 255, 60));
        painter.setPen(Qt::NoPen);

        for (int row = startRow; row <= endRow; ++row) {
            // Skip history rows
            int visibleRow = row - m_buffer->scrollOffset() + m_buffer->historySize();
            if (visibleRow < m_buffer->historySize()) continue;
            int bufferRow = visibleRow - m_buffer->historySize();
            if (bufferRow < 0 || bufferRow >= m_buffer->rows()) continue;

            int colStart = (row == start.y()) ? std::min(start.x(), end.x()) : 0;
            int colEnd = (row == end.y()) ? std::max(start.x(), end.x()) : m_buffer->columns() - 1;

            QRect selRect(rect().x() + colStart * cellSize.width(),
                         rect().y() + bufferRow * cellSize.height(),
                         (colEnd - colStart + 1) * cellSize.width(),
                         cellSize.height());
            painter.drawRect(selRect);
        }
    }
}

void TerminalWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    QSize cellSize = m_renderer->cellSize();
    if (cellSize.width() > 0 && cellSize.height() > 0) {
        int cols = event->size().width() / cellSize.width();
        int rows = event->size().height() / cellSize.height();
        if (cols > 0 && rows > 0) {
            m_buffer->resize(cols, rows);
        }
    }
}

// ─── Keyboard ─────────────────────────────────────────────────────────

void TerminalWidget::keyPressEvent(QKeyEvent* event)
{
    if (!m_connected) {
        QWidget::keyPressEvent(event);
        return;
    }

    // Handle Ctrl+key shortcuts
    if (event->modifiers() == Qt::ControlModifier) {
        switch (event->key()) {
        case Qt::Key_C:
            if (m_selection->hasSelection()) {
                QString text = m_selection->selectedText();
                m_clipboard->copy(text);
                m_selection->clearSelection();
            }
            return;
        case Qt::Key_V:
            m_clipboard->paste();
            return;
        case Qt::Key_L:
            clearTerminal();
            return;
        case Qt::Key_A:
            // Select all
            m_selection->startSelection(0, 0);
            m_selection->updateSelection(m_buffer->columns() - 1, m_buffer->rows() - 1);
            update();
            return;
        case Qt::Key_F:
            // Search - placeholder
            return;
        default:
            break;
        }
    }

    // Handle Shift+Ctrl+C (send Ctrl+C to remote)
    if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (event->key() == Qt::Key_C) {
            emit dataReady(QByteArray(1, '\x03')); // Send Ctrl+C
            return;
        }
    }

    handleSpecialKey(event);

    // Regular text
    QString text = event->text();
    if (!text.isEmpty()) {
        if (text == "\r" || text == "\n") {
            emit dataReady(QByteArray("\r"));
        } else {
            emit dataReady(text.toUtf8());
        }
    }

    event->accept();
}

void TerminalWidget::handleSpecialKey(QKeyEvent* event)
{
    QByteArray data;

    switch (event->key()) {
    case Qt::Key_Up:       data = "\x1b[A"; break;
    case Qt::Key_Down:     data = "\x1b[B"; break;
    case Qt::Key_Right:    data = "\x1b[C"; break;
    case Qt::Key_Left:     data = "\x1b[D"; break;
    case Qt::Key_Tab:      data = "\t"; break;
    case Qt::Key_Escape:   data = "\x1b"; break;
    case Qt::Key_Delete:   data = "\x1b[3~"; break;
    case Qt::Key_Home:     data = "\x1b[H"; break;
    case Qt::Key_End:      data = "\x1b[F"; break;
    case Qt::Key_PageUp:   data = "\x1b[5~"; break;
    case Qt::Key_PageDown: data = "\x1b[6~"; break;
    case Qt::Key_Insert:   data = "\x1b[2~"; break;
    case Qt::Key_F1:  data = "\x1bOP"; break;
    case Qt::Key_F2:  data = "\x1bOQ"; break;
    case Qt::Key_F3:  data = "\x1bOR"; break;
    case Qt::Key_F4:  data = "\x1bOS"; break;
    case Qt::Key_F5:  data = "\x1b[15~"; break;
    case Qt::Key_F6:  data = "\x1b[17~"; break;
    case Qt::Key_F7:  data = "\x1b[18~"; break;
    case Qt::Key_F8:  data = "\x1b[19~"; break;
    case Qt::Key_F9:  data = "\x1b[20~"; break;
    case Qt::Key_F10: data = "\x1b[21~"; break;
    case Qt::Key_F11: data = "\x1b[23~"; break;
    case Qt::Key_F12: data = "\x1b[24~"; break;
    default: return;
    }

    if (!data.isEmpty()) {
        emit dataReady(data);
        event->accept();
    }
}

// ─── Mouse ────────────────────────────────────────────────────────────

QPoint TerminalWidget::cellPosFromPixel(const QPoint& pixel) const
{
    QSize cellSize = m_renderer->cellSize();
    if (cellSize.width() <= 0 || cellSize.height() <= 0) {
        return QPoint(0, 0);
    }
    int col = pixel.x() / cellSize.width();
    int row = pixel.y() / cellSize.height() + m_buffer->scrollOffset();
    return QPoint(col, row);
}

void TerminalWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint cell = cellPosFromPixel(event->pos());
        m_selection->startSelection(cell.x(), cell.y());
        m_selecting = true;
        update();
        event->accept();
    } else if (event->button() == Qt::MiddleButton) {
        m_clipboard->paste();
        event->accept();
    }
}

void TerminalWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_selecting) {
        QPoint cell = cellPosFromPixel(event->pos());
        m_selection->updateSelection(cell.x(), cell.y());
        update();
        event->accept();
    }
}

void TerminalWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        m_selection->endSelection();
        update();
        event->accept();
    }
}

void TerminalWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint cell = cellPosFromPixel(event->pos());
        // Select word at position
        if (cell.y() >= 0 && cell.y() < m_buffer->rows()) {
            int col = cell.x();
            int start = col;
            int end = col;

            // Find word boundaries
            while (start > 0 && m_buffer->cellAt(start - 1, cell.y()).ch != QChar(' ')) {
                start--;
            }
            while (end < m_buffer->columns() - 1 &&
                   m_buffer->cellAt(end + 1, cell.y()).ch != QChar(' ')) {
                end++;
            }

            m_selection->startSelection(start, cell.y());
            m_selection->updateSelection(end, cell.y());
            m_selection->endSelection();
            update();
        }
        event->accept();
    }
}

void TerminalWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #25262B; color: #DCDCDC; border: 1px solid #3C3F41; }"
        "QMenu::item:selected { background-color: #30323A; }"
    );

    QAction* copyAction = menu.addAction("复制 (Ctrl+C)");
    copyAction->setEnabled(m_selection->hasSelection());

    QAction* pasteAction = menu.addAction("粘贴 (Ctrl+V)");
    pasteAction->setEnabled(m_connected);

    menu.addSeparator();
    QAction* clearAction = menu.addAction("清屏 (Ctrl+L)");
    QAction* selectAllAction = menu.addAction("全选 (Ctrl+A)");

    QAction* selected = menu.exec(event->globalPos());
    if (selected == copyAction) {
        m_clipboard->copy(m_selection->selectedText());
    } else if (selected == pasteAction) {
        m_clipboard->paste();
    } else if (selected == clearAction) {
        clearTerminal();
    } else if (selected == selectAllAction) {
        m_selection->startSelection(0, 0);
        m_selection->updateSelection(m_buffer->columns() - 1, m_buffer->rows() - 1);
        update();
    }
}

void TerminalWidget::wheelEvent(QWheelEvent* event)
{
    int delta = event->angleDelta().y() / 120; // Lines to scroll
    int newOffset = m_buffer->scrollOffset() - delta;
    m_buffer->setScrollOffset(newOffset);
    update();
    event->accept();
}

// ─── Helpers ──────────────────────────────────────────────────────────

void TerminalWidget::sendText(const QString& text)
{
    emit dataReady(text.toUtf8());
}