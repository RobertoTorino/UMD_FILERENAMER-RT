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
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QUrl>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>

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
    central->setObjectName(QStringLiteral("centralRoot"));
    setCentralWidget(central);

    QVBoxLayout *root = new QVBoxLayout(central);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    QWidget *topPanel = new QWidget(this);
    topPanel->setObjectName(QStringLiteral("topPanel"));
    QHBoxLayout *topBar = new QHBoxLayout(topPanel);
    topBar->setContentsMargins(8, 6, 8, 6);
    topBar->addWidget(new QLabel(QStringLiteral("Source:"), this));

    m_sourceLabel = new QLabel(QStringLiteral("(none)"), this);
    m_sourceLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_sourceLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    topBar->addWidget(m_sourceLabel, 1);

    topBar->addWidget(new QLabel(QStringLiteral("Naming:"), this));
    m_templateCombo = new QComboBox(this);
    m_templateCombo->addItem(QStringLiteral("ID - Title"));
    m_templateCombo->addItem(QStringLiteral("ID Title"));
    m_templateCombo->addItem(QStringLiteral("ID - Title - Version"));
    m_templateCombo->addItem(QStringLiteral("Title - ID"));
    m_templateCombo->addItem(QStringLiteral("Title Id"));
    m_templateCombo->addItem(QStringLiteral("Title only"));
    m_templateCombo->addItem(QStringLiteral("Title [ID]"));
    m_templateCombo->addItem(QStringLiteral("Title [ID] [Version]"));
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
    QMenu *settingsMenu = new QMenu(this);
    settingsMenu->addAction(QStringLiteral("Set PPSSPP Path..."), this, &MainWindow::onSetPpssppPath);
    settingsMenu->addAction(QStringLiteral("Set UMDGEN Path..."), this, &MainWindow::onSetUmdGenPath);
    settingsMenu->addAction(QStringLiteral("Set WQSG_UMD Path..."), this, &MainWindow::onSetWqsgUmdPath);
    connect(m_settingsBtn, &QToolButton::clicked, this, [this, settingsMenu]() {
        settingsMenu->exec(m_settingsBtn->mapToGlobal(QPoint(0, m_settingsBtn->height())));
    });
    topBar->addWidget(m_settingsBtn);

    root->addWidget(topPanel);

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
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_table->horizontalHeader()->setSortIndicatorShown(false);
    m_table->setObjectName(QStringLiteral("gamesTable"));
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setStyleSheet(QStringLiteral("QTableView::item { padding-left: 8px; padding-right: 8px; }"));
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
    m_titleEdit    = new QLineEdit(this); m_titleEdit->setReadOnly(true);
    m_translateBtn = new QPushButton(QStringLiteral("Translate"), this);
    m_translateBtn->setToolTip(QStringLiteral("Open Google Translate for the selected title"));
    m_translateBtn->setEnabled(false);
    connect(m_translateBtn, &QPushButton::clicked, this, [this]() {
        const QString title = m_titleEdit->text().trimmed();
        QUrl url(QStringLiteral("https://translate.google.com/?hl=en&sl=ja&tl=en&op=translate"));
        if (!title.isEmpty())
            url.setQuery(url.query() + QStringLiteral("&text=") + QString::fromLatin1(QUrl::toPercentEncoding(title)));
        QDesktopServices::openUrl(url);
    });
    m_translationEdit = new QLineEdit(this);
    m_versionEdit  = new QLineEdit(this); m_versionEdit->setReadOnly(true);
    m_firmwareEdit = new QLineEdit(this); m_firmwareEdit->setReadOnly(true);
    m_titleEdit->installEventFilter(this);
    form->addRow(QStringLiteral("ID:"), m_idEdit);
    QWidget *titleRow = new QWidget(this);
    QHBoxLayout *titleRowLayout = new QHBoxLayout(titleRow);
    titleRowLayout->setContentsMargins(0, 0, 0, 0);
    titleRowLayout->setSpacing(6);
    titleRowLayout->addWidget(m_titleEdit, 1);
    titleRowLayout->addWidget(m_translateBtn);
    form->addRow(QStringLiteral("Title:"), titleRow);
    form->addRow(QStringLiteral("Translation:"), m_translationEdit);
    form->addRow(QStringLiteral("Version:"), m_versionEdit);
    form->addRow(QStringLiteral("Firmware:"), m_firmwareEdit);
    detailLayout->addLayout(form, 1);

    QVBoxLayout *actionsLayout = new QVBoxLayout;
    actionsLayout->setSpacing(8);

    m_applyTranslationBtn = new QPushButton(QStringLiteral("Apply translation"), this);
    m_applyTranslationBtn->setEnabled(false);
    connect(m_applyTranslationBtn, &QPushButton::clicked, this, &MainWindow::onApplyTranslation);
    actionsLayout->addWidget(m_applyTranslationBtn);

    m_applyBtn = new QPushButton(QStringLiteral("Apply title change"), this);
    m_applyBtn->setEnabled(false);
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApplyChanges);
    actionsLayout->addWidget(m_applyBtn);
    actionsLayout->addStretch(1);
    detailLayout->addLayout(actionsLayout);

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

void MainWindow::onSetUmdGenPath()
{
    const QString startPath = m_umdGenPath.isEmpty()
        ? m_currentFolder
        : QFileInfo(m_umdGenPath).absolutePath();

    const QString selected = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Select UMDGEN executable"),
        startPath,
        QStringLiteral("Executable (*.exe)"));

    if (selected.isEmpty())
        return;

    m_umdGenPath = selected;
    QSettings settings;
    settings.setValue(QStringLiteral("tools/umdGenPath"), m_umdGenPath);
    statusBar()->showMessage(QStringLiteral("UMDGEN path saved."), 2500);
}

void MainWindow::onSetWqsgUmdPath()
{
    const QString startPath = m_wqsgUmdPath.isEmpty()
        ? m_currentFolder
        : QFileInfo(m_wqsgUmdPath).absolutePath();

    const QString selected = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Select WQSG_UMD executable"),
        startPath,
        QStringLiteral("Executable (*.exe)"));

    if (selected.isEmpty())
        return;

    m_wqsgUmdPath = selected;
    QSettings settings;
    settings.setValue(QStringLiteral("tools/wqsgUmdPath"), m_wqsgUmdPath);
    statusBar()->showMessage(QStringLiteral("WQSG_UMD path saved."), 2500);
}

void MainWindow::onLaunchGameClicked()
{
    QPushButton *playBtn = qobject_cast<QPushButton *>(sender());
    if (playBtn == nullptr)
        return;

    const QString isoPath = playBtn->property("isoPath").toString();
    launchGame(isoPath);
}

void MainWindow::launchGame(const QString &isoPath)
{
    if (isoPath.isEmpty())
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
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);
    m_table->setRowCount(m_umds.size());

    auto makeItem = [](const QString &text) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    for (int row = 0; row < m_umds.size(); ++row)
    {
        const Umd &u = m_umds[row];
        const QString displayTitle = u.translatedTitle.trimmed().isEmpty()
            ? u.title
            : u.translatedTitle.trimmed();
        QTableWidgetItem *idItem = makeItem(u.id);
        QTableWidgetItem *titleItem = makeItem(displayTitle);
        QTableWidgetItem *versionItem = makeItem(u.version);
        QTableWidgetItem *firmwareItem = makeItem(u.firmware);
        QTableWidgetItem *oldNameItem = makeItem(u.fileName());
        QTableWidgetItem *newNameItem = makeItem(QString());
        QTableWidgetItem *launchItem = makeItem(QString());

        idItem->setData(Qt::UserRole, u.filePath);
        titleItem->setData(Qt::UserRole, u.filePath);
        versionItem->setData(Qt::UserRole, u.filePath);
        firmwareItem->setData(Qt::UserRole, u.filePath);
        oldNameItem->setData(Qt::UserRole, u.filePath);
        newNameItem->setData(Qt::UserRole, u.filePath);
        launchItem->setData(Qt::UserRole, u.filePath);

        m_table->setItem(row, ColId, idItem);
        m_table->setItem(row, ColTitle, titleItem);
        m_table->setItem(row, ColVersion, versionItem);
        m_table->setItem(row, ColFirmware, firmwareItem);
        m_table->setItem(row, ColOldName, oldNameItem);
        m_table->setItem(row, ColNewName, newNameItem);
        m_table->setItem(row, ColLaunch, launchItem);

        QPushButton *playBtn = new QPushButton(m_table);
        playBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        playBtn->setToolTip(QStringLiteral("Launch in PPSSPP"));
        playBtn->setProperty("isoPath", u.filePath);
        connect(playBtn, &QPushButton::clicked, this, &MainWindow::onLaunchGameClicked);
        m_table->setCellWidget(row, ColLaunch, playBtn);
    }

    updatePreviewNames();
    m_table->resizeColumnsToContents();
    m_table->setSortingEnabled(true);
}

void MainWindow::updatePreviewNames()
{
    const NamingTemplate namingTemplate = selectedNamingTemplate();
    const bool sortingWasEnabled = m_table->isSortingEnabled();
    if (sortingWasEnabled)
        m_table->setSortingEnabled(false);

    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        QTableWidgetItem *item = m_table->item(row, ColNewName);
        const QTableWidgetItem *keyItem = m_table->item(row, ColOldName);
        if (item == nullptr || keyItem == nullptr)
            continue;

        const QString filePath = keyItem->data(Qt::UserRole).toString();
        const auto it = std::find_if(m_umds.cbegin(), m_umds.cend(), [&filePath](const Umd &u) {
            return u.filePath == filePath;
        });

        if (it != m_umds.cend())
            item->setText(RenamerLogic::getFormattedName(*it, namingTemplate) + it->suffix());
    }

    if (sortingWasEnabled)
        m_table->setSortingEnabled(true);
}

NamingTemplate MainWindow::selectedNamingTemplate() const
{
    switch (m_templateCombo->currentIndex())
    {
    case 1:
        return NamingTemplate::IdTitleSpace;
    case 2:
        return NamingTemplate::IdTitleVersion;
    case 3:
        return NamingTemplate::TitleId;
    case 4:
        return NamingTemplate::TitleIdSpace;
    case 5:
        return NamingTemplate::TitleOnly;
    case 6:
        return NamingTemplate::TitleBracketId;
    case 7:
        return NamingTemplate::TitleBracketIdVersion;
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
        m_translationEdit->clear();
        m_translateBtn->setEnabled(false);
        m_versionEdit->clear();
        m_firmwareEdit->clear();
        m_applyBtn->setEnabled(false);
        m_applyTranslationBtn->setEnabled(false);
        return;
    }

    const Umd &u = m_umds[row];
    m_previewIconIndex = 0;
    showSelectedPreviewIcon();

    m_idEdit->setText(u.id);
    m_titleEdit->setText(u.title);
    m_translateBtn->setEnabled(!u.title.trimmed().isEmpty());
    m_translationEdit->setText(u.translatedTitle.trimmed());
    m_versionEdit->setText(u.version);
    m_firmwareEdit->setText(u.firmware);
    m_applyBtn->setEnabled(true);
    m_applyTranslationBtn->setEnabled(true);
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

    const int tableRow = m_table->row(sel.first());
    const QTableWidgetItem *keyItem = m_table->item(tableRow, ColOldName);
    if (keyItem == nullptr)
        return;

    const QString filePath = keyItem->data(Qt::UserRole).toString();
    m_selectedRow = -1;
    for (int i = 0; i < m_umds.size(); ++i)
    {
        if (m_umds[i].filePath == filePath)
        {
            m_selectedRow = i;
            break;
        }
    }

    refreshDetail(m_selectedRow);
}

void MainWindow::onApplyChanges()
{
    if (m_selectedRow < 0 || m_selectedRow >= m_umds.size())
        return;

    Umd &u = m_umds[m_selectedRow];
    u.title = m_titleEdit->text().trimmed();

    refreshTable();
    refreshDetail(m_selectedRow);
}

void MainWindow::onApplyTranslation()
{
    if (m_selectedRow < 0 || m_selectedRow >= m_umds.size())
        return;

    Umd &u = m_umds[m_selectedRow];
    u.translatedTitle = m_translationEdit->text().trimmed();

    refreshTable();
    refreshDetail(m_selectedRow);
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
    m_umdGenPath = settings.value(QStringLiteral("tools/umdGenPath"), QString()).toString();
    m_wqsgUmdPath = settings.value(QStringLiteral("tools/wqsgUmdPath"), QString()).toString();
}

void MainWindow::saveSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("ui/namingTemplate"), m_templateCombo->currentIndex());
    settings.setValue(QStringLiteral("emulator/ppssppPath"), m_ppssppPath);
    settings.setValue(QStringLiteral("tools/umdGenPath"), m_umdGenPath);
    settings.setValue(QStringLiteral("tools/wqsgUmdPath"), m_wqsgUmdPath);
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

    refreshTable();
}
