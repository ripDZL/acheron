#include "LogViewer.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

#include "Core/LogBroadcaster.hpp"

namespace Acheron {
namespace UI {

int LogViewer::severityRank(int type)
{
    // QtMsgType is not ordered by severity (QtInfoMsg == 4), so map explicitly.
    switch (type) {
    case QtDebugMsg:
        return 0;
    case QtInfoMsg:
        return 1;
    case QtWarningMsg:
        return 2;
    case QtCriticalMsg:
        return 3;
    case QtFatalMsg:
        return 4;
    default:
        return 1;
    }
}

LogViewer::LogViewer(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(tr("Acheron — Event Log"));
    setWindowFlag(Qt::Window, true);
    resize(960, 540);

    auto *layout = new QVBoxLayout(this);

    auto *controls = new QHBoxLayout();
    controls->addWidget(new QLabel(tr("Level:"), this));
    m_levelFilter = new QComboBox(this);
    m_levelFilter->addItem(tr("All"), 0);
    m_levelFilter->addItem(tr("Info and above"), 1);
    m_levelFilter->addItem(tr("Warnings and above"), 2);
    m_levelFilter->addItem(tr("Errors only"), 3);
    controls->addWidget(m_levelFilter);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Filter text…"));
    m_search->setClearButtonEnabled(true);
    controls->addWidget(m_search, 1);

    m_autoscroll = new QCheckBox(tr("Auto-scroll"), this);
    m_autoscroll->setChecked(true);
    controls->addWidget(m_autoscroll);

    auto *copyButton = new QPushButton(tr("Copy"), this);
    controls->addWidget(copyButton);
    auto *clearButton = new QPushButton(tr("Clear"), this);
    controls->addWidget(clearButton);

    layout->addLayout(controls);

    m_text = new QPlainTextEdit(this);
    m_text->setReadOnly(true);
    m_text->setMaximumBlockCount(10000);
    m_text->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_text, 1);

    rebuild();

    Core::LogBroadcaster::instance().addListener();
    connect(&Core::LogBroadcaster::instance(), &Core::LogBroadcaster::appended, this,
            &LogViewer::onAppended);
    connect(m_levelFilter, &QComboBox::currentIndexChanged, this, [this]() { rebuild(); });
    connect(m_search, &QLineEdit::textChanged, this, [this]() { rebuild(); });
    connect(copyButton, &QPushButton::clicked, this,
            [this]() { QApplication::clipboard()->setText(m_text->toPlainText()); });
    connect(clearButton, &QPushButton::clicked, this, [this]() { m_text->clear(); });
}

LogViewer::~LogViewer()
{
    Core::LogBroadcaster::instance().removeListener();
}

bool LogViewer::passes(const QString &line, int type) const
{
    if (severityRank(type) < m_levelFilter->currentData().toInt())
        return false;
    const QString needle = m_search->text();
    return needle.isEmpty() || line.contains(needle, Qt::CaseInsensitive);
}

void LogViewer::appendLine(const QString &line, int type)
{
    QString color;
    switch (type) {
    case QtWarningMsg:
        color = QStringLiteral("#f0b232");
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        color = QStringLiteral("#f23f43");
        break;
    case QtDebugMsg:
        color = QStringLiteral("#80848e");
        break;
    default:
        color.clear();
        break;
    }

    const QString escaped = line.toHtmlEscaped();
    if (color.isEmpty())
        m_text->appendHtml(QStringLiteral("<span>%1</span>").arg(escaped));
    else
        m_text->appendHtml(QStringLiteral("<span style=\"color:%1\">%2</span>").arg(color, escaped));
}

void LogViewer::onAppended(const QString &line, int type)
{
    if (!passes(line, type))
        return;
    appendLine(line, type);
    if (m_autoscroll->isChecked())
        m_text->verticalScrollBar()->setValue(m_text->verticalScrollBar()->maximum());
}

void LogViewer::rebuild()
{
    m_text->clear();
    const auto entries = Core::LogBroadcaster::instance().snapshot();
    for (const auto &e : entries) {
        if (passes(e.text, e.type))
            appendLine(e.text, e.type);
    }
    if (m_autoscroll->isChecked())
        m_text->verticalScrollBar()->setValue(m_text->verticalScrollBar()->maximum());
}

} // namespace UI
} // namespace Acheron
