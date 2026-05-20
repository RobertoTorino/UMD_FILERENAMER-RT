#include "isoreader.h"

#include <QtEndian>

IsoReader::IsoReader(const QString &filePath)
    : m_file(filePath)
{}

bool IsoReader::open()
{
    if (!m_file.open(QIODevice::ReadOnly))
        return false;

    // Primary Volume Descriptor lives at sector 16.
    if (!m_file.seek(static_cast<qint64>(PVD_SECTOR) * SECTOR_SIZE))
        return false;

    const QByteArray pvd = m_file.read(SECTOR_SIZE);
    if (pvd.size() < SECTOR_SIZE)
        return false;

    // Byte 0 = descriptor type (1 = PVD); bytes 1-5 = "CD001".
    if (static_cast<quint8>(pvd[0]) != 1u)
        return false;
    if (pvd.mid(1, 5) != QByteArray("CD001"))
        return false;

    // Root Directory Record is embedded at byte 156 inside the PVD.
    const auto *root = reinterpret_cast<const quint8 *>(pvd.constData() + 156);
    m_rootLba  = qFromLittleEndian<quint32>(root + 2);
    m_rootSize = qFromLittleEndian<quint32>(root + 10);

    return true;
}

void IsoReader::close()
{
    m_file.close();
}

// ---------------------------------------------------------------------------

QByteArray IsoReader::readSectors(quint32 lba, quint32 byteCount)
{
    if (!m_file.seek(static_cast<qint64>(lba) * SECTOR_SIZE))
        return {};
    return m_file.read(byteCount);
}

QList<IsoReader::DirRecord> IsoReader::readDirectory(quint32 lba, quint32 byteSize)
{
    QList<DirRecord> entries;
    const QByteArray data = readSectors(lba, byteSize);
    if (data.isEmpty())
        return entries;

    int pos = 0;
    while (pos < static_cast<int>(byteSize))
    {
        const auto len = static_cast<quint8>(data[pos]);

        if (len == 0)
        {
            // Padding at end of sector — advance to next sector boundary.
            const int nextSector = (pos / SECTOR_SIZE + 1) * SECTOR_SIZE;
            pos = nextSector;
            continue;
        }

        if (pos + len > static_cast<int>(byteSize))
            break;

        const auto *rec = reinterpret_cast<const quint8 *>(data.constData() + pos);

        DirRecord dr;
        dr.lba   = qFromLittleEndian<quint32>(rec + 2);
        dr.size  = qFromLittleEndian<quint32>(rec + 10);
        dr.isDir = (rec[25] & 0x02) != 0;

        const quint8 nameLen = rec[32];
        if (nameLen == 1 && rec[33] == 0x00)
        {
            dr.name = QStringLiteral(".");
        }
        else if (nameLen == 1 && rec[33] == 0x01)
        {
            dr.name = QStringLiteral("..");
        }
        else
        {
            dr.name = QString::fromLatin1(
                reinterpret_cast<const char *>(rec + 33), nameLen);
            // ISO 9660 appends a version suffix ";1" to file identifiers.
            const int semi = dr.name.indexOf(QLatin1Char(';'));
            if (semi >= 0)
                dr.name = dr.name.left(semi);
        }

        entries.append(dr);
        pos += len;
    }

    return entries;
}

std::optional<IsoReader::DirRecord>
IsoReader::findEntry(quint32 dirLba, quint32 dirSize, const QString &name)
{
    const QString upper = name.toUpper();
    for (const DirRecord &dr : readDirectory(dirLba, dirSize))
    {
        if (dr.name.toUpper() == upper)
            return dr;
    }
    return std::nullopt;
}

QByteArray IsoReader::readFile(const QString &path)
{
    const QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return {};

    quint32 curLba  = m_rootLba;
    quint32 curSize = m_rootSize;

    for (int i = 0; i < parts.size(); ++i)
    {
        auto entry = findEntry(curLba, curSize, parts[i]);
        if (!entry)
            return {};

        if (i == parts.size() - 1)
        {
            // Final component — read the file data.
            return readSectors(entry->lba, entry->size);
        }
        else
        {
            // Intermediate component must be a directory.
            if (!entry->isDir)
                return {};
            curLba  = entry->lba;
            curSize = entry->size;
        }
    }

    return {};
}
