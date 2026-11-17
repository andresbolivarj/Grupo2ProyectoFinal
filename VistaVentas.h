#ifndef VISTAVENTAS_H
#define VISTAVENTAS_H

#include <QComboBox>
#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>
#include "ModelosDatos.h"

class QButtonGroup;

class VistaVentas : public QWidget {
    Q_OBJECT

private:
    QLineEdit* txtBuscar = nullptr;
    QVBoxLayout* layoutListaCarrito = nullptr;
    QWidget* contenedorCarrito = nullptr;
    QGridLayout* gridProductos = nullptr;
    QButtonGroup* grupoCategorias = nullptr;

    QLabel* lblSubtotal = nullptr;
    QLabel* lblDescuento = nullptr;
    QLabel* lblIVA = nullptr;
    QComboBox* comboTipoDescuento = nullptr;
    QLineEdit* txtDescuento = nullptr;
    QLabel* lblTotal = nullptr;
    QLabel* lblResumenItems = nullptr;

    QLineEdit* txtMontoPaga = nullptr;
    QLabel* lblCambio = nullptr;
    QPushButton* btnCheckout = nullptr;
    QPushButton* btnVaciarCarrito = nullptr;

    std::vector<ProductoItem> catalogo;
    std::vector<ItemCarrito> carrito;
    QString categoriaSeleccionada;
    QString nombreUsuario;

    double subtotalActual = 0.0;
    double ivaActual = 0.0;
    double descuentoActual = 0.0;
    double totalActual = 0.0;

public:
    explicit VistaVentas(const QString& nombreUsuario, QWidget* parent = nullptr);

public slots:
    // Recarga el catálogo desde el JSON y resincroniza el carrito.
    void actualizar();

signals:
    void ventaRegistrada(const QString& idVenta);
    void catalogoActualizado();

private slots:
    void cambiarCategoriaSlot(const QString& categoria);

private:
    void cargarCatalogo();
    void sincronizarCarrito();
    void configurarInterfaz();

    void renderizarProductos();
    void renderizarCarrito();

    QWidget* crearTarjetaProducto(const ProductoItem& producto);
    QWidget* crearWidgetCarritoItem(const ItemCarrito& item, int index);

    void agregarAlCarrito(const ProductoItem& producto);
    void cambiarCantidadItem(int index, int delta);
    void eliminarDelCarrito(int index);
    void vaciarCarrito();
    void actualizarTotales();
    void calcularCambioEfectivo();
    double valorDescuento() const;
    double montoPagado() const;
    void filtrarProductos();
    void procesarCheckout();

    void mostrarYGuardarTicket(const DatosVentaTicket& datosVenta);
};

#endif // VISTAVENTAS_H
