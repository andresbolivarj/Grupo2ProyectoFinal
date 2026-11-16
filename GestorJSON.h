#ifndef GESTORJSON_H
#define GESTORJSON_H

#include <QString>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>
#include "ModelosDatos.h"
#include "Usuario.h"

class GestorJSON {
private:
    QString rutaProductos;
    QString rutaVentas;
    QString rutaAuditoriaProductos;

public:
    explicit GestorJSON(const QString& rutaProds = QString(), const QString& rutaVnts = QString());
    GestorJSON(const QString& rutaProds, const QString& rutaVnts, const QString& rutaAuditoria);

    // Productos
    std::vector<ProductoItem> cargarProductos();
    bool guardarProductos(const std::vector<ProductoItem>& productos);

    // Ventas
    std::vector<DatosVentaTicket> cargarVentas();
    bool guardarVentas(const std::vector<DatosVentaTicket>& ventas);
    bool registrarVenta(const DatosVentaTicket& venta);
    QString siguienteIdVenta();

    // Auditoría de productos
    std::vector<RegistroAuditoriaProducto> cargarAuditoriaProductos();
    bool registrarCambioProducto(const RegistroAuditoriaProducto& registro);

    // Usuarios
    std::vector<std::shared_ptr<Usuario>> cargarUsuarios();
    bool guardarUsuarios(const std::vector<std::shared_ptr<Usuario>>& usuarios);
};

#endif // GESTORJSON_H
