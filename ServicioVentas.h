#ifndef SERVICIOVENTAS_H
#define SERVICIOVENTAS_H

#include <QString>
#include <memory>
#include <vector>
#include "CategoriasProducto.h"
#include "GestorJSON.h"
#include "ModelosDatos.h"
#include "Usuario.h"

// Reglas de negocio de la venta: validación contra inventario vigente,
// cálculo centralizado, rebaja única de stock y anulación lógica.
class ServicioVentas {
private:
    GestorJSON gestor;

    static bool esAdministrador(const std::shared_ptr<Usuario>& usuario);
    bool registrarReintegros(const DatosVentaTicket& venta, QString* error = nullptr);
    void avisarFalloAuditoria(const QString& errorAuditoria);
    QString ultimaAdvertencia_;

public:
    explicit ServicioVentas(const QString& rutaProductos = QString(),
                            const QString& rutaVentas = QString(),
                            const QString& rutaAuditoria = QString());

    std::vector<DatosVentaTicket> listar(QString* error = nullptr);

    // El consecutivo se genera dentro de registrar(), bajo el bloqueo y dentro
    // de la misma transacción. Este método queda solo por compatibilidad:
    // no "reserva" el ID, por lo que usarlo para armar ventas a mano puede
    // duplicarlo o saltarse la rebaja de inventario.
    [[deprecated("Use registrar(): el ID se genera dentro de la transacción")]]
    QString siguienteIdVenta(QString* error = nullptr);

    // Advertencia no bloqueante: fallo al registrar el reintegro en auditoría.
    const QString& ultimaAdvertencia() const { return ultimaAdvertencia_; }

    bool registrar(const std::vector<ItemCarrito>& carrito,
                   const QString& usuario,
                   int tipoDescuento,
                   double valorDescuento,
                   double pagaCon,
                   DatosVentaTicket* ventaGenerada = nullptr,
                   QString* error = nullptr);

    bool anular(const QString& idVenta,
                const std::shared_ptr<Usuario>& solicitante,
                QString* error = nullptr);
};

#endif // SERVICIOVENTAS_H
