#ifndef CALCULADORAVENTA_H
#define CALCULADORAVENTA_H

#include <QString>
#include <vector>
#include "ModelosDatos.h"

struct ResultadoVenta {
    double precioBruto = 0.0;
    double descuento = 0.0;
    double subtotal = 0.0;
    double iva = 0.0;
    double total = 0.0;
    QString error;

    bool valido() const { return error.isEmpty(); }
};

// Única fuente de verdad para los importes de una venta.
class CalculadoraVenta {
public:
    static constexpr int DescuentoMonto = 0;
    static constexpr int DescuentoPorcentaje = 1;
    static constexpr double TasaIVA = 0.13;

    static ResultadoVenta calcular(const std::vector<ItemCarrito>& carrito,
                                   int tipoDescuento,
                                   double valorDescuento);
    static ResultadoVenta calcularDesdeBruto(double precioBruto,
                                             int tipoDescuento,
                                             double valorDescuento);
    static double calcularCambio(double pagaCon, double total);
    static double redondear(double valor);
};

#endif // CALCULADORAVENTA_H
