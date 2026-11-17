#include "VentanaPrincipal.h"

#include "VistaHistorial.h"
#include "VistaInventario.h"
#include "VistaUsuarios.h"
#include "VistaVentas.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

VentanaPrincipal::VentanaPrincipal(std::shared_ptr<Usuario> usuario, QWidget* parent)
    : QMainWindow(parent), usuarioActual(std::move(usuario)) {
    setWindowTitle(QStringLiteral("UNA Pulpe - Sistema POS"));
    setMinimumSize(1024, 680);
    resize(1240, 780);

    configurarInterfaz();
    aplicarPermisosRol();
}

void VentanaPrincipal::configurarInterfaz() {
    QWidget* widgetCentral = new QWidget(this);
    QHBoxLayout* layoutRaiz = new QHBoxLayout(widgetCentral);
    layoutRaiz->setContentsMargins(0, 0, 0, 0);
    layoutRaiz->setSpacing(0);
    widgetCentral->setStyleSheet(QStringLiteral("background-color: #f8f9fa;"));
    setCentralWidget(widgetCentral);

    // Barra lateral de navegación.
    QFrame* panelLateral = new QFrame(widgetCentral);
    panelLateral->setFixedWidth(230);
    panelLateral->setStyleSheet(
        QStringLiteral("QFrame { background-color: #ffffff; border-right: 1px solid #e5e7eb; }"));

    QVBoxLayout* layoutMenu = new QVBoxLayout(panelLateral);
    layoutMenu->setContentsMargins(20, 25, 20, 25);
    layoutMenu->setSpacing(10);

    QLabel* lblLogoTexto = new QLabel(QStringLiteral("UNA Pulpe"), panelLateral);
    lblLogoTexto->setStyleSheet(
        QStringLiteral("font-size: 22px; font-weight: bold; color: #047857;"
                       " border: none; background: transparent;"));
    layoutMenu->addWidget(lblLogoTexto);

    QLabel* lblSub = new QLabel(QStringLiteral("Punto de Venta"), panelLateral);
    lblSub->setStyleSheet(
        QStringLiteral("font-size: 12px; color: #6b7280; border: none;"
                       " background: transparent; margin-bottom: 10px;"));
    layoutMenu->addWidget(lblSub);

    const QString rol = usuarioActual ? usuarioActual->rol() : QStringLiteral("Cajero");
    QLabel* lblRol = new QLabel(rol.toUpper(), panelLateral);
    lblRol->setAlignment(Qt::AlignCenter);
    lblRol->setStyleSheet(
        QStringLiteral("background-color: #ecfdf5; color: #047857; font-weight: bold;"
                       " padding: 8px 12px; border-radius: 8px; font-size: 12px;"
                       " border: none; margin-bottom: 15px;"));
    layoutMenu->addWidget(lblRol);

    btnVentas = crearBotonMenu(QStringLiteral("Ventas"));
    btnInventario = crearBotonMenu(QStringLiteral("Inventario"));
    btnHistorial = crearBotonMenu(QStringLiteral("Historial"));
    btnUsuarios = crearBotonMenu(QStringLiteral("Usuarios"));
    botonesMenu = {btnVentas, btnInventario, btnHistorial, btnUsuarios};

    layoutMenu->addWidget(btnVentas);
    layoutMenu->addWidget(btnInventario);
    layoutMenu->addWidget(btnHistorial);
    layoutMenu->addWidget(btnUsuarios);
    layoutMenu->addStretch();

    btnCerrarSesion = new QPushButton(QStringLiteral("Cerrar Sesión"), panelLateral);
    btnCerrarSesion->setCursor(Qt::PointingHandCursor);
    btnCerrarSesion->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #fef2f2; color: #b91c1c;"
                       " font-weight: bold; border: none; border-radius: 10px; padding: 12px;"
                       " font-size: 13px; text-align: left; padding-left: 15px; }"
                       "QPushButton:hover { background-color: #fee2e2; }"));
    layoutMenu->addWidget(btnCerrarSesion);

    layoutRaiz->addWidget(panelLateral);

    // Área derecha.
    QWidget* areaDerecha = new QWidget(widgetCentral);
    QVBoxLayout* layoutDerecha = new QVBoxLayout(areaDerecha);
    layoutDerecha->setContentsMargins(25, 20, 25, 20);
    layoutDerecha->setSpacing(16);

    QHBoxLayout* layoutHeader = new QHBoxLayout();

    QLabel* lblSaludo = new QLabel(
        QStringLiteral("Bienvenido, %1")
            .arg(usuarioActual ? usuarioActual->getNombreUsuario() : QString()),
        areaDerecha);
    lblSaludo->setStyleSheet(
        QStringLiteral("font-size: 18px; font-weight: bold; color: #111827;"));

    QLabel* lblEstado = new QLabel(QStringLiteral("Sistema POS Activo"), areaDerecha);
    lblEstado->setStyleSheet(QStringLiteral("font-size: 13px; color: #6b7280;"));

    layoutHeader->addWidget(lblSaludo);
    layoutHeader->addStretch();
    layoutHeader->addWidget(lblEstado);
    layoutDerecha->addLayout(layoutHeader);

    panelCentral = new QStackedWidget(areaDerecha);
    panelCentral->setStyleSheet(QStringLiteral("background: transparent;"));

    VistaVentas* vistaVentas = new VistaVentas(
        usuarioActual ? usuarioActual->getNombreUsuario() : QString(), panelCentral);
    VistaInventario* vistaInventario = new VistaInventario(usuarioActual, panelCentral);
    VistaHistorial* vistaHistorial = new VistaHistorial(usuarioActual, panelCentral);
    VistaUsuarios* vistaUsuarios = new VistaUsuarios(usuarioActual, panelCentral);

    panelCentral->addWidget(vistaVentas);
    panelCentral->addWidget(vistaInventario);
    panelCentral->addWidget(vistaHistorial);
    panelCentral->addWidget(vistaUsuarios);

    layoutDerecha->addWidget(panelCentral);
    layoutRaiz->addWidget(areaDerecha, 1);

    connect(btnVentas, &QPushButton::clicked, this, [this, vistaVentas]() {
        // Fuente única: siempre se recarga desde el JSON al entrar a Ventas.
        vistaVentas->actualizar();
        panelCentral->setCurrentIndex(0);
        marcarBotonActivo(btnVentas);
    });

    connect(btnInventario, &QPushButton::clicked, this, [this, vistaInventario]() {
        vistaInventario->actualizar();
        panelCentral->setCurrentIndex(1);
        marcarBotonActivo(btnInventario);
    });

    connect(btnHistorial, &QPushButton::clicked, this, [this, vistaHistorial]() {
        vistaHistorial->actualizar();
        panelCentral->setCurrentIndex(2);
        marcarBotonActivo(btnHistorial);
    });

    connect(btnUsuarios, &QPushButton::clicked, this, [this, vistaUsuarios]() {
        vistaUsuarios->actualizar();
        panelCentral->setCurrentIndex(3);
        marcarBotonActivo(btnUsuarios);
    });

    connect(btnCerrarSesion, &QPushButton::clicked, this, [this]() {
        cierrePorSesion = true;
        close();
    });

    // Sincronización entre vistas: quien modifica datos emite una señal y
    // quien los consume recarga desde el JSON (nunca copias en memoria vieja).
    connect(vistaInventario, &VistaInventario::inventarioModificado,
            this, [vistaVentas]() {
                vistaVentas->actualizar();
            });

    connect(vistaHistorial, &VistaHistorial::ventaAnulada,
            this, [vistaVentas, vistaInventario, vistaHistorial](const QString&) {
                vistaVentas->actualizar();       // stock devuelto disponible al instante
                vistaInventario->actualizar();   // tabla e historial de auditoría
                vistaHistorial->actualizar();    // estado "Anulada" en la tabla
            });

    connect(vistaVentas, &VistaVentas::ventaRegistrada,
            this, [vistaInventario, vistaHistorial](const QString&) {
                vistaInventario->actualizar();   // stock rebajado ya visible
                vistaHistorial->actualizar();    // nueva venta en el historial
            });

    marcarBotonActivo(btnVentas);
}

QPushButton* VentanaPrincipal::crearBotonMenu(const QString& texto) {
    QPushButton* boton = new QPushButton(texto, this);
    boton->setCursor(Qt::PointingHandCursor);
    aplicarEstiloMenu(boton, false);
    return boton;
}

void VentanaPrincipal::aplicarEstiloMenu(QPushButton* boton, bool activo) {
    if (!boton) return;
    if (activo) {
        boton->setStyleSheet(
            QStringLiteral("QPushButton { background-color: #047857; color: #ffffff;"
                           " font-weight: bold; border: none; border-radius: 10px;"
                           " padding: 12px; font-size: 13px; text-align: left;"
                           " padding-left: 15px; }"));
    } else {
        boton->setStyleSheet(
            QStringLiteral("QPushButton { background-color: transparent; color: #4b5563;"
                           " font-weight: 500; border: none; border-radius: 10px;"
                           " padding: 12px; font-size: 13px; text-align: left;"
                           " padding-left: 15px; }"
                           "QPushButton:hover { background-color: #f3f4f6; color: #111827; }"
                           "QPushButton:disabled { color: #9ca3af; }"));
    }
}

void VentanaPrincipal::marcarBotonActivo(QPushButton* boton) {
    for (QPushButton* candidato : botonesMenu) {
        aplicarEstiloMenu(candidato, candidato == boton);
    }
}

void VentanaPrincipal::aplicarPermisosRol() {
    if (!usuarioActual || usuarioActual->rol() != QStringLiteral("Administrador")) {
        // El cajero no administra usuarios ni anula ventas; el inventario
        // queda en modo consulta dentro de VistaInventario.
        if (btnUsuarios) {
            btnUsuarios->setVisible(false);
            btnUsuarios->setEnabled(false);
        }
    }
}
