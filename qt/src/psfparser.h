#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

/**
 * Parser for Sony's PSF (param.sfo) binary format used in PSP games.
 *
 * Binary layout
 * ─────────────
 * Header (20 bytes)
 *   magic            uint32  0x46535000  ("\0PSF")
 *   version          uint32  0x00000101
 *   key_table_offset uint32
 *   val_table_offset uint32
 *   num_entries      uint32
 *
 * Index table  (num_entries × 16 bytes)
 *   key_offset  uint16
 *   align_type  uint8
 *   data_type   uint8   0=binary  2=utf-8 string  4=int32
 *   val_size    uint32  (used bytes)
 *   total_size  uint32  (padded bytes)
 *   data_offset uint32
 *
 * Key table   – null-terminated strings
 * Value table – raw data at data_offset, val_size bytes
 */
class PsfParser
{
public:
    struct PsfData
    {
        QString title;
        QString discId;
        QString discVersion;
        QString pspSystemVer;
    };

    /** Returns parsed metadata, or std::nullopt if @p data is not a valid PSF. */
    static std::optional<PsfData> parse(const QByteArray &data);

private:
    static constexpr quint32 PSF_MAGIC        = 0x46535000u;
    static constexpr quint8  TYPE_BINARY      = 0;
    static constexpr quint8  TYPE_UTF8_STRING = 2;
    static constexpr quint8  TYPE_INT32       = 4;
};
