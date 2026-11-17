#include "VentanaLogin.h"

#include "Estilos.h"
#include "ServicioAutenticacion.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QVBoxLayout>

VentanaLogin::VentanaLogin(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("UNA Pulpe - Iniciar Sesión"));
    setMinimumSize(380, 470);
    resize(410, 510);
    setStyleSheet(QStringLiteral("QDialog { background-color: #ffffff; }"));

    QVBoxLayout* layoutPrincipal = new QVBoxLayout(this);
    layoutPrincipal->setContentsMargins(32, 26, 32, 26);
    layoutPrincipal->setSpacing(12);

    QLabel* lblLogoImagen = new QLabel(this);
    lblLogoImagen->setAlignment(Qt::AlignCenter);
    lblLogoImagen->setMinimumHeight(140);

    const QPixmap pixmapLogo(QStringLiteral(":/logo.jpeg"));
    if (!pixmapLogo.isNull()) {
        lblLogoImagen->setPixmap(pixmapLogo.scaled(140, 140, Qt::KeepAspectRatio,
                                                   Qt::SmoothTransformation));
    } else {
        lblLogoImagen->setText(QStringLiteral("UNA Pulpe"));
        lblLogoImagen->setStyleSheet(
            QStringLiteral("font-size: 26px; font-weight: bold; color: #047857;"));
    }
    layoutPrincipal->addWidget(lblLogoImagen);

    QLabel* lblTitulo = new QLabel(QStringLiteral("Iniciar Sesión"), this);
    lblTitulo->setAlignment(Qt::AlignCenter);
    lblTitulo->setStyleSheet(
        QStringLiteral("color: #111827; font-size: 18px; font-weight: bold;"));
    layoutPrincipal->addWidget(lblTitulo);
    layoutPrincipal->addSpacing(6);

    QLabel* lblUser = new QLabel(QStringLiteral("Usuario:"), this);
    lblUser->setStyleSheet(QStringLiteral("color: #374151; font-size: 12px; font-weight: bold;"));
    layoutPrincipal->addWidget(lblUser);

    txtUsuario = new QLineEdit(this);
    txtUsuario->setPlaceholderText(QStringLiteral("Nombre de usuario"));
    txtUsuario->setMaxLength(64);
    txtUsuario->setStyleSheet(Estilos::estiloEntrada());
    layoutPrincipal->addWidget(txtUsuario);

    QLabel* lblPass = new QLabel(QStringLiteral("Contraseña:"), this);
    lblPass->setStyleSheet(QStringLiteral("color: #374151; font-size: 12px; font-weight: bold;"));
    layoutPrincipal->addWidget(lblPass);

    QHBoxLayout* layoutPass = new QHBoxLayout();
    layoutPass->setSpacing(6);

    txtClave = new QLineEdit(this);
    txtClave->setPlaceholderText(QStringLiteral("Contraseña"));
    txtClave->setEchoMode(QLineEdit::Password);
    txtClave->setMaxLength(128);
    txtClave->setStyleSheet(Estilos::estiloEntrada());

    QPushButton* btnVerPass = new QPushButton(QStringLiteral("Ver"), this);
    btnVerPass->setFixedWidth(68);
    btnVerPass->setCursor(Qt::PointingHandCursor);
    btnVerPass->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #f3f4f6; color: #374151;"
                       " font-weight: bold; border: 1px solid #d1d5db; border-radius: 8px;"
                       " padding: 8px; font-size: 11px; }"
                       "QPushButton:hover { background-color: #e5e7eb; }"));
    connect(btnVerPass, &QPushButton::clicked, this, [this, btnVerPass]() {
        const bool oculta = txtClave->echoMode() == QLineEdit::Password;
        txtClave->setEchoMode(oculta ? QLineEdit::Normal : QLineEdit::Password);
        btnVerPass->setText(oculta ? QStringLiteral("Ocultar") : QStringLiteral("Ver"));
    });

    layoutPass->addWidget(txtClave, 1);
    layoutPass->addWidget(btnVerPass);
    layoutPrincipal->addLayout(layoutPass);
    layoutPrincipal->addSpacing(10);

    btnIngresar = new QPushButton(QStringLiteral("Iniciar Sesión"), this);
    btnIngresar->setMinimumHeight(44);
    btnIngresar->setDefault(true);
    btnIngresar->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #047857; color: #ffffff;"
                       " font-weight: bold; border: none; border-radius: 8px; font-size: 14px; }"
                       "QPushButton:hover { background-color: #065f46; }"
                       "QPushButton:disabled { background-color: #9ca3af; }"));
    connect(btnIngresar, &QPushButton::clicked, this, &VentanaLogin::intentarLogin);
    layoutPrincipal->addWidget(btnIngresar);

    layoutPrincipal->addStretch();

    connect(txtUsuario, &QLineEdit::returnPressed, this, [this]() { txtClave->setFocus(); });
    connect(txtClave, &QLineEdit::returnPressed, this, &VentanaLogin::intentarLogin);
    setTabOrder(txtUsuario, txtClave);
    setTabOrder(txtClave, btnVerPass);
    setTabOrder(btnVerPass, btnIngresar);
    txtUsuario->setFocus();
}

void VentanaLogin::intentarLogin() {
    const QString usuario = txtUsuario->text().trimmed();
    const QString clave = txtClave->text();

    if (usuario.isEmpty() || clave.isEmpty()) {
        Estilos::mostrarAdvertencia(this, QStringLiteral("Datos incompletos"),
                                    QStringLiteral("Por favor ingrese su usuario y contraseña."));
        return;
    }

    btnIngresar->setEnabled(false);
    ServicioAutenticacion servicio;
    QString error;
    const auto autenticado = servicio.autenticar(usuario, clave, &error);
    btnIngresar->setEnabled(true);

    if (autenticado) {
        usuarioAutenticado = autenticado;
        accept();
        return;
    }

    const bool credencialesInvalidas = error.contains(QStringLiteral("incorrectos"));
    const bool datosCorruptos = error.contains(QStringLiteral("JSON"))
                                || error.contains(QStringLiteral("válido"))
                                || error.contains(QStringLiteral("registro"))
                                || error.contains(QStringLiteral("corrupto"));

    if (credencialesInvalidas) {
        // No se registran credenciales incorrectas para no llenar el log de ruido.
        Estilos::mostrarAdvertencia(this, QStringLiteral("Acceso denegado"),
                                    QStringLiteral("Usuario o contraseña incorrectos.\n"
                                                   "Verifique sus credenciales e intente de nuevo."));
    } else if (datosCorruptos) {
        qWarning().noquote() << QStringLiteral("LaPulpe: datos corruptos en el login: %1")
                                    .arg(error);
        Estilos::mostrarError(
            this, QStringLiteral("Datos del sistema dañados"),
            QStringLiteral("El archivo de usuarios contiene información inválida:\n\n%1\n\n"
                           "No se modificó ningún archivo. Contacte al administrador.")
                .arg(error));
    } else {
        qWarning().noquote() << QStringLiteral("LaPulpe: fallo técnico en el login: %1")
                                    .arg(error);
        Estilos::mostrarError(
            this, QStringLiteral("No se pudo iniciar sesión"),
            error.isEmpty() ? QStringLiteral("No fue posible validar las credenciales.")
                            : error);
    }
    txtClave->selectAll();
    txtClave->setFocus();
}

std::shared_ptr<Usuario> VentanaLogin::getUsuarioAutenticado() const {
    return usuarioAutenticado;
}
