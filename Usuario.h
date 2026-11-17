#ifndef USUARIO_H
#define USUARIO_H

#include <QString>
#include <QByteArray>
#include <QCryptographicHash>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QStringList>

// Entidad de dominio con estado encapsulado. La contraseña nunca se expone
// en texto plano: se guarda como hash PBKDF2-SHA256 (o el valor heredado que
// ya exista en el archivo de datos).
class Usuario {
private:
    QString id;
    QString nombre;
    QString nombreUsuario;
    QString hashContrasena;
    bool activo;

    static bool compararSeguro(const QByteArray& a, const QByteArray& b) {
        if (a.size() != b.size()) return false;
        unsigned char diferencia = 0;
        for (qsizetype i = 0; i < a.size(); ++i) {
            diferencia |= static_cast<unsigned char>(a.at(i) ^ b.at(i));
        }
        return diferencia == 0;
    }

protected:
    Usuario(const QString& id, const QString& nombre, const QString& nombreUsuario,
            const QString& hashContrasena, bool activo = true)
        : id(id), nombre(nombre), nombreUsuario(nombreUsuario),
          hashContrasena(hashContrasena), activo(activo) {}

    // Constructor de compatibilidad para datos heredados.
    Usuario(const QString& usuario, const QString& secreto)
        : id(usuario), nombre(usuario), nombreUsuario(usuario),
          hashContrasena(secreto), activo(true) {}

public:
    virtual ~Usuario() = default;

    const QString& getId() const { return id; }
    const QString& getNombre() const { return nombre; }
    const QString& getNombreUsuario() const { return nombreUsuario; }
    const QString& getHashContrasena() const { return hashContrasena; }
    bool isActivo() const { return activo; }

    void setNombre(const QString& nuevoNombre) { nombre = nuevoNombre; }
    void setActivo(bool estado) { activo = estado; }
    void setHashContrasena(const QString& nuevoHash) { hashContrasena = nuevoHash; }

    virtual QString rol() const = 0;

    static bool esHashPbkdf2(const QString& valor) {
        return valor.startsWith(QStringLiteral("pbkdf2$"));
    }

    // Hash heredado SHA-256 en hexadecimal (64 caracteres). Se distingue del
    // texto plano para no compararlo como contraseña literal.
    static bool esHashSha256Hex(const QString& valor) {
        if (valor.size() != 64) return false;
        for (const QChar caracter : valor) {
            const ushort codigo = caracter.unicode();
            const bool esHex = (codigo >= '0' && codigo <= '9')
                               || (codigo >= 'a' && codigo <= 'f')
                               || (codigo >= 'A' && codigo <= 'F');
            if (!esHex) return false;
        }
        return true;
    }

    // Genera pbkdf2$iteraciones$salHex$hashHex con PBKDF2-SHA256.
    static QString hashearClave(const QString& clave) {
        constexpr int iteraciones = 120000;
        QByteArray sal(16, '\0');
        for (int i = 0; i < sal.size(); ++i) {
            sal[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
        }
        const QByteArray derivada = QPasswordDigestor::deriveKeyPbkdf2(
            QCryptographicHash::Sha256, clave.toUtf8(), sal, iteraciones, 32);
        return QStringLiteral("pbkdf2$%1$%2$%3")
            .arg(iteraciones)
            .arg(QString::fromLatin1(sal.toHex()))
            .arg(QString::fromLatin1(derivada.toHex()));
    }

    // Acepta PBKDF2, hashes SHA-256 heredados y contraseñas heredadas en texto plano.
    bool verificarClave(const QString& clave) const {
        if (hashContrasena.isEmpty() || clave.isEmpty()) return false;

        if (!esHashPbkdf2(hashContrasena)) {
            if (esHashSha256Hex(hashContrasena)) {
                const QByteArray esperado = QByteArray::fromHex(hashContrasena.toLatin1());
                const QByteArray obtenido = QCryptographicHash::hash(
                    clave.toUtf8(), QCryptographicHash::Sha256);
                return compararSeguro(obtenido, esperado);
            }
            return compararSeguro(hashContrasena.toUtf8(), clave.toUtf8());
        }

        const QStringList partes = hashContrasena.split(QLatin1Char('$'));
        if (partes.size() != 4) return false;

        bool conversionValida = false;
        const int iteraciones = partes.at(1).toInt(&conversionValida);
        if (!conversionValida || iteraciones < 10000 || iteraciones > 2000000) {
            return false;
        }

        const QByteArray sal = QByteArray::fromHex(partes.at(2).toLatin1());
        const QByteArray esperado = QByteArray::fromHex(partes.at(3).toLatin1());
        if (sal.isEmpty() || esperado.isEmpty()) return false;

        const QByteArray derivada = QPasswordDigestor::deriveKeyPbkdf2(
            QCryptographicHash::Sha256, clave.toUtf8(), sal, iteraciones, esperado.size());
        return compararSeguro(derivada, esperado);
    }
};

class Administrador : public Usuario {
public:
    Administrador(const QString& id, const QString& nombre, const QString& nombreUsuario,
                  const QString& hashContrasena, bool activo = true)
        : Usuario(id, nombre, nombreUsuario, hashContrasena, activo) {}

    Administrador(const QString& usuario, const QString& secreto)
        : Usuario(usuario, secreto) {}

    QString rol() const override { return QStringLiteral("Administrador"); }
};

class Cajero : public Usuario {
public:
    Cajero(const QString& id, const QString& nombre, const QString& nombreUsuario,
           const QString& hashContrasena, bool activo = true)
        : Usuario(id, nombre, nombreUsuario, hashContrasena, activo) {}

    Cajero(const QString& usuario, const QString& secreto)
        : Usuario(usuario, secreto) {}

    QString rol() const override { return QStringLiteral("Cajero"); }
};

#endif // USUARIO_H
