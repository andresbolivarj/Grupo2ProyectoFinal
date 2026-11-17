#ifndef GESTORJSON_H
#define GESTORJSON_H

#include <QJsonArray>
#include <QJsonObject>
#include <QLockFile>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>
#include "ModelosDatos.h"
#include "Usuario.h"

// Adaptador de persistencia JSON. No contiene reglas de negocio: traduce entre
// los modelos y los archivos, informa errores y mantiene exclusión mutua en
// las transacciones críticas. Las escrituras son atómicas mediante QSaveFile.
class GestorJSON {
private:
    QString rutaProductos;
    QString rutaVentas;
    QString rutaAuditoriaProductos;
    QString rutaUsuarios;

    mutable QString ultimoError_;
    mutable QStringList erroresSerializacion_;
    std::unique_ptr<QLockFile> bloqueo_;
    bool bloqueoActivo_ = false;

    void asignarError(const QString& mensaje) const;
    bool leerArreglo(const QString& ruta, QJsonArray* arreglo) const;

    static bool insertarNumero(QJsonObject* objeto, const QString& clave,
                               double valor, QString* error);
    static bool leerCadena(const QJsonObject& objeto, const QString& clave,
                           QString* destino, const QString& valorPorDefecto,
                           bool obligatorio, const QString& contexto,
                           QStringList* errores);
    static bool leerNumero(const QJsonObject& objeto, const QString& clave,
                           double* destino, double minimo, double maximo,
                           double valorPorDefecto, bool obligatorio,
                           const QString& contexto, QStringList* errores);
    static bool leerEntero(const QJsonObject& objeto, const QString& clave,
                           int* destino, int minimo, int maximo,
                           int valorPorDefecto, bool obligatorio,
                           const QString& contexto, QStringList* errores);
    static bool leerBooleano(const QJsonObject& objeto, const QString& clave,
                             bool* destino, bool valorPorDefecto,
                             const QString& contexto, QStringList* errores);
    void resumirErrores(const QString& nombreArchivo);

public:
    explicit GestorJSON(const QString& rutaProds = QString(),
                        const QString& rutaVnts = QString(),
                        const QString& rutaAuditoria = QString(),
                        const QString& rutaUsuarios = QString());

    const QString& ultimoError() const { return ultimoError_; }
    const QStringList& erroresSerializacion() const { return erroresSerializacion_; }
    void limpiarError() const;

    // Transacción: mantiene el QLockFile vivo durante varias lecturas/escrituras.
    bool iniciarTransaccion(QString* error = nullptr);
    void finalizarTransaccion();

    // Productos
    std::vector<ProductoItem> cargarProductos();
    bool guardarProductos(const std::vector<ProductoItem>& productos);

    // Ventas
    std::vector<DatosVentaTicket> cargarVentas();

    // Escritura de bajo nivel: SOLO debe usarla ServicioVentas dentro de su
    // flujo transaccional (validación + rebaja de stock). No registra ventas
    // por sí sola.
    bool guardarVentas(const std::vector<DatosVentaTicket>& ventas);

    // Única fuente de verdad del consecutivo V-XXXX. Adquiere el QLockFile
    // cuando no hay una transacción abierta y reutiliza la activa si la hay.
    QString siguienteIdVenta();

    // Auditoría de productos
    std::vector<RegistroAuditoriaProducto> cargarAuditoriaProductos();
    bool registrarCambioProducto(const RegistroAuditoriaProducto& registro);

    // Usuarios
    std::vector<std::shared_ptr<Usuario>> cargarUsuarios();
    bool guardarUsuarios(const std::vector<std::shared_ptr<Usuario>>& usuarios);
    bool existeArchivoUsuarios() const;
};

// Guardia RAII para envolver operaciones compuestas.
class TransaccionJSON {
public:
    explicit TransaccionJSON(GestorJSON& gestor, QString* error = nullptr)
        : gestor_(gestor), activa_(gestor.iniciarTransaccion(error)) {}
    ~TransaccionJSON() {
        if (activa_) gestor_.finalizarTransaccion();
    }
    bool activa() const { return activa_; }

    TransaccionJSON(const TransaccionJSON&) = delete;
    TransaccionJSON& operator=(const TransaccionJSON&) = delete;

private:
    GestorJSON& gestor_;
    bool activa_;
};

#endif // GESTORJSON_H
