#include <QString>
#include <QClipboard>
#include <QApplication>
#include <QRegularExpression>
#include <QThreadPool>
#include <KRunner/QueryMatch>
#include <QStringList>
#include <QIcon>
#include <QPainter>
#include <QBrush>
#include <krunner/abstractrunner.h>
#include <qlogging.h>
#include "commands.h"
#include "kastel.h"

const static QStringList formats({QLatin1String("hsl"), QLatin1String("cmyk"), QLatin1String("rgb"), QLatin1String("lab")});

static QIcon generateCircleIcon(const QColor& color, int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(color));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, size, size);
    return QIcon(pixmap);
}

kastel::kastel(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
{}

void kastel::match(KRunner::RunnerContext &context)
{
    if(!context.isValid()) return;

    QString query = context.query();
    static QRegularExpression hex(QLatin1String("^#([A-Fa-f0-9]{3}|[A-Fa-f0-9]{6})$"));
    static QRegularExpression rgb(QLatin1String("^rg(?:b|ba)\\(\\s*(\\d{1,3})\\s*,\\s*(\\d{1,3})\\s*,\\s*(\\d{1,3})\\s*(,\\s*(0|1|0?\\.\\d+))?\\s*\\)$"));
    static QRegularExpression mix(QLatin1String("^(.+?)\\s*\\+\\s*(.+?)(?:\\s+(darken|lighten)\\s+(\\d*\\.?\\d+))?$"));

    auto mixMatch = mix.match(query);
    if (mixMatch.hasMatch()) {
        if (!checkIfPastelInstalled()) {
            KRunner::QueryMatch m(this);
            m.setText(QLatin1String("Pastel execution error."));
            m.setIconName(QLatin1String("dialog-warning"));
            context.addMatch(m);
            return;
        }

        QString c1 = mixMatch.captured(1).trimmed();
        QString c2 = mixMatch.captured(2).trimmed();
        QString op = mixMatch.captured(3);
        QString amount = mixMatch.captured(4);

        QString mixed = execCommand(QLatin1String("pastel"),
            {QLatin1String("mix"), c1, c2}).second.trimmed();

        QString finalColor = mixed;
        if (!op.isEmpty() && !amount.isEmpty()) {
            finalColor = execCommand(QLatin1String("pastel"),
                {op, amount, mixed}).second.trimmed();
        }

        QString finalHex = execCommand(QLatin1String("pastel"),
            {QLatin1String("format"), QLatin1String("hex"), finalColor}).second.trimmed();

        QColor finalQColor = QColor::fromString(finalHex);
        if (!finalQColor.isValid()) return;

        QIcon icon = generateCircleIcon(finalQColor, 64);

        // Base result
        {
            KRunner::QueryMatch m(this);
            m.setText(finalHex);
            m.setIcon(icon);
            m.setSubtext(!op.isEmpty()
                ? QString(QLatin1String("%1(%2) of %3 + %4")).arg(op, amount, c1, c2)
                : QString(QLatin1String("mix of %1 + %2")).arg(c1, c2));
            m.setRelevance(1.0);
            context.addMatch(m);
        }

        // Format conversions
        for (const auto &format : formats) {
            QThreadPool::globalInstance()->start([&, format, finalColor, icon]() {
                QString out = execCommand(QLatin1String("pastel"),
                    {QLatin1String("format"), format, finalColor}).second.trimmed();
                KRunner::QueryMatch m(this);
                m.setText(out);
                m.setIcon(icon);
                m.setSubtext(format);
                m.setRelevance(0.95);
                context.addMatch(m);
            });
        }

        QThreadPool::globalInstance()->waitForDone();
        return;
    }

    QString match_format;
    if(rgb.match(query).hasMatch()) match_format = QLatin1String("rgb");
    else if(hex.match(query).hasMatch()) match_format = QLatin1String("hex");
    else {
        auto [exitCode, hexOut] = execCommand(QLatin1String("pastel"),
            {QLatin1String("format"), QLatin1String("hex"), query});
        if(exitCode != 0) return;
        match_format = QLatin1String("hex");
        query = hexOut.trimmed();
    }

    QIcon icon;
    if(checkIfPastelInstalled()) {
        if(match_format == QLatin1String("hex")) {
            icon = generateCircleIcon(QColor::fromString(query), 64);
        } else {
            QString out = execCommand(QLatin1String("pastel"), {QLatin1String("format"), QLatin1String("hex"), query}).second;
            icon = generateCircleIcon(QColor::fromString(out.mid(0, out.size()-1)), 64);
            KRunner::QueryMatch match(this);
            match.setText(out);
            match.setIcon(icon);
            match.setSubtext(QLatin1String("hex"));
            match.setRelevance(1.0);
            context.addMatch(match);
        }
    } else {
        KRunner::QueryMatch match(this);
        match.setText(QLatin1String("Pastel execution error."));
        match.setIconName(QLatin1String("dialog-warning"));
        context.addMatch(match);
        return;
    }

    for(const auto &format : formats) {
        if(match_format == format) continue;
        QThreadPool::globalInstance()->start([&, format, icon]() {
            KRunner::QueryMatch match(this);
            QString out = execCommand(QLatin1String("pastel"), {QLatin1String("format"), format, query}).second;
            match.setText(out);
            match.setIcon(icon);
            match.setSubtext(format);
            if(format == QLatin1String("rgb"))
                match.setRelevance(1.0);
            context.addMatch(match);
        });
    }
    QThreadPool::globalInstance()->waitForDone();
}

void kastel::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match)
{
    Q_UNUSED(context);
    QApplication::clipboard()->setText(match.text().chopped(1));
}

void kastel::reloadConfiguration() { }

K_PLUGIN_CLASS_WITH_JSON(kastel, "kastel.json")
#include "kastel.moc"
