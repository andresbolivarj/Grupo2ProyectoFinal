#ifndef SERVICIOUSUARIOS_H
#define SERVICIOUSUARIOS_H

#include <QString>
#include <memory>
#include <vector>
#include "GestorJSON.h"
#include "Usuario.h"

// Reglas de negocio de usuarios: validación, alta, edición, activación y
// desactivación lógica. La vista no accede al archivo JSON directamente.
class ServicioUsuarios {
private:
    GestorJSON gestor;

    static bool esAdministrador(const std::shared_ptr<Usuario>& solicitante);

public:
    explicit ServicioUsuarios(const QString& rutaUsuarios = QString());

    std::vector<std::shared_ptr<Usuario>> listar(QString* error = nullptr);
    bool guardar(const std::vector<std::shared_ptr<Usuario>>& usuarios, QString* error = nullptr);
    bool asegurarUsuariosIniciales(QString* error = nullptr);

    bool crear(const QString& usuario, const QString& clave, const QString& rol,
               const std::shared_ptr<Usuario>& solicitante, QString* error = nullptr);
    bool actualizar(const QString& usuario, const QString& claveNueva, const QString& rol,
                    bool activo, const std::shared_ptr<Usuario>& solicitante,
                    QString* error = nullptr);
    bool desactivar(const QString& usuario, const std::shared_ptr<Usuario>& solicitante,
                    QString* error = nullptr);
    bool reactivar(const QString& usuario, const std::shared_ptr<Usuario>& solicitante,
                   QString* error = nullptr);

    static bool validarNombreUsuario(const QString& usuario, QString* error = nullptr);
    static bool validarClave(const QString& clave, QString* error = nullptr);
};

#endif // SERVICIOUSUARIOS_H
