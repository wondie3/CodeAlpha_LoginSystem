// Application-wide Qt style sheet.
#pragma once

inline const char *appStyleSheet()
{
    return R"(
        QMainWindow, QDialog, QWidget#dashboard, QWidget#authPage { background: #f1f5f9; }
        QLabel { color: #1e293b; }
        QLabel#welcome { font-size: 15pt; font-weight: 600; color: #0f172a; }
        QLabel#title { font-size: 20pt; font-weight: 700; color: #0f172a; }
        QLabel#subtitle, QLabel#hint { color: #64748b; }
        QLabel#errorLabel { color: #b91c1c; background: #fee2e2; border-radius: 6px; padding: 6px 8px; }
        QLabel#successLabel { color: #166534; background: #dcfce7; border-radius: 6px; padding: 6px 8px; }
        QLabel#badge { color: #1d4ed8; background: #dbeafe; border-radius: 9px; padding: 2px 10px; font-weight: 600; }
        QLabel#strengthLabel { font-weight: 600; }
        QFrame#card { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 14px; }
        QTabWidget::pane { border: none; }
        QTabBar::tab {
            background: transparent; color: #64748b; padding: 9px 22px; font-weight: 600;
            border-bottom: 3px solid transparent;
        }
        QTabBar::tab:selected { color: #2563eb; border-bottom: 3px solid #2563eb; }
        QTabBar::tab:hover:!selected { color: #1e293b; }
        QGroupBox {
            font-weight: 600; color: #334155; background: #ffffff;
            border: 1px solid #e2e8f0; border-radius: 10px;
            margin-top: 14px; padding: 16px 12px 12px 12px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 6px; }
        QLineEdit {
            background: #ffffff; color: #0f172a; border: 1px solid #cbd5e1;
            border-radius: 6px; padding: 7px 9px; selection-background-color: #bfdbfe;
        }
        QLineEdit:focus { border: 2px solid #2563eb; padding: 6px 8px; }
        QPushButton {
            background: #ffffff; color: #1e293b; border: 1px solid #cbd5e1;
            border-radius: 6px; padding: 8px 14px;
        }
        QPushButton:hover { background: #f1f5f9; }
        QPushButton:pressed { background: #e2e8f0; }
        QPushButton:disabled { color: #94a3b8; background: #f8fafc; }
        QPushButton#primaryButton {
            background: #2563eb; color: #ffffff; border: none; font-weight: 600; padding: 10px 16px;
        }
        QPushButton#primaryButton:hover { background: #1d4ed8; }
        QToolButton#showButton { border: none; color: #2563eb; padding: 4px 8px; font-weight: 600; }
        QToolButton#showButton:hover { background: #eff6ff; border-radius: 6px; }
        QProgressBar { background: #e2e8f0; border: none; border-radius: 4px; }
        QProgressBar::chunk { border-radius: 4px; background: #94a3b8; }
        QProgressBar[level="1"]::chunk { background: #dc2626; }
        QProgressBar[level="2"]::chunk { background: #f97316; }
        QProgressBar[level="3"]::chunk { background: #eab308; }
        QProgressBar[level="4"]::chunk { background: #16a34a; }
       
QTableWidget {
    background: #ffffff;
    color: #1e293b;
    alternate-background-color: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 6px;
    selection-background-color: #dbeafe;
    selection-color: #0f172a;
}

QTableWidget::item {
    color: #1e293b;
}

QTableWidget::item:selected {
    background: #dbeafe;
    color: #0f172a;
}

        QHeaderView::section {
            background: #f1f5f9; color: #475569; font-weight: 600; border: none;
            border-bottom: 1px solid #e2e8f0; padding: 6px;
        }
        QStatusBar { color: #475569; }
        QMenuBar { background: #ffffff; }
    )";
}
