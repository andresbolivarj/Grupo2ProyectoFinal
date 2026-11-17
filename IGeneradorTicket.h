#ifndef IGENERADORTICKET_H
#define IGENERADORTICKET_H

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QString>
#include <QStringConverter>
#include <QTextStream>
#include "ModelosDatos.h"

inline QString escaparHtml(const QString& texto) {
    QString resultado = texto;
    resultado.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    resultado.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    resultado.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    resultado.replace(QLatin1Char('"'), QStringLiteral("&quot;"));
    resultado.replace(QLatin1Char('\''), QStringLiteral("&#39;"));
    return resultado;
}

namespace {
inline bool guardarTextoUtf8(const QString& ruta, const QString& contenido, QString* error) {
    if (error) error->clear();
    if (ruta.isEmpty()) {
        if (error) *error = QStringLiteral("No se indicó la ruta del comprobante.");
        return false;
    }

    const QFileInfo info(ruta);
    if (!QDir().mkpath(info.absolutePath())) {
        if (error) {
            *error = QStringLiteral("No se pudo crear la carpeta %1.").arg(info.absolutePath());
        }
        return false;
    }

    QSaveFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) {
            *error = QStringLiteral("No se pudo abrir %1 para escribir: %2")
                         .arg(ruta, archivo.errorString());
        }
        return false;
    }
    {
        QTextStream salida(&archivo);
        salida.setEncoding(QStringConverter::Utf8);
        salida << contenido;
    }
    if (!archivo.commit()) {
        if (error) {
            *error = QStringLiteral("No se pudo guardar %1: %2")
                         .arg(ruta, archivo.errorString());
        }
        return false;
    }
    return true;
}
} // namespace

class IGeneradorTicket {
public:
    virtual ~IGeneradorTicket() = default;
    virtual QString generarContenido(const DatosVentaTicket& datos) = 0;
    virtual QString obtenerExtension() const = 0;
    virtual bool guardar(const DatosVentaTicket& datos, const QString& ruta,
                         QString* error = nullptr) = 0;
};

class GeneradorTicketTXT : public IGeneradorTicket {
public:
    QString obtenerExtension() const override { return QStringLiteral(".txt"); }

    QString generarContenido(const DatosVentaTicket& datos) override {
        QString texto;
        texto += QStringLiteral("========================================\n");
        texto += QStringLiteral("              UNA PULPE                 \n");
        texto += QStringLiteral("========================================\n");
        texto += QStringLiteral("Ticket ID : %1\n").arg(datos.getIdVenta());
        texto += QStringLiteral("Usuario   : %1\n").arg(datos.getUsuario());
        texto += QStringLiteral("Fecha     : %1\n")
                     .arg(datos.getFechaHora().toString(QStringLiteral("yyyy-MM-dd hh:mm AP")));
        texto += QStringLiteral("----------------------------------------\n");
        texto += QStringLiteral("%1 %2 %3\n")
                     .arg(QStringLiteral("Cant/Prod"), -25)
                     .arg(QStringLiteral("P.Unit"), 7)
                     .arg(QStringLiteral("Total"), 8);
        texto += QStringLiteral("----------------------------------------\n");

        for (const auto& item : datos.getItems()) {
            QString linea = QStringLiteral("%1x %2").arg(item.getCantidad()).arg(item.getNombreProducto());
            if (linea.length() > 24) linea = linea.left(21) + QStringLiteral("...");
            texto += QStringLiteral("%1 ₡%2 ₡%3\n")
                         .arg(linea, -25)
                         .arg(item.getPrecioUnitario(), 6, 'f', 2)
                         .arg(item.getSubtotal(), 7, 'f', 2);
        }

        texto += QStringLiteral("----------------------------------------\n");
        texto += QStringLiteral("Subtotal       : ₡%1\n").arg(datos.getSubtotal(), 0, 'f', 2);
        if (datos.getDescuento() > 0.0) {
            texto += QStringLiteral("Descuento      : -₡%1\n").arg(datos.getDescuento(), 0, 'f', 2);
        }
        texto += QStringLiteral("IVA incluido   : ₡%1\n").arg(datos.getIva(), 0, 'f', 2);
        texto += QStringLiteral("TOTAL          : ₡%1\n").arg(datos.getTotal(), 0, 'f', 2);
        texto += QStringLiteral("----------------------------------------\n");
        texto += QStringLiteral("Paga con       : ₡%1\n").arg(datos.getPagaCon(), 0, 'f', 2);
        texto += QStringLiteral("Cambio         : ₡%1\n").arg(datos.getCambio(), 0, 'f', 2);
        texto += QStringLiteral("========================================\n");

        if (datos.isAnulada()) {
            texto += QStringLiteral("*** VENTA ANULADA POR %1 ***\n").arg(datos.getUsuarioAnulacion());
            texto += QStringLiteral("*** %1 ***\n")
                         .arg(datos.getFechaAnulacion().toString(QStringLiteral("yyyy-MM-dd hh:mm AP")));
            texto += QStringLiteral("========================================\n");
        }

        texto += QStringLiteral("        ¡Gracias por su compra!         \n");
        texto += QStringLiteral("========================================\n");
        return texto;
    }

    bool guardar(const DatosVentaTicket& datos, const QString& ruta,
                 QString* error = nullptr) override {
        return guardarTextoUtf8(ruta, generarContenido(datos), error);
    }
};

class GeneradorTicketHTML : public IGeneradorTicket {
public:
    QString obtenerExtension() const override { return QStringLiteral(".html"); }

    QString generarContenido(const DatosVentaTicket& datos) override {
        QString html = QStringLiteral(
            "<html><body style='font-family: sans-serif; font-size: 12px; color: #111827;'>");
        html += QStringLiteral(
            "<h2 style='text-align: center; color: #047857; margin-bottom: 5px;'>UNA Pulpe</h2>");
        html += QStringLiteral("<hr style='border: none; border-top: 1px dashed #d1d5db;'>");
        html += QStringLiteral("<p><b>Ticket #:</b> %1<br>").arg(escaparHtml(datos.getIdVenta()));
        html += QStringLiteral("<b>Usuario:</b> %1<br>").arg(escaparHtml(datos.getUsuario()));
        html += QStringLiteral("<b>Fecha:</b> %1</p>")
                    .arg(escaparHtml(datos.getFechaHora().toString(
                        QStringLiteral("yyyy-MM-dd hh:mm AP"))));
        html += QStringLiteral("<hr style='border: none; border-top: 1px dashed #d1d5db;'>");

        if (datos.isAnulada()) {
            html += QStringLiteral(
                "<p style='color:#b91c1c; font-weight:bold; text-align:center;'>"
                "VENTA ANULADA</p>");
            html += QStringLiteral("<p style='color:#6b7280; font-size:11px; text-align:center;'>"
                                   "Por %1 · %2</p>")
                        .arg(escaparHtml(datos.getUsuarioAnulacion()),
                             escaparHtml(datos.getFechaAnulacion().toString(
                                 QStringLiteral("yyyy-MM-dd hh:mm AP"))));
        }

        html += QStringLiteral(
            "<table width='100%' style='border-collapse: collapse; font-size: 12px;'>");
        html += QStringLiteral(
            "<tr style='color: #6b7280;'><th align='left'>Cant/Prod</th>"
            "<th align='right'>P. Unit</th><th align='right'>Total</th></tr>");

        for (const auto& item : datos.getItems()) {
            html += QStringLiteral(
                "<tr><td style='padding: 3px 0;'>%1x %2</td>"
                "<td align='right'>₡%3</td><td align='right'>₡%4</td></tr>")
                        .arg(item.getCantidad())
                        .arg(escaparHtml(item.getNombreProducto()))
                        .arg(item.getPrecioUnitario(), 0, 'f', 2)
                        .arg(item.getSubtotal(), 0, 'f', 2);
        }

        html += QStringLiteral("</table><hr style='border: none; border-top: 1px dashed #d1d5db;'>");
        html += QStringLiteral("<p align='right' style='margin: 3px 0;'>Subtotal: ₡%1</p>")
                    .arg(datos.getSubtotal(), 0, 'f', 2);
        if (datos.getDescuento() > 0.0) {
            html += QStringLiteral("<p align='right' style='margin: 3px 0; color:#b91c1c;'>"
                                   "Descuento: -₡%1</p>")
                        .arg(datos.getDescuento(), 0, 'f', 2);
        }
        html += QStringLiteral("<p align='right' style='margin: 3px 0;'>IVA incluido (13%): ₡%1</p>")
                    .arg(datos.getIva(), 0, 'f', 2);
        html += QStringLiteral("<p align='right' style='font-size: 14px; color: #047857;"
                               " margin: 5px 0;'><b>TOTAL: ₡%1</b></p>")
                    .arg(datos.getTotal(), 0, 'f', 2);
        html += QStringLiteral("<hr style='border: none; border-top: 1px dashed #d1d5db;'>");
        html += QStringLiteral("<p align='right' style='margin: 3px 0;'>Paga con: ₡%1</p>")
                    .arg(datos.getPagaCon(), 0, 'f', 2);
        html += QStringLiteral("<p align='right' style='margin: 3px 0;'>Cambio: ₡%1</p>")
                    .arg(datos.getCambio(), 0, 'f', 2);
        html += QStringLiteral("<p style='text-align: center; margin-top: 15px;"
                               " color: #6b7280;'>¡Gracias por su compra!</p>");
        html += QStringLiteral("</body></html>");
        return html;
    }

    bool guardar(const DatosVentaTicket& datos, const QString& ruta,
                 QString* error = nullptr) override {
        return guardarTextoUtf8(ruta, generarContenido(datos), error);
    }
};

#endif // IGENERADORTICKET_H
