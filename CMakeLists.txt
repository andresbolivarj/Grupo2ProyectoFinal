cmake_minimum_required(VERSION 3.16)
project(LaPulpe)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport)

#Quitar los # según vayan necesitando sus partes
add_executable(LaPulpe
        main.cpp
        #VentanaLogin.h
        #VentanaLogin.cpp
        VentanaPrincipal.h
        VentanaPrincipal.cpp
        #VistaVentas.h
        #VistaVentas.cpp
        CategoriasProducto.h
        VistaInventario.h
        VistaInventario.cpp
        #VistaHistorial.h
        #VistaHistorial.cpp
        GestorJSON.h
        GestorJSON.cpp
        ModelosDatos.h
        IGeneradorTicket.h
        #TicketPdf.h
        #TicketPdf.cpp
        recursos.qrc
        #VistaUsuarios.cpp
        #VistaUsuarios.h
        Usuario.h
)

target_link_libraries(LaPulpe PRIVATE Qt6::Widgets Qt6::PrintSupport)


