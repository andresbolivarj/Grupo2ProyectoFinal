#ifndef SERVICIOAUTENTICACION_H
#define SERVICIOAUTENTICACION_H

#include <QString>
#include <memory>
#include "ServicioUsuarios.h"
#include "Usuario.h"

// Autenticación y migración transparente de contraseñas heredadas a PBKDF2.
class ServicioAutenticacion {
private:
    ServicioUsuarios usuarios;
    QString ultimaAdvertencia_;

public:
    explicit ServicioAutenticacion(const QString& rutaUsuarios = QString());

    std::shared_ptr<Usuario> autenticar(const QString& nombreUsuario,
                                        const QString& clave,
                                        QString* error = nullptr);

    // Fallo no bloqueante al migrar una contraseña heredada a PBKDF2.
    const QString& ultimaAdvertencia() const { return ultimaAdvertencia_; }
};

#endif // SERVICIOAUTENTICACION_H
