#include "VistaHistorial.h"

#include "Estilos.h"
#include "ServicioReportes.h"
#include "ServicioTickets.h"
#include "ServicioVentas.h"

#include <QBrush>
#include <QColor>
#include <QDate>
#include <QFileDialog>
#include <QShowEvent>
#include <QStandardPaths>
#include <algorithm>
#include <utility>

VistaHistorial::VistaHistorial(QWidget* parent)
    : VistaHistorial(nullptr, parent) {}

VistaHistorial::VistaHistorial(std::shared_ptr<Usuario> usuario, QWidget* parent)
    : QWidget(parent), usuarioActual(std::move(usuario)) {
    configurarInterfaz();
    cargarDatos();
}

bool VistaHistorial::esAdministrador() const {
    return usuarioActual && usuarioActual->isActivo()
           && usuarioActual->rol() == QStringLiteral("Administrador");
}

void VistaHistorial::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    actualizar();
}

void VistaHistorial::actualizar() {
    cargarDatos();
}

void VistaHistorial::configurarInterfaz() {
    QVBoxLayout* layoutPrincipal = new QVBoxLayout(this);
    layoutPrincipal->setContentsMargins(16, 16, 16, 16);
    layoutPrincipal->setSpacing(12);
    setStyleSheet(QStringLiteral("background-color: #f8f9fa;"));

    QHBoxLayout* layoutCards = new QHBoxLayout();
    layoutCards->setSpacing(15);

    auto crearCard = [this](const QString& titulo, QLabel*& valor, const QString& colorTexto) {
        QFrame* card = new QFrame(this);
        card->setStyleSheet(
            QStringLiteral("QFrame { background-color: #ffffff; border-radius: 12px;"
                           " border: 1px solid #e5e7eb; }"));
        QVBoxLayout* layoutCard = new QVBoxLayout(card);
        layoutCard->setContentsMargins(15, 12, 15, 12);

        QLabel* etiqueta = new QLabel(titulo, card);
        etiqueta->setStyleSheet(
            QStringLiteral("color: #6b7280; font-size: 11px; font-weight: bold;"
                           " border: none;"));

        valor = new QLabel(QStringLiteral("₡ 0.00"), card);
        valor->setStyleSheet(
            QStringLiteral("color: %1; font-size: 18px; font-weight: bold;"
                           " border: none;").arg(colorTexto));

        layoutCard->addWidget(etiqueta);
        layoutCard->addWidget(valor);
        return card;
    };

    layoutCards->addWidget(crearCard(QStringLiteral("TOTAL VENTAS"), lblTotalVendido,
                                     QStringLiteral("#047857")));
    layoutCards->addWidget(crearCard(QStringLiteral("IVA RECAUDADO (13%)"), lblTotalIVA,
                                     QStringLiteral("#1d4ed8")));
    layoutCards->addWidget(crearCard(QStringLiteral("TRANSACCIONES"), lblCantidadVentas,
                                     QStringLiteral("#111827")));
    layoutPrincipal->addLayout(layoutCards);

    QHBoxLayout* layoutTop = new QHBoxLayout();
    layoutTop->setSpacing(8);

    txtBuscar = new QLineEdit(this);
    txtBuscar->setPlaceholderText(QStringLiteral("Buscar por ID o usuario..."));
    txtBuscar->setStyleSheet(Estilos::estiloEntrada());
    connect(txtBuscar, &QLineEdit::textChanged, this, &VistaHistorial::filtrarTabla);

    comboRangoFecha = new QComboBox(this);
    comboRangoFecha->addItems({
        QStringLiteral("Todas las Fechas"), QStringLiteral("Hoy"),
        QStringLiteral("Últimos 7 Días"), QStringLiteral("Este Mes"),
        QStringLiteral("Rango Personalizado")
    });
    comboRangoFecha->setStyleSheet(Estilos::estiloEntrada());

    dateInicio = new QDateEdit(QDate::currentDate(), this);
    dateInicio->setCalendarPopup(true);
    dateInicio->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dateInicio->setVisible(false);
    dateInicio->setStyleSheet(Estilos::estiloEntrada());

    dateFin = new QDateEdit(QDate::currentDate(), this);
    dateFin->setCalendarPopup(true);
    dateFin->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dateFin->setVisible(false);
    dateFin->setStyleSheet(Estilos::estiloEntrada());

    connect(comboRangoFecha, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                const bool personalizado =
                    comboRangoFecha->itemText(index) == QStringLiteral("Rango Personalizado");
                dateInicio->setVisible(personalizado);
                dateFin->setVisible(personalizado);
                filtrarTabla();
            });
    connect(dateInicio, &QDateEdit::dateChanged, this, [this](const QDate& fecha) {
        dateFin->setMinimumDate(fecha);
        filtrarTabla();
    });
    connect(dateFin, &QDateEdit::dateChanged, this, [this](const QDate& fecha) {
        dateInicio->setMaximumDate(fecha);
        filtrarTabla();
    });

    btnVerTicket = new QPushButton(QStringLiteral("Ver Ticket"), this);
    btnVerTicket->setStyleSheet(Estilos::estiloBotonPrincipal());
    connect(btnVerTicket, &QPushButton::clicked,
            this, &VistaHistorial::mostrarTicketSeleccionado);

    btnDescargarPDF = new QPushButton(QStringLiteral("Descargar PDF"), this);
    btnDescargarPDF->setStyleSheet(Estilos::estiloBotonPrincipal());
    connect(btnDescargarPDF, &QPushButton::clicked,
            this, &VistaHistorial::descargarTicketPDF);

    btnExportarCSV = new QPushButton(QStringLiteral("Exportar CSV"), this);
    btnExportarCSV->setStyleSheet(Estilos::estiloBotonSecundario());
    connect(btnExportarCSV, &QPushButton::clicked, this, &VistaHistorial::exportarCSV);

    btnAnularVenta = new QPushButton(QStringLiteral("Anular Venta"), this);
    btnAnularVenta->setStyleSheet(Estilos::estiloBotonPeligro());
    connect(btnAnularVenta, &QPushButton::clicked,
            this, &VistaHistorial::anularVentaSeleccionada);
    if (!esAdministrador()) {
        btnAnularVenta->setVisible(false);
        btnAnularVenta->setEnabled(false);
    }

    layoutTop->addWidget(txtBuscar, 2);
    layoutTop->addWidget(comboRangoFecha, 2);
    layoutTop->addWidget(dateInicio, 1);
    layoutTop->addWidget(dateFin, 1);
    layoutTop->addWidget(btnVerTicket, 1);
    layoutTop->addWidget(btnDescargarPDF, 1);
    layoutTop->addWidget(btnExportarCSV, 1);
    layoutTop->addWidget(btnAnularVenta, 1);
    layoutPrincipal->addLayout(layoutTop);

    tablaVentas = new QTableWidget(this);
    tablaVentas->setColumnCount(7);
    tablaVentas->setHorizontalHeaderLabels({
        QStringLiteral("ID Venta"), QStringLiteral("Fecha y Hora"),
        QStringLiteral("Usuario"), QStringLiteral("Subtotal (₡)"),
        QStringLiteral("IVA (₡)"), QStringLiteral("Total (₡)"),
        QStringLiteral("Estado")
    });
    tablaVentas->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tablaVentas->verticalHeader()->setVisible(false);
    tablaVentas->setSelectionBehavior(QAbstractItemView::SelectRows);
    tablaVentas->setSelectionMode(QAbstractItemView::SingleSelection);
    tablaVentas->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tablaVentas->setAlternatingRowColors(true);
    tablaVentas->setStyleSheet(Estilos::estiloTabla());
    layoutPrincipal->addWidget(tablaVentas, 1);
}

void VistaHistorial::cargarDatos() {
    ServicioVentas servicio;
    QString error;
    listaVentas = servicio.listar(&error);
    if (!error.isEmpty()) {
        Estilos::mostrarError(this, QStringLiteral("No se pudo leer el historial"), error);
        listaVentas.clear();
    }
    renderizarTabla();
    emit historialActualizado();
}

void VistaHistorial::renderizarTabla() {
    if (!tablaVentas) return;
    tablaVentas->setRowCount(0);

    for (const auto& venta : listaVentas) {
        const int fila = tablaVentas->rowCount();
        tablaVentas->insertRow(fila);

        auto* itemID = new QTableWidgetItem(venta.getIdVenta());
        auto* itemFecha = new QTableWidgetItem(
            venta.getFechaHora().isValid()
                ? venta.getFechaHora().toString(QStringLiteral("yyyy-MM-dd hh:mm AP"))
                : QStringLiteral("—"));
        auto* itemUsuario = new QTableWidgetItem(venta.getUsuario());
        auto* itemSubtotal = new QTableWidgetItem(
            QStringLiteral("₡ %1").arg(venta.getSubtotal(), 0, 'f', 2));
        auto* itemIVA = new QTableWidgetItem(
            QStringLiteral("₡ %1").arg(venta.getIva(), 0, 'f', 2));
        auto* itemTotal = new QTableWidgetItem(
            QStringLiteral("₡ %1").arg(venta.getTotal(), 0, 'f', 2));
        auto* itemEstado = new QTableWidgetItem(
            venta.isAnulada() ? QStringLiteral("Anulada") : QStringLiteral("Vigente"));

        itemSubtotal->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemIVA->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemTotal->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        if (venta.isAnulada()) {
            const QString detalle = QStringLiteral("Anulada por %1 el %2")
                                        .arg(venta.getUsuarioAnulacion().isEmpty()
                                                 ? QStringLiteral("—")
                                                 : venta.getUsuarioAnulacion(),
                                             venta.getFechaAnulacion().isValid()
                                                 ? venta.getFechaAnulacion().toString(
                                                       QStringLiteral("yyyy-MM-dd hh:mm AP"))
                                                 : QStringLiteral("—"));
            itemEstado->setForeground(QBrush(QColor(QStringLiteral("#b91c1c"))));
            itemEstado->setToolTip(detalle);
            itemTotal->setForeground(QBrush(QColor(QStringLiteral("#6b7280"))));
            itemID->setForeground(QBrush(QColor(QStringLiteral("#6b7280"))));
        } else {
            itemEstado->setForeground(QBrush(QColor(QStringLiteral("#047857"))));
            itemTotal->setForeground(QBrush(QColor(QStringLiteral("#047857"))));
        }

        tablaVentas->setItem(fila, 0, itemID);
        tablaVentas->setItem(fila, 1, itemFecha);
        tablaVentas->setItem(fila, 2, itemUsuario);
        tablaVentas->setItem(fila, 3, itemSubtotal);
        tablaVentas->setItem(fila, 4, itemIVA);
        tablaVentas->setItem(fila, 5, itemTotal);
        tablaVentas->setItem(fila, 6, itemEstado);
    }

    filtrarTabla();
}

void VistaHistorial::filtrarTabla() {
    if (!tablaVentas || !txtBuscar || !comboRangoFecha) return;

    const QString texto = txtBuscar->text().trimmed().toLower();
    const QString opcionFecha = comboRangoFecha->currentText();
    const QDate hoy = QDate::currentDate();

    double totalVisible = 0.0;
    double ivaVisible = 0.0;
    int transaccionesVisibles = 0;

    for (int fila = 0; fila < tablaVentas->rowCount(); ++fila) {
        if (fila >= static_cast<int>(listaVentas.size())) break;

        const auto* itemID = tablaVentas->item(fila, 0);
        const auto* itemUsuario = tablaVentas->item(fila, 2);
        if (!itemID || !itemUsuario) continue;

        const QString id = itemID->text().toLower();
        const QString usuario = itemUsuario->text().toLower();
        const QDate fecha = listaVentas[fila].getFechaHora().date();

        bool coincideTexto = texto.isEmpty() || id.contains(texto) || usuario.contains(texto);
        bool coincideFecha = true;

        if (opcionFecha == QStringLiteral("Hoy")) {
            coincideFecha = (fecha == hoy);
        } else if (opcionFecha == QStringLiteral("Últimos 7 Días")) {
            coincideFecha = (fecha >= hoy.addDays(-7) && fecha <= hoy);
        } else if (opcionFecha == QStringLiteral("Este Mes")) {
            coincideFecha = (fecha.year() == hoy.year() && fecha.month() == hoy.month());
        } else if (opcionFecha == QStringLiteral("Rango Personalizado")) {
            coincideFecha = (fecha >= dateInicio->date() && fecha <= dateFin->date());
        }

        const bool mostrar = coincideTexto && coincideFecha;
        tablaVentas->setRowHidden(fila, !mostrar);

        if (mostrar) {
            ++transaccionesVisibles;
            if (!listaVentas[fila].isAnulada()) {
                totalVisible += listaVentas[fila].getTotal();
                ivaVisible += listaVentas[fila].getIva();
            }
        }
    }

    lblTotalVendido->setText(QStringLiteral("₡ %1").arg(totalVisible, 0, 'f', 2));
    lblTotalIVA->setText(QStringLiteral("₡ %1").arg(ivaVisible, 0, 'f', 2));
    lblCantidadVentas->setText(QString::number(transaccionesVisibles));
}

void VistaHistorial::mostrarTicketSeleccionado() {
    const int fila = tablaVentas->currentRow();
    if (fila < 0 || fila >= static_cast<int>(listaVentas.size())) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Selección requerida"),
            QStringLiteral("Seleccione una venta de la tabla para visualizar su ticket."));
        return;
    }

    const auto& venta = listaVentas[fila];
    ServicioTickets tickets;
    QDialog dialogo(this);
    dialogo.setWindowTitle(QStringLiteral("Ticket Venta %1").arg(venta.getIdVenta()));
    dialogo.setMinimumSize(340, 480);
    dialogo.resize(400, 620);
    dialogo.setStyleSheet(QStringLiteral("QDialog { background-color: #ffffff; }"));

    QVBoxLayout* layout = new QVBoxLayout(&dialogo);
    layout->setContentsMargins(15, 15, 15, 15);

    QTextBrowser* visor = new QTextBrowser(&dialogo);
    visor->setHtml(tickets.generarContenido(venta, FormatoTicket::Html));
    visor->setStyleSheet(
        QStringLiteral("QTextBrowser { border: 1px solid #e5e7eb; border-radius: 8px;"
                       " background-color: #ffffff; color: #111827; padding: 10px; }"));

    QPushButton* btnCerrar = new QPushButton(QStringLiteral("Cerrar Visualización"), &dialogo);
    btnCerrar->setStyleSheet(Estilos::estiloBotonPrincipal());
    connect(btnCerrar, &QPushButton::clicked, &dialogo, &QDialog::accept);

    layout->addWidget(visor, 1);
    layout->addWidget(btnCerrar);
    dialogo.exec();
}

void VistaHistorial::descargarTicketPDF() {
    const int fila = tablaVentas->currentRow();
    if (fila < 0 || fila >= static_cast<int>(listaVentas.size())) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Selección requerida"),
            QStringLiteral("Seleccione una venta para descargar su ticket en PDF."));
        return;
    }

    const auto& venta = listaVentas[fila];
    const QString sugerida = ServicioTickets::rutaSugerida(venta, FormatoTicket::Pdf);
    QString ruta = QFileDialog::getSaveFileName(
        this, QStringLiteral("Guardar ticket en PDF"), sugerida,
        QStringLiteral("Documento PDF (*.pdf)"));
    if (ruta.isEmpty()) return;
    if (!ruta.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
        ruta += QStringLiteral(".pdf");
    }

    ServicioTickets tickets;
    QString error;
    if (tickets.guardar(venta, FormatoTicket::Pdf, ruta, &error)) {
        Estilos::mostrarInfo(this, QStringLiteral("PDF creado"),
                             QStringLiteral("El ticket se guardó en:\n%1").arg(ruta));
    } else {
        Estilos::mostrarError(this, QStringLiteral("Error al guardar"), error);
    }
}

void VistaHistorial::exportarCSV() {
    std::vector<DatosVentaTicket> visibles;
    for (int fila = 0; fila < tablaVentas->rowCount(); ++fila) {
        if (tablaVentas->isRowHidden(fila)) continue;
        if (fila < static_cast<int>(listaVentas.size())) {
            visibles.push_back(listaVentas[fila]);
        }
    }

    if (visibles.empty()) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Sin datos"),
            QStringLiteral("No hay ventas visibles para exportar."));
        return;
    }

    QString ruta = QFileDialog::getSaveFileName(
        this, QStringLiteral("Guardar Reporte CSV"),
        ServicioReportes::rutaSugeridaCSV(), QStringLiteral("Archivos CSV (*.csv)"));
    if (ruta.isEmpty()) return;
    if (!ruta.endsWith(QStringLiteral(".csv"), Qt::CaseInsensitive)) {
        ruta += QStringLiteral(".csv");
    }

    QString error;
    if (ServicioReportes::exportarVentasCSV(visibles, ruta, &error)) {
        Estilos::mostrarInfo(this, QStringLiteral("Reporte creado"),
                             QStringLiteral("El reporte CSV se exportó correctamente en:\n%1")
                                 .arg(ruta));
    } else {
        Estilos::mostrarError(this, QStringLiteral("Error de archivo"), error);
    }
}

void VistaHistorial::anularVentaSeleccionada() {
    if (!esAdministrador()) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Permiso denegado"),
            QStringLiteral("Solo un administrador puede anular ventas."));
        return;
    }

    const int fila = tablaVentas->currentRow();
    if (fila < 0 || fila >= static_cast<int>(listaVentas.size())) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Selección requerida"),
            QStringLiteral("Seleccione una venta para anular."));
        return;
    }

    const auto& venta = listaVentas[fila];
    if (venta.isAnulada()) {
        Estilos::mostrarAdvertencia(
            this, QStringLiteral("Venta ya anulada"),
            QStringLiteral("La venta '%1' ya fue anulada.").arg(venta.getIdVenta()));
        return;
    }

    QMessageBox confirmacion(this);
    confirmacion.setWindowTitle(QStringLiteral("Confirmar anulación"));
    confirmacion.setText(
        QStringLiteral("¿Desea anular la venta '%1'?\n\n"
                       "El registro se conservará en el historial y el stock "
                       "correspondiente será devuelto al inventario una única vez.")
            .arg(venta.getIdVenta()));
    confirmacion.setIcon(QMessageBox::Question);
    QPushButton* btnSi = confirmacion.addButton(QStringLiteral("Sí, anular"),
                                                QMessageBox::YesRole);
    confirmacion.addButton(QStringLiteral("Cancelar"), QMessageBox::NoRole);
    Estilos::estilizarMensaje(confirmacion, true);
    confirmacion.exec();
    if (confirmacion.clickedButton() != btnSi) return;

    ServicioVentas servicio;
    QString error;
    if (servicio.anular(venta.getIdVenta(), usuarioActual, &error)) {
        cargarDatos();
        emit ventaAnulada(venta.getIdVenta());
        if (!servicio.ultimaAdvertencia().isEmpty()) {
            Estilos::mostrarAdvertencia(this, QStringLiteral("Auditoría incompleta"),
                                        servicio.ultimaAdvertencia());
        }
        Estilos::mostrarInfo(
            this, QStringLiteral("Venta anulada"),
            QStringLiteral("La venta se anuló y las unidades fueron devueltas al inventario."));
    } else {
        Estilos::mostrarError(this, QStringLiteral("No se pudo anular la venta"), error);
    }
}
