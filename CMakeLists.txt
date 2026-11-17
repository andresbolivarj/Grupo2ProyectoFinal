cmake_minimum_required(VERSION 3.16)
project(LaPulpe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport Network)

add_executable(LaPulpe
        main.cpp

        # Dominio y utilidades
        ModelosDatos.h
        Usuario.h
        CategoriasProducto.h
        Estilos.h

        # Persistencia
        GestorJSON.h
        GestorJSON.cpp

        # Lógica de negocio (servicios)
        CalculadoraVenta.h
        CalculadoraVenta.cpp
        ServicioAutenticacion.h
        ServicioAutenticacion.cpp
        ServicioInventario.h
        ServicioInventario.cpp
        ServicioReportes.h
        ServicioReportes.cpp
        ServicioTickets.h
        ServicioTickets.cpp
        ServicioUsuarios.h
        ServicioUsuarios.cpp
        ServicioVentas.h
        ServicioVentas.cpp

        # Comprobantes (abstracción + implementaciones concretas)
        IGeneradorTicket.h
        TicketPdf.h
        TicketPdf.cpp

        # Interfaz
        VentanaLogin.h
        VentanaLogin.cpp
        VentanaPrincipal.h
        VentanaPrincipal.cpp
        VistaVentas.h
        VistaVentas.cpp
        VistaInventario.h
        VistaInventario.cpp
        VistaHistorial.h
        VistaHistorial.cpp
        VistaUsuarios.h
        VistaUsuarios.cpp

        recursos.qrc
)

# En Windows se usa el punto de entrada de Qt (WinMain); en macOS se mantiene
# un ejecutable plano para no depender de una estructura de bundle.
set_target_properties(LaPulpe PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE FALSE
)

target_link_libraries(LaPulpe PRIVATE
        Qt6::Widgets
        Qt6::PrintSupport
        Qt6::Network
)

# Codificación explícita de los literales UTF-8 en todos los compiladores.
if(MSVC)
    target_compile_options(LaPulpe PRIVATE /utf-8)
else()
    target_compile_options(LaPulpe PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()
