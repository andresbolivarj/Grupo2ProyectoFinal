#ifndef TICKETPDF_H
#define TICKETPDF_H

#include "ModelosDatos.h"
#include <QString>

bool generarTicketPDF(const DatosVentaTicket& venta, const QString& rutaArchivo);

#endif // TICKETPDF_H