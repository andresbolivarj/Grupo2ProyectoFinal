#include "GestorJSON.h"
#include "IGeneradorTicket.h"
#include "TicketPdf.h"

#include <QFile>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <cstdlib>
#include <iostream>

namespace {
void comprobar(bool condicion, const char* mensaje) {
    if (!condicion) {
        std::cerr << "FALLO: " << mensaje << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QTemporaryDir directorio;
    comprobar(directorio.isValid(), "crear directorio temporal");

    const QString rutaProductos = directorio.filePath("productos.json");
    const QString rutaVentas = directorio.filePath("ventas.json");
    GestorJSON gestor(rutaProductos, rutaVentas);

    ProductoItem producto;
    producto.id = "p-1";
    producto.nombre = "Café";
    producto.precio = 1250.5;
    producto.descripcion = "Molido";
    producto.categoria = "Bebidas";
    producto.stock = 12;

    comprobar(gestor.guardarProductos({producto}), "guardar productos");
    const auto productos = gestor.cargarProductos();
    comprobar(productos.size() == 1, "cargar cantidad de productos");
    comprobar(productos[0].id == producto.id, "conservar id del producto");
    comprobar(productos[0].nombre == producto.nombre, "conservar nombre Unicode");
    comprobar(qAbs(productos[0].precio - producto.precio) < 0.001, "conservar precio");
    comprobar(productos[0].stock == producto.stock, "conservar stock");

    DatosVentaTicket venta;
    venta.idVenta = gestor.siguienteIdVenta();
    venta.usuario = "cajero1";
    venta.fechaHora = QDateTime::fromString("2026-11-08T10:15:00", Qt::ISODate);
    venta.subtotal = 1250.5;
    venta.iva = 162.565;
    venta.total = 1413.065;
    venta.pagaCon = 2000.0;
    venta.cambio = venta.pagaCon - venta.total;
    venta.items.push_back({producto.nombre, 1, producto.precio, producto.precio});

    comprobar(gestor.registrarVenta(venta), "registrar venta");
    const auto ventas = gestor.cargarVentas();
    comprobar(ventas.size() == 1, "cargar cantidad de ventas");
    comprobar(ventas[0].idVenta == venta.idVenta, "conservar id de venta");
    comprobar(ventas[0].usuario == venta.usuario, "conservar usuario que hizo la venta");
    comprobar(ventas[0].fechaHora == venta.fechaHora, "conservar fecha de venta");
    comprobar(ventas[0].items.size() == 1, "cargar artículos de venta");
    comprobar(ventas[0].items[0].nombreProducto == producto.nombre,
              "conservar nombre de artículo");
    comprobar(qAbs(ventas[0].cambio - venta.cambio) < 0.001, "conservar cambio");

    GeneradorTicketTXT generadorTexto;
    GeneradorTicketHTML generadorHtml;
    comprobar(generadorTexto.generarContenido(ventas[0]).contains("Usuario   : cajero1"),
              "mostrar el usuario en el ticket de texto");
    comprobar(generadorHtml.generarContenido(ventas[0]).contains("<b>Usuario:</b> cajero1"),
              "mostrar el usuario en el ticket HTML");
    comprobar(!generadorTexto.generarContenido(ventas[0]).contains("Rol"),
              "no mostrar el rol en el ticket");
    comprobar(gestor.siguienteIdVenta() == "V-1002",
              "generar un ID distinto para la siguiente venta");
    GestorJSON gestorTrasReinicio(rutaProductos, rutaVentas);
    comprobar(gestorTrasReinicio.siguienteIdVenta() == "V-1002",
              "continuar la secuencia de IDs después de reiniciar el gestor");

    const QString rutaTicketPDF = directorio.filePath("ticket.pdf");
    comprobar(generarTicketPDF(ventas[0], rutaTicketPDF), "generar el ticket en PDF");
    QFile archivoPDF(rutaTicketPDF);
    comprobar(archivoPDF.open(QIODevice::ReadOnly), "abrir PDF generado");
    comprobar(archivoPDF.read(4) == "%PDF", "crear un archivo PDF válido");
    archivoPDF.close();

    QFile archivoVentaAnterior(rutaVentas);
    comprobar(archivoVentaAnterior.open(QIODevice::WriteOnly | QIODevice::Truncate),
              "abrir archivo para venta anterior sin rol");
    archivoVentaAnterior.write(
        "[{\"idVenta\":\"V-LEGACY\",\"cajero\":\"cajero-viejo\",\"items\":[]}]");
    archivoVentaAnterior.close();
    const auto ventasAnteriores = gestor.cargarVentas();
    comprobar(ventasAnteriores.size() == 1, "cargar venta anterior al cambio de rol");
    comprobar(ventasAnteriores[0].usuario == "cajero-viejo",
              "leer el usuario de ventas guardadas con el formato anterior");

    QFile archivoMalformado(rutaProductos);
    comprobar(archivoMalformado.open(QIODevice::WriteOnly | QIODevice::Truncate),
              "abrir archivo para prueba de JSON malformado");
    archivoMalformado.write("{json invalido");
    archivoMalformado.close();
    comprobar(gestor.cargarProductos().empty(), "rechazar JSON malformado sin fallar");

    std::cout << "Todas las pruebas de GestorJSON pasaron.\n";
    return EXIT_SUCCESS;
}
