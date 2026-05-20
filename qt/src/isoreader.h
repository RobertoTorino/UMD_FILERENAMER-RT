#pragma once

#include <QByteArray>
#include <QFile>
#include <QList>
#include <QString>
#include <optional>

/**
 * Minimal read-only ISO 9660 reader.
 * Opens a .iso image and extracts files by their in-image path,
 * e.g. "PSP_GAME/PARAM.SFO".
 */
class IsoReader
{
public:
    explicit IsoReader(const QString &filePath);

    /** Open the image and parse the Primary Volume Descriptor. */
    bool open();
    void close();

    /**
     * Read the raw bytes of a file inside the image.
     * @param path  Forward-slash separated path, e.g. "PSP_GAME/ICON0.PNG".
     *              Case-insensitive (ISO 9660 stores names in upper-case).
     * @return File bytes, or an empty QByteArray if not found.
     */
    QByteArray readFile(const QString &path);

private:
    static constexpr int SECTOR_SIZE = 2048;
    static constexpr int PVD_SECTOR  = 16;

    struct DirRecord
    {
        quint32 lba;
        quint32 size;
        bool    isDir;
        QString name;   // upper-case, version suffix stripped
    };

    QFile   m_file;
    quint32 m_rootLba  = 0;
    quint32 m_rootSize = 0;

    QByteArray               readSectors(quint32 lba, quint32 byteCount);
    QList<DirRecord>         readDirectory(quint32 lba, quint32 byteSize);
    std::optional<DirRecord> findEntry(quint32 dirLba, quint32 dirSize, const QString &name);
};
