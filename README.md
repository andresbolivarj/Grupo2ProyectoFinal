# UNA Pulpe — Sistema de Punto de Venta (POS)

Sistema de escritorio para ventas, control de inventario y auditoría desarrollado en **C++17** y **Qt 6**.

## Equipo de desarrollo

- **andresbolivarj** — https://github.com/andresbolivarj
- **alisongonzalezv** — https://github.com/alisongonzalezv
- **mmora5852-crypto** — https://github.com/mmora5852-crypto
- **adrielvillalobostoruno-coder** — https://github.com/adrielvillalobostoruno-coder

## Requisitos del sistema

- **CMake:** versión 3.16 o superior.
- **Compilador:** soporte para C++17 (Clang, GCC o MSVC).
- **Qt 6:** módulos `Widgets`, `PrintSupport` y `Network`.
- No requiere librerías externas adicionales.

## Compilación y ejecución

1. **Compilar desde la carpeta raíz del proyecto:**
   - `cmake -S . -B cmake-build-debug`
   - `cmake --build cmake-build-debug -j4`
2. **Ejecutar el programa:**
   - **macOS / Linux:** `./cmake-build-debug/LaPulpe`
   - **Windows:** `cmake-build-debug\LaPulpe.exe`

## Inicio de sesión y credenciales

En un entorno limpio (sin archivo `usuarios.json`), el sistema crea automáticamente estas dos cuentas:

- **Administrador:** usuario `admin` / contraseña `admin123`
- **Cajero:** usuario `cajero1` / contraseña `cajero123`

Si el archivo `usuarios.json` ya existe, se utilizan las credenciales guardadas. Puedes presionar el botón **Ver** para mostrar u ocultar la contraseña o pulsar `Enter` para entrar.

## Permisos por rol

- **Administrador:**
  - Registrar ventas y generar comprobantes (TXT y PDF).
  - Consultar el historial de ventas y exportar reportes CSV.
  - Crear, editar, desactivar y reactivar productos.
  - Anular ventas y ejecutar el reintegro de stock.
  - Administrar cuentas de usuario y contraseñas.
  - Ver la bitácora de auditoría del inventario.
- **Cajero:**
  - Registrar ventas y generar comprobantes.
  - Consultar el historial de ventas.
  - *Restringido:* No puede alterar precios, modificar productos, anular ventas, gestionar usuarios ni ver la auditoría.

## Uso del sistema

### 1. Ventas
- Busca productos por nombre, código o categoría y agrégalos a la orden con el botón `+`.
- Ajusta cantidades (`+` / `-`), aplica descuentos opcionales (por monto o porcentaje) e ingresa el efectivo recibido para calcular el vuelto automáticamente.
- Al confirmar (**Procesar Venta**), el sistema valida stock activo, guarda la factura, descuenta el inventario de forma atómica y guarda una copia del recibo en `Documentos/UNA Pulpe/Tickets/`.

### 2. Inventario (Solo Administrador)
- Permite crear, editar, desactivar y reactivar productos.
- **Borrado lógico:** Los productos desactivados permanecen como inactivos para no afectar el historial de ventas anteriores.
- Incluye la pestaña **Historial de Cambios** con la auditoría de movimientos y reintegros.

### 3. Historial de ventas
- Permite buscar por ID o usuario, filtrar por fecha, ver comprobantes, descargar PDF y exportar a **CSV** (UTF-8 con BOM compatible con Excel).
- **Anulación (Solo Administrador):** Marca la venta como *Anulada*, la excluye de los totales, devuelve el stock una única vez y registra un movimiento `REINTEGRO`.

### 4. Usuarios (Solo Administrador)
- Permite crear, editar, desactivar y reactivar usuarios (contraseña mínima de 6 caracteres).
- Al editar un usuario, deja el campo de contraseña vacío si deseas conservar la clave actual.
- Impide desactivar tu propia sesión o dejar el sistema sin administradores activos.

## Arquitectura y persistencia

- **Arquitectura en capas:** Separación limpia entre vistas Qt, capa de servicios (`Autenticación`, `Ventas`, `Inventario`, `Tickets`, `Reportes`), cálculo (`CalculadoraVenta`) y almacenamiento (`GestorJSON`).
- **Seguridad y transacciones:** Guardado atómico con `QSaveFile`, bloqueos de concurrencia con `QLockFile`, soporte completo **UTF-8** y contraseñas cifradas con **PBKDF2-SHA256** (con migración automática desde hashes anteriores).
- **Sincronización:** Las vistas se actualizan al instante mediante señales y *slots* de Qt tras cada venta, anulación o edición de producto.

### Ubicación de archivos de datos
- **macOS:** `~/Library/Application Support/UNA/LaPulpe/`
- **Windows:** `%APPDATA%\UNA\LaPulpe\`
- **Archivos:** `productos.json`, `ventas.json`, `usuarios.json`, `auditoria_productos.json` y bloqueos temporales `*.lock`. *(Evita editarlos manualmente con el programa abierto).*

### Recuperación de contraseña y modo demo
- **Si olvidaste la clave de administrador:** Restablécela desde otro usuario administrador o, con la aplicación cerrada, respalda y renombra `usuarios.json` para que se regeneren los usuarios por defecto.
- **Modo demo:** Si `usuarios.json` existe vacío, puedes forzar la creación de usuarios de ejemplo ejecutando el programa con la variable de entorno `LAPULPE_DEMO_MODE=1` o `LAPULPE_SEED_USERS=1`.

## Solución de problemas

- **El proyecto no compila:** Verifica que Qt 6 esté configurado en CMake con los módulos `Widgets`, `PrintSupport` y `Network`.
- **No aparece el inventario o falla el login:** Comprueba permisos de escritura en la carpeta de datos y revisa que no quede un archivo `.lock` residual tras un cierre inesperado.
- **No se guarda el respaldo TXT:** Revisa permisos en `Documentos/UNA Pulpe/Tickets/`. La venta confirmada en el sistema se conserva intacta.
