#ifndef VISTAINVENTARIO_H
#define VISTAINVENTARIO_H

#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <vector>
#include "ModelosDatos.h"
#include "Usuario.h"

class VistaInventario : public QWidget {
    Q_OBJECT

private:
    QLineEdit* txtBuscar = nullptr;
    QComboBox* comboCategoriaFiltro = nullptr;
    QTableWidget* tablaProductos = nullptr;
    QTableWidget* tablaAuditoria = nullptr;
    QLineEdit* txtBuscarAuditoria = nullptr;
    QTabWidget* pestanas = nullptr;
    QWidget* paginaInventario = nullptr;
    std::shared_ptr<Usuario> usuarioActual;

    QPushButton* btnAgregar = nullptr;
    QPushButton* btnEditar = nullptr;
    QPushButton* btnDesactivar = nullptr;
    QPushButton* btnReactivar = nullptr;

    std::vector<ProductoItem> listaProductos;

    bool esAdministrador() const;

public:
    explicit VistaInventario(QWidget* parent = nullptr);
    VistaInventario(std::shared_ptr<Usuario> usuario, QWidget* parent);
    void actualizar();

signals:
    // Se emite después de cualquier alta, edición, desactivación o
    // reactivación para que las demás vistas recarguen desde el JSON.
    void inventarioModificado();

protected:
    void showEvent(QShowEvent* event) override;

private:
    void configurarInterfaz();
    void cargarDatos();
    void renderizarTabla();

    void abrirFormularioProducto(ProductoItem* productoExistente = nullptr);
    void desactivarProductoSeleccionado();
    void reactivarProductoSeleccionado();
    void filtrarTabla();
    void cargarAuditoria();
    void filtrarAuditoria();
};

#endif // VISTAINVENTARIO_H
