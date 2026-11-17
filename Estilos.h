#ifndef ESTILOS_H
#define ESTILOS_H

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QStyleFactory>
#include <QWidget>

// Tema claro explícito e independiente del tema del sistema operativo.
// Evita texto claro sobre fondos claros / texto oscuro sobre fondos oscuros.
namespace Estilos {

inline QString familiaFuente() {
    const QStringList disponibles = QFontDatabase::families();
    const QStringList candidatas = {
#if defined(Q_OS_WIN)
        QStringLiteral("Segoe UI"),
        QStringLiteral("Calibri"),
        QStringLiteral("Arial")
#elif defined(Q_OS_MACOS) || defined(Q_OS_MAC)
        QStringLiteral("Helvetica Neue"),
        QStringLiteral("SF Pro Text"),
        QStringLiteral(".AppleSystemUIFont"),
        QStringLiteral("Arial")
#else
        QStringLiteral("Noto Sans"),
        QStringLiteral("DejaVu Sans"),
        QStringLiteral("Liberation Sans")
#endif
    };
    for (const QString& candidata : candidatas) {
        if (disponibles.contains(candidata, Qt::CaseInsensitive)) return candidata;
    }
    return QApplication::font().family();
}

inline QString hojaGlobal() {
    return QStringLiteral(
        "QWidget { color: #111827; }"
        "QMainWindow, QDialog { background-color: #f8f9fa; }"
        "QToolTip { background-color: #ffffff; color: #111827; border: 1px solid #d1d5db; padding: 4px; }"
        "QMenu { background-color: #ffffff; color: #111827; border: 1px solid #d1d5db; }"
        "QMenu::item:selected { background-color: #e5e7eb; color: #111827; }"
        "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QDateEdit, QTextEdit, QPlainTextEdit {"
        "  color: #111827; background-color: #ffffff;"
        "  selection-background-color: #047857; selection-color: #ffffff; }"
        "QComboBox QAbstractItemView { background-color: #ffffff; color: #111827;"
        "  selection-background-color: #d1fae5; selection-color: #065f46; }"
        "QTableWidget, QTableView, QTreeView, QListView {"
        "  background-color: #ffffff; color: #111827; alternate-background-color: #f9fafb; }"
        "QHeaderView::section { background-color: #f9fafb; color: #374151; font-weight: bold; }"
        "QTableCornerButton::section { background-color: #f9fafb; }"
        "QTabWidget::pane { border: 1px solid #e5e7eb; background-color: #ffffff; }"
        "QTabBar::tab { background-color: #f3f4f6; color: #374151; padding: 8px 16px;"
        "  border: 1px solid #e5e7eb; border-bottom: none; }"
        "QTabBar::tab:selected { background-color: #ffffff; color: #047857; font-weight: bold; }"
        "QCheckBox, QRadioButton { color: #111827; }"
        "QTextBrowser { background-color: #ffffff; color: #111827; }"
        "QCalendarWidget QWidget { alternate-background-color: #f9fafb; }"
        "QCalendarWidget QAbstractItemView { background-color: #ffffff; color: #111827;"
        "  selection-background-color: #047857; selection-color: #ffffff; }"
        "QStatusBar { color: #374151; }"
    );
}

inline void aplicarTemaClaro(QApplication& app) {
    if (QStyleFactory::keys().contains(QStringLiteral("Fusion"), Qt::CaseInsensitive)) {
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    }

    QPalette paleta;
    paleta.setColor(QPalette::Window, QColor(QStringLiteral("#f8f9fa")));
    paleta.setColor(QPalette::WindowText, QColor(QStringLiteral("#111827")));
    paleta.setColor(QPalette::Base, QColor(QStringLiteral("#ffffff")));
    paleta.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#f9fafb")));
    paleta.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#ffffff")));
    paleta.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#111827")));
    paleta.setColor(QPalette::Text, QColor(QStringLiteral("#111827")));
    paleta.setColor(QPalette::Button, QColor(QStringLiteral("#ffffff")));
    paleta.setColor(QPalette::ButtonText, QColor(QStringLiteral("#111827")));
    paleta.setColor(QPalette::BrightText, QColor(QStringLiteral("#b91c1c")));
    paleta.setColor(QPalette::Link, QColor(QStringLiteral("#047857")));
    paleta.setColor(QPalette::Highlight, QColor(QStringLiteral("#047857")));
    paleta.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#ffffff")));
    paleta.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#6b7280")));

    paleta.setColor(QPalette::Disabled, QPalette::WindowText, QColor(QStringLiteral("#9ca3af")));
    paleta.setColor(QPalette::Disabled, QPalette::Text, QColor(QStringLiteral("#9ca3af")));
    paleta.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(QStringLiteral("#9ca3af")));
    paleta.setColor(QPalette::Disabled, QPalette::Base, QColor(QStringLiteral("#f3f4f6")));
    paleta.setColor(QPalette::Disabled, QPalette::Button, QColor(QStringLiteral("#f3f4f6")));
    app.setPalette(paleta);

    QFont fuente = app.font();
    fuente.setFamily(familiaFuente());
    fuente.setPointSize(10);
    app.setFont(fuente);

    app.setStyleSheet(hojaGlobal());
}

inline void estilizarMensaje(QMessageBox& mensaje, bool esError = false) {
    const QString principal = esError ? QStringLiteral("#b91c1c") : QStringLiteral("#047857");
    const QString hover = esError ? QStringLiteral("#991b1b") : QStringLiteral("#065f46");
    mensaje.setStyleSheet(QStringLiteral(
        "QMessageBox { background-color: #ffffff; }"
        "QLabel { color: #111827; font-size: 13px; }"
        "QPushButton { background-color: %1; color: #ffffff; font-weight: bold;"
        "  border: none; border-radius: 6px; padding: 6px 15px; }"
        "QPushButton:hover { background-color: %2; }").arg(principal, hover));
}

inline void mostrarInfo(QWidget* padre, const QString& titulo, const QString& texto) {
    QMessageBox mensaje(padre);
    mensaje.setWindowTitle(titulo);
    mensaje.setText(texto);
    mensaje.setIcon(QMessageBox::Information);
    estilizarMensaje(mensaje);
    mensaje.exec();
}

inline void mostrarAdvertencia(QWidget* padre, const QString& titulo, const QString& texto) {
    QMessageBox mensaje(padre);
    mensaje.setWindowTitle(titulo);
    mensaje.setText(texto);
    mensaje.setIcon(QMessageBox::Warning);
    estilizarMensaje(mensaje);
    mensaje.exec();
}

inline void mostrarError(QWidget* padre, const QString& titulo, const QString& texto) {
    QMessageBox mensaje(padre);
    mensaje.setWindowTitle(titulo);
    mensaje.setText(texto);
    mensaje.setIcon(QMessageBox::Critical);
    estilizarMensaje(mensaje, true);
    mensaje.exec();
}

inline QString estiloBotonPrincipal() {
    return QStringLiteral(
        "QPushButton { background-color: #047857; color: #ffffff; font-weight: bold;"
        "  border: none; border-radius: 8px; padding: 8px 16px; font-size: 13px; }"
        "QPushButton:hover { background-color: #065f46; }"
        "QPushButton:disabled { background-color: #d1d5db; color: #6b7280; }");
}

inline QString estiloBotonSecundario() {
    return QStringLiteral(
        "QPushButton { background-color: #1d4ed8; color: #ffffff; font-weight: bold;"
        "  border: none; border-radius: 8px; padding: 8px 16px; font-size: 13px; }"
        "QPushButton:hover { background-color: #1e40af; }"
        "QPushButton:disabled { background-color: #d1d5db; color: #6b7280; }");
}

inline QString estiloBotonPeligro() {
    return QStringLiteral(
        "QPushButton { background-color: #b91c1c; color: #ffffff; font-weight: bold;"
        "  border: none; border-radius: 8px; padding: 8px 16px; font-size: 13px; }"
        "QPushButton:hover { background-color: #991b1b; }"
        "QPushButton:disabled { background-color: #d1d5db; color: #6b7280; }");
}

inline QString estiloEntrada() {
    return QStringLiteral(
        "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QDateEdit {"
        "  background-color: #ffffff; border: 1px solid #d1d5db; border-radius: 8px;"
        "  padding: 8px 12px; font-size: 13px; color: #111827; min-height: 20px; }"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus, QDateEdit:focus {"
        "  border: 1px solid #047857; }");
}

inline QString estiloTabla() {
    return QStringLiteral(
        "QTableWidget { background-color: #ffffff; color: #111827;"
        "  border: 1px solid #e5e7eb; border-radius: 12px; gridline-color: #f3f4f6; }"
        "QHeaderView::section { background-color: #f9fafb; color: #374151; font-weight: bold;"
        "  border: none; padding: 10px; font-size: 12px; }"
        "QTableWidget::item { padding: 8px; font-size: 13px; color: #111827; }"
        "QTableWidget::item:selected { background-color: #e0f2fe; color: #0369a1; }");
}

} // namespace Estilos

#endif // ESTILOS_H
