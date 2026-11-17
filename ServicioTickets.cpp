#include "ServicioTickets.h"

#include "TicketPdf.h"

#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {
QString sanitizar(const QString& valor) {
    QString limpio = valor.trimmed();
    limpio.replace(QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}._-]+")),
                   QStringLiteral("_"));
    return limpio.isEmpty() ? QStringLiteral("ticket") : limpio;
}
} // namespace

std::unique_ptr<IGeneradorTicket> ServicioTickets::crear(FormatoTicket formato) {
    switch (formato) {
    case FormatoTicket::Pdf:
        return std::make_unique<GeneradorTicketPDF>();
    case FormatoTicket::Html:
        return std::make_unique<GeneradorTicketHTML>();
    case FormatoTicket::Txt:
    default:
        return std::make_unique<GeneradorTicketTXT>();
    }
}

QString ServicioTickets::nombreArchivo(const DatosVentaTicket& datos, FormatoTicket formato) {
    const auto generador = crear(formato);
    return QStringLiteral("Ticket_%1%2")
        .arg(sanitizar(datos.getIdVenta()), generador->obtenerExtension());
}

QString ServicioTickets::carpetaTickets(QString* error) {
    if (error) error->clear();

    QString base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (base.isEmpty()) {
        base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    if (base.isEmpty()) {
        base = QDir::homePath();
    }

    const QString carpeta = QDir(base).filePath(QStringLiteral("UNA Pulpe/Tickets"));
    if (!QDir().mkpath(carpeta)) {
        if (error) {
            *error = QStringLiteral("No se pudo crear la carpeta de tickets %1.").arg(carpeta);
        }
        return QString();
    }
    return carpeta;
}

QString ServicioTickets::rutaSugerida(const DatosVentaTicket& datos, FormatoTicket formato,
                                      QString* error) {
    const QString carpeta = carpetaTickets(error);
    if (carpeta.isEmpty()) return QString();
    return QDir(carpeta).filePath(nombreArchivo(datos, formato));
}

QString ServicioTickets::generarContenido(const DatosVentaTicket& datos,
                                          FormatoTicket formato) const {
    const auto generador = crear(formato);
    return generador->generarContenido(datos);
}

bool ServicioTickets::guardar(const DatosVentaTicket& datos, FormatoTicket formato,
                              const QString& ruta, QString* error) const {
    if (error) error->clear();
    if (ruta.isEmpty()) {
        if (error) *error = QStringLiteral("No se indicó la ruta del comprobante.");
        return false;
    }

    const auto generador = crear(formato);
    if (!generador->guardar(datos, ruta)) {
        if (error) {
            *error = QStringLiteral("No se pudo guardar el comprobante en:\n%1").arg(ruta);
        }
        return false;
    }
    return true;
}
