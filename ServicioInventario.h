#ifndef SERVICIOINVENTARIO_H
#define SERVICIOINVENTARIO_H

#include <QString>
#include <memory>
#include <vector>
#include "CategoriasProducto.h"
#include "GestorJSON.h"
#include "ModelosDatos.h"
#include "Usuario.h"

// Reglas de negocio del inventario: validación, CRUD lógico y auditoría.
class ServicioInventario {
private:
    GestorJSON gestor;

    bool guardarLista(const std::vector<ProductoItem>& productos, QString* error);
    bool registrarAuditoria(Auditoria::Accion accion, const ProductoItem& producto,
                            const QString& detalle,
                            const std::shared_ptr<Usuario>& responsable,
                            QString* error = nullptr);
    static bool esAdministrador(const std::shared_ptr<Usuario>& usuario);
    void avisarFalloAuditoria(const QString& errorAuditoria);
    QString ultimaAdvertencia_;

public:
    explicit ServicioInventario(const QString& rutaProductos = QString(),
                                const QString& rutaAuditoria = QString());

    std::vector<ProductoItem> listar(QString* error = nullptr);
    std::vector<ProductoItem> listarActivos(QString* error = nullptr);
    std::vector<RegistroAuditoriaProducto> listarAuditoria(QString* error = nullptr);
    bool asegurarInventarioInicial(QString* error = nullptr);

    // Advertencia no bloqueante: p. ej. fallo al escribir la auditoría.
    const QString& ultimaAdvertencia() const { return ultimaAdvertencia_; }

    bool crear(ProductoItem producto, const std::shared_ptr<Usuario>& responsable,
               QString* error = nullptr);
    bool actualizar(ProductoItem producto, const std::shared_ptr<Usuario>& responsable,
                    QString* error = nullptr);
    bool desactivar(const QString& id, const std::shared_ptr<Usuario>& responsable,
                    QString* error = nullptr);
    bool reactivar(const QString& id, const std::shared_ptr<Usuario>& responsable,
                   QString* error = nullptr);

    static bool validarProducto(const ProductoItem& producto, QString* error = nullptr);
};

#endif // SERVICIOINVENTARIO_H
