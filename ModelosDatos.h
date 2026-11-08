#ifndef MODELOSDATOS_H
#define MODELOSDATOS_H

#include <QDateTime>
#include <QString>
#include <vector>

// Modelos compartidos por las vistas y la persistencia JSON.
struct ProductoItem {
    QString id;
    QString nombre;
    double precio = 0.0;
    QString descripcion;
    QString categoria;
    int stock = 0;
};

struct ItemCarrito {
    ProductoItem producto;
    int cantidad = 0;
};

struct ItemTicket {
    QString nombreProducto;
    int cantidad = 0;
    double precioUnitario = 0.0;
    double subtotal = 0.0;
};

struct DatosVentaTicket {
    QString idVenta;
    QString usuario;
    QDateTime fechaHora;
    std::vector<ItemTicket> items;
    double subtotal = 0.0;
    double iva = 0.0;
    double total = 0.0;
    double pagaCon = 0.0;
    double cambio = 0.0;
};

#endif // MODELOSDATOS_H
