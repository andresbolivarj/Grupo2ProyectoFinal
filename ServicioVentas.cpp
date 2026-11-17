#include "ServicioVentas.h"

#include "CalculadoraVenta.h"

#include <QDateTime>
#include <QSet>
#include <algorithm>
#include <cmath>

namespace {
void establecerError(QString* destino, const QString& mensaje) {
    if (destino) *destino = mensaje;
}

bool esNumeroValido(double valor) {
    return std::isfinite(valor);
}
} // namespace

ServicioVentas::ServicioVentas(const QString& rutaProductos, const QString& rutaVentas,
                               const QString& rutaAuditoria)
    : gestor(rutaProductos, rutaVentas, rutaAuditoria) {}

bool ServicioVentas::esAdministrador(const std::shared_ptr<Usuario>& usuario) {
    return usuario && usuario->isActivo()
           && usuario->rol() == QStringLiteral("Administrador");
}

std::vector<DatosVentaTicket> ServicioVentas::listar(QString* error) {
    if (error) error->clear();
    gestor.limpiarError();
    auto ventas = gestor.cargarVentas();
    if (!gestor.ultimoError().isEmpty()) {
        establecerError(error, gestor.ultimoError());
    }
    return ventas;
}

QString ServicioVentas::siguienteIdVenta(QString* error) {
    if (error) error->clear();
    const QString id = gestor.siguienteIdVenta();
    if (id.isEmpty()) {
        establecerError(error, gestor.ultimoError().isEmpty()
                                   ? QStringLiteral("No se pudo generar el ID de venta.")
                                   : gestor.ultimoError());
    }
    return id;
}

bool ServicioVentas::registrar(const std::vector<ItemCarrito>& carrito,
                               const QString& usuario,
                               int tipoDescuento,
                               double valorDescuento,
                               double pagaCon,
                               DatosVentaTicket* ventaGenerada,
                               QString* error) {
    if (error) error->clear();
    ultimaAdvertencia_.clear();

    if (usuario.trimmed().isEmpty()) {
        establecerError(error, QStringLiteral("No hay un usuario autenticado para registrar la venta."));
        return false;
    }
    if (carrito.empty()) {
        establecerError(error, QStringLiteral("Debe agregar al menos un producto antes de cobrar."));
        return false;
    }

    // Un solo bloqueo para todo el flujo: inventario + ventas + consecutivo.
    TransaccionJSON transaccion(gestor, error);
    if (!transaccion.activa()) return false;

    gestor.limpiarError();
    auto productos = gestor.cargarProductos();
    if (!gestor.ultimoError().isEmpty()) {
        establecerError(error, gestor.ultimoError());
        return false;
    }

    gestor.limpiarError();
    auto ventas = gestor.cargarVentas();
    if (!gestor.ultimoError().isEmpty()) {
        establecerError(error, gestor.ultimoError());
        return false;
    }

    // Se valida y se cotiza con el inventario vigente en disco, no con la copia
    // que muestra la interfaz, para evitar precios manipulados o sobreventa.
    std::vector<ItemCarrito> carritoValidado;
    QSet<QString> idsEnCarrito;
    for (const auto& item : carrito) {
        if (item.cantidad <= 0) {
            establecerError(error, QStringLiteral("La cantidad de '%1' debe ser mayor que cero.")
                                      .arg(item.producto.getNombre()));
            return false;
        }
        if (idsEnCarrito.contains(item.producto.getId())) {
            establecerError(error, QStringLiteral("El producto '%1' está duplicado en la orden.")
                                      .arg(item.producto.getNombre()));
            return false;
        }
        idsEnCarrito.insert(item.producto.getId());

        auto producto = std::find_if(productos.begin(), productos.end(),
                                     [&item](const ProductoItem& p) {
                                         return p.getId().compare(item.producto.getId(),
                                                                  Qt::CaseInsensitive) == 0;
                                     });
        if (producto == productos.end()) {
            establecerError(error, QStringLiteral("El producto '%1' ya no existe en el inventario.")
                                      .arg(item.producto.getNombre()));
            return false;
        }
        if (!producto->isActivo()) {
            establecerError(error, QStringLiteral("El producto '%1' está desactivado y no se puede vender.")
                                      .arg(producto->getNombre()));
            return false;
        }
        if (producto->getStock() < item.cantidad) {
            establecerError(error,
                            QStringLiteral("Stock insuficiente de '%1'. Disponible: %2, solicitado: %3.")
                                .arg(producto->getNombre())
                                .arg(producto->getStock())
                                .arg(item.cantidad));
            return false;
        }

        carritoValidado.push_back({*producto, item.cantidad});
    }

    const ResultadoVenta resultado = CalculadoraVenta::calcular(
        carritoValidado, tipoDescuento, valorDescuento);
    if (!resultado.valido()) {
        establecerError(error, resultado.error);
        return false;
    }
    if (resultado.total <= 0.0) {
        establecerError(error, QStringLiteral("El total de la venta debe ser mayor que cero."));
        return false;
    }
    if (!esNumeroValido(pagaCon) || pagaCon < 0.0) {
        establecerError(error, QStringLiteral("El monto pagado no es válido."));
        return false;
    }
    if (pagaCon + 0.001 < resultado.total) {
        establecerError(error,
                        QStringLiteral("Pago insuficiente. Total: ₡%1, pagado: ₡%2, falta: ₡%3.")
                            .arg(resultado.total, 0, 'f', 2)
                            .arg(pagaCon, 0, 'f', 2)
                            .arg(resultado.total - pagaCon, 0, 'f', 2));
        return false;
    }

    // El consecutivo se reserva antes de escribir cualquier archivo.
    const QString idVenta = gestor.siguienteIdVenta();
    if (idVenta.isEmpty()) {
        establecerError(error, gestor.ultimoError().isEmpty()
                                   ? QStringLiteral("No se pudo generar el ID de venta.")
                                   : gestor.ultimoError());
        return false;
    }

    DatosVentaTicket venta;
    venta.setIdVenta(idVenta);
    venta.setUsuario(usuario.trimmed());
    venta.setFechaHora(QDateTime::currentDateTime());
    venta.setSubtotal(resultado.subtotal);
    venta.setIva(resultado.iva);
    venta.setTotal(resultado.total);
    venta.setPagaCon(CalculadoraVenta::redondear(pagaCon));
    venta.setCambio(CalculadoraVenta::calcularCambio(pagaCon, resultado.total));
    venta.setDescuento(resultado.descuento);
    venta.setTipoDescuento((tipoDescuento == CalculadoraVenta::DescuentoPorcentaje)
                               ? QStringLiteral("Porcentaje")
                               : QStringLiteral("Monto"));
    venta.setValorDescuento(valorDescuento);

    for (const auto& item : carritoValidado) {
        const auto detalle = ItemTicket::crear(
            item.producto.getId(), item.producto.getNombre(), item.cantidad,
            CalculadoraVenta::redondear(item.producto.getPrecio()),
            CalculadoraVenta::redondear(item.producto.getPrecio() * item.cantidad));
        if (detalle.has_value()) venta.anadirItem(*detalle);
    }

    // Orden transaccional: el comprobante financiero es la fuente de verdad y
    // se persiste primero. Si falla aquí, el inventario aún no se ha tocado.
    const auto ventasOriginales = ventas;
    ventas.push_back(venta);

    gestor.limpiarError();
    if (!gestor.guardarVentas(ventas)) {
        establecerError(error, gestor.ultimoError().isEmpty()
                                   ? QStringLiteral("No se pudo registrar la venta.")
                                   : gestor.ultimoError());
        return false;
    }

    // Rebaja de inventario una sola vez, después de asegurar la venta.
    std::vector<ProductoItem> productosConRebaja = productos;
    for (const auto& item : carritoValidado) {
        for (auto& producto : productosConRebaja) {
            if (producto.getId().compare(item.producto.getId(), Qt::CaseInsensitive) == 0) {
                producto.setStock(producto.getStock() - item.cantidad);
                break;
            }
        }
    }

    gestor.limpiarError();
    if (!gestor.guardarProductos(productosConRebaja)) {
        const QString errorStock = gestor.ultimoError();
        gestor.limpiarError();

        // Reversión de la venta recién escrita: nunca se confirmó al usuario.
        if (!gestor.guardarVentas(ventasOriginales)) {
            establecerError(
                error,
                QStringLiteral("No se pudo actualizar el inventario (%1) y tampoco se pudo "
                               "revertir la venta (%2). Revise los archivos manualmente.")
                    .arg(errorStock.isEmpty() ? QStringLiteral("error desconocido") : errorStock,
                         gestor.ultimoError().isEmpty()
                             ? QStringLiteral("error desconocido")
                             : gestor.ultimoError()));
        } else {
            establecerError(
                error,
                QStringLiteral("No se pudo actualizar el inventario; la venta fue revertida. %1")
                    .arg(errorStock.isEmpty() ? QStringLiteral("Error desconocido.")
                                              : errorStock));
        }
        return false;
    }

    if (ventaGenerada) *ventaGenerada = venta;
    return true;
}

bool ServicioVentas::anular(const QString& idVenta,
                            const std::shared_ptr<Usuario>& solicitante,
                            QString* error) {
    if (error) error->clear();
    ultimaAdvertencia_.clear();

    if (!esAdministrador(solicitante)) {
        establecerError(error, QStringLiteral("Solo un administrador puede anular ventas."));
        return false;
    }

    // Estado de la venta, stock y auditoría quedan bajo el mismo bloqueo.
    TransaccionJSON transaccion(gestor, error);
    if (!transaccion.activa()) return false;

    QString errorLectura;
    auto ventas = listar(&errorLectura);
    if (!errorLectura.isEmpty()) {
        establecerError(error, errorLectura);
        return false;
    }

    auto venta = std::find_if(ventas.begin(), ventas.end(),
                              [&idVenta](const DatosVentaTicket& v) {
                                  return v.getIdVenta() == idVenta;
                              });
    if (venta == ventas.end()) {
        establecerError(error, QStringLiteral("La venta indicada no existe."));
        return false;
    }
    if (venta->isAnulada()) {
        establecerError(error, QStringLiteral("La venta ya se encuentra anulada."));
        return false;
    }

    gestor.limpiarError();
    auto productos = gestor.cargarProductos();
    if (!gestor.ultimoError().isEmpty()) {
        establecerError(error, gestor.ultimoError());
        return false;
    }

    const auto ventasOriginales = ventas;

    // Reintegra el stock usando el id histórico; las ventas antiguas sin id
    // se resuelven por nombre como respaldo.
    for (const auto& item : venta->getItems()) {
        for (auto& producto : productos) {
            const bool coincideId = !item.getIdProducto().isEmpty()
                                    && producto.getId().compare(item.getIdProducto(),
                                                                Qt::CaseInsensitive) == 0;
            const bool coincideNombre = item.getIdProducto().isEmpty()
                                        && producto.getNombre() == item.getNombreProducto();
            if (coincideId || coincideNombre) {
                producto.setStock(producto.getStock() + item.getCantidad());
                break;
            }
        }
    }

    venta->setAnulada(true);
    venta->setUsuarioAnulacion(solicitante->getNombreUsuario());
    venta->setFechaAnulacion(QDateTime::currentDateTime());

    gestor.limpiarError();
    if (!gestor.guardarVentas(ventas)) {
        establecerError(error, gestor.ultimoError().isEmpty()
                                   ? QStringLiteral("No se pudo anular la venta.")
                                   : gestor.ultimoError());
        return false;
    }

    gestor.limpiarError();
    if (!gestor.guardarProductos(productos)) {
        const QString errorStock = gestor.ultimoError();
        gestor.limpiarError();

        // QSaveFile es atómico: si la escritura del inventario falló, el archivo
        // original sigue intacto. Solo hay que deshacer el estado de la venta.
        if (!gestor.guardarVentas(ventasOriginales)) {
            establecerError(
                error,
                QStringLiteral("No se pudo devolver el stock (%1) y tampoco se pudo revertir "
                               "la anulación (%2). Revise los archivos manualmente.")
                    .arg(errorStock.isEmpty() ? QStringLiteral("error desconocido") : errorStock,
                         gestor.ultimoError().isEmpty()
                             ? QStringLiteral("error desconocido")
                             : gestor.ultimoError()));
        } else {
            establecerError(
                error,
                QStringLiteral("No se pudo devolver el stock al inventario; la anulación "
                               "fue revertida. %1")
                    .arg(errorStock.isEmpty() ? QStringLiteral("Error desconocido.")
                                              : errorStock));
        }
        return false;
    }

    // El reintegro es un movimiento de inventario: se registra en la misma
    // auditoría que usa el CRUD de productos, dentro de la transacción actual.
    QString errorAuditoria;
    if (!registrarReintegros(*venta, &errorAuditoria)) {
        avisarFalloAuditoria(errorAuditoria);
    }
    return true;
}

bool ServicioVentas::registrarReintegros(const DatosVentaTicket& venta, QString* error) {
    if (error) error->clear();
    const QDateTime fecha = venta.getFechaAnulacion().isValid()
                                ? venta.getFechaAnulacion()
                                : QDateTime::currentDateTime();

    for (const auto& item : venta.getItems()) {
        RegistroAuditoriaProducto registro;
        registro.fechaHora = fecha.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        registro.accion = Auditoria::aCadena(Auditoria::Accion::Reintegro);
        registro.nombreProducto = QStringLiteral("%1 (%2)")
                                      .arg(item.getNombreProducto(),
                                           item.getIdProducto().isEmpty()
                                               ? QStringLiteral("sin-id")
                                               : item.getIdProducto());
        registro.detalleCambio =
            QStringLiteral("Venta %1 anulada por %2: +%3 unidades devueltas")
                .arg(venta.getIdVenta(), venta.getUsuarioAnulacion())
                .arg(item.getCantidad());
        registro.usuarioResponsable =
            QStringLiteral("%1 (Administrador)").arg(venta.getUsuarioAnulacion());

        gestor.limpiarError();
        if (!gestor.registrarCambioProducto(registro)) {
            establecerError(error, gestor.ultimoError().isEmpty()
                                       ? QStringLiteral("No se pudo registrar el reintegro en auditoría.")
                                       : gestor.ultimoError());
            return false;
        }
    }
    return true;
}

void ServicioVentas::avisarFalloAuditoria(const QString& errorAuditoria) {
    ultimaAdvertencia_ =
        QStringLiteral("La venta se anuló, pero no se pudo registrar el reintegro en auditoría: %1")
            .arg(errorAuditoria);
}
