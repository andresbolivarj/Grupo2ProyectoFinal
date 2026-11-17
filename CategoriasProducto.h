#ifndef CATEGORIASPRODUCTO_H
#define CATEGORIASPRODUCTO_H

#include <QString>
#include <QStringList>

// Único lugar donde viven los literales de categorías y las acciones de
// auditoría. QStringLiteral evita conversiones y centralized el texto visible.
namespace Categorias {
inline const QString Todos = QStringLiteral("Todos");
inline const QString Snacks = QStringLiteral("Snacks");
inline const QString Bebidas = QStringLiteral("Bebidas");
inline const QString Comida = QStringLiteral("Comida");
inline const QString Utiles = QStringLiteral("Útiles");
inline const QString Higiene = QStringLiteral("Higiene");

inline QStringList lista() {
    return {Snacks, Bebidas, Comida, Utiles, Higiene};
}
} // namespace Categorias

namespace Auditoria {
enum class Accion {
    Anadido,
    Modificado,
    Desactivado,
    Reactivado,
    Reintegro
};

inline const QString kAnadido = QStringLiteral("AÑADIDO");
inline const QString kModificado = QStringLiteral("MODIFICADO");
inline const QString kDesactivado = QStringLiteral("DESACTIVADO");
inline const QString kReactivado = QStringLiteral("REACTIVADO");
inline const QString kReintegro = QStringLiteral("REINTEGRO");
inline const QString kDesconocida = QStringLiteral("DESCONOCIDA");

inline const QString& aCadena(Accion accion) {
    switch (accion) {
    case Accion::Anadido:     return kAnadido;
    case Accion::Modificado:  return kModificado;
    case Accion::Desactivado: return kDesactivado;
    case Accion::Reactivado:  return kReactivado;
    case Accion::Reintegro:   return kReintegro;
    }
    return kDesconocida;
}
} // namespace Auditoria

inline QStringList categoriasProductoPredefinidas() {
    return Categorias::lista();
}

#endif // CATEGORIASPRODUCTO_H
