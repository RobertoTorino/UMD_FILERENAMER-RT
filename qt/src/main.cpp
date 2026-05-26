#include "mainwindow.h"

#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QPalette>

static void applyMetallicBlueTheme(QApplication &app)
{
    app.setStyle(QStringLiteral("Fusion"));

    QPalette palette;
    const QColor windowBg("#071726");
    const QColor panelBg("#0C2035");
    const QColor controlBg("#0A1B2D");
    const QColor border("#4E80B4");
    const QColor text("#D9F4FF");
    const QColor mutedText("#7FA6BF");
    const QColor accent("#2E6FA7");
    const QColor accentLight("#64B7FF");

    palette.setColor(QPalette::Window, windowBg);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, controlBg);
    palette.setColor(QPalette::AlternateBase, panelBg);
    palette.setColor(QPalette::ToolTipBase, text);
    palette.setColor(QPalette::ToolTipText, windowBg);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, panelBg);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, accentLight);
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    palette.setColor(QPalette::Light, QColor("#7BB4E8"));
    palette.setColor(QPalette::Mid, border);
    palette.setColor(QPalette::Dark, windowBg.darker(150));
    palette.setColor(QPalette::Midlight, QColor("#3C6B99"));
    palette.setColor(QPalette::PlaceholderText, mutedText);

    app.setPalette(palette);
    app.setStyleSheet(QStringLiteral(R"(
        QMainWindow {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #061321,
                                        stop:0.55 #0A2237,
                                        stop:1 #05111E);
            color: #D9F4FF;
        }

        QWidget#centralRoot {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #0A1D30,
                                        stop:0.5 #0D2840,
                                        stop:1 #0A1C2F);
            border: 1px solid #4E80B4;
            border-radius: 14px;
        }

        QWidget#topPanel {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #12314C,
                                        stop:0.5 #1D4C73,
                                        stop:1 #143552);
            border: 1px solid #5C93C6;
            border-radius: 10px;
        }

        QGroupBox {
            border: 1px solid #4E80B4;
            border-radius: 10px;
            margin-top: 10px;
            padding-top: 10px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #102840,
                                        stop:1 #0B1E31);
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
            color: #BDEEFF;
        }

        QLabel {
            color: #D9F4FF;
            background: transparent;
        }

        QLineEdit, QComboBox, QMenu {
            background-color: #0A1B2D;
            color: #D9F4FF;
            border: 1px solid #4E80B4;
            border-radius: 8px;
            padding: 4px 6px;
            selection-background-color: #2E6FA7;
            selection-color: #FFFFFF;
        }

        QComboBox::drop-down {
            border: none;
            width: 18px;
            background: transparent;
        }

        QComboBox QAbstractItemView, QMenu {
            background-color: #0A1B2D;
            color: #D9F4FF;
            selection-background-color: #2E6FA7;
            selection-color: #FFFFFF;
            border: 1px solid #4E80B4;
            border-radius: 8px;
        }

        QTableWidget {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #0B2136,
                                        stop:1 #091A2B);
            border: 1px solid #4E80B4;
            border-radius: 10px;
            gridline-color: #355C82;
            alternate-background-color: #0B2134;
            color: #D9F4FF;
        }

        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #2D6FA2,
                                        stop:1 #18456D);
            color: #D9F4FF;
            padding: 6px;
            border: 1px solid #5A95CA;
        }

        QHeaderView::section:first {
            border-top-left-radius: 10px;
        }

        QHeaderView::section:last {
            border-top-right-radius: 10px;
        }

        QHeaderView::up-arrow, QHeaderView::down-arrow {
            width: 0px;
            height: 0px;
            image: none;
        }

        QTableWidget::item:selected {
            background-color: #265F93;
            color: #FFFFFF;
        }

        QPushButton, QToolButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #2E6FA4,
                                        stop:1 #18476F);
            color: #D9F4FF;
            border: 1px solid #7CC2FF;
            border-radius: 8px;
            padding: 6px 10px;
        }

        QPushButton:hover, QToolButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #3D84BC,
                                        stop:1 #1F5C8D);
        }

        QPushButton:pressed, QToolButton:pressed {
            background: #113450;
        }

        QPushButton:disabled, QToolButton:disabled {
            color: #7A93A8;
            background-color: #0A1A2A;
            border-color: #3A5875;
        }

        QMenu::item {
            padding: 6px 12px;
            border-radius: 6px;
        }

        QMenu::item:selected {
            background-color: #265F93;
        }
    )"));
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("UMD File Renamer"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("diegohp"));
    app.setWindowIcon(QIcon(QStringLiteral(":/app.ico")));
    applyMetallicBlueTheme(app);

    MainWindow w;
    w.setWindowIcon(QIcon(QStringLiteral(":/app.ico")));
    w.show();

    return app.exec();
}
