#pragma once

#include <QByteArray>
#include <QFileInfo>
#include <QString>

struct Umd
{
    QString    id;
    QString    title;
    QString    translatedTitle;
    QString    version;
    QString    firmware;
    QByteArray icon0;      // raw PNG bytes; empty if not found
    QByteArray pic0;       // raw PNG bytes; empty if not found
    QByteArray pic1;       // raw PNG bytes; empty if not found
    QString    filePath;   // absolute path to the .iso file

    QString fileName() const { return QFileInfo(filePath).fileName(); }
    QString dir()      const { return QFileInfo(filePath).absolutePath(); }
    QString suffix()   const
    {
        const QString s = QFileInfo(filePath).suffix();
        return s.isEmpty() ? QString() : "." + s;
    }
};
