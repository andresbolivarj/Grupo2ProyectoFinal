#include "VistaInventario.h"

#include "CategoriasProducto.h"
#include "Estilos.h"
#include "ServicioInventario.h"

#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QDateTime>
#include <QRegularExpression>
#include <QShowEvent>
#include <QStringList>
#include <algorithm>
#include <utility>

VistaInventario::VistaInventario(QWidget* parent)
    : VistaInventario(nullptr, parent) {}

VistaInventario::VistaInventario(std::shared_ptr<Usuario> usuario, QWidget* parent)
    : QWidget(parent), usuarioActual(std::move(usuario)) {
    configurarInterfaz();
    cargarDatos();
}

bool VistaInventario::esAdministrador() const {
    return usuarioActual && usuarioActual->isActivo()
           && usuarioActual->rol() == QStringLiteral("Administrador");
}

void VistaInventario::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    actualizar();
}

void VistaInventario::actualizar() {
    cargarDatos();
}

void VistaInventario::configurarInterfaz() {
    QVBoxLayout* layoutRaiz = new QVBoxLayout(this);
    layoutRaiz->setContentsMargins(16, 16, 16, 16);

    pestanas = new QTabWidget(this);
    paginaInventario = new QWidget(pestanas);
    QVBoxLayout* layoutPrincipal = new QVBoxLayout(paginaInventario);
    layoutPrincipal->setContentsMargins(0, 0, 0, 0);
    layoutPrincipal->setSpacing(12);
    setStyleSheet(QStringLiteral("background-color: #f8f9fa;"));

    QHBoxLayout* layoutTop = new QHBoxLayout();
    layoutTop->setSpacing(8);

    txtBuscar = new QLineEdit(paginaInventario);
    txtBuscar->setPlaceholderText(QStringLiteral("Buscar por nombre o código..."));
    txtBuscar->setStyleSheet(Estilos::estiloEntrada());
    connect(txtBuscar, &QLineEdit::textChanged, this, &VistaInventario::filtrarTabla);

    comboCategoriaFiltro = new QComboBox(paginaInventario);
    comboCategoriaFiltro->addItem(QStringLiteral("Todas las Categorías"));
    comboCategoriaFiltro->addItems(categoriasProductoPredefinidas());
    comboCategoriaFiltro->setStyleSheet(Estilos::estiloEntrada());
    connect(comboCategoriaFiltro, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VistaInventario::filtrarTabla);

    btnAgregar = new QPushButton(QStringLiteral("+ Nuevo Producto"), paginaInventario);
    btnAgregar->setCursor(Qt::PointingHandCursor);
    btnAgregar->setStyleSheet(Estilos::estiloBotonPrincipal());
    connect(btnAgregar, &QPushButton::clicked, this,
            [this]() { abrirFormularioProducto(nullptr); });

    btnEditar = new QPushButton(QStringLiteral("Editar"), paginaInventario);
    btnEditar->setCursor(Qt::PointingHandCursor);
    btnEditar->setStyleSheet(Estilos::estiloBotonSecundario());
    connect(btnEditar, &QPushButton::clicked, this, [this]() {
        const int fila = tablaProductos->currentRow();
        if (fila < 0) {
            Estilos::mostrarAdvertencia(
                this, QStringLiteral("Selección requerida"),
                QStringLiteral("Seleccione un producto de la tabla para editar."));
            return;
        }
        const QString id = tablaProductos->item(fila, 0)->text();
        for (auto& producto : listaProductos) {
            if (producto.getId() == id) {
                abrirFormularioProducto(&producto);
                return;
            }
        }
    });

    btnDesactivar = new QPushButton(QStringLiteral("Desactivar"), paginaInventario);
    btnDesactivar->setCursor(Qt::PointingHandCursor);
    btnDesactivar->setStyleSheet(Estilos::estiloBotonPeligro());
    connect(btnDesactivar, &QPushButton::clicked, this,
            &VistaInventario::desactivarProductoSeleccionado);

    btnReactivar = new QPushButton(QStringLiteral("Reactivar"), paginaInventario);
    btnReactivar->setCursor(Qt::PointingHandCursor);
    btnReactivar->setStyleSheet(Estilos::estiloBotonPrincipal());
    connect(btnReactivar, &QPushButton::clicked, this,
            &VistaInventario::reactivarProductoSeleccionado);

    layoutTop->addWidget(txtBuscar, 4);
    layoutTop->addWidget(comboCategoriaFiltro, 2);
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
            QStringLiteral("Modo consulta: solo un administrador puede modificar el inventario."),
            paginaInventario);
        lblConsulta->setStyleSheet(
            QStringLiteral("color: #6b7280; font-size: 12px; font-style: italic;"));
        layoutPrincipal->addWidget(lblConsulta);
    }

    layoutPrincipal->addLayout(layoutTop);

    tablaProductos = new QTableWidget(paginaInventario);
    tablaProductos->setColumnCount(6);
    tablaProductos->setHorizontalHeaderLabels({
        QStringLiteral("ID / Código"), QStringLiteral("Nombre"),
        QStringLiteral("Categoría"), QStringLiteral("Precio (₡)"),
        QStringLiteral("Stock"), QStringLiteral("Estado")
    });
    tablaProductos->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tablaProductos->verticalHeader()->setVisible(false);
    tablaProductos->setSelectionBehavior(QAbstractItemView::SelectRows);
    tablaProductos->setSelectionMode(QAbstractItemView::SingleSelection);
    tablaProductos->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tablaProductos->setAlternatingRowColors(true);
    tablaProductos->setStyleSheet(Estilos::estiloTabla());
    layoutPrincipal->addWidget(tablaProductos, 1);

    pestanas->addTab(paginaInventario, QStringLiteral("Gestión de Inventario"));

    if (esAdministrador()) {
        QWidget* paginaAuditoria = new QWidget(pestanas);
        QVBoxLayout* layoutAuditoria = new QVBoxLayout(paginaAuditoria);
        QHBoxLayout* barraAuditoria = new QHBoxLayout();

        txtBuscarAuditoria = new QLineEdit(paginaAuditoria);
        txtBuscarAuditoria->setPlaceholderText(QStringLiteral("Buscar en el historial..."));
        txtBuscarAuditoria->setStyleSheet(Estilos::estiloEntrada());

        QPushButton* btnActualizarAuditoria = new QPushButton(
            QStringLiteral("Actualizar"), paginaAuditoria);
        btnActualizarAuditoria->setStyleSheet(Estilos::estiloBotonSecundario());

        barraAuditoria->addWidget(txtBuscarAuditoria, 1);
        barraAuditoria->addWidget(btnActualizarAuditoria);

        tablaAuditoria = new QTableWidget(paginaAuditoria);
        tablaAuditoria->setColumnCount(5);
        tablaAuditoria->setHorizontalHeaderLabels({
            QStringLiteral("Fecha y Hora"), QStringLiteral("Acción"),
            QStringLiteral("Producto"), QStringLiteral("Detalle del Cambio"),
            QStringLiteral("Realizado por")
        });
        tablaAuditoria->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tablaAuditoria->verticalHeader()->setVisible(false);
        tablaAuditoria->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tablaAuditoria->setSelectionBehavior(QAbstractItemView::SelectRows);
        tablaAuditoria->setAlternatingRowColors(true);
        tablaAuditoria->setStyleSheet(Estilos::estiloTabla());

        layoutAuditoria->addLayout(barraAuditoria);
        layoutAuditoria->addWidget(tablaAuditoria, 1);
        pestanas->addTab(paginaAuditoria, QStringLiteral("Historial de Cambios"));

        connect(btnActualizarAuditoria, &QPushButton::clicked,
                this, &VistaInventario::cargarAuditoria);
        connect(txtBuscarAuditoria, &QLineEdit::textChanged,
                this, &VistaInventario::filtrarAuditoria);
    }

    layoutRaiz->addWidget(pestanas);
}

void VistaInventario::cargarDatos() {
    ServicioInventario inventario;
    QString error;

    if (!inventario.asegurarInventarioInicial(&error)) {
        Estilos::mostrarError(this, QStringLiteral("Inventario no disponible"), error);
        listaProductos.clear();
        renderizarTabla();
        return;
    }

    listaProductos = inventario.listar(&error);
    if (!error.isEmpty()) {
        Estilos::mostrarError(this, QStringLiteral("No se pudo leer el inventario"), error);
        listaProductos.clear();
    }

    renderizarTabla();
    cargarAuditoria();
}

void VistaInventario::renderizarTabla() {
    if (!tablaProductos) return;
    tablaProductos->setRowCount(0);

    for (const auto& producto : listaProductos) {
        const int fila = tablaProductos->rowCount();
        tablaProductos->insertRow(fila);

        auto* itemID = new QTableWidgetItem(producto.getId());
        auto* itemNombre = new QTableWidgetItem(producto.getNombre());
        auto* itemCategoria = new QTableWidgetItem(producto.getCategoria());
        auto* itemPrecio = new QTableWidgetItem(
            QStringLiteral("₡ %1").arg(producto.getPrecio(), 0, 'f', 2));
        auto* itemStock = new QTableWidgetItem(QString::number(producto.getStock()));
        auto* itemEstado = new QTableWidgetItem();

        itemPrecio->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemStock->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        if (!producto.isActivo()) {
            itemEstado->setText(QStringLiteral("Inactivo"));
            itemEstado->setForeground(QBrush(QColor(QStringLiteral("#6b7280"))));
            itemNombre->setForeground(QBrush(QColor(QStringLiteral("#6b7280"))));
        } else if (producto.getStock() <= 0) {
            itemEstado->setText(QStringLiteral("Agotado"));
            itemEstado->setForeground(QBrush(QColor(QStringLiteral("#b91c1c"))));
            itemStock->setForeground(QBrush(QColor(QStringLiteral("#b91c1c"))));
        } else if (producto.getStock() <= 5) {
            itemEstado->setText(QStringLiteral("Stock Bajo"));
            itemEstado->setForeground(QBrush(QColor(QStringLiteral("#b45309"))));
            itemStock->setForeground(QBrush(QColor(QStringLiteral("#b45309"))));
        } else {
            itemEstado->setText(QStringLiteral("Disponible"));
            itemEstado->setForeground(QBrush(QColor(QStringLiteral("#047857"))));
        }

        tablaProductos->setItem(fila, 0, itemID);
        tablaProductos->setItem(fila, 1, itemNombre);
        tablaProductos->setItem(fila, 2, itemCategoria);
        tablaProductos->setItem(fila, 3, itemPrecio);
        tablaProductos->setItem(fila, 4, itemStock);
        tablaProductos->setItem(fila, 5, itemEstado);
    }

    filtrarTabla();
}

void VistaInventario::filtrarTabla() {
    if (!tablaProductos || !txtBuscar || !comboCategoriaFiltro) return;

    const QString texto = txtBuscar->text().trimmed().toLower();
    const QString categoriaFiltro = comboCategoriaFiltro->currentText();

    for (int fila = 0; fila < tablaProductos->rowCount(); ++fila) {
        const auto* itemId = tablaProductos->item(fila, 0);
        const auto* itemNombre = tablaProductos->item(fila, 1);
        const auto* itemCategoria = tablaProductos->item(fila, 2);
        if (!itemId || !itemNombre || !itemCategoria) continue;

        const QString id = itemId->text().toLower();
        const QString nombre = itemNombre->text().toLower();
        const QString categoria = itemCategoria->text();

        const bool coincideTexto = texto.isEmpty() || id.contains(texto)
                                   || nombre.contains(texto);
        const bool coincideCategoria = categoriaFiltro == QStringLiteral("Todas las Categorías")
                                       || categoria == categoriaFiltro;
        tablaProductos->setRowHidden(fila, !(coincideTexto && coincideCategoria));
    }
}

void VistaInventario::abrirFormularioProducto(ProductoItem* productoExistente) {
    if (!esAdministrador()) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Permiso denegado"),
            QStringLiteral("Solo un administrador puede modificar el inventario."));
        return;
    }

    const bool esEdicion = (productoExistente != nullptr);

    QDialog dialogo(this);
    dialogo.setWindowTitle(esEdicion ? QStringLiteral("Editar Producto")
                                     : QStringLiteral("Nuevo Producto"));
    dialogo.setMinimumSize(390, 500);
    dialogo.resize(430, 560);
    dialogo.setStyleSheet(
        QStringLiteral("QDialog { background-color: #ffffff; }"
                       "QLabel { color: #111827; font-size: 12px; font-weight: 500; }"));

    QVBoxLayout* layout = new QVBoxLayout(&dialogo);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(8);

    QLabel* lblTitulo = new QLabel(
        esEdicion ? QStringLiteral("Modificar Datos de Producto")
                  : QStringLiteral("Registrar Nuevo Producto"),
        &dialogo);
    lblTitulo->setStyleSheet(
        QStringLiteral("font-size: 16px; font-weight: bold; color: #111827;"
                       " margin-bottom: 5px;"));
    layout->addWidget(lblTitulo);

    QLineEdit* inputID = new QLineEdit(&dialogo);
    inputID->setPlaceholderText(QStringLiteral("Código ID (ej: S006)"));
    inputID->setMaxLength(20);
    if (esEdicion) {
        inputID->setText(productoExistente->getId());
        inputID->setEnabled(false);
    }

    QLineEdit* inputNombre = new QLineEdit(&dialogo);
    inputNombre->setPlaceholderText(QStringLiteral("Nombre del producto"));
    inputNombre->setMaxLength(80);
    if (esEdicion) inputNombre->setText(productoExistente->getNombre());

    QLineEdit* inputDescripcion = new QLineEdit(&dialogo);
    inputDescripcion->setPlaceholderText(QStringLiteral("Descripción breve"));
    inputDescripcion->setMaxLength(200);
    if (esEdicion) inputDescripcion->setText(productoExistente->getDescripcion());

    QComboBox* comboCategoria = new QComboBox(&dialogo);
    comboCategoria->addItems(categoriasProductoPredefinidas());
    if (esEdicion) comboCategoria->setCurrentText(productoExistente->getCategoria());

    QDoubleSpinBox* spinPrecio = new QDoubleSpinBox(&dialogo);
    spinPrecio->setRange(0.01, 10000000.0);
    spinPrecio->setDecimals(2);
    spinPrecio->setPrefix(QStringLiteral("₡ "));
    if (esEdicion) spinPrecio->setValue(productoExistente->getPrecio());

    QSpinBox* spinStock = new QSpinBox(&dialogo);
    spinStock->setRange(0, 1000000);
    if (esEdicion) spinStock->setValue(productoExistente->getStock());

    QCheckBox* checkActivo = new QCheckBox(QStringLiteral("Producto activo"), &dialogo);
    checkActivo->setChecked(esEdicion ? productoExistente->isActivo() : true);
    checkActivo->setStyleSheet(QStringLiteral("color: #111827; font-size: 12px;"));

    const QString estiloCampos = Estilos::estiloEntrada();
    inputID->setStyleSheet(estiloCampos);
    inputNombre->setStyleSheet(estiloCampos);
    inputDescripcion->setStyleSheet(estiloCampos);
    comboCategoria->setStyleSheet(estiloCampos);
    spinPrecio->setStyleSheet(estiloCampos);
    spinStock->setStyleSheet(estiloCampos);

    layout->addWidget(new QLabel(QStringLiteral("Código ID:"), &dialogo));
    layout->addWidget(inputID);
    layout->addWidget(new QLabel(QStringLiteral("Nombre:"), &dialogo));
    layout->addWidget(inputNombre);
    layout->addWidget(new QLabel(QStringLiteral("Descripción:"), &dialogo));
    layout->addWidget(inputDescripcion);
    layout->addWidget(new QLabel(QStringLiteral("Categoría:"), &dialogo));
    layout->addWidget(comboCategoria);

    QHBoxLayout* layoutValores = new QHBoxLayout();
    layoutValores->setSpacing(10);
    QVBoxLayout* columnaPrecio = new QVBoxLayout();
    columnaPrecio->addWidget(new QLabel(QStringLiteral("Precio:"), &dialogo));
    columnaPrecio->addWidget(spinPrecio);
    QVBoxLayout* columnaStock = new QVBoxLayout();
    columnaStock->addWidget(new QLabel(QStringLiteral("Stock:"), &dialogo));
    columnaStock->addWidget(spinStock);
    layoutValores->addLayout(columnaPrecio);
    layoutValores->addLayout(columnaStock);
    layout->addLayout(layoutValores);
    layout->addWidget(checkActivo);

    QPushButton* btnGuardar = new QPushButton(QStringLiteral("Guardar Producto"), &dialogo);
    btnGuardar->setMinimumHeight(40);
    btnGuardar->setStyleSheet(Estilos::estiloBotonPrincipal());

    connect(btnGuardar, &QPushButton::clicked, &dialogo, [&]() {
        const QString id = inputID->text().trimmed();
        const QString nombre = inputNombre->text().trimmed();
        if (id.isEmpty() || nombre.isEmpty()) {
            Estilos::mostrarAdvertencia(&dialogo, QStringLiteral("Campos vacíos"),
                                        QStringLiteral("El código ID y el Nombre son obligatorios."));
            return;
        }

        ProductoItem producto;
        producto.setId(id);
        producto.setNombre(nombre);
        producto.setDescripcion(inputDescripcion->text().trimmed());
        producto.setCategoria(comboCategoria->currentText());
        producto.setPrecio(spinPrecio->value());
        producto.setStock(spinStock->value());
        producto.setActivo(checkActivo->isChecked());

        ServicioInventario inventario;
        QString error;
        const bool correcto = esEdicion
                                  ? inventario.actualizar(producto, usuarioActual, &error)
                                  : inventario.crear(producto, usuarioActual, &error);
        if (!correcto) {
            Estilos::mostrarError(&dialogo, QStringLiteral("No se pudo guardar"), error);
            return;
        }

        dialogo.accept();
        cargarDatos();
        emit inventarioModificado();
        if (!inventario.ultimaAdvertencia().isEmpty()) {
            Estilos::mostrarAdvertencia(this, QStringLiteral("Auditoría incompleta"),
                                        inventario.ultimaAdvertencia());
        }
        Estilos::mostrarInfo(this, QStringLiteral("Inventario actualizado"),
                             QStringLiteral("El producto se guardó correctamente."));
    });

    layout->addSpacing(8);
    layout->addWidget(btnGuardar);
    dialogo.exec();
}

void VistaInventario::desactivarProductoSeleccionado() {
    if (!esAdministrador()) return;

    const int fila = tablaProductos->currentRow();
    if (fila < 0) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Selección requerida"),
            QStringLiteral("Seleccione un producto de la tabla para desactivar."));
        return;
    }

    const QString id = tablaProductos->item(fila, 0)->text();
    const QString nombre = tablaProductos->item(fila, 1)->text();

    QMessageBox confirmacion(this);
    confirmacion.setWindowTitle(QStringLiteral("Confirmar desactivación"));
    confirmacion.setText(
        QStringLiteral("¿Desea desactivar '%1'?\n\nEl producto no se eliminará físicamente "
                       "y su historial de ventas se conservará.").arg(nombre));
    confirmacion.setIcon(QMessageBox::Question);
    QPushButton* btnSi = confirmacion.addButton(QStringLiteral("Sí, desactivar"),
                                                QMessageBox::YesRole);
    confirmacion.addButton(QStringLiteral("Cancelar"), QMessageBox::NoRole);
    Estilos::estilizarMensaje(confirmacion, true);
    confirmacion.exec();
    if (confirmacion.clickedButton() != btnSi) return;

    ServicioInventario inventario;
    QString error;
    if (inventario.desactivar(id, usuarioActual, &error)) {
        cargarDatos();
        emit inventarioModificado();
        if (!inventario.ultimaAdvertencia().isEmpty()) {
            Estilos::mostrarAdvertencia(this, QStringLiteral("Auditoría incompleta"),
                                        inventario.ultimaAdvertencia());
        }
        Estilos::mostrarInfo(this, QStringLiteral("Producto desactivado"),
                             QStringLiteral("El producto ya no estará disponible para la venta."));
    } else {
        Estilos::mostrarError(this, QStringLiteral("No se pudo desactivar"), error);
    }
}

void VistaInventario::reactivarProductoSeleccionado() {
    if (!esAdministrador()) return;

    const int fila = tablaProductos->currentRow();
    if (fila < 0) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Selección requerida"),
            QStringLiteral("Seleccione un producto de la tabla para reactivar."));
        return;
    }

    const QString id = tablaProductos->item(fila, 0)->text();
    ServicioInventario inventario;
    QString error;
    if (inventario.reactivar(id, usuarioActual, &error)) {
        cargarDatos();
        emit inventarioModificado();
        if (!inventario.ultimaAdvertencia().isEmpty()) {
            Estilos::mostrarAdvertencia(this, QStringLiteral("Auditoría incompleta"),
                                        inventario.ultimaAdvertencia());
        }
        Estilos::mostrarInfo(this, QStringLiteral("Producto reactivado"),
                             QStringLiteral("El producto vuelve a estar disponible para la venta."));
    } else {
        Estilos::mostrarError(this, QStringLiteral("No se pudo reactivar"), error);
    }
}

void VistaInventario::cargarAuditoria() {
    if (!tablaAuditoria) return;

    ServicioInventario inventario;
    QString error;
    auto registros = inventario.listarAuditoria(&error);
    if (!error.isEmpty()) {
        Estilos::mostrarError(this, QStringLiteral("No se pudo leer la auditoría"), error);
        registros.clear();
    }

    std::stable_sort(registros.begin(), registros.end(), [](const auto& a, const auto& b) {
        return QDateTime::fromString(a.fechaHora, QStringLiteral("yyyy-MM-dd HH:mm:ss"))
               > QDateTime::fromString(b.fechaHora, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    });

    tablaAuditoria->setRowCount(0);
    for (const auto& registro : registros) {
        const int fila = tablaAuditoria->rowCount();
        tablaAuditoria->insertRow(fila);
        const QStringList valores{
            registro.fechaHora, registro.accion, registro.nombreProducto,
            registro.detalleCambio, registro.usuarioResponsable
        };
        for (int columna = 0; columna < valores.size(); ++columna) {
            tablaAuditoria->setItem(fila, columna,
                                    new QTableWidgetItem(valores.at(columna)));
        }

        QString color = QStringLiteral("#1d4ed8");
        if (registro.accion == Auditoria::aCadena(Auditoria::Accion::Anadido)) {
            color = QStringLiteral("#047857");
        } else if (registro.accion == Auditoria::aCadena(Auditoria::Accion::Desactivado)) {
            color = QStringLiteral("#b91c1c");
        } else if (registro.accion == Auditoria::aCadena(Auditoria::Accion::Reintegro)) {
            color = QStringLiteral("#b45309");
        }
        tablaAuditoria->item(fila, 1)->setForeground(QBrush(QColor(color)));
    }
    filtrarAuditoria();
}

void VistaInventario::filtrarAuditoria() {
    if (!tablaAuditoria || !txtBuscarAuditoria) return;

    const QString filtro = txtBuscarAuditoria->text().trimmed();
    for (int fila = 0; fila < tablaAuditoria->rowCount(); ++fila) {
        bool coincide = filtro.isEmpty();
        for (int columna = 0; !coincide && columna < tablaAuditoria->columnCount(); ++columna) {
            const auto* item = tablaAuditoria->item(fila, columna);
            coincide = item && item->text().contains(filtro, Qt::CaseInsensitive);
        }
        tablaAuditoria->setRowHidden(fila, !coincide);
    }
}
