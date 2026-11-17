#ifndef VISTAUSUARIOS_H
#define VISTAUSUARIOS_H

#include <QComboBox>
#include <QDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <vector>
#include "Usuario.h"

class VistaUsuarios : public QWidget {
    Q_OBJECT

private:
    QLineEdit* txtBuscar = nullptr;
    QTableWidget* tablaUsuarios = nullptr;
    QPushButton* btnAgregar = nullptr;
    QPushButton* btnEditar = nullptr;
    QPushButton* btnDesactivar = nullptr;
    QPushButton* btnReactivar = nullptr;

    std::vector<std::shared_ptr<Usuario>> listaUsuarios;
    std::shared_ptr<Usuario> usuarioActual;

    bool esAdministrador() const;

public:
    explicit VistaUsuarios(QWidget* parent = nullptr);
    VistaUsuarios(std::shared_ptr<Usuario> usuario, QWidget* parent);
    void actualizar();

protected:
    void showEvent(QShowEvent* event) override;

private:
    void configurarInterfaz();
    void cargarDatos();
    void renderizarTabla();
    void filtrarTabla();
    void abrirFormularioUsuario(std::shared_ptr<Usuario> usuarioExistente = nullptr);
    void desactivarUsuarioSeleccionado();
    void reactivarUsuarioSeleccionado();
};

#endif // VISTAUSUARIOS_H
