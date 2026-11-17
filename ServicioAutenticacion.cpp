#include "ServicioAutenticacion.h"

#include <QDebug>

namespace {
void establecerError(QString* destino, const QString& mensaje) {
    if (destino) *destino = mensaje;
}
}

ServicioAutenticacion::ServicioAutenticacion(const QString& rutaUsuarios)
    : usuarios(rutaUsuarios) {}

std::shared_ptr<Usuario> ServicioAutenticacion::autenticar(const QString& nombreUsuario,
                                                           const QString& clave,
                                                           QString* error) {
    if (error) error->clear();
    ultimaAdvertencia_.clear();

    if (nombreUsuario.trimmed().isEmpty() || clave.isEmpty()) {
        establecerError(error, QStringLiteral("Ingrese su usuario y contraseña."));
        return nullptr;
    }

    QString errorInicializacion;
    if (!usuarios.asegurarUsuariosIniciales(&errorInicializacion)) {
        qWarning().noquote() << QStringLiteral("LaPulpe: error al inicializar usuarios: %1")
                                    .arg(errorInicializacion);
        establecerError(error, errorInicializacion);
        return nullptr;
    }

    QString errorLectura;
    auto lista = usuarios.listar(&errorLectura);
    if (!errorLectura.isEmpty()) {
        qWarning().noquote() << QStringLiteral("LaPulpe: error al leer usuarios.json: %1")
                                    .arg(errorLectura);
        establecerError(error, errorLectura);
        return nullptr;
    }

    const QString buscado = nombreUsuario.trimmed();
    for (const auto& usuario : lista) {
        if (!usuario) continue;
        if (usuario->getNombreUsuario().compare(buscado, Qt::CaseInsensitive) != 0) continue;

        // No se revela si el usuario existe, está inactivo o la clave es incorrecta.
        if (!usuario->isActivo() || !usuario->verificarClave(clave)) {
            establecerError(error, QStringLiteral("Usuario o contraseña incorrectos."));
            return nullptr;
        }

        // Migración transparente de contraseñas heredadas (texto plano o
        // SHA-256 hexadecimal) a PBKDF2 tras un login exitoso.
        if (!Usuario::esHashPbkdf2(usuario->getHashContrasena())) {
            const QString hashHeredado = usuario->getHashContrasena();
            usuario->setHashContrasena(Usuario::hashearClave(clave));

            QString errorMigracion;
            if (!usuarios.guardar(lista, &errorMigracion)) {
                // Se conserva el valor heredado en memoria para no dejar el
                // objeto inconsistente con lo que quedó en el archivo.
                usuario->setHashContrasena(hashHeredado);
                ultimaAdvertencia_ =
                    QStringLiteral("La contraseña es válida, pero no se pudo migrar a PBKDF2: %1")
                        .arg(errorMigracion.isEmpty()
                                 ? QStringLiteral("error desconocido al guardar usuarios.json")
                                 : errorMigracion);
                qWarning().noquote() << ultimaAdvertencia_;
                // No se bloquea el acceso: la migración se reintentará en el
                // próximo inicio de sesión.
            }
        }

        return usuario;
    }

    establecerError(error, QStringLiteral("Usuario o contraseña incorrectos."));
    return nullptr;
}
