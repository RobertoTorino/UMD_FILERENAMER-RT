#pragma once

#include "renamerlogic.h"
#include "umd.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QEvent>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>
#include <QToolButton>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onOpenFolder();
    void onOpenFile();
    void onSetPpssppPath();
    void onLaunchGame(int row);
    void onRenameAll();
    void onApplyChanges();
    void onSelectionChanged();
    void onNamingTemplateChanged(int index);

private:
    void setupUi();
    void loadFolder(const QString &path);
    void loadSingleFile(const QString &path);
    void refreshTable();
    void refreshDetail(int row);
    void updatePreviewNames();
    NamingTemplate selectedNamingTemplate() const;
    void loadSettings();
    void saveSettings() const;
    void showSelectedPreviewIcon();
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    QString      m_currentFolder;
    QString      m_ppssppPath;
    QList<Umd>   m_umds;
    int          m_selectedRow = -1;
    int          m_previewIconIndex = 0;

    // ── toolbar / header ──────────────────────────────────────
    QLabel       *m_sourceLabel   = nullptr;
    QComboBox    *m_templateCombo = nullptr;
    QPushButton  *m_openBtn       = nullptr;
    QPushButton  *m_openFileBtn   = nullptr;
    QToolButton  *m_settingsBtn   = nullptr;

    // ── game list ─────────────────────────────────────────────
    QTableWidget *m_table         = nullptr;

    // ── detail panel ──────────────────────────────────────────
    QLabel       *m_iconLabel     = nullptr;
    QLabel       *m_iconCaptionLabel = nullptr;
    QLineEdit    *m_idEdit        = nullptr;
    QLineEdit    *m_titleEdit     = nullptr;
    QLineEdit    *m_versionEdit   = nullptr;
    QLineEdit    *m_firmwareEdit  = nullptr;
    QPushButton  *m_applyBtn      = nullptr;

    // ── bottom bar ────────────────────────────────────────────
    QPushButton  *m_renameAllBtn  = nullptr;
};
