.TH LPE 1 "23 November 1999"
.SH NAME
lpe \- Editor ligero para programadores
.SH SYNOPSIS
\fBlpe\fP [ --help | --version ]
.br
\fBlpe\fP [--mode <mode>] [ -- ] \fIfile\fP
.SH DESCRIPCION
\fILpe\fP es un editor peque�o, vistoso y r�pido dise�ado para facilitar la
tarea de editar c�digo. LPE provee todas las caracter�sticas que son necesarias
en un buen editor de c�digo, siendo a la vez ligero e intuitivo, que lo hace
muy f�cil de usar.
.SS Opciones
.TP
\fB--help\fP
Imprime en pantalla una breve descripci�n del uso del programa y termina.
Esta debe ser la primera opci�n de \fBlpe\fP, siendo ignoradas todas las
dem�s opciones.
.TP
\fB--version\fP
Imprime en la pantalla un mensaje que indica la versi�n del programa y
termina. Esta debe ser la primera opci�n de \fBlpe\fP, siendo ignoradas
todas las dem�s opciones.
.TP
\fB--mode\fP
Especifica el modo de edici�n a usar, en vez de buscar un modo concreto de
entre todos los disponibles.
.TP
\fB--\fP
Tratar posteriores argumentos como nombres de ficheros, no como opciones.
Esto permite a \fBlpe\fP editar fichero que comiencen por el caracter \-.
.SH NOTAS
Lo que sigue es la lista de las teclas de control usadas en \fBlpe\fP.
Flechas, Inicio, Fin, Borrar, Suprimir y todas las dem�s hacen lo que deben
de hacer. Algunas funciones b�sicas, como Inicio o Fin, est�n disponibles
como teclas de control, ya que pueden no estar disponibles en algunos
sistemas.
.SS Teclas de control
.TP
\fBCtrl-Q\fP
Mueve el cursor al pincipio de la linea (alternativa a Inicio)
.TP
\fBCtrl-W\fP
Mueve el cursor al final de la linea (alternativa a Fin)
.TP
\fBCtrl-R\fP
Avanza una p�gina (alternativa a AvP�g)
.TP
\fBCtrl-T\fP
Retrocede una p�gina (alternativa a ReP�g)
.TP
\fBCtrl-O\fP
Avanza hasta la siguiente palabra
.TP
\fBCtrl-P\fP
Retrocede a la palabra anterior
.TP
\fBCtrl-K\fP
Borra la linea actual.
.TP
\fBCtrl-Y\fP o \fBCtrl-U\fP
Inserta el bloque m�s reciente de lineas borradas
.TP
\fBCtrl-S\fP
Busca una cadena espec�fica en el fichero
.TP
\fBCtrl-A\fP
Repite la �ltima busqueda realizada
.TP
\fBCtrl-F Ctrl-O\fP
Abre un fichero para editar, reemplazando el actual
.TP
\fBCtrl-F Ctrl-S\fP
Grabar la cach� a disco
.TP
\fBCtrl-F Ctrl-A\fP
Guardar a disco con un nombre de fichero diferente
.TP
\fBCtrl-F Ctrl-R\fP
Leer un fichero e insertarlo en la posici�n del cursor
.TP
\fBCtrl-F Ctrl-E\fP
Indicar que la cach� no ha sido modificada
.TP
\fBCtrl-B Ctrl-S\fP
Seleccionar el modo de edici�n
.TP
\fBCtrl-B Ctrl-T\fP
Intercambiar entre tabulados fuertes y tabulados suaves
.TP
\fBCtrl-B Ctrl-A\fP
Activar o Desactivar el sangrado autom�tico
.TP
\fBCtrl-G Ctrl-A\fP
Ir a la primera linea de la cach�
.TP
\fBCtrl-G Ctrl-S\fP
Ir a la �ltima linea de la cach�
.TP
\fBCtrl-G Ctrl-G\fP
Ir a una linea espec�fica
.TP
\fBCtrl-N Ctrl-R\fP
Introducir un valor para el repetidor de comandos
.TP
\fBCtrl-N Ctrl-T\fP
Multiplicar el repetidor de comandor por cuatro
.TP
\fBCtrl-N Ctrl-O\fP
Iniciar o parar la grabaci�n de un macro
.TP
\fBCtrl-N Ctrl-P\fP
Reproducir la �ltima macro grabada
.TP
\fBCtrl-V Ctrl-V\fP
Pasar toda la cach� a trav�s de un comando shell
.TP
\fBCtrl-V Ctrl-A\fP
Pasar toda la cach� a trav�s de un script awk
.TP
\fBCtrl-V Ctrl-S\fP
Pasar toda la cach� a trav�s de un script sed
.TP
\fBCtrl-V Ctrl-B\fP
Pasar un grupo de lineas a trav�s de un comando shell
.TP
\fBCtrl-V Ctrl-D\fP
Pasar un grupo de lineas a trav�s de un script awk
.TP
\fBCtrl-V Ctrl-F\fP
Pasar un grupo de lineas a trav�s de un script sed
.TP
\fBCtrl-D\fP
Realizar un depurado interno
.TP
\fBCtrl-X\fP
Escribe la cach� a disco y termina
.TP
\fB<interrupt>\fP
Termina sin guardar la cach� en disco
.TP
\fBCtrl-Z\fP
Detiene la edici�n y sale al int�rprete de comandos
.TP
\fBCtrl-L\fP
Borra y refresca la pantalla completa

\fB<interrupt>\fP se refiere a la tecla de interrupci�n de tu terminal.
Normalmente suele ser Ctrl-C, pero puede variar seg�n el tipo de terminal.
Por otro lado, Ctrl-Z est� fijada, a no ser que hayas definido otra tecla
con el mismo prop�sito en tu terminal.

.SH MODULOS

A pesar que \fBlpe\fP es peque�o, tiene la capacidad de realizar acciones
m�s avanzadas gracias a una caracter�stica llamada modos de cach�. Los modos
de cach� permiten a \fBlpe\fP actuar de diferente manera seg�n en el
lenguaje que est�s programando. Est�n implementado a traves de m�dulos de
lenguaje, que son cargados en tiempo de ejecuci�n por \fBlpe\fP para manejar
la cach� a la que son aplicados.

Todos los m�dulos de lenguaje deben de situarse en el directorio de m�dulos.
Este directorio est� definido en la variable de entorno
\fBLPE_MODULE_PATH\fP, y contiene una lista de directorios separados por el
signo punto y coma (;). Si esta variable no est� definida, \fBlpe\fP
buscar� los m�dulos es los siguientes directorios, y por este orden

\fB$HOME/.lpe\fP
\fB/usr/local/lib/lpe\fP
\fB/usr/lib/lpe\fP
\fB/etc/lpe\fP

Recuerda que el directorio \fB/etc/lpe\fP es antiguo, obsoleto y no es est�ndar.
No debe ser usado para poner los m�dulos. Como dichos m�dulos son binarios y
espec�ficos a la arquitectura de la m�quina que los usa, deben estar
situados en los directorios \fBlib\fP mencionados arriba.

Estos directorios son procesados en tiempo de ejecuci�n, y cualquier fichero
normal que se encuentre en ese directorio son interpretados como potenciales
m�dulos de lenguajes a usar en \fBlpe\fP. Los subdirectorios no son
procesados. Un fichero ser� asignado al primer m�dulo de lenguaje encontrado que
concuerde con ese fichero. Esto quiere decir que un m�dulo encontrado en el
directorio HOME de un usuario tiene preferencia sobre los m�dulos de los
directorios lib del sistema.

.SH "Ver tambi�n"
emacs(1), pico(1)
.SH AUTOR
Chris Smith, cd_smith@ou.edu
.SH BUGS
Muchos de ellos -- puedes ver una lista parcial en el fichero BUGS de la
distribuci�n. No estoy pensando en actualizar esta p�gina de manual tan a
menudo como para mantenerla igualada con la lista de fallos, as� que no
tratar� de listarlos aqui.

