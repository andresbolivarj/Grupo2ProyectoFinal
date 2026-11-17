#include "VistaVentas.h"

#include "CalculadoraVenta.h"
#include "Estilos.h"
#include "ServicioInventario.h"
#include "ServicioTickets.h"
#include "ServicioVentas.h"

#include <QButtonGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QDoubleValidator>
#include <QLocale>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <algorithm>
#include <cmath>
#include <utility>

VistaVentas::VistaVentas(const QString& nombreUsuario, QWidget* parent)
    : QWidget(parent), nombreUsuario(nombreUsuario) {
    categoriaSeleccionada = Categorias::Todos;
    configurarInterfaz();
    actualizar();
}

void VistaVentas::actualizar() {
    cargarCatalogo();
    sincronizarCarrito();
    renderizarProductos();
    renderizarCarrito();
    actualizarTotales();
    emit catalogoActualizado();
}

void VistaVentas::cargarCatalogo() {
    ServicioInventario inventario;
    QString error;
    if (!inventario.asegurarInventarioInicial(&error)) {
        Estilos::mostrarError(this, QStringLiteral("Inventario no disponible"), error);
        catalogo.clear();
        return;
    }

    catalogo = inventario.listarActivos(&error);
    if (!error.isEmpty()) {
        Estilos::mostrarError(this, QStringLiteral("Inventario no disponible"), error);
        catalogo.clear();
    }
}

void VistaVentas::sincronizarCarrito() {
    // El inventario puede haber cambiado desde otra pestaña (alta, edición,
    // desactivación o anulación de una venta). Se refrescan las copias que
    // tiene el carrito y se descartan renglones que ya no son vendibles.
    std::vector<ItemCarrito> sincronizado;
    sincronizado.reserve(carrito.size());

    for (const auto& item : carrito) {
        const auto coincide = std::find_if(
            catalogo.begin(), catalogo.end(), [&item](const ProductoItem& producto) {
                return producto.getId().compare(item.producto.getId(), Qt::CaseInsensitive) == 0;
            });
        if (coincide == catalogo.end() || !coincide->isActivo()
            || coincide->getStock() <= 0) {
            continue;
        }

        ItemCarrito actualizado = item;
        actualizado.producto = *coincide;
        actualizado.cantidad = std::min(actualizado.cantidad, coincide->getStock());
        if (actualizado.cantidad > 0) {
            sincronizado.push_back(actualizado);
        }
    }

    carrito = std::move(sincronizado);
}

void VistaVentas::configurarInterfaz() {
    QHBoxLayout* layoutPrincipal = new QHBoxLayout(this);
    layoutPrincipal->setContentsMargins(15, 15, 15, 15);
    layoutPrincipal->setSpacing(15);
    setStyleSheet(QStringLiteral("background-color: #f8f9fa;"));

    // Panel izquierdo: catálogo.
    QWidget* panelIzquierdo = new QWidget(this);
    QVBoxLayout* layoutIzquierdo = new QVBoxLayout(panelIzquierdo);
    layoutIzquierdo->setContentsMargins(0, 0, 0, 0);
    layoutIzquierdo->setSpacing(12);

    txtBuscar = new QLineEdit(panelIzquierdo);
    txtBuscar->setPlaceholderText(QStringLiteral("Buscar producto por nombre o código..."));
    txtBuscar->setStyleSheet(Estilos::estiloEntrada());
    connect(txtBuscar, &QLineEdit::textChanged, this, &VistaVentas::filtrarProductos);
    layoutIzquierdo->addWidget(txtBuscar);

    QHBoxLayout* layoutCategorias = new QHBoxLayout();
    layoutCategorias->setSpacing(8);
    grupoCategorias = new QButtonGroup(this);
    grupoCategorias->setExclusive(true);

    const QStringList categorias = {
        Categorias::Todos, Categorias::Snacks, Categorias::Bebidas,
        Categorias::Comida, Categorias::Utiles, Categorias::Higiene
    };
    for (const QString& categoria : categorias) {
        QPushButton* boton = new QPushButton(categoria, panelIzquierdo);
        boton->setCheckable(true);
        boton->setCursor(Qt::PointingHandCursor);
        boton->setStyleSheet(
            QStringLiteral("QPushButton { background-color: #ffffff; color: #4b5563;"
                           " font-weight: 500; border: 1px solid #e5e7eb; border-radius: 8px;"
                           " padding: 8px 16px; font-size: 12px; }"
                           "QPushButton:hover { background-color: #f3f4f6; }"
                           "QPushButton:checked { background-color: #047857; color: #ffffff;"
                           " border: 1px solid #047857; font-weight: bold; }"));
        if (categoria == QStringLiteral("Todos")) boton->setChecked(true);
        grupoCategorias->addButton(boton);
        connect(boton, &QPushButton::clicked, this, [this, categoria]() {
            cambiarCategoriaSlot(categoria);
        });
        layoutCategorias->addWidget(boton);
    }
    layoutCategorias->addStretch();
    layoutIzquierdo->addLayout(layoutCategorias);

    QScrollArea* scrollProductos = new QScrollArea(panelIzquierdo);
    scrollProductos->setWidgetResizable(true);
    scrollProductos->setStyleSheet(
        QStringLiteral("QScrollArea { border: none; background: transparent; }"));

    QWidget* contenidoGrid = new QWidget();
    contenidoGrid->setStyleSheet(QStringLiteral("background: transparent;"));
    gridProductos = new QGridLayout(contenidoGrid);
    gridProductos->setSpacing(15);
    gridProductos->setAlignment(Qt::AlignTop);
    scrollProductos->setWidget(contenidoGrid);
    layoutIzquierdo->addWidget(scrollProductos);

    layoutPrincipal->addWidget(panelIzquierdo, 7);

    // Panel derecho: orden.
    QFrame* panelDerecho = new QFrame(this);
    panelDerecho->setStyleSheet(
        QStringLiteral("QFrame { background-color: #ffffff; border-radius: 16px;"
                       " border: 1px solid #e5e7eb; }"));
    QVBoxLayout* layoutDerecho = new QVBoxLayout(panelDerecho);
    layoutDerecho->setContentsMargins(18, 18, 18, 18);
    layoutDerecho->setSpacing(8);

    QHBoxLayout* layoutHeaderOrden = new QHBoxLayout();
    QLabel* lblTituloOrden = new QLabel(QStringLiteral("Mi Orden"), panelDerecho);
    lblTituloOrden->setStyleSheet(
        QStringLiteral("font-size: 18px; font-weight: bold; color: #111827; border: none;"));

    btnVaciarCarrito = new QPushButton(QStringLiteral("Vaciar"), panelDerecho);
    btnVaciarCarrito->setCursor(Qt::PointingHandCursor);
    btnVaciarCarrito->setStyleSheet(
        QStringLiteral("QPushButton { background: transparent; color: #b91c1c;"
                       " font-size: 11px; font-weight: bold; border: none; }"
                       "QPushButton:hover { text-decoration: underline; }"));
    connect(btnVaciarCarrito, &QPushButton::clicked, this, &VistaVentas::vaciarCarrito);

    layoutHeaderOrden->addWidget(lblTituloOrden);
    layoutHeaderOrden->addStretch();
    layoutHeaderOrden->addWidget(btnVaciarCarrito);
    layoutDerecho->addLayout(layoutHeaderOrden);

    lblResumenItems = new QLabel(QStringLiteral("0 artículos en la orden"), panelDerecho);
    lblResumenItems->setStyleSheet(
        QStringLiteral("color: #6b7280; font-size: 11px; border: none;"));
    layoutDerecho->addWidget(lblResumenItems);

    QScrollArea* scrollCarrito = new QScrollArea(panelDerecho);
    scrollCarrito->setWidgetResizable(true);
    scrollCarrito->setMinimumHeight(150);
    scrollCarrito->setStyleSheet(
        QStringLiteral("QScrollArea { border: none; background: transparent; }"));

    contenedorCarrito = new QWidget();
    contenedorCarrito->setStyleSheet(QStringLiteral("background: transparent;"));
    layoutListaCarrito = new QVBoxLayout(contenedorCarrito);
    layoutListaCarrito->setSpacing(6);
    layoutListaCarrito->setContentsMargins(0, 0, 0, 0);
    scrollCarrito->setWidget(contenedorCarrito);
    layoutDerecho->addWidget(scrollCarrito, 1);

    QFrame* linea = new QFrame(panelDerecho);
    linea->setFrameShape(QFrame::HLine);
    linea->setStyleSheet(QStringLiteral("color: #e5e7eb;"));
    layoutDerecho->addWidget(linea);

    auto agregarFilaDesglose = [panelDerecho, layoutDerecho](const QString& etiqueta,
                                                              QLabel*& etiquetaValor) {
        QHBoxLayout* fila = new QHBoxLayout();
        QLabel* texto = new QLabel(etiqueta, panelDerecho);
        texto->setStyleSheet(QStringLiteral("color: #6b7280; font-size: 12px; border: none;"));
        etiquetaValor = new QLabel(QStringLiteral("₡ 0.00"), panelDerecho);
        etiquetaValor->setStyleSheet(
            QStringLiteral("color: #374151; font-weight: 500; font-size: 12px; border: none;"));
        fila->addWidget(texto);
        fila->addStretch();
        fila->addWidget(etiquetaValor);
        layoutDerecho->addLayout(fila);
    };

    agregarFilaDesglose(QStringLiteral("Subtotal"), lblSubtotal);
    agregarFilaDesglose(QStringLiteral("Descuento"), lblDescuento);
    agregarFilaDesglose(QStringLiteral("IVA incluido (13%)"), lblIVA);

    QHBoxLayout* layoutDesc = new QHBoxLayout();
    QLabel* lblEtiquetaDesc = new QLabel(QStringLiteral("Descuento:"), panelDerecho);
    lblEtiquetaDesc->setStyleSheet(
        QStringLiteral("color: #6b7280; font-size: 12px; border: none;"));

    comboTipoDescuento = new QComboBox(panelDerecho);
    comboTipoDescuento->addItems({QStringLiteral("Monto (₡)"),
                                  QStringLiteral("Porcentaje (%)")});
    comboTipoDescuento->setStyleSheet(Estilos::estiloEntrada());

    txtDescuento = new QLineEdit(panelDerecho);
    txtDescuento->setAlignment(Qt::AlignRight);
    txtDescuento->setText(QStringLiteral("0.00"));
    txtDescuento->setPlaceholderText(QStringLiteral("0.00"));
    auto* validadorDescuento = new QDoubleValidator(0.0, 10000000.0, 2, txtDescuento);
    validadorDescuento->setNotation(QDoubleValidator::StandardNotation);
    validadorDescuento->setLocale(QLocale::c());
    txtDescuento->setValidator(validadorDescuento);
    txtDescuento->setStyleSheet(Estilos::estiloEntrada());

    connect(comboTipoDescuento, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, validadorDescuento](int index) {
                QSignalBlocker bloqueo(txtDescuento);
                if (index == CalculadoraVenta::DescuentoPorcentaje) {
                    validadorDescuento->setTop(100.0);
                } else {
                    validadorDescuento->setTop(10000000.0);
                }
                txtDescuento->setText(QStringLiteral("0.00"));
                actualizarTotales();
            });
    connect(txtDescuento, &QLineEdit::textChanged,
            this, &VistaVentas::actualizarTotales);

    layoutDesc->addWidget(lblEtiquetaDesc);
    layoutDesc->addWidget(comboTipoDescuento, 1);
    layoutDesc->addWidget(txtDescuento, 1);
    layoutDerecho->addLayout(layoutDesc);

    QHBoxLayout* layoutTotal = new QHBoxLayout();
    QLabel* lblTextTotal = new QLabel(QStringLiteral("Total a Pagar"), panelDerecho);
    lblTextTotal->setStyleSheet(
        QStringLiteral("color: #111827; font-weight: bold; font-size: 15px; border: none;"));
    lblTotal = new QLabel(QStringLiteral("₡ 0.00"), panelDerecho);
    lblTotal->setStyleSheet(
        QStringLiteral("color: #047857; font-weight: bold; font-size: 18px; border: none;"));
    layoutTotal->addWidget(lblTextTotal);
    layoutTotal->addStretch();
    layoutTotal->addWidget(lblTotal);
    layoutDerecho->addLayout(layoutTotal);

    QLabel* lblPago = new QLabel(QStringLiteral("Pago en Efectivo"), panelDerecho);
    lblPago->setStyleSheet(
        QStringLiteral("font-size: 13px; font-weight: bold; color: #111827;"
                       " border: none; margin-top: 4px;"));
    layoutDerecho->addWidget(lblPago);

    QHBoxLayout* layoutMonto = new QHBoxLayout();
    QLabel* lblPagaCon = new QLabel(QStringLiteral("Paga con: ₡"), panelDerecho);
    lblPagaCon->setStyleSheet(
        QStringLiteral("font-size: 12px; color: #374151; font-weight: bold; border: none;"));

    txtMontoPaga = new QLineEdit(panelDerecho);
    txtMontoPaga->setMinimumWidth(90);
    txtMontoPaga->setAlignment(Qt::AlignRight);
    txtMontoPaga->setText(QStringLiteral("0.00"));
    txtMontoPaga->setPlaceholderText(QStringLiteral("0.00"));
    auto* validadorMonto = new QDoubleValidator(0.0, 10000000.0, 2, txtMontoPaga);
    validadorMonto->setNotation(QDoubleValidator::StandardNotation);
    validadorMonto->setLocale(QLocale::c());
    txtMontoPaga->setValidator(validadorMonto);
    txtMontoPaga->setStyleSheet(Estilos::estiloEntrada());

    layoutMonto->addWidget(lblPagaCon);
    layoutMonto->addWidget(txtMontoPaga, 1);
    layoutDerecho->addLayout(layoutMonto);

    QHBoxLayout* layoutCambio = new QHBoxLayout();
    QLabel* lblTextoCambio = new QLabel(QStringLiteral("Vuelto / Cambio:"), panelDerecho);
    lblTextoCambio->setStyleSheet(
        QStringLiteral("font-size: 12px; color: #6b7280; border: none;"));
    lblCambio = new QLabel(QStringLiteral("₡ 0.00"), panelDerecho);
    lblCambio->setStyleSheet(
        QStringLiteral("font-size: 13px; font-weight: bold; color: #047857; border: none;"));
    layoutCambio->addWidget(lblTextoCambio);
    layoutCambio->addStretch();
    layoutCambio->addWidget(lblCambio);
    layoutDerecho->addLayout(layoutCambio);

    connect(txtMontoPaga, &QLineEdit::textChanged,
            this, &VistaVentas::calcularCambioEfectivo);

    btnCheckout = new QPushButton(QStringLiteral("Procesar Venta"), panelDerecho);
    btnCheckout->setCursor(Qt::PointingHandCursor);
    btnCheckout->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #047857; color: #ffffff;"
                       " font-weight: bold; font-size: 14px; border: none;"
                       " border-radius: 10px; padding: 12px; }"
                       "QPushButton:hover { background-color: #065f46; }"
                       "QPushButton:disabled { background-color: #9ca3af; }"));
    connect(btnCheckout, &QPushButton::clicked, this, &VistaVentas::procesarCheckout);
    layoutDerecho->addWidget(btnCheckout);

    layoutPrincipal->addWidget(panelDerecho, 3);
}

void VistaVentas::cambiarCategoriaSlot(const QString& categoria) {
    categoriaSeleccionada = categoria;
    filtrarProductos();
}

void VistaVentas::filtrarProductos() {
    renderizarProductos();
}

void VistaVentas::renderizarProductos() {
    if (!gridProductos) return;

    QLayoutItem* item = nullptr;
    while ((item = gridProductos->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    const QString filtro = txtBuscar ? txtBuscar->text().trimmed().toLower() : QString();
    int fila = 0;
    int columna = 0;

    for (const auto& producto : catalogo) {
        const bool coincideCategoria = (categoriaSeleccionada == Categorias::Todos
                                        || producto.getCategoria() == categoriaSeleccionada);
        const bool coincideTexto = filtro.isEmpty()
                                   || producto.getNombre().toLower().contains(filtro)
                                   || producto.getId().toLower().contains(filtro);
        if (!coincideCategoria || !coincideTexto) continue;

        gridProductos->addWidget(crearTarjetaProducto(producto), fila, columna);
        if (++columna >= 3) {
            columna = 0;
            ++fila;
        }
    }

    if (catalogo.empty()) {
        QLabel* vacio = new QLabel(QStringLiteral("No hay productos activos en el inventario."),
                                   this);
        vacio->setAlignment(Qt::AlignCenter);
        vacio->setStyleSheet(
            QStringLiteral("color: #6b7280; font-size: 13px; padding: 30px;"));
        gridProductos->addWidget(vacio, 0, 0);
    }
}

QWidget* VistaVentas::crearTarjetaProducto(const ProductoItem& producto) {
    QFrame* tarjeta = new QFrame();
    tarjeta->setStyleSheet(
        QStringLiteral("QFrame { background-color: #ffffff; border-radius: 12px;"
                       " border: 1px solid #e5e7eb; }"
                       "QFrame:hover { border: 1px solid #047857; }"));
    QVBoxLayout* layout = new QVBoxLayout(tarjeta);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(5);

    QLabel* lblNombre = new QLabel(producto.getNombre(), tarjeta);
    lblNombre->setWordWrap(true);
    lblNombre->setStyleSheet(
        QStringLiteral("font-weight: bold; font-size: 13px; color: #111827; border: none;"));

    QLabel* lblDesc = new QLabel(
        QStringLiteral("%1 | Stock: %2").arg(producto.getDescripcion()).arg(producto.getStock()), tarjeta);
    lblDesc->setWordWrap(true);
    if (producto.getStock() <= 3) {
        lblDesc->setStyleSheet(
            QStringLiteral("color: #b91c1c; font-weight: bold; font-size: 11px; border: none;"));
    } else {
        lblDesc->setStyleSheet(
            QStringLiteral("color: #6b7280; font-size: 11px; border: none;"));
    }

    QHBoxLayout* layoutBottom = new QHBoxLayout();
    QLabel* lblPrecio = new QLabel(
        QStringLiteral("₡ %1").arg(producto.getPrecio(), 0, 'f', 2), tarjeta);
    lblPrecio->setStyleSheet(
        QStringLiteral("color: #047857; font-weight: bold; font-size: 14px; border: none;"));

    QPushButton* btnAgregar = new QPushButton(QStringLiteral("+"), tarjeta);
    btnAgregar->setCursor(Qt::PointingHandCursor);
    btnAgregar->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #047857; color: #ffffff;"
                       " font-weight: bold; border-radius: 6px; border: none;"
                       " padding: 4px 14px; font-size: 14px; }"
                       "QPushButton:hover { background-color: #065f46; }"
                       "QPushButton:disabled { background-color: #f3f4f6; color: #6b7280;"
                       " font-size: 11px; border: 1px solid #e5e7eb; }"));

    if (producto.getStock() > 0) {
        connect(btnAgregar, &QPushButton::clicked, this, [this, producto]() {
            agregarAlCarrito(producto);
        });
    } else {
        btnAgregar->setText(QStringLiteral("Agotado"));
        btnAgregar->setEnabled(false);
        btnAgregar->setStyleSheet(
            QStringLiteral("QPushButton { background-color: #f3f4f6; color: #6b7280;"
                           " font-size: 11px; font-weight: bold; border-radius: 6px;"
                           " border: 1px solid #e5e7eb; padding: 4px 8px; }"));
    }

    layoutBottom->addWidget(lblPrecio);
    layoutBottom->addStretch();
    layoutBottom->addWidget(btnAgregar);

    layout->addWidget(lblNombre);
    layout->addWidget(lblDesc);
    layout->addStretch();
    layout->addLayout(layoutBottom);
    return tarjeta;
}

void VistaVentas::agregarAlCarrito(const ProductoItem& producto) {
    int cantidadEnCarrito = 0;
    for (const auto& item : carrito) {
        if (item.producto.getId() == producto.getId()) {
            cantidadEnCarrito = item.cantidad;
            break;
        }
    }

    if (cantidadEnCarrito + 1 > producto.getStock()) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Stock insuficiente"),
            QStringLiteral("No se pueden agregar más unidades de '%1'.\nStock disponible: %2")
                .arg(producto.getNombre())
                .arg(producto.getStock()));
        return;
    }

    for (auto& item : carrito) {
        if (item.producto.getId() == producto.getId()) {
            ++item.cantidad;
            renderizarCarrito();
            actualizarTotales();
            return;
        }
    }

    carrito.push_back({producto, 1});
    renderizarCarrito();
    actualizarTotales();
}

void VistaVentas::renderizarCarrito() {
    if (!layoutListaCarrito) return;

    QLayoutItem* hijo = nullptr;
    while ((hijo = layoutListaCarrito->takeAt(0)) != nullptr) {
        if (hijo->widget()) hijo->widget()->deleteLater();
        delete hijo;
    }

    int totalArticulos = 0;
    if (carrito.empty()) {
        QLabel* vacio = new QLabel(QStringLiteral("El carrito está vacío"), contenedorCarrito);
        vacio->setAlignment(Qt::AlignCenter);
        vacio->setStyleSheet(
            QStringLiteral("color: #4b5563; font-size: 13px; padding: 20px; border: none;"));
        layoutListaCarrito->addWidget(vacio);
    } else {
        for (size_t i = 0; i < carrito.size(); ++i) {
            layoutListaCarrito->addWidget(
                crearWidgetCarritoItem(carrito[i], static_cast<int>(i)));
            totalArticulos += carrito[i].cantidad;
        }
    }

    lblResumenItems->setText(
        QStringLiteral("%1 artículo%2 en la orden")
            .arg(totalArticulos)
            .arg(totalArticulos == 1 ? QString() : QStringLiteral("s")));
    layoutListaCarrito->addStretch();
}

QWidget* VistaVentas::crearWidgetCarritoItem(const ItemCarrito& item, int index) {
    QWidget* contenedor = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(contenedor);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(6);

    QVBoxLayout* info = new QVBoxLayout();
    info->setSpacing(1);
    QLabel* lblNombre = new QLabel(item.producto.getNombre(), contenedor);
    lblNombre->setWordWrap(true);
    lblNombre->setStyleSheet(
        QStringLiteral("font-weight: bold; font-size: 12px; color: #111827; border: none;"));

    const double totalItem = item.producto.getPrecio() * item.cantidad;
    QLabel* lblSubtotal = new QLabel(
        QStringLiteral("₡ %1").arg(totalItem, 0, 'f', 2), contenedor);
    lblSubtotal->setStyleSheet(
        QStringLiteral("color: #047857; font-weight: bold; font-size: 12px; border: none;"));

    info->addWidget(lblNombre);
    info->addWidget(lblSubtotal);

    QPushButton* btnMenos = new QPushButton(QStringLiteral("-"), contenedor);
    QLabel* lblCantidad = new QLabel(QString::number(item.cantidad), contenedor);
    QPushButton* btnMas = new QPushButton(QStringLiteral("+"), contenedor);
    const QString estiloCantidad =
        QStringLiteral("QPushButton { background-color: #f3f4f6; color: #111827;"
                       " font-weight: bold; border-radius: 4px; border: none;"
                       " padding: 2px 8px; font-size: 12px; }"
                       "QPushButton:hover { background-color: #e5e7eb; }");
    btnMenos->setStyleSheet(estiloCantidad);
    btnMas->setStyleSheet(estiloCantidad);
    lblCantidad->setStyleSheet(
        QStringLiteral("font-size: 12px; font-weight: bold; color: #111827;"
                       " padding: 0 4px; border: none;"));
    btnMenos->setCursor(Qt::PointingHandCursor);
    btnMas->setCursor(Qt::PointingHandCursor);

    connect(btnMenos, &QPushButton::clicked, this,
            [this, index]() { cambiarCantidadItem(index, -1); });
    connect(btnMas, &QPushButton::clicked, this,
            [this, index]() { cambiarCantidadItem(index, 1); });

    QHBoxLayout* controles = new QHBoxLayout();
    controles->setSpacing(2);
    controles->addWidget(btnMenos);
    controles->addWidget(lblCantidad);
    controles->addWidget(btnMas);

    QPushButton* btnEliminar = new QPushButton(QStringLiteral("Quitar"), contenedor);
    btnEliminar->setCursor(Qt::PointingHandCursor);
    btnEliminar->setStyleSheet(
        QStringLiteral("QPushButton { border: none; color: #b91c1c; font-size: 11px;"
                       " background: transparent; }"
                       "QPushButton:hover { text-decoration: underline; }"));
    connect(btnEliminar, &QPushButton::clicked, this,
            [this, index]() { eliminarDelCarrito(index); });

    layout->addLayout(info, 1);
    layout->addLayout(controles);
    layout->addWidget(btnEliminar);
    return contenedor;
}

void VistaVentas::cambiarCantidadItem(int index, int delta) {
    if (index < 0 || index >= static_cast<int>(carrito.size())) return;

    if (delta > 0 && carrito[index].cantidad + 1 > carrito[index].producto.getStock()) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Stock insuficiente"),
            QStringLiteral("No hay más stock de '%1'.\nMáximo disponible: %2")
                .arg(carrito[index].producto.getNombre())
                .arg(carrito[index].producto.getStock()));
        return;
    }

    carrito[index].cantidad += delta;
    if (carrito[index].cantidad <= 0) {
        carrito.erase(carrito.begin() + index);
    }
    renderizarCarrito();
    actualizarTotales();
}

void VistaVentas::eliminarDelCarrito(int index) {
    if (index < 0 || index >= static_cast<int>(carrito.size())) return;
    carrito.erase(carrito.begin() + index);
    renderizarCarrito();
    actualizarTotales();
}

void VistaVentas::vaciarCarrito() {
    if (carrito.empty()) return;

    QMessageBox confirmacion(this);
    confirmacion.setWindowTitle(QStringLiteral("Vaciar orden"));
    confirmacion.setText(
        QStringLiteral("¿Está seguro de que desea quitar todos los productos de la orden?"));
    confirmacion.setIcon(QMessageBox::Question);
    QPushButton* btnSi = confirmacion.addButton(QStringLiteral("Sí"), QMessageBox::YesRole);
    confirmacion.addButton(QStringLiteral("No"), QMessageBox::NoRole);
    Estilos::estilizarMensaje(confirmacion);
    confirmacion.exec();

    if (confirmacion.clickedButton() == btnSi) {
        carrito.clear();
        {
            QSignalBlocker bloqueoDescuento(txtDescuento);
            txtDescuento->setText(QStringLiteral("0.00"));
        }
        {
            QSignalBlocker bloqueoPago(txtMontoPaga);
            txtMontoPaga->setText(QStringLiteral("0.00"));
        }
        renderizarCarrito();
        actualizarTotales();
    }
}

void VistaVentas::actualizarTotales() {
    if (!lblSubtotal || !lblIVA || !lblTotal) return;

    const int tipo = comboTipoDescuento ? comboTipoDescuento->currentIndex()
                                        : CalculadoraVenta::DescuentoMonto;
    double valor = valorDescuento();

    double bruto = 0.0;
    for (const auto& item : carrito) {
        bruto += item.producto.getPrecio() * item.cantidad;
    }

    // El descuento por monto no puede superar el subtotal: se ajusta en vivo
    // para evitar estados inválidos mientras el cajero escribe.
    if (tipo == CalculadoraVenta::DescuentoMonto && valor > bruto) {
        QSignalBlocker bloqueo(txtDescuento);
        txtDescuento->setText(QString::number(bruto, 'f', 2));
        valor = bruto;
    }

    const ResultadoVenta resultado =
        CalculadoraVenta::calcular(carrito, tipo, valor);

    if (!resultado.valido()) {
        subtotalActual = 0.0;
        ivaActual = 0.0;
        descuentoActual = 0.0;
        totalActual = 0.0;
        lblSubtotal->setText(QStringLiteral("₡ 0.00"));
        lblDescuento->setText(QStringLiteral("₡ 0.00"));
        lblIVA->setText(QStringLiteral("₡ 0.00"));
        lblTotal->setText(QStringLiteral("₡ 0.00"));
        return;
    }

    subtotalActual = resultado.subtotal;
    ivaActual = resultado.iva;
    descuentoActual = resultado.descuento;
    totalActual = resultado.total;

    lblSubtotal->setText(QStringLiteral("₡ %1").arg(subtotalActual, 0, 'f', 2));
    lblDescuento->setText(
        descuentoActual > 0.0
            ? QStringLiteral("-₡ %1").arg(descuentoActual, 0, 'f', 2)
            : QStringLiteral("₡ 0.00"));
    lblDescuento->setStyleSheet(
        descuentoActual > 0.0
            ? QStringLiteral("color: #b91c1c; font-weight: bold; font-size: 12px; border: none;")
            : QStringLiteral("color: #374151; font-weight: 500; font-size: 12px; border: none;"));
    lblIVA->setText(QStringLiteral("₡ %1").arg(ivaActual, 0, 'f', 2));
    lblTotal->setText(QStringLiteral("₡ %1").arg(totalActual, 0, 'f', 2));

    calcularCambioEfectivo();
}

void VistaVentas::calcularCambioEfectivo() {
    if (!lblCambio || !txtMontoPaga) return;

    const double pagaCon = montoPagado();
    const double cambio = CalculadoraVenta::calcularCambio(pagaCon, totalActual);

    if (totalActual <= 0.0) {
        lblCambio->setText(QStringLiteral("₡ 0.00"));
        lblCambio->setStyleSheet(
            QStringLiteral("font-size: 13px; font-weight: bold; color: #047857; border: none;"));
    } else if (cambio < 0.0) {
        lblCambio->setText(
            QStringLiteral("Falta ₡ %1").arg(std::abs(cambio), 0, 'f', 2));
        lblCambio->setStyleSheet(
            QStringLiteral("font-size: 12px; font-weight: bold; color: #b91c1c; border: none;"));
    } else {
        lblCambio->setText(QStringLiteral("₡ %1").arg(cambio, 0, 'f', 2));
        lblCambio->setStyleSheet(
            QStringLiteral("font-size: 13px; font-weight: bold; color: #047857; border: none;"));
    }
}

double VistaVentas::valorDescuento() const {
    bool valido = false;
    const double valor = txtDescuento ? txtDescuento->text().toDouble(&valido) : 0.0;
    return valido ? valor : 0.0;
}

double VistaVentas::montoPagado() const {
    bool valido = false;
    const double monto = txtMontoPaga ? txtMontoPaga->text().toDouble(&valido) : 0.0;
    return valido ? monto : 0.0;
}

void VistaVentas::procesarCheckout() {
    if (carrito.empty()) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Orden vacía"),
            QStringLiteral("Debe agregar al menos un producto antes de procesar la venta."));
        return;
    }
    if (totalActual <= 0.0) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Total inválido"),
            QStringLiteral("El total de la venta debe ser mayor que cero."));
        return;
    }
    const double pagaCon = montoPagado();
    if (pagaCon + 0.001 < totalActual) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Pago insuficiente"),
            QStringLiteral("El monto pagado es menor que el total.\n"
                           "Total: ₡ %1\nPaga con: ₡ %2\nFalta: ₡ %3")
                .arg(totalActual, 0, 'f', 2)
                .arg(pagaCon, 0, 'f', 2)
                .arg(totalActual - pagaCon, 0, 'f', 2));
        return;
    }

    ServicioVentas servicio;
    QString error;
    DatosVentaTicket venta;
    const bool registrada = servicio.registrar(
        carrito, nombreUsuario, comboTipoDescuento->currentIndex(),
        valorDescuento(), pagaCon, &venta, &error);

    if (!registrada) {
        Estilos::mostrarError(this, QStringLiteral("No se pudo procesar la venta"),
                              error.isEmpty()
                                  ? QStringLiteral("Ocurrió un error al registrar la venta.")
                                  : error);
        // El inventario pudo haber cambiado: se recarga todo (catálogo y carrito).
        actualizar();
        return;
    }

    mostrarYGuardarTicket(venta);
    carrito.clear();
    {
        QSignalBlocker bloqueoDescuento(txtDescuento);
        txtDescuento->setText(QStringLiteral("0.00"));
    }
    {
        QSignalBlocker bloqueoPago(txtMontoPaga);
        txtMontoPaga->setText(QStringLiteral("0.00"));
    }
    actualizar();
    emit ventaRegistrada(venta.getIdVenta());
}

void VistaVentas::mostrarYGuardarTicket(const DatosVentaTicket& datosVenta) {
    ServicioTickets tickets;
    QString errorRuta;
    const QString rutaTXT =
        ServicioTickets::rutaSugerida(datosVenta, FormatoTicket::Txt, &errorRuta);
    QString errorGuardado = errorRuta;
    const bool guardado = !rutaTXT.isEmpty()
                          && tickets.guardar(datosVenta, FormatoTicket::Txt, rutaTXT,
                                             &errorGuardado);

    QDialog dialogo(this);
    dialogo.setWindowTitle(QStringLiteral("Ticket de Compra - UNA Pulpe"));
    dialogo.setMinimumSize(340, 480);
    dialogo.resize(400, 620);
    dialogo.setStyleSheet(QStringLiteral("QDialog { background-color: #ffffff; }"));

    QVBoxLayout* layout = new QVBoxLayout(&dialogo);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(8);

    QLabel* lblEstado = new QLabel(&dialogo);
    lblEstado->setWordWrap(true);
    lblEstado->setTextInteractionFlags(Qt::TextSelectableByMouse);
    if (guardado) {
        lblEstado->setText(QStringLiteral("Ticket guardado en:\n%1").arg(rutaTXT));
        lblEstado->setStyleSheet(QStringLiteral("color: #047857; font-size: 11px;"));
    } else {
        lblEstado->setText(
            QStringLiteral("El ticket se muestra en pantalla, pero no se pudo guardar el "
                           "archivo TXT.\n%1").arg(errorGuardado));
        lblEstado->setStyleSheet(QStringLiteral("color: #b91c1c; font-size: 11px;"));
    }
    layout->addWidget(lblEstado);

    QTextBrowser* visor = new QTextBrowser(&dialogo);
    visor->setHtml(tickets.generarContenido(datosVenta, FormatoTicket::Html));
    visor->setStyleSheet(
        QStringLiteral("QTextBrowser { border: 1px solid #e5e7eb; border-radius: 8px;"
                       " background-color: #ffffff; color: #111827; padding: 10px; }"));
    layout->addWidget(visor, 1);

    QHBoxLayout* botones = new QHBoxLayout();
    QPushButton* btnPDF = new QPushButton(QStringLiteral("Guardar PDF..."), &dialogo);
    btnPDF->setStyleSheet(Estilos::estiloBotonSecundario());
    QPushButton* btnCerrar = new QPushButton(QStringLiteral("Aceptar"), &dialogo);
    btnCerrar->setStyleSheet(Estilos::estiloBotonPrincipal());

    connect(btnPDF, &QPushButton::clicked, &dialogo,
            [&dialogo, &tickets, datosVenta]() {
        const QString sugerida = ServicioTickets::rutaSugerida(datosVenta, FormatoTicket::Pdf);
        QString ruta = QFileDialog::getSaveFileName(
            &dialogo, QStringLiteral("Guardar ticket en PDF"), sugerida,
            QStringLiteral("Documento PDF (*.pdf)"));
        if (ruta.isEmpty()) return;
        if (!ruta.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
            ruta += QStringLiteral(".pdf");
        }
        QString error;
        if (tickets.guardar(datosVenta, FormatoTicket::Pdf, ruta, &error)) {
            Estilos::mostrarInfo(&dialogo, QStringLiteral("PDF creado"),
                                 QStringLiteral("El ticket se guardó en:\n%1").arg(ruta));
        } else {
            Estilos::mostrarError(&dialogo, QStringLiteral("Error al guardar"), error);
        }
    });
    connect(btnCerrar, &QPushButton::clicked, &dialogo, &QDialog::accept);

    botones->addWidget(btnPDF);
    botones->addStretch();
    botones->addWidget(btnCerrar);
    layout->addLayout(botones);

    dialogo.exec();
}
