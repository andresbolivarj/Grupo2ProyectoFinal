#include "ServicioUsuarios.h"

#include <QDebug>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QUuid>
#include <algorithm>

namespace {
void establecerError(QString* destino, const QString& mensaje) {
    if (destino) *destino = mensaje;
}

bool esRolValido(const QString& rol) {
    return rol == QStringLiteral("Administrador") || rol == QStringLiteral("Cajero");
}

std::shared_ptr<Usuario> construirUsuario(const QString& rol, const QString& id,
                                          const QString& nombre, const QString& usuario,
                                          const QString& hash, bool activo) {
    if (rol == QStringLiteral("Administrador")) {
        return std::make_shared<Administrador>(id, nombre, usuario, hash, activo);
    }
    return std::make_shared<Cajero>(id, nombre, usuario, hash, activo);
}

int contarAdministradoresActivos(const std::vector<std::shared_ptr<Usuario>>& usuarios) {
    return static_cast<int>(std::count_if(usuarios.begin(), usuarios.end(), [](const auto& u) {
        return u && u->isActivo() && u->rol() == QStringLiteral("Administrador");
    }));
}

bool mismoUsuario(const QString& a, const QString& b) {
    return a.compare(b, Qt::CaseInsensitive) == 0;
}
} // namespace

ServicioUsuarios::ServicioUsuarios(const QString& rutaUsuarios)
    : gestor(QString(), QString(), QString(), rutaUsuarios) {}

bool ServicioUsuarios::esAdministrador(const std::shared_ptr<Usuario>& solicitante) {
    return solicitante && solicitante->isActivo()
           && solicitante->rol() == QStringLiteral("Administrador");
}

std::vector<std::shared_ptr<Usuario>> ServicioUsuarios::listar(QString* error) {
    if (error) error->clear();
    gestor.limpiarError();
    auto usuarios = gestor.cargarUsuarios();
    if (!gestor.ultimoError().isEmpty()) {
        establecerError(error, gestor.ultimoError());
    }
    return usuarios;
}

bool ServicioUsuarios::guardar(const std::vector<std::shared_ptr<Usuario>>& usuarios,
                               QString* error) {
    if (error) error->clear();
    gestor.limpiarError();
    const bool guardado = gestor.guardarUsuarios(usuarios);
    if (!guardado) {
        establecerError(error, gestor.ultimoError().isEmpty()
                                   ? QStringLiteral("No se pudieron guardar los usuarios.")
                                   : gestor.ultimoError());
    }
    return guardado;
}

bool ServicioUsuarios::asegurarUsuariosIniciales(QString* error) {
    QString errorLectura;
    auto usuarios = listar(&errorLectura);
    if (!errorLectura.isEmpty()) {
        establecerError(error, errorLectura);
        return false;
    }
    if (!usuarios.empty()) return true;

    // Las credenciales por defecto solo se crean en el primer arranque (archivo
    // inexistente) o cuando se habilita explícitamente el modo demo/académico.
    // Así no aparecen cuentas conocidas si alguien vacía usuarios.json.
    const QProcessEnvironment entorno = QProcessEnvironment::systemEnvironment();
    const bool modoDemo = entorno.contains(QStringLiteral("LAPULPE_DEMO_MODE"))
                          || entorno.contains(QStringLiteral("LAPULPE_SEED_USERS"));
    if (gestor.existeArchivoUsuarios() && !modoDemo) {
        establecerError(
            error,
            QStringLiteral("No hay usuarios registrados y la creación automática está "
                           "desactivada. Defina LAPULPE_DEMO_MODE=1 para generar las cuentas "
                           "de ejemplo o cree un usuario con un administrador autorizado."));
        return false;
    }

    qWarning().noquote()
        << QStringLiteral("LaPulpe: sembrando credenciales por defecto (admin/admin123, "
                          "cajero1/cajero123). Desactive LAPULPE_DEMO_MODE en producción.");

    usuarios.push_back(std::make_shared<Administrador>(
        QStringLiteral("u-admin"), QStringLiteral("Administrador"),
        QStringLiteral("admin"), Usuario::hashearClave(QStringLiteral("admin123")), true));
    usuarios.push_back(std::make_shared<Cajero>(
        QStringLiteral("u-cajero1"), QStringLiteral("Cajero"),
        QStringLiteral("cajero1"), Usuario::hashearClave(QStringLiteral("cajero123")), true));

    return guardar(usuarios, error);
}

bool ServicioUsuarios::validarNombreUsuario(const QString& usuario, QString* error) {
    if (usuario.trimmed().isEmpty()) {
        establecerError(error, QStringLiteral("El nombre de usuario es obligatorio."));
        return false;
    }
    const QRegularExpression patron(
        QStringLiteral("^[\\p{L}\\p{N}._-]{3,32}$"),
        QRegularExpression::UseUnicodePropertiesOption);
    if (!patron.match(usuario.trimmed()).hasMatch()) {
        establecerError(error,
                        QStringLiteral("El usuario debe tener entre 3 y 32 caracteres y solo "
                                       "puede contener letras, números, punto, guion o guion bajo."));
        return false;
    }
    return true;
}

bool ServicioUsuarios::validarClave(const QString& clave, QString* error) {
    if (clave.size() < 6) {
        establecerError(error, QStringLiteral("La contraseña debe tener al menos 6 caracteres."));
        return false;
    }
    if (clave.size() > 128) {
        establecerError(error, QStringLiteral("La contraseña es demasiado larga."));
        return false;
    }
    return true;
}

bool ServicioUsuarios::crear(const QString& usuario, const QString& clave, const QString& rol,
                             const std::shared_ptr<Usuario>& solicitante, QString* error) {
    if (!esAdministrador(solicitante)) {
        establecerError(error, QStringLiteral("Solo un administrador puede crear usuarios."));
        return false;
    }
    if (!validarNombreUsuario(usuario, error)) return false;
    if (!validarClave(clave, error)) return false;
    if (!esRolValido(rol)) {
        establecerError(error, QStringLiteral("El rol seleccionado no es válido."));
        return false;
    }

    const QString usuarioLimpio = usuario.trimmed();
    QString errorLocal;
    auto usuarios = listar(error ? error : &errorLocal);
    if (!(error ? *error : errorLocal).isEmpty()) return false;

    for (const auto& existente : usuarios) {
        if (mismoUsuario(existente->getNombreUsuario(), usuarioLimpio)) {
            establecerError(error, QStringLiteral("El nombre de usuario ya está registrado."));
            return false;
        }
    }

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    usuarios.push_back(construirUsuario(rol, id, usuarioLimpio, usuarioLimpio,
                                        Usuario::hashearClave(clave), true));
    return guardar(usuarios, error);
}

bool ServicioUsuarios::actualizar(const QString& usuario, const QString& claveNueva,
                                  const QString& rol, bool activo,
                                  const std::shared_ptr<Usuario>& solicitante,
                                  QString* error) {
    if (!esAdministrador(solicitante)) {
        establecerError(error, QStringLiteral("Solo un administrador puede modificar usuarios."));
        return false;
    }
    if (!esRolValido(rol)) {
        establecerError(error, QStringLiteral("El rol seleccionado no es válido."));
        return false;
    }
    if (!claveNueva.isEmpty() && !validarClave(claveNueva, error)) return false;

    QString errorLocal;
    auto usuarios = listar(error ? error : &errorLocal);
    if (!(error ? *error : errorLocal).isEmpty()) return false;

    for (auto& actual : usuarios) {
        if (!mismoUsuario(actual->getNombreUsuario(), usuario)) continue;

        if (mismoUsuario(actual->getNombreUsuario(), solicitante->getNombreUsuario()) && !activo) {
            establecerError(error, QStringLiteral("No puede desactivar su propia cuenta."));
            return false;
        }

        const bool eraAdminActivo = actual->isActivo()
                                    && actual->rol() == QStringLiteral("Administrador");
        const bool seguiraAdminActivo = activo && rol == QStringLiteral("Administrador");
        if (eraAdminActivo && !seguiraAdminActivo && contarAdministradoresActivos(usuarios) <= 1) {
            establecerError(error,
                            QStringLiteral("Debe existir al menos un administrador activo."));
            return false;
        }

        const QString hash = claveNueva.isEmpty() ? actual->getHashContrasena()
                                                  : Usuario::hashearClave(claveNueva);
        actual = construirUsuario(rol, actual->getId(), actual->getNombre(),
                                  actual->getNombreUsuario(), hash, activo);
        return guardar(usuarios, error);
    }

    establecerError(error, QStringLiteral("El usuario indicado no existe."));
    return false;
}

bool ServicioUsuarios::desactivar(const QString& usuario,
                                  const std::shared_ptr<Usuario>& solicitante,
                                  QString* error) {
    if (!esAdministrador(solicitante)) {
        establecerError(error, QStringLiteral("Solo un administrador puede desactivar usuarios."));
        return false;
    }
    if (mismoUsuario(usuario, solicitante->getNombreUsuario())) {
        establecerError(error, QStringLiteral("No puede desactivar su propia cuenta."));
        return false;
    }

    auto usuarios = listar(error);
    if (!(error ? *error : QString()).isEmpty()) return false;

    for (auto& actual : usuarios) {
        if (!mismoUsuario(actual->getNombreUsuario(), usuario)) continue;
        if (!actual->isActivo()) {
            establecerError(error, QStringLiteral("El usuario ya está inactivo."));
            return false;
        }
        if (actual->rol() == QStringLiteral("Administrador")
            && contarAdministradoresActivos(usuarios) <= 1) {
            establecerError(error,
                            QStringLiteral("Debe existir al menos un administrador activo."));
            return false;
        }
        actual = construirUsuario(actual->rol(), actual->getId(), actual->getNombre(),
                                  actual->getNombreUsuario(), actual->getHashContrasena(), false);
        return guardar(usuarios, error);
    }

    establecerError(error, QStringLiteral("El usuario indicado no existe."));
    return false;
}

bool ServicioUsuarios::reactivar(const QString& usuario,
                                 const std::shared_ptr<Usuario>& solicitante,
                                 QString* error) {
    if (!esAdministrador(solicitante)) {
        establecerError(error, QStringLiteral("Solo un administrador puede reactivar usuarios."));
        return false;
    }

    auto usuarios = listar(error);
    if (!(error ? *error : QString()).isEmpty()) return false;

    for (auto& actual : usuarios) {
        if (!mismoUsuario(actual->getNombreUsuario(), usuario)) continue;
        if (actual->isActivo()) {
            establecerError(error, QStringLiteral("El usuario ya está activo."));
            return false;
        }
        actual = construirUsuario(actual->rol(), actual->getId(), actual->getNombre(),
                                  actual->getNombreUsuario(), actual->getHashContrasena(), true);
        return guardar(usuarios, error);
    }

    establecerError(error, QStringLiteral("El usuario indicado no existe."));
    return false;
}
