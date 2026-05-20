#include "renamerlogic.h"
#include "isoreader.h"
#include "psfparser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

std::optional<Umd> RenamerLogic::readUmd(const QString &filePath)
{
    IsoReader iso(filePath);
    if (!iso.open())
        return std::nullopt;

    const QByteArray sfoData = iso.readFile(QStringLiteral("PSP_GAME/PARAM.SFO"));
    if (sfoData.isEmpty())
        return std::nullopt;

    // Attempt to read the game images; failure is non-fatal.
    const QByteArray icon0 = iso.readFile(QStringLiteral("PSP_GAME/ICON0.PNG"));
    const QByteArray pic0  = iso.readFile(QStringLiteral("PSP_GAME/PIC0.PNG"));
    const QByteArray pic1  = iso.readFile(QStringLiteral("PSP_GAME/PIC1.PNG"));
    iso.close();

    const auto psf = PsfParser::parse(sfoData);
    if (!psf)
        return std::nullopt;

    Umd umd;
    umd.id       = psf->discId;
    umd.title    = psf->title;
    umd.version  = psf->discVersion;
    umd.firmware = psf->pspSystemVer;
    umd.icon0    = icon0;
    umd.pic0     = pic0;
    umd.pic1     = pic1;
    umd.filePath = filePath;

    return umd;
}

QList<Umd> RenamerLogic::scanDirectory(const QString &dirPath)
{
    QList<Umd> result;

    QDir dir(dirPath);
    dir.setNameFilters({QStringLiteral("*.iso")});
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    for (const QFileInfo &fi : dir.entryInfoList())
    {
        auto umd = readUmd(fi.absoluteFilePath());
        if (umd)
            result.append(std::move(*umd));
    }

    return result;
}

QString RenamerLogic::getFormattedName(const Umd &umd, NamingTemplate namingTemplate)
{
    QString name;

    const QString version = umd.version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)
        ? umd.version
        : QStringLiteral("v") + umd.version;

    switch (namingTemplate)
    {
    case NamingTemplate::IdTitle:
        name = umd.id + QStringLiteral(" - ") + umd.title;
        break;
    case NamingTemplate::IdTitleVersion:
        name = umd.id + QStringLiteral(" - ") + umd.title + QStringLiteral(" - ") + version;
        break;
    case NamingTemplate::TitleId:
        name = umd.title + QStringLiteral(" - ") + umd.id;
        break;
    case NamingTemplate::TitleOnly:
        name = umd.title;
        break;
    }

    // Remove characters illegal in common filesystems.
    static const QString invalidChars = QStringLiteral("\\/:*?\"<>|");
    for (const QChar c : invalidChars)
        name.remove(c);

    // Remove trademark symbol (U+2122).
    name.remove(QChar(0x2122));

    return name;
}

bool RenamerLogic::renameFile(Umd &umd, const QString &newBaseName)
{
    const QFileInfo fi(umd.filePath);
    const QString newPath = fi.absolutePath()
                          + QLatin1Char('/')
                          + newBaseName
                          + QLatin1Char('.')
                          + fi.suffix();

    if (QFile::rename(umd.filePath, newPath))
    {
        umd.filePath = newPath;
        return true;
    }
    return false;
}
