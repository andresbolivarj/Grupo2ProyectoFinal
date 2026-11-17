#include "ServicioReportes.h"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringConverter>
#include <QTextStream>

QString ServicioReportes::escaparCampoCSV(const QString& valor) {
    QString escapado = valor;
    escapado.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(escapado);
}

QString ServicioReportes::rutaSugeridaCSV() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (base.isEmpty()) base = QDir::homePath();
    const QString carpeta = QDir(base).filePath(QStringLiteral("UNA Pulpe/Reportes"));
    QDir().mkpath(carpeta);
    return QDir(carpeta).filePath(QStringLiteral("Reporte_Ventas.csv"));
}

bool ServicioReportes::exportarVentasCSV(const std::vector<DatosVentaTicket>& ventas,
                                         const QString& ruta,
                                         QString* error) {
    if (error) error->clear();
    if (ruta.isEmpty()) {
        if (error) *error = QStringLiteral("No se indicó la ruta del reporte.");
        return false;
    }

    const QFileInfo info(ruta);
    if (!QDir().mkpath(info.absolutePath())) {
        if (error) *error = QStringLiteral("No se pudo crear la carpeta del reporte.");
        return false;
    }

    QSaveFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) {
            *error = QStringLiteral("No se pudo crear el archivo de reporte:\n%1")
                         .arg(archivo.errorString());
        }
        return false;
    }

    {
        QTextStream salida(&archivo);
        salida.setEncoding(QStringConverter::Utf8);
        salida.setGenerateByteOrderMark(true); // Compatibilidad con Excel en Windows.

        salida << QStringLiteral("ID Venta,Fecha y Hora,Usuario,Subtotal,IVA,Total,"
                                 "Descuento,Tipo Descuento,Valor Descuento,Estado,"
                                 "Anulada Por,Fecha Anulacion\n");

        for (const auto& venta : ventas) {
            salida << escaparCampoCSV(venta.getIdVenta()) << ','
                   << escaparCampoCSV(venta.getFechaHora().toString(
                          QStringLiteral("yyyy-MM-dd hh:mm AP"))) << ','
                   << escaparCampoCSV(venta.getUsuario()) << ','
                   << QString::number(venta.getSubtotal(), 'f', 2) << ','
                   << QString::number(venta.getIva(), 'f', 2) << ','
                   << QString::number(venta.getTotal(), 'f', 2) << ','
                   << QString::number(venta.getDescuento(), 'f', 2) << ','
                   << escaparCampoCSV(venta.getTipoDescuento()) << ','
                   << QString::number(venta.getValorDescuento(), 'f', 2) << ','
                   << escaparCampoCSV(venta.isAnulada() ? QStringLiteral("Anulada")
                                                        : QStringLiteral("Vigente")) << ','
                   << escaparCampoCSV(venta.getUsuarioAnulacion()) << ','
                   << escaparCampoCSV(venta.getFechaAnulacion().isValid()
                                          ? venta.getFechaAnulacion().toString(
                                                QStringLiteral("yyyy-MM-dd hh:mm AP"))
                                          : QString())
                   << '\n';
        }
    }

    if (!archivo.commit()) {
        if (error) {
            *error = QStringLiteral("No se pudo guardar el reporte:\n%1")
                         .arg(archivo.errorString());
        }
        return false;
    }
    return true;
}
