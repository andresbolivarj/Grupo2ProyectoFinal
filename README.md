<div align="center">

# 🛒 UNA Pulpe — Sistema de Punto de Venta

**Sistema integral de ventas, inventario y auditoría desarrollado en C++17 y Qt 6**

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Qt 6](https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.16%2B-064F8C?style=for-the-badge&logo=cmake&logoColor=white)
![Platform](https://img.shields.io/badge/Plataforma-macOS%20%7C%20Windows%20%7C%20Linux-lightgrey?style=for-the-badge)

*Arquitectura desacoplada por capas · Persistencia JSON atómica · Seguridad PBKDF2-SHA256*

[Requisitos](#-requisitos-del-sistema) •
[Compilación](#-compilación-y-ejecución) •
[Roles y Permisos](#-roles-y-control-de-acceso) •
[Guía de Uso](#-guía-de-uso) •
[Arquitectura](#-arquitectura-y-seguridad) •
[Datos](#-rutas-de-datos-y-recuperación)

</div>

---

## 📋 Requisitos del sistema

El proyecto está construido únicamente sobre módulos estándar de Qt 6, por lo que **no requiere librerías externas de terceros**:

- **Compilador:** Soporte completo para **C++17** (`Clang`, `GCC` o `MSVC`).
- **Build System:** **CMake 3.16** o superior.
- **Framework:** **Qt 6** con los módulos:
  - `Qt6::Widgets` *(Interfaz gráfica nativa)*
  - `Qt6::PrintSupport` *(Generación y exportación de tickets PDF)*
  - `Qt6::Network` *(Utilidades de soporte)*

---

## 🚀 Compilación y ejecución

### 1. Compilar desde la raíz del proyecto

```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug -j4
2. Ejecutar la aplicaciónSistema OperativoComando de ejecuciónmacOS / Linux./cmake-build-debug/LaPulpeWindowscmake-build-debug\LaPulpe.exe🔐 Inicio de sesiónEn un entorno limpio (cuando aún no existe el archivo usuarios.json), el sistema inicializa automáticamente dos cuentas base para operar de inmediato:UsuarioContraseñaRol asignadoadminadmin123🛡️ Administradorcajero1cajero123🧾 Cajero[!TIP]Puedes hacer clic en el botón Ver para inspeccionar la contraseña escrita o presionar Enter directamente para ingresar. Si usuarios.json ya existe, el sistema respetará las credenciales guardadas.🛡️ Roles y control de accesoLa separación de privilegios garantiza que el personal operativo de caja no pueda alterar precios, modificar existencias ni anular transacciones registradas.Capacidad del sistema🛡️ Administrador🧾 CajeroProcesar ventas y emitir tickets✅✅Consultar historial de ventas y exportar CSV/PDF✅✅Crear, editar, desactivar y reactivar productos✅❌Anular ventas y ejecutar reintegros de stock✅❌Crear, editar y gestionar cuentas de usuario✅❌Auditar el historial de cambios en inventario✅❌🧭 Guía de uso1. Módulo de VentasBúsqueda rápida: Filtra el catálogo por nombre, código o categoría.Armado de orden: Agrega productos con + y ajusta cantidades con + / -.Descuentos y cobro: Aplica descuentos opcionales (monto fijo o porcentaje) e ingresa el efectivo recibido.Cálculo automático: El sistema desglosa al instante subtotal, descuento, IVA incluido, total y vuelto.Confirmación: Al presionar Procesar Venta, se valida el stock activo, se persiste la venta, se rebaja el inventario de forma atómica, se muestra el comprobante en pantalla y se respalda en Documentos/UNA Pulpe/Tickets/ (TXT y opción PDF).2. Módulo de Inventario (Solo Administrador)Gestión completa: Alta, edición, desactivación y reactivación de productos.Borrado lógico: Los artículos desactivados conservan su registro histórico para mantener intactas las facturas emitidas anteriormente.Auditoría de cambios: Bitácora detallada de creaciones, ediciones, cambios de estado y reintegros por anulación.3. Historial de VentasFiltros y búsqueda: Localiza operaciones por ID, cajero o rango de fechas.Exportación: Descarga comprobantes individuales en PDF o exporta reportes completos en CSV (con codificación UTF-8 + BOM para abrir directamente en Excel sin caracteres corruptos).Anulación controlada (Solo Administrador): Marca la factura como Anulada, la excluye del cierre financiero, devuelve el stock una única vez y deja trazabilidad REINTEGRO en auditoría.4. Gestión de Usuarios (Solo Administrador)Administración segura: Creación y edición de cuentas (contraseña mínima de 6 caracteres). Al editar, deja la clave en blanco si deseas mantener la actual.Protección contra bloqueo: El sistema impide que el administrador desactive su propia sesión o deje la aplicación sin administradores activos.🏗️ Arquitectura y seguridadPlaintext┌─────────────────────────────────────────────────────────────────┐
│                   CAPA DE PRESENTACIÓN (Qt 6)                   │
│   VentanaLogin · VentanaPrincipal · Ventas · Inventario · UI    │
└───────────────────────────────┬─────────────────────────────────┘
                                │ Señales y Slots (Sincronización)
┌───────────────────────────────▼─────────────────────────────────┐
│                  CAPA DE SERVICIOS Y DOMINIO                    │
│   Autenticación · Ventas · Inventario · Tickets · Reportes      │
│   CalculadoraVenta · Modelos Encapsulados · IGeneradorTicket    │
└───────────────────────────────┬─────────────────────────────────┘
                                │ Transacciones Atómicas
┌───────────────────────────────▼─────────────────────────────────┐
│               CAPA DE PERSISTENCIA (GestorJSON)                 │
│     QLockFile (Concurrencia) + QSaveFile (Escritura Segura)     │
└─────────────────────────────────────────────────────────────────┘
Garantías técnicas implementadasTransacciones atómicas ("todo o nada"): Las ventas registran primero la factura y luego descuentan el inventario; si ocurre un error en disco, la operación se revierte automáticamente.Cifrado PBKDF2-SHA256: Las claves se guardan exclusivamente como hashes con sal (pbkdf2$iteraciones$salt$hash). Si detecta claves heredadas en texto plano o SHA-256, las migra automáticamente tras un inicio de sesión exitoso.Concurrencia y codificación: Uso de QLockFile, QSaveFile y estándar UTF-8 transversal en toda la aplicación.💾 Rutas de datos y recuperaciónLos archivos de persistencia se almacenan de forma automática en el directorio de datos del sistema:macOS: ~/Library/Application Support/UNA/LaPulpe/Windows: %APPDATA%\UNA\LaPulpe\ArchivoPropósitoproductos.jsonCatálogo maestro y existencias actuales.ventas.jsonHistorial de facturas y estado de anulaciones.usuarios.jsonCredenciales cifradas y roles del personal.auditoria_productos.jsonBitácora cronológica de movimientos de inventario.*.lockBloqueos temporales para evitar corrupción por escrituras simultáneas.[!IMPORTANT]Evita modificar manualmente los archivos .json mientras el programa esté abierto.Dado que los hashes PBKDF2 son unidireccionales, no es posible descifrar la clave anterior:Inicia sesión con otra cuenta de Administrador y restablécela desde el módulo Usuarios.Si no tienes otra cuenta administrativa, cierra el programa, respalda tu archivo usuarios.json y renómbralo para regenerar los usuarios por defecto:Bashcd "$HOME/Library/Application Support/UNA/LaPulpe"
cp usuarios.json usuarios.json.bak
mv usuarios.json usuarios.json.old
Si usuarios.json existe pero está vacío, el sistema no crea usuarios automáticamente por seguridad. Para forzar la carga de usuarios de prueba al iniciar:BashLAPULPE_DEMO_MODE=1 ./cmake-build-debug/LaPulpe
# O alternativamente:
LAPULPE_SEED_USERS=1 ./cmake-build-debug/LaPulpe
🛠️ Solución de problemas frecuentesProblemaCausa probable y soluciónEl proyecto no compilaVerifica que Qt 6 esté configurado en CMake y cuente con Widgets, PrintSupport y Network.No aparece el inventarioComprueba que la carpeta de datos tenga permisos de escritura o que productos.json no esté vacío/corrupto.No permite iniciar sesiónVerifica que el usuario esté activo y que no exista un archivo .lock residual tras un cierre forzoso.No se guarda el respaldo TXTRevisa permisos de escritura en Documentos/UNA Pulpe/Tickets/. La venta confirmada en el sistema permanece intacta.
