#ifndef VISTAHISTORIAL_H
#define VISTAHISTORIAL_H

#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <vector>
#include "ModelosDatos.h"
#include "Usuario.h"

class VistaHistorial : public QWidget {
    Q_OBJECT

private:
    QLineEdit* txtBuscar = nullptr;
    QComboBox* comboRangoFecha = nullptr;
    QDateEdit* dateInicio = nullptr;
    QDateEdit* dateFin = nullptr;
    QTableWidget* tablaVentas = nullptr;
    QPushButton* btnVerTicket = nullptr;
    QPushButton* btnDescargarPDF = nullptr;
    QPushButton* btnExportarCSV = nullptr;
    QPushButton* btnAnularVenta = nullptr;

    QLabel* lblTotalVendido = nullptr;
    QLabel* lblTotalIVA = nullptr;
    QLabel* lblCantidadVentas = nullptr;

    std::vector<DatosVentaTicket> listaVentas;
    std::shared_ptr<Usuario> usuarioActual;

    bool esAdministrador() const;

public:
    explicit VistaHistorial(QWidget* parent = nullptr);
    VistaHistorial(std::shared_ptr<Usuario> usuario, QWidget* parent);
    void actualizar();

signals:
    void historialActualizado();
    void ventaAnulada(const QString& idVenta);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void configurarInterfaz();
    void cargarDatos();
    void renderizarTabla();
    void filtrarTabla();
    void mostrarTicketSeleccionado();
    void descargarTicketPDF();
    void exportarCSV();
    void anularVentaSeleccionada();
};

#endif // VISTAHISTORIAL_H
