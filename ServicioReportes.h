#ifndef SERVICIOREPORTES_H
#define SERVICIOREPORTES_H

#include <QString>
#include <vector>
#include "ModelosDatos.h"

// Exportación de reportes con escape correcto y UTF-8 para macOS y Windows.
class ServicioReportes {
public:
    static bool exportarVentasCSV(const std::vector<DatosVentaTicket>& ventas,
                                  const QString& ruta,
                                  QString* error = nullptr);

    static QString rutaSugeridaCSV();
    static QString escaparCampoCSV(const QString& valor);
};

#endif // SERVICIOREPORTES_H
