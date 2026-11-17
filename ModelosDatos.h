#ifndef MODELOSDATOS_H
#define MODELOSDATOS_H

#include <QDateTime>
#include <QString>
#include <cmath>
#include <optional>
#include <vector>

namespace Dominio {
inline bool numeroValido(double valor) {
    return std::isfinite(valor);
}
}

// Entidad de inventario con invariantes de dominio. Los campos críticos son
// privados; la serialización en GestorJSON usa únicamente getters/setters.
class ProductoItem {
private:
    QString id_;
    QString nombre_;
    double precio_ = 0.0;
    QString descripcion_;
    QString categoria_;
    int stock_ = 0;
    bool activo_ = true;

public:
    ProductoItem() = default;
    ProductoItem(const QString& id, const QString& nombre, double precio,
                 const QString& descripcion, const QString& categoria,
                 int stock, bool activo = true)
        : activo_(activo) {
        setId(id);
        setNombre(nombre);
        setPrecio(precio);
        setDescripcion(descripcion);
        setCategoria(categoria);
        setStock(stock);
    }

    const QString& getId() const { return id_; }
    const QString& getNombre() const { return nombre_; }
    double getPrecio() const { return precio_; }
    const QString& getDescripcion() const { return descripcion_; }
    const QString& getCategoria() const { return categoria_; }
    int getStock() const { return stock_; }
    bool isActivo() const { return activo_; }

    bool setId(const QString& valor) {
        const QString limpio = valor.trimmed();
        if (limpio.isEmpty() || limpio.size() > 20) return false;
        id_ = limpio;
        return true;
    }
    bool setNombre(const QString& valor) {
        const QString limpio = valor.trimmed();
        if (limpio.isEmpty() || limpio.size() > 80) return false;
        nombre_ = limpio;
        return true;
    }
    bool setPrecio(double valor) {
        if (!Dominio::numeroValido(valor) || valor < 0.0 || valor > 10000000.0) return false;
        precio_ = valor;
        return true;
    }
    bool setDescripcion(const QString& valor) {
        if (valor.size() > 200) return false;
        descripcion_ = valor.trimmed();
        return true;
    }
    bool setCategoria(const QString& valor) {
        if (valor.trimmed().isEmpty()) return false;
        categoria_ = valor.trimmed();
        return true;
    }
    bool setStock(int valor) {
        if (valor < 0 || valor > 1000000) return false;
        stock_ = valor;
        return true;
    }
    void setActivo(bool valor) { activo_ = valor; }

    bool esValido() const {
        return !id_.isEmpty() && !nombre_.isEmpty()
               && Dominio::numeroValido(precio_) && precio_ >= 0.0 && stock_ >= 0;
    }

    static std::optional<ProductoItem> crear(const QString& id, const QString& nombre,
                                             double precio, const QString& descripcion,
                                             const QString& categoria, int stock,
                                             bool activo = true) {
        ProductoItem producto(id, nombre, precio, descripcion, categoria, stock, activo);
        if (!producto.esValido()) return std::nullopt;
        return producto;
    }
};

struct RegistroAuditoriaProducto {
    QString fechaHora;
    QString accion;
    QString nombreProducto;
    QString detalleCambio;
    QString usuarioResponsable;
};

struct ItemCarrito {
    ProductoItem producto;
    int cantidad = 0;
};

// Detalle congelado de una línea de venta (precio histórico).
class ItemTicket {
private:
    QString idProducto_;
    QString nombreProducto_;
    int cantidad_ = 0;
    double precioUnitario_ = 0.0;
    double subtotal_ = 0.0;

public:
    ItemTicket() = default;
    ItemTicket(const QString& idProducto, const QString& nombreProducto, int cantidad,
               double precioUnitario, double subtotal = 0.0)
        : idProducto_(idProducto), nombreProducto_(nombreProducto) {
        setCantidad(cantidad);
        setPrecioUnitario(precioUnitario);
        setSubtotal(subtotal);
    }

    const QString& getIdProducto() const { return idProducto_; }
    const QString& getNombreProducto() const { return nombreProducto_; }
    int getCantidad() const { return cantidad_; }
    double getPrecioUnitario() const { return precioUnitario_; }
    double getSubtotal() const { return subtotal_; }

    void setIdProducto(const QString& valor) { idProducto_ = valor.trimmed(); }
    bool setNombreProducto(const QString& valor) {
        const QString limpio = valor.trimmed();
        if (limpio.isEmpty()) return false;
        nombreProducto_ = limpio;
        return true;
    }
    bool setCantidad(int valor) {
        if (valor <= 0 || valor > 1000000) return false;
        cantidad_ = valor;
        return true;
    }
    bool setPrecioUnitario(double valor) {
        if (!Dominio::numeroValido(valor) || valor < 0.0 || valor > 1e9) return false;
        precioUnitario_ = valor;
        return true;
    }
    bool setSubtotal(double valor) {
        if (!Dominio::numeroValido(valor) || valor < 0.0 || valor > 1e9) return false;
        subtotal_ = valor;
        return true;
    }

    bool esValido() const {
        return !nombreProducto_.isEmpty() && cantidad_ > 0
               && Dominio::numeroValido(precioUnitario_) && precioUnitario_ >= 0.0
               && Dominio::numeroValido(subtotal_) && subtotal_ >= 0.0;
    }

    static std::optional<ItemTicket> crear(const QString& idProducto,
                                           const QString& nombreProducto,
                                           int cantidad, double precioUnitario,
                                           double subtotal = 0.0) {
        ItemTicket item(idProducto, nombreProducto, cantidad, precioUnitario, subtotal);
        if (!item.esValido()) return std::nullopt;
        return item;
    }
};

// Comprobante de venta con estado de anulación y trazabilidad.
class DatosVentaTicket {
private:
    QString idVenta_;
    QString usuario_;
    QDateTime fechaHora_;
    std::vector<ItemTicket> items_;
    double subtotal_ = 0.0;
    double iva_ = 0.0;
    double total_ = 0.0;
    double pagaCon_ = 0.0;
    double cambio_ = 0.0;
    double descuento_ = 0.0;
    QString tipoDescuento_;
    double valorDescuento_ = 0.0;
    bool anulada_ = false;
    QString usuarioAnulacion_;
    QDateTime fechaAnulacion_;

    bool numeroAceptable(double valor) const {
        return Dominio::numeroValido(valor) && valor >= 0.0 && valor <= 1e9;
    }

public:
    DatosVentaTicket() = default;

    const QString& getIdVenta() const { return idVenta_; }
    const QString& getUsuario() const { return usuario_; }
    const QDateTime& getFechaHora() const { return fechaHora_; }
    const std::vector<ItemTicket>& getItems() const { return items_; }
    double getSubtotal() const { return subtotal_; }
    double getIva() const { return iva_; }
    double getTotal() const { return total_; }
    double getPagaCon() const { return pagaCon_; }
    double getCambio() const { return cambio_; }
    double getDescuento() const { return descuento_; }
    const QString& getTipoDescuento() const { return tipoDescuento_; }
    double getValorDescuento() const { return valorDescuento_; }
    bool isAnulada() const { return anulada_; }
    const QString& getUsuarioAnulacion() const { return usuarioAnulacion_; }
    const QDateTime& getFechaAnulacion() const { return fechaAnulacion_; }

    bool setIdVenta(const QString& valor) {
        const QString limpio = valor.trimmed();
        if (limpio.isEmpty()) return false;
        idVenta_ = limpio;
        return true;
    }
    void setUsuario(const QString& valor) { usuario_ = valor.trimmed(); }
    void setFechaHora(const QDateTime& valor) { fechaHora_ = valor; }

    void anadirItem(const ItemTicket& item) {
        if (item.esValido()) items_.push_back(item);
    }
    void setItems(const std::vector<ItemTicket>& items) {
        items_.clear();
        for (const auto& item : items) anadirItem(item);
    }

    bool setSubtotal(double valor) {
        if (!numeroAceptable(valor)) return false;
        subtotal_ = valor;
        return true;
    }
    bool setIva(double valor) {
        if (!numeroAceptable(valor)) return false;
        iva_ = valor;
        return true;
    }
    bool setTotal(double valor) {
        if (!numeroAceptable(valor)) return false;
        total_ = valor;
        return true;
    }
    bool setPagaCon(double valor) {
        if (!numeroAceptable(valor)) return false;
        pagaCon_ = valor;
        return true;
    }
    bool setCambio(double valor) {
        if (!numeroAceptable(valor)) return false;
        cambio_ = valor;
        return true;
    }
    bool setDescuento(double valor) {
        if (!numeroAceptable(valor)) return false;
        descuento_ = valor;
        return true;
    }
    void setTipoDescuento(const QString& valor) { tipoDescuento_ = valor.trimmed(); }
    bool setValorDescuento(double valor) {
        if (!numeroAceptable(valor)) return false;
        valorDescuento_ = valor;
        return true;
    }
    void setAnulada(bool valor) { anulada_ = valor; }
    void setUsuarioAnulacion(const QString& valor) { usuarioAnulacion_ = valor.trimmed(); }
    void setFechaAnulacion(const QDateTime& valor) { fechaAnulacion_ = valor; }

    bool esValido() const {
        return !idVenta_.isEmpty() && !items_.empty() && total_ >= 0.0
               && Dominio::numeroValido(total_);
    }
};

#endif // MODELOSDATOS_H
