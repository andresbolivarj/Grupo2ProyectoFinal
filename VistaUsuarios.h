#ifndef VISTAUSUARIOS_H
#define VISTAUSUARIOS_H

//libreria de QT
#include <QWidget>
#include <QTableWidget>     
#include <QLineEdit>     
#include <QPushButton>     
#include <QLabel>           
#include <QVBoxLayout>  
#include <QHBoxLayout>     
#include <QHeaderView>      
#include <QMessageBox>     
#include <QDialog>          
#include <QComboBox>        

//librerias estandar del proyecto
#include <vector>     
#include <memory>         
#include "Usuario.h"       
#include "GestorJson.h"     

class VistaUsuarios : public QWidget {
    Q_OBJECT

private:
    //Componentes visuales de la interfaz gráfica
    QLineEdit* txtBuscar;
    QTableWidget* tablaUsuarios;
    QPushButton* btnAgregar;
    QPushButton* btnEditar;
    QPushButton* btnEliminar;

    //Estructura de datos
    // Lista en memoria que almacena los usuarios mediante punteros
    std::vector<std::shared_ptr<Usuario>> listaUsuarios;

public:

    explicit VistaUsuarios(QWidget* parent = nullptr);

    void actualizar();

protected:
    //Sobrecarga del evento que se ejecuta automáticamente cuando la vista se muestra en pantalla. event Evento de visualización de Qt
    void showEvent(QShowEvent* event) override;

private:
    //Métodos privados y interacción

    //Inicializa y organiza los componentes visuales en la ventana (Layouts)
    void configurarInterfaz();

    //Lee los usuarios desde la fuente de datos (GestorJSON) y los carga en la lista.
    void cargarDatos();

    //Limpia y llena la tabla gráfica con los datos de la lista de usuarios.
    void renderizarTabla();

    //Filtra las filas de la tabla según el texto ingresado en el campo de búsqueda.
    void filtrarTabla();

    //abre la ventana emergente para crear un nuevo usuario o editar uno
    void abrirFormularioUsuario(std::shared_ptr<Usuario> usuarioExistente = nullptr);

    //eliminación del usuario seleccionado en la tabla
    void eliminarUsuarioSeleccionado();
};

#endif // VISTAUSUARIOS_H
//LISTO