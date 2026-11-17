#include "GestorJSON.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSaveFile>
#include <QStandardPaths>
#include <cmath>

namespace {

QString rutaDatos(const QString& nombreArchivo) {
    QString directorio = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (directorio.isEmpty()) {
        directorio = QDir::home().filePath(QStringLiteral(".una-pulpe"));
    }
    QDir().mkpath(directorio);

    const QString destino = QDir(directorio).filePath(nombreArchivo);
    if (!QFile::exists(destino)) {
        const QStringList candidatos = {
            QDir::current().filePath(nombreArchivo),
            QDir(QCoreApplication::applicationDirPath()).filePath(nombreArchivo)
        };
        for (const QString& origen : candidatos) {
            if (QFileInfo(origen).absoluteFilePath() == QFileInfo(destino).absoluteFilePath()) {
                continue;
            }
            if (QFile::exists(origen) && QFile::copy(origen, destino)) {
                break;
            }
        }
    }
    return destino;
}

bool documentoJsonValido(const QJsonDocument& documento, QString* error) {
    if (documento.isNull() || !documento.isArray()) {
        if (error) *error = QStringLiteral("El documento JSON debe ser un arreglo válido.");
        return false;
    }

    const QByteArray contenido = documento.toJson(QJsonDocument::Compact);
    if (contenido.isEmpty() || contenido.size() < 2) {
        if (error) *error = QStringLiteral("El documento JSON quedó vacío al serializar.");
        return false;
    }

    QJsonParseError resultado{};
    QJsonDocument::fromJson(contenido, &resultado);
    if (resultado.error != QJsonParseError::NoError) {
        if (error) {
            *error = QStringLiteral("El JSON generado no es válido: %1")
                         .arg(resultado.errorString());
        }
        return false;
    }
    return true;
}

bool guardarJsonAtomico(const QString& ruta, const QJsonDocument& documento, QString* error) {
    QString errorDocumento;
    if (!documentoJsonValido(documento, &errorDocumento)) {
        if (error) *error = errorDocumento;
        return false;
    }

    const QFileInfo info(ruta);
    if (!QDir().mkpath(info.absolutePath())) {
        if (error) {
            *error = QStringLiteral("No se pudo crear la carpeta de datos: %1")
                         .arg(info.absolutePath());
        }
        return false;
    }

    QSaveFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) {
            *error = QStringLiteral("No se pudo escribir el archivo %1: %2")
                         .arg(ruta, archivo.errorString());
        }
        return false;
    }

    const QByteArray contenido = documento.toJson(QJsonDocument::Indented);
    if (archivo.write(contenido) != contenido.size()) {
        if (error) {
            *error = QStringLiteral("No se pudo escribir todo el contenido en %1.").arg(ruta);
        }
        archivo.cancelWriting();
        return false;
    }

    if (!archivo.commit()) {
        if (error) {
            *error = QStringLiteral("No se pudo guardar el archivo %1: %2")
                         .arg(ruta, archivo.errorString());
        }
        return false;
    }
    return true;
}

struct GuardiaBloqueo {
    GestorJSON* gestor;
    bool liberarAlSalir;
    ~GuardiaBloqueo() {
        if (liberarAlSalir) gestor->finalizarTransaccion();
    }
};

QString fechaIso(const QDateTime& fecha) {
    return fecha.isValid() ? fecha.toString(Qt::ISODate) : QString();
}

QDateTime fechaDesdeIso(const QJsonObject& objeto, const QString& clave,
                        bool obligatoria, const QString& contexto,
                        QStringList* errores) {
    const QJsonValue valor = objeto.value(clave);
    if (valor.isNull() || valor.isUndefined()) {
        if (obligatoria) {
            errores->append(QStringLiteral("%1: falta '%2'").arg(contexto, clave));
        }
        return {};
    }
    if (!valor.isString()) {
        errores->append(QStringLiteral("%1: '%2' debe ser texto").arg(contexto, clave));
        return {};
    }
    // Fechas opcionales guardadas como cadena vacía se tratan como ausentes.
    if (valor.toString().trimmed().isEmpty()) return {};
    const QDateTime fecha = QDateTime::fromString(valor.toString(), Qt::ISODate);
    if (!fecha.isValid()) {
        errores->append(QStringLiteral("%1: '%2' no es una fecha ISO válida")
                            .arg(contexto, clave));
    }
    return fecha;
}

QString idSiguiente(const std::vector<DatosVentaTicket>& ventas) {
    int numeroMayor = 1000;
    for (const auto& venta : ventas) {
        if (!venta.getIdVenta().startsWith(QStringLiteral("V-"))) continue;
        bool conversionValida = false;
        const int numero = venta.getIdVenta().mid(2).toInt(&conversionValida);
        if (conversionValida && numero > numeroMayor) {
            numeroMayor = numero;
        }
    }
    return QStringLiteral("V-%1").arg(numeroMayor + 1, 4, 10, QLatin1Char('0'));
}

} // namespace

GestorJSON::GestorJSON(const QString& rutaProds, const QString& rutaVnts,
                       const QString& rutaAuditoria, const QString& rutaUsuarios)
    : rutaProductos(rutaProds.isEmpty() ? rutaDatos(QStringLiteral("productos.json")) : rutaProds),
      rutaVentas(rutaVnts.isEmpty() ? rutaDatos(QStringLiteral("ventas.json")) : rutaVnts),
      rutaAuditoriaProductos(rutaAuditoria.isEmpty()
                                 ? rutaDatos(QStringLiteral("auditoria_productos.json"))
                                 : rutaAuditoria),
      rutaUsuarios(rutaUsuarios.isEmpty() ? rutaDatos(QStringLiteral("usuarios.json"))
                                          : rutaUsuarios) {}

void GestorJSON::asignarError(const QString& mensaje) const {
    ultimoError_ = mensaje;
}

void GestorJSON::limpiarError() const {
    ultimoError_.clear();
    erroresSerializacion_.clear();
}

void GestorJSON::resumirErrores(const QString& nombreArchivo) {
    if (erroresSerializacion_.isEmpty()) return;

    asignarError(QStringLiteral("Se omitieron %1 registro(s) inválido(s) en %2. %3")
                     .arg(erroresSerializacion_.size())
                     .arg(nombreArchivo)
                     .arg(erroresSerializacion_.mid(0, 3).join(QStringLiteral(" | "))));
}

bool GestorJSON::iniciarTransaccion(QString* error) {
    if (bloqueoActivo_) return true;

    if (!bloqueo_) {
        bloqueo_ = std::make_unique<QLockFile>(rutaProductos + QStringLiteral(".lock"));
        bloqueo_->setStaleLockTime(30000); // 30 s: tolera procesos caídos.
    }

    if (!bloqueo_->tryLock()) {
        QString detalle;
        switch (bloqueo_->error()) {
        case QLockFile::LockFailedError:
            detalle = QStringLiteral("el archivo está en uso por otro proceso");
            break;
        case QLockFile::PermissionError:
            detalle = QStringLiteral("sin permisos para crear el archivo de bloqueo");
            break;
        case QLockFile::UnknownError:
        default:
            detalle = QStringLiteral("error desconocido al bloquear");
            break;
        }
        const QString mensaje =
            QStringLiteral("No se pudo bloquear el archivo de datos (%1): %2")
                .arg(bloqueo_->fileName(), detalle);
        asignarError(mensaje);
        if (error) *error = mensaje;
        return false;
    }

    bloqueoActivo_ = true;
    return true;
}

void GestorJSON::finalizarTransaccion() {
    if (bloqueoActivo_ && bloqueo_) {
        bloqueo_->unlock();
    }
    bloqueoActivo_ = false;
}

bool GestorJSON::leerArreglo(const QString& ruta, QJsonArray* arreglo) const {
    *arreglo = QJsonArray();

    QFile archivo(ruta);
    if (!archivo.exists()) {
        return true; // Aún no creado: no es error de lectura.
    }
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        asignarError(QStringLiteral("No se pudo abrir el archivo %1: %2")
                         .arg(ruta, archivo.errorString()));
        return false;
    }

    QJsonParseError errorLectura{};
    const QJsonDocument documento = QJsonDocument::fromJson(archivo.readAll(), &errorLectura);
    if (errorLectura.error != QJsonParseError::NoError) {
        asignarError(QStringLiteral("El archivo %1 no contiene JSON válido (posición %2): %3")
                         .arg(ruta)
                         .arg(errorLectura.offset)
                         .arg(errorLectura.errorString()));
        return false;
    }
    if (!documento.isArray()) {
        asignarError(QStringLiteral("El archivo %1 debe contener un arreglo JSON.").arg(ruta));
        return false;
    }

    *arreglo = documento.array();
    return true;
}

bool GestorJSON::insertarNumero(QJsonObject* objeto, const QString& clave,
                                double valor, QString* error) {
    if (!std::isfinite(valor)) {
        if (error) {
            *error = QStringLiteral("Valor no finito en el campo '%1'; no se serializará.")
                         .arg(clave);
        }
        return false;
    }
    objeto->insert(clave, valor);
    return true;
}

bool GestorJSON::leerCadena(const QJsonObject& objeto, const QString& clave,
                            QString* destino, const QString& valorPorDefecto,
                            bool obligatorio, const QString& contexto,
                            QStringList* errores) {
    const auto campo = objeto.constFind(clave);
    if (campo == objeto.constEnd() || campo->isNull() || campo->isUndefined()) {
        if (obligatorio) {
            errores->append(QStringLiteral("%1: falta '%2'").arg(contexto, clave));
            return false;
        }
        *destino = valorPorDefecto;
        return true;
    }
    if (!campo->isString()) {
        errores->append(QStringLiteral("%1: '%2' debe ser texto").arg(contexto, clave));
        return false;
    }
    *destino = campo->toString();
    return true;
}

bool GestorJSON::leerNumero(const QJsonObject& objeto, const QString& clave,
                            double* destino, double minimo, double maximo,
                            double valorPorDefecto, bool obligatorio,
                            const QString& contexto, QStringList* errores) {
    const auto campo = objeto.constFind(clave);
    if (campo == objeto.constEnd() || campo->isNull() || campo->isUndefined()) {
        if (obligatorio) {
            errores->append(QStringLiteral("%1: falta '%2'").arg(contexto, clave));
            return false;
        }
        *destino = valorPorDefecto;
        return true;
    }
    if (!campo->isDouble()) {
        errores->append(QStringLiteral("%1: '%2' debe ser numérico").arg(contexto, clave));
        return false;
    }

    const double valor = campo->toDouble();
    if (!std::isfinite(valor) || valor < minimo || valor > maximo) {
        errores->append(QStringLiteral("%1: '%2' fuera de rango (%3)")
                            .arg(contexto, clave)
                            .arg(valor));
        return false;
    }
    *destino = valor;
    return true;
}

bool GestorJSON::leerEntero(const QJsonObject& objeto, const QString& clave,
                            int* destino, int minimo, int maximo,
                            int valorPorDefecto, bool obligatorio,
                            const QString& contexto, QStringList* errores) {
    const auto campo = objeto.constFind(clave);
    if (campo == objeto.constEnd() || campo->isNull() || campo->isUndefined()) {
        if (obligatorio) {
            errores->append(QStringLiteral("%1: falta '%2'").arg(contexto, clave));
            return false;
        }
        *destino = valorPorDefecto;
        return true;
    }
    if (!campo->isDouble()) {
        errores->append(QStringLiteral("%1: '%2' debe ser entero").arg(contexto, clave));
        return false;
    }

    const double valor = campo->toDouble();
    if (!std::isfinite(valor) || std::floor(valor) != valor
        || valor < minimo || valor > maximo) {
        errores->append(QStringLiteral("%1: '%2' entero fuera de rango (%3)")
                            .arg(contexto, clave)
                            .arg(valor));
        return false;
    }
    *destino = static_cast<int>(valor);
    return true;
}

bool GestorJSON::leerBooleano(const QJsonObject& objeto, const QString& clave,
                              bool* destino, bool valorPorDefecto,
                              const QString& contexto, QStringList* errores) {
    const auto campo = objeto.constFind(clave);
    if (campo == objeto.constEnd() || campo->isNull() || campo->isUndefined()) {
        *destino = valorPorDefecto;
        return true;
    }
    if (!campo->isBool()) {
        errores->append(QStringLiteral("%1: '%2' debe ser booleano").arg(contexto, clave));
        return false;
    }
    *destino = campo->toBool();
    return true;
}

bool GestorJSON::existeArchivoUsuarios() const {
    return QFile::exists(rutaUsuarios);
}

std::vector<RegistroAuditoriaProducto> GestorJSON::cargarAuditoriaProductos() {
    erroresSerializacion_.clear();
    ultimoError_.clear();

    std::vector<RegistroAuditoriaProducto> registros;
    const bool bloqueoPrevio = bloqueoActivo_;
    GuardiaBloqueo guardia{this, !bloqueoPrevio};
    if (!bloqueoPrevio && !iniciarTransaccion()) return registros;

    QJsonArray arreglo;
    if (!leerArreglo(rutaAuditoriaProductos, &arreglo)) return registros;

    for (int i = 0; i < arreglo.size(); ++i) {
        if (!arreglo.at(i).isObject()) {
            erroresSerializacion_.append(
                QStringLiteral("auditoria[%1]: no es un objeto").arg(i));
            continue;
        }
        const QJsonObject obj = arreglo.at(i).toObject();
        const QString contexto = QStringLiteral("auditoria[%1]").arg(i);
        RegistroAuditoriaProducto registro;
        bool correcto = true;
        correcto &= leerCadena(obj, QStringLiteral("fechaHora"), &registro.fechaHora,
                               QString(), true, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("accion"), &registro.accion,
                               QString(), true, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("nombreProducto"), &registro.nombreProducto,
                               QString(), false, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("detalleCambio"), &registro.detalleCambio,
                               QString(), false, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("usuarioResponsable"),
                               &registro.usuarioResponsable, QString(), false,
                               contexto, &erroresSerializacion_);
        if (correcto) registros.push_back(registro);
    }

    resumirErrores(QStringLiteral("auditoria_productos.json"));
    return registros;
}

bool GestorJSON::registrarCambioProducto(const RegistroAuditoriaProducto& registro) {
    ultimoError_.clear();
    erroresSerializacion_.clear();

    const bool bloqueoPrevio = bloqueoActivo_;
    GuardiaBloqueo guardia{this, !bloqueoPrevio};
    if (!bloqueoPrevio && !iniciarTransaccion()) return false;

    QJsonArray arreglo;
    if (!leerArreglo(rutaAuditoriaProductos, &arreglo)) return false;

    QJsonObject obj;
    obj.insert(QStringLiteral("fechaHora"), registro.fechaHora);
    obj.insert(QStringLiteral("accion"), registro.accion);
    obj.insert(QStringLiteral("nombreProducto"), registro.nombreProducto);
    obj.insert(QStringLiteral("detalleCambio"), registro.detalleCambio);
    obj.insert(QStringLiteral("usuarioResponsable"), registro.usuarioResponsable);
    arreglo.append(obj);

    return guardarJsonAtomico(rutaAuditoriaProductos, QJsonDocument(arreglo), &ultimoError_);
}

std::vector<std::shared_ptr<Usuario>> GestorJSON::cargarUsuarios() {
    erroresSerializacion_.clear();
    ultimoError_.clear();

    std::vector<std::shared_ptr<Usuario>> lista;
    const bool bloqueoPrevio = bloqueoActivo_;
    GuardiaBloqueo guardia{this, !bloqueoPrevio};
    if (!bloqueoPrevio && !iniciarTransaccion()) return lista;

    QJsonArray arreglo;
    if (!leerArreglo(rutaUsuarios, &arreglo)) return lista;

    for (int i = 0; i < arreglo.size(); ++i) {
        if (!arreglo.at(i).isObject()) {
            erroresSerializacion_.append(QStringLiteral("usuarios[%1]: no es un objeto").arg(i));
            continue;
        }
        const QJsonObject obj = arreglo.at(i).toObject();
        const QString contexto = QStringLiteral("usuarios[%1]").arg(i);

        QString usuario;
        QString hash;
        QString rol;
        QString nombre;
        QString id;
        bool activo = true;
        bool correcto = true;
        correcto &= leerCadena(obj, QStringLiteral("usuario"), &usuario, QString(),
                               true, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("hashContrasena"), &hash, QString(),
                               false, contexto, &erroresSerializacion_);
        if (hash.isEmpty()) {
            correcto &= leerCadena(obj, QStringLiteral("clave"), &hash, QString(),
                                   true, contexto, &erroresSerializacion_);
        }
        correcto &= leerCadena(obj, QStringLiteral("rol"), &rol, QStringLiteral("Cajero"),
                               false, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("nombre"), &nombre, usuario,
                               false, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("id"), &id, usuario,
                               false, contexto, &erroresSerializacion_);
        correcto &= leerBooleano(obj, QStringLiteral("activo"), &activo, true,
                                 contexto, &erroresSerializacion_);

        if (!correcto || usuario.trimmed().isEmpty() || hash.isEmpty()) {
            if (correcto) {
                erroresSerializacion_.append(
                    QStringLiteral("%1: usuario o contraseña vacíos").arg(contexto));
            }
            continue;
        }

        if (rol == QStringLiteral("Administrador")) {
            lista.push_back(std::make_shared<Administrador>(id, nombre, usuario, hash, activo));
        } else {
            lista.push_back(std::make_shared<Cajero>(id, nombre, usuario, hash, activo));
        }
    }

    resumirErrores(QStringLiteral("usuarios.json"));
    return lista;
}

bool GestorJSON::guardarUsuarios(const std::vector<std::shared_ptr<Usuario>>& usuarios) {
    ultimoError_.clear();
    erroresSerializacion_.clear();

    const bool bloqueoPrevio = bloqueoActivo_;
    GuardiaBloqueo guardia{this, !bloqueoPrevio};
    if (!bloqueoPrevio && !iniciarTransaccion()) return false;

    QJsonArray arreglo;
    for (const auto& usuario : usuarios) {
        if (!usuario) continue;
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), usuario->getId());
        obj.insert(QStringLiteral("nombre"), usuario->getNombre());
        obj.insert(QStringLiteral("usuario"), usuario->getNombreUsuario());
        obj.insert(QStringLiteral("hashContrasena"), usuario->getHashContrasena());
        obj.insert(QStringLiteral("rol"), usuario->rol());
        obj.insert(QStringLiteral("activo"), usuario->isActivo());
        arreglo.append(obj);
    }
    return guardarJsonAtomico(rutaUsuarios, QJsonDocument(arreglo), &ultimoError_);
}

std::vector<ProductoItem> GestorJSON::cargarProductos() {
    erroresSerializacion_.clear();
    ultimoError_.clear();

    std::vector<ProductoItem> lista;
    const bool bloqueoPrevio = bloqueoActivo_;
    GuardiaBloqueo guardia{this, !bloqueoPrevio};
    if (!bloqueoPrevio && !iniciarTransaccion()) return lista;

    QJsonArray arreglo;
    if (!leerArreglo(rutaProductos, &arreglo)) return lista;

    for (int i = 0; i < arreglo.size(); ++i) {
        if (!arreglo.at(i).isObject()) {
            erroresSerializacion_.append(QStringLiteral("productos[%1]: no es un objeto").arg(i));
            continue;
        }
        const QJsonObject obj = arreglo.at(i).toObject();
        const QString contexto = QStringLiteral("productos[%1]").arg(i);
        QString id;
        QString nombre;
        QString descripcion;
        QString categoria;
        double precio = 0.0;
        int stock = 0;
        bool activo = true;
        bool correcto = true;
        correcto &= leerCadena(obj, QStringLiteral("id"), &id, QString(),
                               true, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("nombre"), &nombre, QString(),
                               true, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("descripcion"), &descripcion,
                               QString(), false, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("categoria"), &categoria,
                               QString(), false, contexto, &erroresSerializacion_);
        correcto &= leerNumero(obj, QStringLiteral("precio"), &precio,
                               0.0, 10000000.0, 0.0, true,
                               contexto, &erroresSerializacion_);
        correcto &= leerEntero(obj, QStringLiteral("stock"), &stock,
                               0, 1000000, 0, false,
                               contexto, &erroresSerializacion_);
        correcto &= leerBooleano(obj, QStringLiteral("activo"), &activo,
                                 true, contexto, &erroresSerializacion_);

        ProductoItem producto;
        const bool setterId = producto.setId(id);
        const bool setterNombre = producto.setNombre(nombre);
        const bool setterPrecio = producto.setPrecio(precio);
        const bool setterDescripcion = producto.setDescripcion(descripcion);
        const bool setterCategoria = producto.setCategoria(categoria);
        const bool setterStock = producto.setStock(stock);
        producto.setActivo(activo);

        if (!correcto || !setterId || !setterNombre || !setterPrecio
            || !setterDescripcion || !setterCategoria || !setterStock
            || !producto.esValido()) {
            erroresSerializacion_.append(
                QStringLiteral("%1: producto inválido tras saneo").arg(contexto));
            continue;
        }
        lista.push_back(producto);
    }

    resumirErrores(QStringLiteral("productos.json"));
    return lista;
}

bool GestorJSON::guardarProductos(const std::vector<ProductoItem>& productos) {
    ultimoError_.clear();
    erroresSerializacion_.clear();

    const bool bloqueoPrevio = bloqueoActivo_;
    GuardiaBloqueo guardia{this, !bloqueoPrevio};
    if (!bloqueoPrevio && !iniciarTransaccion()) return false;

    QJsonArray arreglo;
    for (const auto& producto : productos) {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), producto.getId());
        obj.insert(QStringLiteral("nombre"), producto.getNombre());
        QString errorNumero;
        if (!insertarNumero(&obj, QStringLiteral("precio"), producto.getPrecio(), &errorNumero)
            || !insertarNumero(&obj, QStringLiteral("stock"), producto.getStock(), &errorNumero)) {
            asignarError(errorNumero);
            return false;
        }
        obj.insert(QStringLiteral("descripcion"), producto.getDescripcion());
        obj.insert(QStringLiteral("categoria"), producto.getCategoria());
        obj.insert(QStringLiteral("activo"), producto.isActivo());
        arreglo.append(obj);
    }

    return guardarJsonAtomico(rutaProductos, QJsonDocument(arreglo), &ultimoError_);
}

std::vector<DatosVentaTicket> GestorJSON::cargarVentas() {
    erroresSerializacion_.clear();
    ultimoError_.clear();

    std::vector<DatosVentaTicket> lista;
    const bool bloqueoPrevio = bloqueoActivo_;
    GuardiaBloqueo guardia{this, !bloqueoPrevio};
    if (!bloqueoPrevio && !iniciarTransaccion()) return lista;

    QJsonArray arreglo;
    if (!leerArreglo(rutaVentas, &arreglo)) return lista;

    for (int i = 0; i < arreglo.size(); ++i) {
        if (!arreglo.at(i).isObject()) {
            erroresSerializacion_.append(QStringLiteral("ventas[%1]: no es un objeto").arg(i));
            continue;
        }
        const QJsonObject obj = arreglo.at(i).toObject();
        const QString contexto = QStringLiteral("ventas[%1]").arg(i);
        QString idVenta;
        QString usuario;
        QString tipoDescuento;
        QString usuarioAnulacion;
        QDateTime fechaHora;
        QDateTime fechaAnulacion;
        double subtotal = 0.0;
        double iva = 0.0;
        double total = 0.0;
        double pagaCon = 0.0;
        double cambio = 0.0;
        double descuento = 0.0;
        double valorDescuento = 0.0;
        bool anulada = false;
        bool correcto = true;

        correcto &= leerCadena(obj, QStringLiteral("idVenta"), &idVenta, QString(),
                               true, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("usuario"), &usuario, QString(),
                               false, contexto, &erroresSerializacion_);
        if (usuario.isEmpty()) {
            correcto &= leerCadena(obj, QStringLiteral("cajero"), &usuario, QString(),
                                   false, contexto, &erroresSerializacion_);
        }

        const QJsonValue fecha = obj.value(QStringLiteral("fechaHora"));
        if (!fecha.isNull() && !fecha.isUndefined()) {
            fechaHora = fechaDesdeIso(obj, QStringLiteral("fechaHora"), true,
                                      contexto, &erroresSerializacion_);
            if (!fecha.toString().trimmed().isEmpty() && !fechaHora.isValid()) {
                correcto = false;
            }
        }

        correcto &= leerNumero(obj, QStringLiteral("subtotal"), &subtotal,
                               0.0, 1e9, 0.0, false, contexto, &erroresSerializacion_);
        correcto &= leerNumero(obj, QStringLiteral("iva"), &iva,
                               0.0, 1e9, 0.0, false, contexto, &erroresSerializacion_);
        correcto &= leerNumero(obj, QStringLiteral("total"), &total,
                               0.0, 1e9, 0.0, false, contexto, &erroresSerializacion_);
        correcto &= leerNumero(obj, QStringLiteral("pagaCon"), &pagaCon,
                               0.0, 1e9, 0.0, false, contexto, &erroresSerializacion_);
        correcto &= leerNumero(obj, QStringLiteral("cambio"), &cambio,
                               0.0, 1e9, 0.0, false, contexto, &erroresSerializacion_);
        correcto &= leerNumero(obj, QStringLiteral("descuento"), &descuento,
                               0.0, 1e9, 0.0, false, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("tipoDescuento"), &tipoDescuento,
                               QString(), false, contexto, &erroresSerializacion_);
        correcto &= leerNumero(obj, QStringLiteral("valorDescuento"), &valorDescuento,
                               0.0, 1e9, 0.0, false, contexto, &erroresSerializacion_);
        correcto &= leerBooleano(obj, QStringLiteral("anulada"), &anulada,
                                 false, contexto, &erroresSerializacion_);
        correcto &= leerCadena(obj, QStringLiteral("usuarioAnulacion"), &usuarioAnulacion,
                               QString(), false, contexto, &erroresSerializacion_);

        const QJsonValue fechaAnulacionJson = obj.value(QStringLiteral("fechaAnulacion"));
        if (!fechaAnulacionJson.isNull() && !fechaAnulacionJson.isUndefined()) {
            fechaAnulacion = fechaDesdeIso(obj, QStringLiteral("fechaAnulacion"),
                                           true, contexto, &erroresSerializacion_);
            if (!fechaAnulacionJson.toString().trimmed().isEmpty()
                && !fechaAnulacion.isValid()) {
                correcto = false;
            }
        }

        DatosVentaTicket venta;
        correcto &= venta.setIdVenta(idVenta);
        venta.setUsuario(usuario);
        venta.setFechaHora(fechaHora);
        correcto &= venta.setSubtotal(subtotal);
        correcto &= venta.setIva(iva);
        correcto &= venta.setTotal(total);
        correcto &= venta.setPagaCon(pagaCon);
        correcto &= venta.setCambio(cambio);
        correcto &= venta.setDescuento(descuento);
        venta.setTipoDescuento(tipoDescuento);
        correcto &= venta.setValorDescuento(valorDescuento);
        venta.setAnulada(anulada);
        venta.setUsuarioAnulacion(usuarioAnulacion);
        venta.setFechaAnulacion(fechaAnulacion);

        const QJsonValue valorItems = obj.value(QStringLiteral("items"));
        if (!valorItems.isNull() && !valorItems.isUndefined() && !valorItems.isArray()) {
            erroresSerializacion_.append(QStringLiteral("%1: 'items' debe ser arreglo").arg(contexto));
            correcto = false;
        } else if (valorItems.isArray()) {
            const QJsonArray items = valorItems.toArray();
            for (int j = 0; j < items.size(); ++j) {
                const QString contextoItem = QStringLiteral("%1.items[%2]").arg(contexto).arg(j);
                if (!items.at(j).isObject()) {
                    erroresSerializacion_.append(
                        QStringLiteral("%1: no es un objeto").arg(contextoItem));
                    correcto = false;
                    continue;
                }
                const QJsonObject itemObj = items.at(j).toObject();
                QString idProducto;
                QString nombreProducto;
                int cantidad = 0;
                double precioUnitario = 0.0;
                double subtotalItem = 0.0;
                bool correctoItem = true;
                correctoItem &= leerCadena(itemObj, QStringLiteral("idProducto"), &idProducto,
                                           QString(), false, contextoItem, &erroresSerializacion_);
                correctoItem &= leerCadena(itemObj, QStringLiteral("producto"),
                                           &nombreProducto, QString(), true,
                                           contextoItem, &erroresSerializacion_);
                correctoItem &= leerEntero(itemObj, QStringLiteral("cantidad"), &cantidad,
                                           1, 1000000, 0, true,
                                           contextoItem, &erroresSerializacion_);
                correctoItem &= leerNumero(itemObj, QStringLiteral("precioUnitario"),
                                           &precioUnitario, 0.0, 1e9, 0.0, true,
                                           contextoItem, &erroresSerializacion_);
                correctoItem &= leerNumero(itemObj, QStringLiteral("subtotal"),
                                           &subtotalItem, 0.0, 1e9, 0.0, false,
                                           contextoItem, &erroresSerializacion_);

                const auto item = ItemTicket::crear(idProducto, nombreProducto, cantidad,
                                                    precioUnitario, subtotalItem);
                if (correctoItem && !item.has_value()) {
                    erroresSerializacion_.append(
                        QStringLiteral("%1: artículo inválido").arg(contextoItem));
                    correctoItem = false;
                }
                if (correctoItem) venta.anadirItem(*item);
                correcto &= correctoItem;
            }
        }

        if (!correcto || venta.getIdVenta().isEmpty()) {
            if (correcto) {
                erroresSerializacion_.append(QStringLiteral("%1: ID de venta vacío").arg(contexto));
            }
            continue;
        }
        lista.push_back(venta);
    }

    resumirErrores(QStringLiteral("ventas.json"));
    return lista;
}

bool GestorJSON::guardarVentas(const std::vector<DatosVentaTicket>& ventas) {
    ultimoError_.clear();
    erroresSerializacion_.clear();

    const bool bloqueoPrevio = bloqueoActivo_;
    GuardiaBloqueo guardia{this, !bloqueoPrevio};
    if (!bloqueoPrevio && !iniciarTransaccion()) return false;

    QJsonArray arregloVentas;
    for (const auto& venta : ventas) {
        QJsonObject objVenta;
        objVenta.insert(QStringLiteral("idVenta"), venta.getIdVenta());
        objVenta.insert(QStringLiteral("usuario"), venta.getUsuario());
        if (venta.getFechaHora().isValid()) {
            objVenta.insert(QStringLiteral("fechaHora"), fechaIso(venta.getFechaHora()));
        }

        QString errorNumero;
        const double campos[] = {venta.getSubtotal(), venta.getIva(), venta.getTotal(),
                                 venta.getPagaCon(), venta.getCambio(), venta.getDescuento(),
                                 venta.getValorDescuento()};
        const char* nombres[] = {"subtotal", "iva", "total", "pagaCon",
                                 "cambio", "descuento", "valorDescuento"};
        bool camposValidos = true;
        for (int i = 0; i < 7; ++i) {
            camposValidos &= insertarNumero(&objVenta, QString::fromLatin1(nombres[i]),
                                            campos[i], &errorNumero);
            if (!camposValidos) break;
        }
        if (!camposValidos) {
            asignarError(errorNumero);
            return false;
        }

        objVenta.insert(QStringLiteral("tipoDescuento"), venta.getTipoDescuento());
        objVenta.insert(QStringLiteral("anulada"), venta.isAnulada());
        objVenta.insert(QStringLiteral("usuarioAnulacion"), venta.getUsuarioAnulacion());
        if (venta.getFechaAnulacion().isValid()) {
            objVenta.insert(QStringLiteral("fechaAnulacion"), fechaIso(venta.getFechaAnulacion()));
        }

        QJsonArray items;
        for (const auto& item : venta.getItems()) {
            QJsonObject itemObj;
            itemObj.insert(QStringLiteral("idProducto"), item.getIdProducto());
            itemObj.insert(QStringLiteral("producto"), item.getNombreProducto());
            if (!insertarNumero(&itemObj, QStringLiteral("cantidad"), item.getCantidad(), &errorNumero)
                || !insertarNumero(&itemObj, QStringLiteral("precioUnitario"),
                                   item.getPrecioUnitario(), &errorNumero)
                || !insertarNumero(&itemObj, QStringLiteral("subtotal"),
                                   item.getSubtotal(), &errorNumero)) {
                asignarError(errorNumero);
                return false;
            }
            items.append(itemObj);
        }
        objVenta.insert(QStringLiteral("items"), items);
        arregloVentas.append(objVenta);
    }

    return guardarJsonAtomico(rutaVentas, QJsonDocument(arregloVentas), &ultimoError_);
}

QString GestorJSON::siguienteIdVenta() {
    erroresSerializacion_.clear();
    ultimoError_.clear();

    const bool bloqueoPrevio = bloqueoActivo_;
    GuardiaBloqueo guardia{this, !bloqueoPrevio};
    if (!bloqueoPrevio && !iniciarTransaccion()) return QString();

    const auto ventas = cargarVentas();
    if (!ultimoError_.isEmpty()) return QString();
    return idSiguiente(ventas);
}
