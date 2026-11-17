#include "ServicioInventario.h"

#include "CategoriasProducto.h"

#include <QDateTime>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {
void establecerError(QString* destino, const QString& mensaje) {
    if (destino) *destino = mensaje;
}

QStringList cambiosEntre(const ProductoItem& anterior, const ProductoItem& actual) {
    QStringList cambios;
    if (anterior.getNombre() != actual.getNombre()) {
        cambios << QStringLiteral("Nombre: %1 -> %2")
                       .arg(anterior.getNombre(), actual.getNombre());
    }
    if (anterior.getPrecio() != actual.getPrecio()) {
        cambios << QStringLiteral("Precio: %1 -> %2")
                       .arg(anterior.getPrecio(), 0, 'f', 2)
                       .arg(actual.getPrecio(), 0, 'f', 2);
    }
    if (anterior.getStock() != actual.getStock()) {
        cambios << QStringLiteral("Stock: %1 -> %2")
                       .arg(anterior.getStock()).arg(actual.getStock());
    }
    if (anterior.getDescripcion() != actual.getDescripcion()) {
        cambios << QStringLiteral("Descripción actualizada");
    }
    if (anterior.getCategoria() != actual.getCategoria()) {
        cambios << QStringLiteral("Categoría: %1 -> %2")
                       .arg(anterior.getCategoria(), actual.getCategoria());
    }
    if (anterior.isActivo() != actual.isActivo()) {
        cambios << QStringLiteral("Estado: %1 -> %2")
                       .arg(anterior.isActivo() ? QStringLiteral("Activo")
                                                : QStringLiteral("Inactivo"),
                            actual.isActivo() ? QStringLiteral("Activo")
                                              : QStringLiteral("Inactivo"));
    }
    return cambios;
}
} // namespace

ServicioInventario::ServicioInventario(const QString& rutaProductos,
                                       const QString& rutaAuditoria)
    : gestor(rutaProductos, QString(), rutaAuditoria) {}

bool ServicioInventario::esAdministrador(const std::shared_ptr<Usuario>& usuario) {
    return usuario && usuario->isActivo()
           && usuario->rol() == QStringLiteral("Administrador");
}

std::vector<ProductoItem> ServicioInventario::listar(QString* error) {
    if (error) error->clear();
    gestor.limpiarError();
    auto productos = gestor.cargarProductos();
    if (!gestor.ultimoError().isEmpty()) {
        establecerError(error, gestor.ultimoError());
    }
    return productos;
}

std::vector<ProductoItem> ServicioInventario::listarActivos(QString* error) {
    auto productos = listar(error);
    productos.erase(std::remove_if(productos.begin(), productos.end(),
                                   [](const ProductoItem& producto) {
                                       return !producto.isActivo();
                                   }),
                    productos.end());
    return productos;
}

std::vector<RegistroAuditoriaProducto> ServicioInventario::listarAuditoria(QString* error) {
    if (error) error->clear();
    gestor.limpiarError();
    auto registros = gestor.cargarAuditoriaProductos();
    if (!gestor.ultimoError().isEmpty()) {
        establecerError(error, gestor.ultimoError());
    }
    return registros;
}

bool ServicioInventario::guardarLista(const std::vector<ProductoItem>& productos,
                                      QString* error) {
    gestor.limpiarError();
    if (!gestor.guardarProductos(productos)) {
        establecerError(error, gestor.ultimoError().isEmpty()
                                   ? QStringLiteral("No se pudo guardar el inventario.")
                                   : gestor.ultimoError());
        return false;
    }
    return true;
}

bool ServicioInventario::validarProducto(const ProductoItem& producto, QString* error) {
    const QString id = producto.getId().trimmed();
    const QRegularExpression patronId(
        QStringLiteral("^[\\p{L}\\p{N}_-]{1,20}$"),
        QRegularExpression::UseUnicodePropertiesOption);
    if (!patronId.match(id).hasMatch()) {
        establecerError(error,
                        QStringLiteral("El código ID es obligatorio y solo puede contener letras, "
                                       "números, guion o guion bajo (máximo 20)."));
        return false;
    }
    if (producto.getNombre().trimmed().isEmpty()
        || producto.getNombre().trimmed().size() > 80) {
        establecerError(error,
                        QStringLiteral("El nombre es obligatorio y debe tener máximo 80 caracteres."));
        return false;
    }
    if (producto.getDescripcion().trimmed().size() > 200) {
        establecerError(error, QStringLiteral("La descripción no puede superar 200 caracteres."));
        return false;
    }
    if (!categoriasProductoPredefinidas().contains(producto.getCategoria())) {
        establecerError(error, QStringLiteral("Seleccione una categoría válida."));
        return false;
    }
    if (!std::isfinite(producto.getPrecio()) || producto.getPrecio() <= 0.0
        || producto.getPrecio() > 10000000.0) {
        establecerError(error,
                        QStringLiteral("El precio debe ser mayor que cero y menor o igual a ₡10.000.000."));
        return false;
    }
    if (producto.getStock() < 0 || producto.getStock() > 1000000) {
        establecerError(error,
                        QStringLiteral("El stock debe estar entre 0 y 1.000.000 de unidades."));
        return false;
    }
    return true;
}

bool ServicioInventario::asegurarInventarioInicial(QString* error) {
    QString errorLectura;
    auto productos = listar(&errorLectura);
    if (!errorLectura.isEmpty()) {
        establecerError(error, errorLectura);
        return false;
    }
    if (!productos.empty()) return true;

    productos = {
        {"S001", "Papas Tostadas Tosty", 750.0, "Snack de papa 115g", Categorias::Snacks, 25, true},
        {"S002", "Platanitos Salados", 600.0, "Plátano tostado 100g", Categorias::Snacks, 15, true},
        {"S003", "Doritos Queso Nacho", 850.0, "Snack de maíz 120g", Categorias::Snacks, 8, true},
        {"S004", "Chocolate Jet", 350.0, "Barra de chocolate 12g", Categorias::Snacks, 30, true},
        {"S005", "Galletas Chicky", 450.0, "Galletas con cobertura", Categorias::Snacks, 12, true},
        {"B001", "Agua Cristal 600ml", 600.0, "Agua embotellada", Categorias::Bebidas, 50, true},
        {"B002", "Café Caliente 8oz", 700.0, "Café negro / con leche", Categorias::Bebidas, 40, true},
        {"B003", "Coca-Cola 600ml", 1100.0, "Gaseosa en botella", Categorias::Bebidas, 18, true},
        {"B005", "Fresco Leche Fresa", 800.0, "Dos Pinos 315ml", Categorias::Bebidas, 10, true},
        {"C001", "Empanada de Carne", 1200.0, "Arreglada y caliente", Categorias::Comida, 5, true},
        {"C002", "Empanada de Queso", 1100.0, "Arreglada y caliente", Categorias::Comida, 7, true},
        {"C003", "Muffin de Chispas", 950.0, "Repostería dulce", Categorias::Comida, 9, true},
        {"U001", "Lapicero BIC Negro", 300.0, "Punto mediano", Categorias::Utiles, 100, true},
        {"U002", "Lapicero BIC Azul", 300.0, "Punto mediano", Categorias::Utiles, 100, true},
        {"U004", "Cuaderno Universitario", 1850.0, "100 hojas cosido", Categorias::Utiles, 20, true},
        {"H001", "Alcohol en Gel 60ml", 950.0, "Portátil antibacterial", Categorias::Higiene, 14, true},
        {"H002", "Pañuelos Kleenex", 450.0, "Paquete de bolsillo", Categorias::Higiene, 22, true}
    };
    return guardarLista(productos, error);
}

bool ServicioInventario::crear(ProductoItem producto,
                               const std::shared_ptr<Usuario>& responsable,
                               QString* error) {
    ultimaAdvertencia_.clear();
    if (!esAdministrador(responsable)) {
        establecerError(error, QStringLiteral("Solo un administrador puede crear productos."));
        return false;
    }

    producto.setId(producto.getId().trimmed());
    producto.setNombre(producto.getNombre().trimmed());
    producto.setDescripcion(producto.getDescripcion().trimmed());
    producto.setCategoria(producto.getCategoria().trimmed());
    if (!validarProducto(producto, error)) return false;

    auto productos = listar(error);
    if (error && !error->isEmpty()) return false;

    for (const auto& existente : productos) {
        if (existente.getId().compare(producto.getId(), Qt::CaseInsensitive) == 0) {
            establecerError(error,
                            QStringLiteral("Ya existe un producto registrado con este código ID."));
            return false;
        }
    }

    producto.setActivo(true);
    productos.push_back(producto);
    if (!guardarLista(productos, error)) return false;

    QString errorAuditoria;
    if (!registrarAuditoria(Auditoria::Accion::Anadido, producto,
                            QStringLiteral("Nuevo producto registrado"), responsable,
                            &errorAuditoria)) {
        avisarFalloAuditoria(errorAuditoria);
    }
    return true;
}

bool ServicioInventario::actualizar(ProductoItem producto,
                                    const std::shared_ptr<Usuario>& responsable,
                                    QString* error) {
    ultimaAdvertencia_.clear();
    if (!esAdministrador(responsable)) {
        establecerError(error, QStringLiteral("Solo un administrador puede editar productos."));
        return false;
    }

    producto.setId(producto.getId().trimmed());
    producto.setNombre(producto.getNombre().trimmed());
    producto.setDescripcion(producto.getDescripcion().trimmed());
    producto.setCategoria(producto.getCategoria().trimmed());
    if (!validarProducto(producto, error)) return false;

    auto productos = listar(error);
    if (error && !error->isEmpty()) return false;

    for (auto& actual : productos) {
        if (actual.getId().compare(producto.getId(), Qt::CaseInsensitive) != 0) continue;

        const ProductoItem anterior = actual;
        producto.setId(anterior.getId()); // El ID es la clave estable del registro.
        actual = producto;
        if (!guardarLista(productos, error)) return false;

        const QStringList cambios = cambiosEntre(anterior, producto);
        QString errorAuditoria;
        if (!registrarAuditoria(Auditoria::Accion::Modificado, producto,
                                cambios.isEmpty()
                                    ? QStringLiteral("Producto guardado sin cambios en sus campos")
                                    : cambios.join(QStringLiteral(" | ")),
                                responsable, &errorAuditoria)) {
            avisarFalloAuditoria(errorAuditoria);
        }
        return true;
    }

    establecerError(error, QStringLiteral("El producto indicado no existe."));
    return false;
}

bool ServicioInventario::desactivar(const QString& id,
                                    const std::shared_ptr<Usuario>& responsable,
                                    QString* error) {
    ultimaAdvertencia_.clear();
    if (!esAdministrador(responsable)) {
        establecerError(error, QStringLiteral("Solo un administrador puede desactivar productos."));
        return false;
    }

    auto productos = listar(error);
    if (error && !error->isEmpty()) return false;

    for (auto& producto : productos) {
        if (producto.getId().compare(id.trimmed(), Qt::CaseInsensitive) != 0) continue;
        if (!producto.isActivo()) {
            establecerError(error, QStringLiteral("El producto ya está inactivo."));
            return false;
        }
        producto.setActivo(false);
        if (!guardarLista(productos, error)) return false;

        QString errorAuditoria;
        if (!registrarAuditoria(Auditoria::Accion::Desactivado, producto,
                                QStringLiteral("Producto desactivado (borrado lógico); "
                                               "su historial de ventas se conserva"),
                                responsable, &errorAuditoria)) {
            avisarFalloAuditoria(errorAuditoria);
        }
        return true;
    }

    establecerError(error, QStringLiteral("El producto indicado no existe."));
    return false;
}

bool ServicioInventario::reactivar(const QString& id,
                                   const std::shared_ptr<Usuario>& responsable,
                                   QString* error) {
    ultimaAdvertencia_.clear();
    if (!esAdministrador(responsable)) {
        establecerError(error, QStringLiteral("Solo un administrador puede reactivar productos."));
        return false;
    }

    auto productos = listar(error);
    if (error && !error->isEmpty()) return false;

    for (auto& producto : productos) {
        if (producto.getId().compare(id.trimmed(), Qt::CaseInsensitive) != 0) continue;
        if (producto.isActivo()) {
            establecerError(error, QStringLiteral("El producto ya está activo."));
            return false;
        }
        producto.setActivo(true);
        if (!guardarLista(productos, error)) return false;

        QString errorAuditoria;
        if (!registrarAuditoria(Auditoria::Accion::Reactivado, producto,
                                QStringLiteral("Producto reactivado"), responsable,
                                &errorAuditoria)) {
            avisarFalloAuditoria(errorAuditoria);
        }
        return true;
    }

    establecerError(error, QStringLiteral("El producto indicado no existe."));
    return false;
}

bool ServicioInventario::registrarAuditoria(Auditoria::Accion accion,
                                            const ProductoItem& producto,
                                            const QString& detalle,
                                            const std::shared_ptr<Usuario>& responsable,
                                            QString* error) {
    if (error) error->clear();
    if (!responsable) return true;

    RegistroAuditoriaProducto registro;
    registro.fechaHora = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    registro.accion = Auditoria::aCadena(accion);
    registro.nombreProducto = QStringLiteral("%1 (%2)")
                                  .arg(producto.getNombre(), producto.getId());
    registro.detalleCambio = detalle;
    registro.usuarioResponsable = QStringLiteral("%1 (%2)")
                                      .arg(responsable->getNombreUsuario(), responsable->rol());

    gestor.limpiarError();
    if (!gestor.registrarCambioProducto(registro)) {
        establecerError(error, gestor.ultimoError().isEmpty()
                                   ? QStringLiteral("No se pudo escribir la auditoría de productos.")
                                   : gestor.ultimoError());
        return false;
    }
    return true;
}

void ServicioInventario::avisarFalloAuditoria(const QString& errorAuditoria) {
    ultimaAdvertencia_ =
        QStringLiteral("El cambio se guardó, pero no se pudo registrar en la auditoría: %1")
            .arg(errorAuditoria);
}
