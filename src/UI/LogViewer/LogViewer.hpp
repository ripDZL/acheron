#pragma once

#include <QWidget>

class QPlainTextEdit;
class QComboBox;
class QLineEdit;
class QCheckBox;

namespace Acheron {
namespace UI {

// Standalone window showing the live application event log (the same lines that
// go to acheron.log), with level and text filtering. Opened with Ctrl+L.
class LogViewer : public QWidget
{
    Q_OBJECT
public:
    explicit LogViewer(QWidget *parent = nullptr);
    ~LogViewer() override;

private:
    void rebuild();                            // re-render from the broadcaster snapshot
    void onAppended(const QString &line, int type);
    bool passes(const QString &line, int type) const;
    void appendLine(const QString &line, int type);
    static int severityRank(int type);

    QPlainTextEdit *m_text = nullptr;
    QComboBox *m_levelFilter = nullptr;
    QLineEdit *m_search = nullptr;
    QCheckBox *m_autoscroll = nullptr;
};

} // namespace UI
} // namespace Acheron
