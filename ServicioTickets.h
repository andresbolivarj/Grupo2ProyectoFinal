#ifndef SERVICIOTICKETS_H
#define SERVICIOTICKETS_H

#include <QString>
#include <memory>
#include "IGeneradorTicket.h"
#include "ModelosDatos.h"

enum class FormatoTicket {
    Txt,
    Html,
    Pdf
};

// Fábrica y fachada de generación de comprobantes. La vista no conoce las
// clases concretas de ticket (OCP).
class ServicioTickets {
public:
    static std::unique_ptr<IGeneradorTicket> crear(FormatoTicket formato);
    static QString nombreArchivo(const DatosVentaTicket& datos, FormatoTicket formato);
    static QString carpetaTickets(QString* error = nullptr);
    static QString rutaSugerida(const DatosVentaTicket& datos, FormatoTicket formato,
                                QString* error = nullptr);

    QString generarContenido(const DatosVentaTicket& datos, FormatoTicket formato) const;
    bool guardar(const DatosVentaTicket& datos, FormatoTicket formato,
                 const QString& ruta, QString* error = nullptr) const;
};

#endif // SERVICIOTICKETS_H
