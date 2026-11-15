#include "TicketPdf.h"

#include "IGeneradorTicket.h"
#include <QFileInfo>
#include <QPageLayout>
#include <QPageSize>
#include <QPrinter>
#include <QTextDocument>

bool generarTicketPDF(const DatosVentaTicket& venta, const QString& rutaArchivo) {
    if (rutaArchivo.isEmpty()) return false;

    {
        QPrinter impresora(QPrinter::HighResolution);
        impresora.setOutputFormat(QPrinter::PdfFormat);
        impresora.setOutputFileName(rutaArchivo);
        impresora.setPageSize(QPageSize(QSizeF(80, 220), QPageSize::Millimeter, "Ticket"));
        impresora.setPageMargins(QMarginsF(3, 3, 3, 3), QPageLayout::Millimeter);

        GeneradorTicketHTML generador;
        QTextDocument documento;
        documento.setHtml(generador.generarContenido(venta));
        documento.setPageSize(impresora.pageRect(QPrinter::Point).size());
        documento.print(&impresora);
    }

    const QFileInfo archivo(rutaArchivo);
    return archivo.exists() && archivo.size() > 0;
}