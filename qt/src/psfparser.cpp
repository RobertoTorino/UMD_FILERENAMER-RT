#include "psfparser.h"

#include <QtEndian>

std::optional<PsfParser::PsfData> PsfParser::parse(const QByteArray &data)
{
    // Minimum: 20-byte header.
    if (data.size() < 20)
        return std::nullopt;

    const auto *d = reinterpret_cast<const quint8 *>(data.constData());

    const quint32 magic = qFromLittleEndian<quint32>(d);
    if (magic != PSF_MAGIC)
        return std::nullopt;

    const quint32 keyOffset   = qFromLittleEndian<quint32>(d + 8);
    const quint32 valueOffset = qFromLittleEndian<quint32>(d + 12);
    const quint32 numEntries  = qFromLittleEndian<quint32>(d + 16);

    PsfData result;

    for (quint32 i = 0; i < numEntries; ++i)
    {
        const quint32 base = 20u + i * 16u;
        if (base + 16u > static_cast<quint32>(data.size()))
            break;

        const auto *e = d + base;

        const quint16 kOff     = qFromLittleEndian<quint16>(e);
        const quint8  dataType = e[3];
        const quint32 valSize  = qFromLittleEndian<quint32>(e + 4);
        const quint32 dataOff  = qFromLittleEndian<quint32>(e + 12);

        // --- key ---
        const quint32 keyAbs = keyOffset + kOff;
        if (keyAbs >= static_cast<quint32>(data.size()))
            continue;
        const int keyEnd = data.indexOf('\0', static_cast<qsizetype>(keyAbs));
        if (keyEnd < 0)
            continue;
        const QString key = QString::fromLatin1(
            data.constData() + keyAbs,
            keyEnd - static_cast<int>(keyAbs));

        // --- value (only string type needed for metadata) ---
        if (dataType != TYPE_UTF8_STRING)
            continue;

        const quint32 valAbs = valueOffset + dataOff;
        if (valAbs + valSize > static_cast<quint32>(data.size()))
            continue;

        QByteArray raw = data.mid(static_cast<qsizetype>(valAbs),
                                  static_cast<qsizetype>(valSize));
        // Strip trailing null characters that PSF strings may include.
        while (!raw.isEmpty() && raw.back() == '\0')
            raw.chop(1);

        const QString val = QString::fromUtf8(raw);

        if      (key == QStringLiteral("TITLE"))          result.title       = val;
        else if (key == QStringLiteral("DISC_ID"))        result.discId      = val;
        else if (key == QStringLiteral("DISC_VERSION"))   result.discVersion = val;
        else if (key == QStringLiteral("PSP_SYSTEM_VER")) result.pspSystemVer = val;
    }

    return result;
}
