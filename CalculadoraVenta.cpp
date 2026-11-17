#include "CalculadoraVenta.h"

#include <algorithm>
#include <cmath>

namespace {
bool esNumeroValido(double valor) {
    return std::isfinite(valor);
}
}

double CalculadoraVenta::redondear(double valor) {
    return std::round(valor * 100.0) / 100.0;
}

ResultadoVenta CalculadoraVenta::calcular(const std::vector<ItemCarrito>& carrito,
                                          int tipoDescuento,
                                          double valorDescuento) {
    double bruto = 0.0;
    for (const auto& item : carrito) {
        if (item.cantidad <= 0) {
            ResultadoVenta error;
            error.error = QStringLiteral("La cantidad de '%1' debe ser mayor que cero.")
                              .arg(item.producto.getNombre());
            return error;
        }
        if (!esNumeroValido(item.producto.getPrecio()) || item.producto.getPrecio() < 0.0) {
            ResultadoVenta error;
            error.error = QStringLiteral("El precio de '%1' no es válido.")
                              .arg(item.producto.getNombre());
            return error;
        }
        bruto += item.producto.getPrecio() * item.cantidad;
    }
    return calcularDesdeBruto(bruto, tipoDescuento, valorDescuento);
}

ResultadoVenta CalculadoraVenta::calcularDesdeBruto(double precioBruto,
                                                    int tipoDescuento,
                                                    double valorDescuento) {
    ResultadoVenta resultado;

    if (!esNumeroValido(precioBruto) || precioBruto < 0.0) {
        resultado.error = QStringLiteral("El subtotal de la venta no es válido.");
        return resultado;
    }
    if (!esNumeroValido(valorDescuento) || valorDescuento < 0.0) {
        resultado.error = QStringLiteral("El descuento no puede ser negativo.");
        return resultado;
    }
    if (tipoDescuento != DescuentoMonto && tipoDescuento != DescuentoPorcentaje) {
        resultado.error = QStringLiteral("El tipo de descuento seleccionado no es válido.");
        return resultado;
    }

    resultado.precioBruto = redondear(precioBruto);

    double descuento = 0.0;
    if (tipoDescuento == DescuentoPorcentaje) {
        if (valorDescuento > 100.0) {
            resultado.error = QStringLiteral("El descuento porcentual no puede superar el 100%.");
            return resultado;
        }
        descuento = resultado.precioBruto * (valorDescuento / 100.0);
    } else {
        descuento = valorDescuento;
    }

    if (descuento > resultado.precioBruto + 0.001) {
        resultado.error = QStringLiteral("El descuento no puede superar el subtotal de la venta.");
        return resultado;
    }

    resultado.descuento = redondear(std::min(descuento, resultado.precioBruto));
    resultado.total = redondear(resultado.precioBruto - resultado.descuento);

    // Los precios del catálogo incluyen IVA: se desglosa hacia atrás.
    resultado.subtotal = redondear(resultado.total / (1.0 + TasaIVA));
    resultado.iva = redondear(resultado.total - resultado.subtotal);
    return resultado;
}

double CalculadoraVenta::calcularCambio(double pagaCon, double total) {
    if (!esNumeroValido(pagaCon) || !esNumeroValido(total)) return 0.0;
    return redondear(pagaCon - total);
}
