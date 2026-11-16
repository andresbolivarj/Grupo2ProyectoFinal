#ifndef VISTAINVENTARIO_H
#define VISTAINVENTARIO_H

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <vector>
#include <memory>
#include "ModelosDatos.h"
#include "GestorJSON.h"
#include "Usuario.h"

class VistaInventario : public QWidget {
    Q_OBJECT

private:
    QLineEdit* txtBuscar;
    QComboBox* comboCategoriaFiltro;
    QTableWidget* tablaProductos;
    QTableWidget* tablaAuditoria = nullptr;
    QLineEdit* txtBuscarAuditoria = nullptr;
    QTabWidget* pestañas;
    QWidget* paginaInventario;
    std::shared_ptr<Usuario> usuarioActual;

    QPushButton* btnAgregar;
    QPushButton* btnEditar;
    QPushButton* btnEliminar;

    std::vector<ProductoItem> listaProductos;

public:
    explicit VistaInventario(QWidget* parent = nullptr);
    VistaInventario(std::shared_ptr<Usuario> usuario, QWidget* parent);
    void actualizar(); // Permite recargar la tabla externamente

protected:
    void showEvent(QShowEvent* event) override; // Se ejecuta automáticamente al abrir la vista

private:
    void configurarInterfaz();
    void cargarDatos();
    void renderizarTabla();

    void abrirFormularioProducto(ProductoItem* productoExistente = nullptr);
    void guardarProductoEnJSON(const ProductoItem& prod, bool esEdicion);
    void eliminarProductoSeleccionado();
    void filtrarTabla();
    void cargarAuditoria();
    void filtrarAuditoria();
    void registrarAuditoria(const QString& accion, const ProductoItem& producto, const QString& detalle);
};

#endif // VISTAINVENTARIO_H
