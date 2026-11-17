#ifndef TICKETPDF_H
#define TICKETPDF_H

#include <QString>
#include "IGeneradorTicket.h"
#include "ModelosDatos.h"

// Tercera implementación concreta de IGeneradorTicket (OCP): PDF.
class GeneradorTicketPDF : public IGeneradorTicket {
public:
    QString obtenerExtension() const override { return QStringLiteral(".pdf"); }
    QString generarContenido(const DatosVentaTicket& datos) override;
    bool guardar(const DatosVentaTicket& datos, const QString& ruta,
                 QString* error = nullptr) override;
};

#endif // TICKETPDF_H
