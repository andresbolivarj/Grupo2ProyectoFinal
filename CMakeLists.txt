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

add_executable(LaPulpe
        main.cpp
        VentanaLogin.h
        VentanaLogin.cpp
        VentanaPrincipal.h
        VentanaPrincipal.cpp
        VistaVentas.h
        VistaVentas.cpp
        CategoriasProducto.h
        VistaInventario.h
        VistaInventario.cpp
        VistaHistorial.h
        VistaHistorial.cpp
        GestorJSON.h
        GestorJSON.cpp
        ModelosDatos.h
        IGeneradorTicket.h
        TicketPdf.h
        TicketPdf.cpp
        recursos.qrc
        VistaUsuarios.cpp
        VistaUsuarios.h
)

target_link_libraries(LaPulpe PRIVATE Qt6::Widgets Qt6::PrintSupport)

include(CTest)
if(BUILD_TESTING)
    add_executable(gestor_json_test
            tests/gestor_json_test.cpp
            GestorJSON.cpp
            GestorJSON.h
            ModelosDatos.h
            TicketPdf.cpp
            TicketPdf.h
            IGeneradorTicket.h
            Usuario.h
    )
    target_include_directories(gestor_json_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(gestor_json_test PRIVATE Qt6::Core Qt6::PrintSupport)
    add_test(NAME gestor_json_test COMMAND gestor_json_test)
    set_tests_properties(gestor_json_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
