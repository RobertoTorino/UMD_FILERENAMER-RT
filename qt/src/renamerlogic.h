#pragma once

#include "umd.h"

#include <QList>
#include <QString>
#include <optional>

enum class NamingTemplate
{
    IdTitle,
    IdTitleSpace,
    TitleId,
    TitleIdSpace,
    TitleOnly,
    IdTitleVersion,
    TitleBracketId,
    TitleBracketIdVersion
};

/**
 * Core application logic: read, format, and rename PSP ISO files.
 */
class RenamerLogic
{
public:
    /**
        * Open a single .iso file, parse PSP_GAME/PARAM.SFO and the common image
        * resources (ICON0.PNG, PIC0.PNG, PIC1.PNG), and return a populated Umd struct.
     * Returns std::nullopt if the file cannot be read or is not a PSP ISO.
     */
    static std::optional<Umd> readUmd(const QString &filePath);

    /**
     * Scan @p dirPath for *.iso files and return a list of Umd structs
     * for every image that was successfully parsed.
     */
    static QList<Umd> scanDirectory(const QString &dirPath);

    /**
     * Build the target base filename (no extension) from the Umd metadata.
    * Supported formats include "ULJM05437 - Ever17", "ULJM05437 Ever17",
    * "Ever17 - ULJM05437", "Ever17 ULJM05437", "Ever17",
    * "Ever17 [ULJM05437]", "Ever17 [ULJM05437] [v1.01]", and
    * "ULJM05437 - Ever17 - v1.01".
     * Characters that are illegal in Windows/Linux filenames are removed.
     */
    static QString getFormattedName(const Umd &umd,
                                    NamingTemplate namingTemplate = NamingTemplate::IdTitle);

    /**
     * Rename the file backing @p umd to @p newBaseName (no extension).
     * The extension of the original file is preserved.
     * Updates umd.filePath on success.
     * @return true on success.
     */
    static bool renameFile(Umd &umd, const QString &newBaseName);
};
