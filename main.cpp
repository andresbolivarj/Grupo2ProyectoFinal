#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <exception>
#include <new>
#include <stdexcept>

#include "Estilos.h"
#include "VentanaLogin.h"
#include "VentanaPrincipal.h"

namespace {
void mostrarErrorFatal(const QString& titulo, const QString& mensaje) {
    qCritical().noquote() << titulo << ":" << mensaje;
    QMessageBox::critical(nullptr, titulo, mensaje);
}
} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("UNA"));
    app.setApplicationName(QStringLiteral("LaPulpe"));
    Estilos::aplicarTemaClaro(app);

    int resultado = 0;
    try {
        // Permite volver al login al cerrar sesión sin relanzar el proceso.
        for (;;) {
            VentanaLogin login;
            if (login.exec() != QDialog::Accepted) {
                break;
            }

            VentanaPrincipal principal(login.getUsuarioAutenticado());
            principal.show();
            resultado = app.exec();

            if (!principal.cerradaPorSesion()) {
                break;
            }
        }
    } catch (const std::bad_alloc&) {
        mostrarErrorFatal(
            QStringLiteral("Memoria insuficiente"),
            QStringLiteral("No hay memoria suficiente para continuar. Cierre otros programas "
                           "y vuelva a intentarlo."));
        resultado = 1;
    } catch (const std::runtime_error& error) {
        // Errores de E/S o de datos (p. ej. JSON ilegible o archivos bloqueados).
        mostrarErrorFatal(
            QStringLiteral("Error de archivos o datos"),
            QStringLiteral("No se pudo acceder a la información del sistema:\n\n%1")
                .arg(QString::fromUtf8(error.what())));
        resultado = 1;
    } catch (const std::logic_error& error) {
        mostrarErrorFatal(
            QStringLiteral("Error interno"),
            QStringLiteral("Se detectó un estado inválido en la aplicación:\n\n%1")
                .arg(QString::fromUtf8(error.what())));
        resultado = 1;
    } catch (const std::exception& error) {
        mostrarErrorFatal(
            QStringLiteral("Error inesperado"),
            QStringLiteral("La aplicación encontró un error y debe cerrarse:\n\n%1")
                .arg(QString::fromUtf8(error.what())));
        resultado = 1;
    } catch (...) {
        mostrarErrorFatal(
            QStringLiteral("Error desconocido"),
            QStringLiteral("La aplicación encontró un error desconocido y debe cerrarse."));
        resultado = 1;
    }

    return resultado;
}
