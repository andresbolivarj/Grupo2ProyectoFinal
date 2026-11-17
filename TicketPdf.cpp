#include "TicketPdf.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPageLayout>
#include <QPageSize>
#include <QPrinter>
#include <QTextDocument>

namespace {
void fallo(QString* error, const QString& mensaje) {
    if (error) *error = mensaje;
}
}

QString GeneradorTicketPDF::generarContenido(const DatosVentaTicket& datos) {
    return GeneradorTicketHTML().generarContenido(datos);
}

bool GeneradorTicketPDF::guardar(const DatosVentaTicket& datos, const QString& ruta,
                                 QString* error) {
    if (error) error->clear();

    if (ruta.isEmpty()) {
        fallo(error, QStringLiteral("No se indicó la ruta del PDF."));
        return false;
    }

    const QFileInfo info(ruta);
    if (!QDir().mkpath(info.absolutePath())) {
        fallo(error, QStringLiteral("No se pudo crear la carpeta %1.").arg(info.absolutePath()));
        return false;
    }

    const QFileInfo carpeta(info.absolutePath());
    if (carpeta.exists() && !carpeta.isWritable()) {
        fallo(error, QStringLiteral("Sin permisos de escritura en la carpeta %1.")
                         .arg(info.absolutePath()));
        return false;
    }

    {
        QPrinter impresora(QPrinter::HighResolution);
        impresora.setOutputFormat(QPrinter::PdfFormat);
        impresora.setOutputFileName(ruta);
        // Ancho de ticket térmico de 80 mm y alto carta para paginar ventas largas.
        impresora.setPageSize(QPageSize(QSizeF(80, 279), QPageSize::Millimeter,
                                        QStringLiteral("Ticket")));
        impresora.setPageMargins(QMarginsF(4, 4, 4, 4), QPageLayout::Millimeter);

        if (impresora.outputFileName().isEmpty()) {
            fallo(error, QStringLiteral("QPrinter no aceptó la ruta %1.").arg(ruta));
            return false;
        }

        QTextDocument documento;
        documento.setHtml(generarContenido(datos));
        documento.setPageSize(impresora.pageRect(QPrinter::Point).size());
        documento.print(&impresora);
    }

    // QPrinter no expone status() en Qt 6: se valida el resultado inspeccionando
    // el archivo generado (existencia, tamaño y encabezado %PDF).
    const QFileInfo archivo(ruta);
    if (!archivo.exists() || archivo.size() < 5) {
        fallo(error, QStringLiteral("No se generó el archivo PDF en %1.").arg(ruta));
        return false;
    }

    QFile lectura(ruta);
    if (!lectura.open(QIODevice::ReadOnly) || lectura.read(4) != "%PDF") {
        fallo(error, QStringLiteral("El archivo %1 no tiene un encabezado PDF válido.").arg(ruta));
        return false;
    }

    return true;
}
