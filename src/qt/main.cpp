#include <compat/compat.h>

#include <QApplication>
#include <QDebug>
#include <QFontDatabase>
#include <QStringList>

MAIN_FUNCTION
{
    QApplication app(argc, argv);

    // Get the font Qt chose for the app
    QFont defaultFont = QApplication::font();
    qDebug() << "Default application font:";
    qDebug() << "  Family:" << defaultFont.family();
    qDebug() << "  Point size:" << defaultFont.pointSize();
    qDebug() << "  Weight:" << defaultFont.weight();
    qDebug() << "  Style:" << (defaultFont.italic() ? "Italic" : "Normal");

    QFontDatabase db;
    QStringList families = db.families();
    qDebug().noquote() << "=== Available fonts ===";
    for (const QString& family : families) {
        qDebug().noquote() << family;
    }

    return 0;
}
