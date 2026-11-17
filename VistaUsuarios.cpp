#include "VistaUsuarios.h"

#include "Estilos.h"
#include "ServicioUsuarios.h"

#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QShowEvent>
#include <utility>

VistaUsuarios::VistaUsuarios(QWidget* parent)
    : VistaUsuarios(nullptr, parent) {}

VistaUsuarios::VistaUsuarios(std::shared_ptr<Usuario> usuario, QWidget* parent)
    : QWidget(parent), usuarioActual(std::move(usuario)) {
    configurarInterfaz();
    cargarDatos();
}

bool VistaUsuarios::esAdministrador() const {
    return usuarioActual && usuarioActual->isActivo()
           && usuarioActual->rol() == QStringLiteral("Administrador");
}

void VistaUsuarios::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    actualizar();
}

void VistaUsuarios::actualizar() {
    cargarDatos();
}

void VistaUsuarios::configurarInterfaz() {
    QVBoxLayout* layoutPrincipal = new QVBoxLayout(this);
    layoutPrincipal->setContentsMargins(16, 16, 16, 16);
    layoutPrincipal->setSpacing(12);
    setStyleSheet(QStringLiteral("background-color: #f8f9fa;"));

    QHBoxLayout* layoutTop = new QHBoxLayout();
    layoutTop->setSpacing(8);

    txtBuscar = new QLineEdit(this);
    txtBuscar->setPlaceholderText(QStringLiteral("Buscar por nombre de usuario..."));
    txtBuscar->setStyleSheet(Estilos::estiloEntrada());
    connect(txtBuscar, &QLineEdit::textChanged, this, &VistaUsuarios::filtrarTabla);

    btnAgregar = new QPushButton(QStringLiteral("+ Nuevo Usuario"), this);
    btnAgregar->setCursor(Qt::PointingHandCursor);
    btnAgregar->setStyleSheet(Estilos::estiloBotonPrincipal());
    connect(btnAgregar, &QPushButton::clicked, this,
            [this]() { abrirFormularioUsuario(nullptr); });

    btnEditar = new QPushButton(QStringLiteral("Editar"), this);
    btnEditar->setCursor(Qt::PointingHandCursor);
    btnEditar->setStyleSheet(Estilos::estiloBotonSecundario());
    connect(btnEditar, &QPushButton::clicked, this, [this]() {
        const int fila = tablaUsuarios->currentRow();
        if (fila < 0) {
            Estilos::mostrarAdvertencia(
                this, QStringLiteral("Selección requerida"),
                QStringLiteral("Seleccione un usuario para editar."));
            return;
        }
        const QString nombre = tablaUsuarios->item(fila, 0)->text();
        for (const auto& usuario : listaUsuarios) {
            if (usuario->getNombreUsuario() == nombre) {
                abrirFormularioUsuario(usuario);
                return;
            }
        }
    });

    btnDesactivar = new QPushButton(QStringLiteral("Desactivar"), this);
    btnDesactivar->setCursor(Qt::PointingHandCursor);
    btnDesactivar->setStyleSheet(Estilos::estiloBotonPeligro());
    connect(btnDesactivar, &QPushButton::clicked, this,
            &VistaUsuarios::desactivarUsuarioSeleccionado);

    btnReactivar = new QPushButton(QStringLiteral("Reactivar"), this);
    btnReactivar->setCursor(Qt::PointingHandCursor);
    btnReactivar->setStyleSheet(Estilos::estiloBotonPrincipal());
    connect(btnReactivar, &QPushButton::clicked, this,
            &VistaUsuarios::reactivarUsuarioSeleccionado);

    layoutTop->addWidget(txtBuscar, 4);
    layoutTop->addWidget(btnAgregar, 2);
    layoutTop->addWidget(btnEditar, 1);
    layoutTop->addWidget(btnDesactivar, 1);
    layoutTop->addWidget(btnReactivar, 1);

    if (!esAdministrador()) {
        btnAgregar->setVisible(false);
        btnEditar->setVisible(false);
        btnDesactivar->setVisible(false);
        btnReactivar->setVisible(false);
        QLabel* lblConsulta = new QLabel(
            QStringLiteral("Solo un administrador puede gestionar usuarios."), this);
        lblConsulta->setStyleSheet(
            QStringLiteral("color: #6b7280; font-size: 12px; font-style: italic;"));
        layoutPrincipal->addWidget(lblConsulta);
    }

    layoutPrincipal->addLayout(layoutTop);

    tablaUsuarios = new QTableWidget(this);
    tablaUsuarios->setColumnCount(3);
    tablaUsuarios->setHorizontalHeaderLabels({
        QStringLiteral("Usuario"), QStringLiteral("Rol de Acceso"), QStringLiteral("Estado")
    });
    tablaUsuarios->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tablaUsuarios->verticalHeader()->setVisible(false);
    tablaUsuarios->setSelectionBehavior(QAbstractItemView::SelectRows);
    tablaUsuarios->setSelectionMode(QAbstractItemView::SingleSelection);
    tablaUsuarios->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tablaUsuarios->setAlternatingRowColors(true);
    tablaUsuarios->setStyleSheet(Estilos::estiloTabla());
    layoutPrincipal->addWidget(tablaUsuarios, 1);
}

void VistaUsuarios::cargarDatos() {
    ServicioUsuarios servicio;
    QString error;

    if (!servicio.asegurarUsuariosIniciales(&error)) {
        Estilos::mostrarError(this, QStringLiteral("Usuarios no disponibles"), error);
        listaUsuarios.clear();
        renderizarTabla();
        return;
    }

    listaUsuarios = servicio.listar(&error);
    if (!error.isEmpty()) {
        Estilos::mostrarError(this, QStringLiteral("No se pudieron leer los usuarios"), error);
        listaUsuarios.clear();
    }
    renderizarTabla();
}

void VistaUsuarios::renderizarTabla() {
    if (!tablaUsuarios) return;
    tablaUsuarios->setRowCount(0);

    for (const auto& usuario : listaUsuarios) {
        const int fila = tablaUsuarios->rowCount();
        tablaUsuarios->insertRow(fila);

        auto* itemNombre = new QTableWidgetItem(usuario->getNombreUsuario());
        auto* itemRol = new QTableWidgetItem(usuario->rol());
        auto* itemEstado = new QTableWidgetItem(
            usuario->isActivo() ? QStringLiteral("Activo") : QStringLiteral("Inactivo"));

        itemRol->setForeground(QBrush(QColor(
            usuario->rol() == QStringLiteral("Administrador")
                ? QStringLiteral("#3b82f6")
                : QStringLiteral("#047857"))));
        itemEstado->setForeground(QBrush(QColor(
            usuario->isActivo() ? QStringLiteral("#047857")
                                : QStringLiteral("#b91c1c"))));

        tablaUsuarios->setItem(fila, 0, itemNombre);
        tablaUsuarios->setItem(fila, 1, itemRol);
        tablaUsuarios->setItem(fila, 2, itemEstado);
    }

    filtrarTabla();
}

void VistaUsuarios::filtrarTabla() {
    if (!tablaUsuarios || !txtBuscar) return;

    const QString texto = txtBuscar->text().trimmed().toLower();
    for (int fila = 0; fila < tablaUsuarios->rowCount(); ++fila) {
        const auto* item = tablaUsuarios->item(fila, 0);
        if (!item) continue;
        tablaUsuarios->setRowHidden(
            fila, !(texto.isEmpty() || item->text().toLower().contains(texto)));
    }
}

void VistaUsuarios::abrirFormularioUsuario(std::shared_ptr<Usuario> usuarioExistente) {
    if (!esAdministrador()) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Permiso denegado"),
            QStringLiteral("Solo un administrador puede gestionar usuarios."));
        return;
    }

    const bool esEdicion = (usuarioExistente != nullptr);

    QDialog dialogo(this);
    dialogo.setWindowTitle(esEdicion ? QStringLiteral("Editar Usuario")
                                     : QStringLiteral("Nuevo Usuario"));
    dialogo.setMinimumSize(370, 430);
    dialogo.resize(400, 460);
    dialogo.setStyleSheet(
        QStringLiteral("QDialog { background-color: #ffffff; }"
                       "QLabel { color: #111827; font-size: 12px; font-weight: 500; }"));

    QVBoxLayout* layout = new QVBoxLayout(&dialogo);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(8);

    QLabel* lblTitulo = new QLabel(
        esEdicion ? QStringLiteral("Modificar Credenciales")
                  : QStringLiteral("Registrar Nuevo Usuario"),
        &dialogo);
    lblTitulo->setStyleSheet(
        QStringLiteral("font-size: 16px; font-weight: bold; color: #111827;"
                       " margin-bottom: 5px;"));
    layout->addWidget(lblTitulo);

    QLineEdit* inputUsuario = new QLineEdit(&dialogo);
    inputUsuario->setPlaceholderText(QStringLiteral("Nombre de usuario"));
    inputUsuario->setMaxLength(64);
    if (esEdicion) {
        inputUsuario->setText(usuarioExistente->getNombreUsuario());
        inputUsuario->setEnabled(false); // El nombre de usuario es la clave estable.
    }

    QLineEdit* inputClave = new QLineEdit(&dialogo);
    inputClave->setPlaceholderText(
        esEdicion ? QStringLiteral("Dejar vacío para conservar la contraseña actual")
                  : QStringLiteral("Contraseña (mínimo 6 caracteres)"));
    inputClave->setEchoMode(QLineEdit::Password);
    inputClave->setMaxLength(128);
    // Nunca se precarga el hash almacenado en el archivo JSON.

    QPushButton* btnVerClave = new QPushButton(QStringLiteral("Ver"), &dialogo);
    btnVerClave->setFixedWidth(68);
    btnVerClave->setCursor(Qt::PointingHandCursor);
    btnVerClave->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #f3f4f6; color: #374151;"
                       " font-weight: bold; border: 1px solid #d1d5db; border-radius: 8px;"
                       " padding: 8px; font-size: 11px; }"
                       "QPushButton:hover { background-color: #e5e7eb; }"));
    connect(btnVerClave, &QPushButton::clicked, &dialogo, [inputClave, btnVerClave]() {
        const bool oculta = inputClave->echoMode() == QLineEdit::Password;
        inputClave->setEchoMode(oculta ? QLineEdit::Normal : QLineEdit::Password);
        btnVerClave->setText(oculta ? QStringLiteral("Ocultar") : QStringLiteral("Ver"));
    });

    QComboBox* comboRol = new QComboBox(&dialogo);
    comboRol->addItems({QStringLiteral("Cajero"), QStringLiteral("Administrador")});
    if (esEdicion) comboRol->setCurrentText(usuarioExistente->rol());

    QCheckBox* checkActivo = new QCheckBox(QStringLiteral("Usuario activo"), &dialogo);
    checkActivo->setChecked(esEdicion ? usuarioExistente->isActivo() : true);
    checkActivo->setStyleSheet(QStringLiteral("color: #111827; font-size: 12px;"));
    if (usuarioExistente
        && usuarioActual
        && usuarioExistente->getNombreUsuario() == usuarioActual->getNombreUsuario()) {
        checkActivo->setEnabled(false); // No se puede desactivar a sí mismo.
        comboRol->setEnabled(false);    // No se puede cambiar su propio rol.
    }

    const QString estiloCampos = Estilos::estiloEntrada();
    inputUsuario->setStyleSheet(estiloCampos);
    inputClave->setStyleSheet(estiloCampos);
    comboRol->setStyleSheet(estiloCampos);

    layout->addWidget(new QLabel(QStringLiteral("Usuario:"), &dialogo));
    layout->addWidget(inputUsuario);
    layout->addWidget(new QLabel(QStringLiteral("Contraseña:"), &dialogo));

    QHBoxLayout* layoutClave = new QHBoxLayout();
    layoutClave->setSpacing(6);
    layoutClave->addWidget(inputClave, 1);
    layoutClave->addWidget(btnVerClave);
    layout->addLayout(layoutClave);

    layout->addWidget(new QLabel(QStringLiteral("Rol de Acceso:"), &dialogo));
    layout->addWidget(comboRol);
    layout->addWidget(checkActivo);

    QPushButton* btnGuardar = new QPushButton(QStringLiteral("Guardar Usuario"), &dialogo);
    btnGuardar->setMinimumHeight(38);
    btnGuardar->setStyleSheet(Estilos::estiloBotonPrincipal());

    connect(btnGuardar, &QPushButton::clicked, &dialogo, [&]() {
        const QString nombre = inputUsuario->text().trimmed();
        const QString clave = inputClave->text();
        const QString rol = comboRol->currentText();

        if (nombre.isEmpty()) {
            Estilos::mostrarAdvertencia(&dialogo, QStringLiteral("Campo vacío"),
                                        QStringLiteral("El nombre de usuario es obligatorio."));
            return;
        }
        if (!esEdicion && clave.isEmpty()) {
            Estilos::mostrarAdvertencia(&dialogo, QStringLiteral("Contraseña requerida"),
                                        QStringLiteral("Ingrese una contraseña para el nuevo usuario."));
            return;
        }

        ServicioUsuarios servicio;
        QString error;
        const bool correcto =
            esEdicion
                ? servicio.actualizar(nombre, clave, rol, checkActivo->isChecked(),
                                      usuarioActual, &error)
                : servicio.crear(nombre, clave, rol, usuarioActual, &error);

        if (!correcto) {
            Estilos::mostrarError(&dialogo, QStringLiteral("No se pudo guardar"), error);
            return;
        }

        dialogo.accept();
        cargarDatos();
        Estilos::mostrarInfo(this, QStringLiteral("Usuarios actualizados"),
                             QStringLiteral("Los cambios se guardaron correctamente."));
    });

    layout->addSpacing(8);
    layout->addWidget(btnGuardar);
    dialogo.exec();
}

void VistaUsuarios::desactivarUsuarioSeleccionado() {
    if (!esAdministrador()) return;

    const int fila = tablaUsuarios->currentRow();
    if (fila < 0) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Selección requerida"),
            QStringLiteral("Seleccione un usuario para desactivar."));
        return;
    }

    const QString nombre = tablaUsuarios->item(fila, 0)->text();

    QMessageBox confirmacion(this);
    confirmacion.setWindowTitle(QStringLiteral("Confirmar desactivación"));
    confirmacion.setText(
        QStringLiteral("¿Desea desactivar al usuario '%1'?\n\nEl registro se conservará y "
                       "ya no podrá iniciar sesión.").arg(nombre));
    confirmacion.setIcon(QMessageBox::Question);
    QPushButton* btnSi = confirmacion.addButton(QStringLiteral("Sí, desactivar"),
                                                QMessageBox::YesRole);
    confirmacion.addButton(QStringLiteral("Cancelar"), QMessageBox::NoRole);
    Estilos::estilizarMensaje(confirmacion, true);
    confirmacion.exec();
    if (confirmacion.clickedButton() != btnSi) return;

    ServicioUsuarios servicio;
    QString error;
    if (servicio.desactivar(nombre, usuarioActual, &error)) {
        cargarDatos();
        Estilos::mostrarInfo(this, QStringLiteral("Usuario desactivado"),
                             QStringLiteral("El usuario ya no podrá acceder al sistema."));
    } else {
        Estilos::mostrarError(this, QStringLiteral("No se pudo desactivar"), error);
    }
}

void VistaUsuarios::reactivarUsuarioSeleccionado() {
    if (!esAdministrador()) return;

    const int fila = tablaUsuarios->currentRow();
    if (fila < 0) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Selección requerida"),
            QStringLiteral("Seleccione un usuario para reactivar."));
        return;
    }

    const QString nombre = tablaUsuarios->item(fila, 0)->text();
    ServicioUsuarios servicio;
    QString error;
    if (servicio.reactivar(nombre, usuarioActual, &error)) {
        cargarDatos();
        Estilos::mostrarInfo(this, QStringLiteral("Usuario reactivado"),
                             QStringLiteral("El usuario puede volver a acceder al sistema."));
    } else {
        Estilos::mostrarError(this, QStringLiteral("No se pudo reactivar"), error);
    }
}
