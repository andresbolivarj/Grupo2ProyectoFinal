#ifndef VENTANAPRINCIPAL_H
#define VENTANAPRINCIPAL_H

#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <memory>
#include <vector>
#include "Usuario.h"

class VentanaPrincipal : public QMainWindow {
    Q_OBJECT

private:
    std::shared_ptr<Usuario> usuarioActual;
    QStackedWidget* panelCentral = nullptr;

    QPushButton* btnVentas = nullptr;
    QPushButton* btnInventario = nullptr;
    QPushButton* btnHistorial = nullptr;
    QPushButton* btnUsuarios = nullptr;
    QPushButton* btnCerrarSesion = nullptr;
    std::vector<QPushButton*> botonesMenu;
    bool cierrePorSesion = false;

public:
    explicit VentanaPrincipal(std::shared_ptr<Usuario> usuario, QWidget* parent = nullptr);

    // Permite que main vuelva a mostrar el login tras "Cerrar Sesión".
    bool cerradaPorSesion() const { return cierrePorSesion; }

private:
    void configurarInterfaz();
    void aplicarPermisosRol();
    QPushButton* crearBotonMenu(const QString& texto);
    void marcarBotonActivo(QPushButton* boton);
    void aplicarEstiloMenu(QPushButton* boton, bool activo);
};

#endif // VENTANAPRINCIPAL_H
