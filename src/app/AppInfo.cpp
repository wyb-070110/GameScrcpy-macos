#include "AppInfo.h"

#include "QtConversions.h"

#include <gs/base/BuildInfo.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtGlobal>

namespace gs::app {

namespace {

QVariantList readLicenses()
{
    QFile file(QStringLiteral(":/licenses/licenses.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).array().toVariantList();
}

}

AppInfo::AppInfo(QObject* parent)
    : QObject(parent)
{
}

QString AppInfo::version() const
{
    return toQString(gs::buildInfo().version);
}

QString AppInfo::revision() const
{
    return toQString(gs::buildInfo().revision);
}

QString AppInfo::build() const
{
    return QStringLiteral("%1 · Clang %2.%3.%4 · Qt %5")
        .arg(QStringLiteral(GS_BUILD_CONFIG))
        .arg(__clang_major__)
        .arg(__clang_minor__)
        .arg(__clang_patchlevel__)
        .arg(QString::fromLatin1(qVersion()));
}

QVariantList AppInfo::licenses() const
{
    static const QVariantList cached = readLicenses();
    return cached;
}

}
