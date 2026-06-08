#pragma once

#include <QWidget>
#include <QLabel>
#include <QColor>
#include <functional>
#include "Core/Snowflake.hpp"
#include "Core/TypingTracker.hpp"

namespace Acheron {
namespace UI {

class TypingIndicator : public QWidget
{
    Q_OBJECT
public:
    explicit TypingIndicator(QWidget *parent = nullptr);

    using RoleColorResolver = std::function<QColor(Core::Snowflake userId, Core::Snowflake guildId)>;
    void setRoleColorResolver(RoleColorResolver resolver);

    void setTypers(const QList<Core::TyperInfo> &typers);

    // Global preference: when false, the typing indicator is never shown.
    static bool showTyping();
    static void setShowTyping(bool on);

private:
    QLabel *label;
    RoleColorResolver roleColorResolver;

    QString formatText(const QList<Core::TyperInfo> &typers);
    QString coloredName(const Core::TyperInfo &typer);
};

} // namespace UI
} // namespace Acheron
