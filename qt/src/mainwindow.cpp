#include "mainwindow.h"

#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

// ── Column indices ───────────────────────────────────────────────────────────
enum Col {
    ColId = 0,
    ColTitle,
    ColVersion,
    ColFirmware,
    ColOldName,
    ColNewName,
    ColLaunch,
    ColCount
};

// ── Construction ─────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("UMD File Renamer"));
    resize(1100, 700);
    setupUi();
    loadSettings();
}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *root = new QVBoxLayout(central);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    QHBoxLayout *topBar = new QHBoxLayout;
    topBar->addWidget(new QLabel(QStringLiteral("Source:"), this));

    m_sourceLabel = new QLabel(QStringLiteral("(none)"), this);
    m_sourceLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_sourceLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    topBar->addWidget(m_sourceLabel, 1);

    topBar->addWidget(new QLabel(QStringLiteral("Naming:"), this));
    m_templateCombo = new QComboBox(this);
    m_templateCombo->addItem(QStringLiteral("ID - Title"));
    m_templateCombo->addItem(QStringLiteral("ID - Title - Version"));
    m_templateCombo->addItem(QStringLiteral("Title - ID"));
    m_templateCombo->addItem(QStringLiteral("Title only"));
    connect(m_templateCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &MainWindow::onNamingTemplateChanged);
    topBar->addWidget(m_templateCombo);

    m_openBtn = new QPushButton(QStringLiteral("Open Folder..."), this);
    connect(m_openBtn, &QPushButton::clicked, this, &MainWindow::onOpenFolder);
    topBar->addWidget(m_openBtn);

    m_openFileBtn = new QPushButton(QStringLiteral("Open ISO..."), this);
    connect(m_openFileBtn, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    topBar->addWidget(m_openFileBtn);

    m_settingsBtn = new QToolButton(this);
    m_settingsBtn->setText(QStringLiteral("⚙"));
    m_settingsBtn->setToolTip(QStringLiteral("Settings"));
    m_settingsBtn->setAutoRaise(true);
    QMenu *settingsMenu = new QMenu(m_settingsBtn);
    settingsMenu->addAction(QStringLiteral("Set PPSSPP Path..."), this, &MainWindow::onSetPpssppPath);
    m_settingsBtn->setMenu(settingsMenu);
    m_settingsBtn->setPopupMode(QToolButton::InstantPopup);
    topBar->addWidget(m_settingsBtn);

    root->addLayout(topBar);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    root->addWidget(splitter, 1);

    m_table = new QTableWidget(0, ColCount, this);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("ID"),
        QStringLiteral("Title"),
        QStringLiteral("Version"),
        QStringLiteral("Firmware"),
        QStringLiteral("Current filename"),
        QStringLiteral("New filename"),
        QStringLiteral("Launch Game")
    });
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(ColTitle, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColNewName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColLaunch, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onSelectionChanged);
    splitter->addWidget(m_table);

    QGroupBox *detailBox = new QGroupBox(QStringLiteral("Game details"), this);
    QHBoxLayout *detailLayout = new QHBoxLayout(detailBox);

    QWidget *iconPanel = new QWidget(this);
    QVBoxLayout *iconLayout = new QVBoxLayout(iconPanel);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    iconLayout->setSpacing(4);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(288, 160);
    m_iconLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    iconLayout->addWidget(m_iconLabel);

    m_iconCaptionLabel = new QLabel(QStringLiteral("ICON0"), this);
    m_iconCaptionLabel->setAlignment(Qt::AlignCenter);
    iconLayout->addWidget(m_iconCaptionLabel);

    detailLayout->addWidget(iconPanel);

    QFormLayout *form = new QFormLayout;
    m_idEdit       = new QLineEdit(this); m_idEdit->setReadOnly(true);
    m_titleEdit    = new QLineEdit(this);
    m_versionEdit  = new QLineEdit(this); m_versionEdit->setReadOnly(true);
    m_firmwareEdit = new QLineEdit(this); m_firmwareEdit->setReadOnly(true);
    m_titleEdit->installEventFilter(this);
    form->addRow(QStringLiteral("ID:"), m_idEdit);
    form->addRow(QStringLiteral("Title:"), m_titleEdit);
    form->addRow(QStringLiteral("Version:"), m_versionEdit);
    form->addRow(QStringLiteral("Firmware:"), m_firmwareEdit);
    detailLayout->addLayout(form, 1);

    m_applyBtn = new QPushButton(QStringLiteral("Apply title change"), this);
    m_applyBtn->setEnabled(false);
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApplyChanges);
    detailLayout->addWidget(m_applyBtn, 0, Qt::AlignBottom);

    splitter->addWidget(detailBox);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    QHBoxLayout *bottomBar = new QHBoxLayout;
    m_renameAllBtn = new QPushButton(QStringLiteral("Rename All"), this);
    m_renameAllBtn->setEnabled(false);
    connect(m_renameAllBtn, &QPushButton::clicked, this, &MainWindow::onRenameAll);
    bottomBar->addStretch(1);
    bottomBar->addWidget(m_renameAllBtn);
    root->addLayout(bottomBar);

    statusBar()->showMessage(QStringLiteral("Ready"));
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void MainWindow::onOpenFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Select folder containing PSP ISO images"),
        m_currentFolder);

    if (dir.isEmpty())
        return;

    loadFolder(dir);
}

void MainWindow::onOpenFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Select PSP ISO image"),
        m_currentFolder,
        QStringLiteral("PSP ISO Images (*.iso)"));

    if (filePath.isEmpty())
        return;

    loadSingleFile(filePath);
}

void MainWindow::onSetPpssppPath()
{
    const QString startPath = m_ppssppPath.isEmpty()
        ? m_currentFolder
        : QFileInfo(m_ppssppPath).absolutePath();

    const QString selected = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Select PPSSPP executable"),
        startPath,
        QStringLiteral("Executable (*.exe)"));

    if (selected.isEmpty())
        return;

    m_ppssppPath = selected;
    QSettings settings;
    settings.setValue(QStringLiteral("emulator/ppssppPath"), m_ppssppPath);
    statusBar()->showMessage(QStringLiteral("PPSSPP path saved."), 2500);
}

void MainWindow::onLaunchGame(int row)
{
    if (row < 0 || row >= m_umds.size())
        return;

    if (m_ppssppPath.isEmpty() || !QFileInfo::exists(m_ppssppPath))
    {
        QMessageBox::information(
            this,
            QStringLiteral("PPSSPP path required"),
            QStringLiteral("Set the PPSSPP executable path first (Settings -> Set PPSSPP Path...)."));
        onSetPpssppPath();
        if (m_ppssppPath.isEmpty() || !QFileInfo::exists(m_ppssppPath))
            return;
    }

    const QString isoPath = m_umds[row].filePath;
    const QString workingDir = QFileInfo(m_ppssppPath).absolutePath();
    const QStringList args{QStringLiteral("--fullscreen"), isoPath};

    if (!QProcess::startDetached(m_ppssppPath, args, workingDir))
    {
        QMessageBox::warning(
            this,
            QStringLiteral("Launch failed"),
            QStringLiteral("Could not launch PPSSPP with the selected game."));
        return;
    }

    statusBar()->showMessage(QStringLiteral("Launching game in PPSSPP..."), 2500);
}

void MainWindow::loadFolder(const QString &path)
{
    m_currentFolder = path;
    m_sourceLabel->setText(path);
    m_selectedRow = -1;

    statusBar()->showMessage(QStringLiteral("Scanning..."));
    QApplication::processEvents();

    m_umds = RenamerLogic::scanDirectory(path);

    refreshTable();
    refreshDetail(-1);

    m_renameAllBtn->setEnabled(!m_umds.isEmpty());
    m_applyBtn->setEnabled(false);

    statusBar()->showMessage(QString::number(m_umds.size()) + QStringLiteral(" game(s) found."));
}

void MainWindow::loadSingleFile(const QString &path)
{
    m_currentFolder = QFileInfo(path).absolutePath();
    m_sourceLabel->setText(path);
    m_selectedRow = -1;

    statusBar()->showMessage(QStringLiteral("Loading ISO..."));
    QApplication::processEvents();

    m_umds.clear();
    const auto umd = RenamerLogic::readUmd(path);
    if (umd.has_value())
        m_umds.append(*umd);

    refreshTable();
    refreshDetail(-1);

    m_renameAllBtn->setEnabled(!m_umds.isEmpty());
    m_applyBtn->setEnabled(false);

    statusBar()->showMessage(QString::number(m_umds.size()) + QStringLiteral(" game(s) found."));
}

void MainWindow::refreshTable()
{
    m_table->setRowCount(0);
    m_table->setRowCount(m_umds.size());

    for (int row = 0; row < m_umds.size(); ++row)
    {
        const Umd &u = m_umds[row];
        m_table->setItem(row, ColId,       new QTableWidgetItem(u.id));
        m_table->setItem(row, ColTitle,    new QTableWidgetItem(u.title));
        m_table->setItem(row, ColVersion,  new QTableWidgetItem(u.version));
        m_table->setItem(row, ColFirmware, new QTableWidgetItem(u.firmware));
        m_table->setItem(row, ColOldName,  new QTableWidgetItem(u.fileName()));
        m_table->setItem(row, ColNewName,  new QTableWidgetItem());

        QPushButton *playBtn = new QPushButton(this);
        playBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        playBtn->setToolTip(QStringLiteral("Launch in PPSSPP"));
        connect(playBtn, &QPushButton::clicked, this, [this, row]() { onLaunchGame(row); });
        m_table->setCellWidget(row, ColLaunch, playBtn);
    }

    updatePreviewNames();
    m_table->resizeColumnsToContents();
}

void MainWindow::updatePreviewNames()
{
    const NamingTemplate namingTemplate = selectedNamingTemplate();

    for (int row = 0; row < m_umds.size(); ++row)
    {
        const Umd &u = m_umds[row];
        QTableWidgetItem *item = m_table->item(row, ColNewName);
        if (item != nullptr)
            item->setText(RenamerLogic::getFormattedName(u, namingTemplate) + u.suffix());
    }
}

NamingTemplate MainWindow::selectedNamingTemplate() const
{
    switch (m_templateCombo->currentIndex())
    {
    case 1:
        return NamingTemplate::IdTitleVersion;
    case 2:
        return NamingTemplate::TitleId;
    case 3:
        return NamingTemplate::TitleOnly;
    default:
        return NamingTemplate::IdTitle;
    }
}

void MainWindow::refreshDetail(int row)
{
    if (row < 0 || row >= m_umds.size())
    {
        m_iconLabel->clear();
        m_iconLabel->setText(QStringLiteral("No icon"));
        m_iconLabel->setToolTip(QString());
        m_iconCaptionLabel->setText(QStringLiteral("No icon"));
        m_previewIconIndex = 0;
        m_idEdit->clear();
        m_titleEdit->clear();
        m_versionEdit->clear();
        m_firmwareEdit->clear();
        m_applyBtn->setEnabled(false);
        return;
    }

    const Umd &u = m_umds[row];
    m_previewIconIndex = 0;
    showSelectedPreviewIcon();

    m_idEdit->setText(u.id);
    m_titleEdit->setText(u.title);
    m_versionEdit->setText(u.version);
    m_firmwareEdit->setText(u.firmware);
    m_applyBtn->setEnabled(true);
}

void MainWindow::onSelectionChanged()
{
    const QList<QTableWidgetItem *> sel = m_table->selectedItems();
    if (sel.isEmpty())
    {
        m_selectedRow = -1;
        refreshDetail(-1);
        return;
    }

    m_selectedRow = m_table->row(sel.first());
    refreshDetail(m_selectedRow);
}

void MainWindow::onApplyChanges()
{
    if (m_selectedRow < 0 || m_selectedRow >= m_umds.size())
        return;

    Umd &u = m_umds[m_selectedRow];
    u.title = m_titleEdit->text().trimmed();

    m_table->item(m_selectedRow, ColNewName)
        ->setText(RenamerLogic::getFormattedName(u, selectedNamingTemplate()) + u.suffix());
}

void MainWindow::onNamingTemplateChanged(int index)
{
    QSettings settings;
    settings.setValue(QStringLiteral("ui/namingTemplate"), index);
    updatePreviewNames();
}

void MainWindow::loadSettings()
{
    QSettings settings;
    const int index = settings.value(QStringLiteral("ui/namingTemplate"), 0).toInt();
    m_templateCombo->setCurrentIndex(index < 0 || index >= m_templateCombo->count() ? 0 : index);
    m_ppssppPath = settings.value(QStringLiteral("emulator/ppssppPath"), QString()).toString();
}

void MainWindow::saveSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("ui/namingTemplate"), m_templateCombo->currentIndex());
    settings.setValue(QStringLiteral("emulator/ppssppPath"), m_ppssppPath);
}

void MainWindow::showSelectedPreviewIcon()
{
    if (m_selectedRow < 0 || m_selectedRow >= m_umds.size())
        return;

    const Umd &u = m_umds[m_selectedRow];
    const QByteArray *iconData = nullptr;
    QString iconName;

    switch (m_previewIconIndex)
    {
    case 1:
        iconData = &u.pic0;
        iconName = QStringLiteral("PIC0.PNG");
        break;
    case 2:
        iconData = &u.pic1;
        iconName = QStringLiteral("PIC1.PNG");
        break;
    default:
        iconData = &u.icon0;
        iconName = QStringLiteral("ICON0.PNG");
        break;
    }

    if (iconData != nullptr && !iconData->isEmpty())
    {
        QPixmap px;
        if (px.loadFromData(*iconData))
        {
            m_iconLabel->setPixmap(px.scaled(m_iconLabel->size(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation));
            m_iconLabel->setToolTip(iconName);
            m_iconCaptionLabel->setText(iconName.left(iconName.indexOf(QLatin1Char('.'))));
            return;
        }
    }

    m_iconLabel->clear();
    m_iconLabel->setText(QStringLiteral("No icon"));
    m_iconLabel->setToolTip(QString());
    m_iconCaptionLabel->setText(QStringLiteral("No icon"));
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_titleEdit && event->type() == QEvent::MouseButtonPress)
    {
        if (m_selectedRow >= 0 && m_selectedRow < m_umds.size())
        {
            const Umd &u = m_umds[m_selectedRow];
            if (!u.icon0.isEmpty() || !u.pic0.isEmpty() || !u.pic1.isEmpty())
            {
                m_previewIconIndex = (m_previewIconIndex + 1) % 3;
                showSelectedPreviewIcon();
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::onRenameAll()
{
    int renamed = 0;
    int failed = 0;
    QStringList errors;

    for (int i = 0; i < m_umds.size(); ++i)
    {
        Umd &u = m_umds[i];
        const QString newBase = RenamerLogic::getFormattedName(u, selectedNamingTemplate());

        if (RenamerLogic::renameFile(u, newBase))
        {
            ++renamed;
            m_table->item(i, ColOldName)->setText(u.fileName());
            m_table->item(i, ColNewName)->setText(newBase + u.suffix());
        }
        else
        {
            ++failed;
            errors << QStringLiteral("  ") + u.fileName()
                   + QStringLiteral(" -> ") + newBase + u.suffix();
        }
    }

    if (failed == 0)
    {
        statusBar()->showMessage(QString::number(renamed) + QStringLiteral(" file(s) renamed successfully."));
    }
    else
    {
        const QString msg = QString::number(renamed)
            + QStringLiteral(" renamed, ")
            + QString::number(failed)
            + QStringLiteral(" failed:\n")
            + errors.join(QLatin1Char('\n'));
        QMessageBox::warning(this, QStringLiteral("Rename errors"), msg);
        statusBar()->showMessage(QStringLiteral("Done with errors."));
    }
}
