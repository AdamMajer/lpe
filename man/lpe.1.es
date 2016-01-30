.TH LPE 1 "23 November 1999"
.SH NAME
lpe \- Editor ligero para programadores
.SH SYNOPSIS
\fBlpe\fP [ --help | --version ]
.br
\fBlpe\fP [--mode <mode>] [ -- ] \fIfile\fP
.SH DESCRIPCION
\fILpe\fP es un editor pequeño, vistoso y rápido diseñado para facilitar la
tarea de editar código. LPE provee todas las características que son necesarias
en un buen editor de código, siendo a la vez ligero e intuitivo, que lo hace
muy fácil de usar.
.SS Opciones
.TP
\fB--help\fP
Imprime en pantalla una breve descripción del uso del programa y termina.
Esta debe ser la primera opción de \fBlpe\fP, siendo ignoradas todas las
demás opciones.
.TP
\fB--version\fP
Imprime en la pantalla un mensaje que indica la versión del programa y
termina. Esta debe ser la primera opción de \fBlpe\fP, siendo ignoradas
todas las demás opciones.
.TP
\fB--mode\fP
Especifica el modo de edición a usar, en vez de buscar un modo concreto de
entre todos los disponibles.
.TP
\fB--\fP
Tratar posteriores argumentos como nombres de ficheros, no como opciones.
Esto permite a \fBlpe\fP editar fichero que comiencen por el caracter \-.
.SH NOTAS
Lo que sigue es la lista de las teclas de control usadas en \fBlpe\fP.
Flechas, Inicio, Fin, Borrar, Suprimir y todas las demás hacen lo que deben
de hacer. Algunas funciones básicas, como Inicio o Fin, están disponibles
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
Avanza una página (alternativa a AvPág)
.TP
\fBCtrl-T\fP
Retrocede una página (alternativa a RePág)
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
Inserta el bloque más reciente de lineas borradas
.TP
\fBCtrl-S\fP
Busca una cadena específica en el fichero
.TP
\fBCtrl-A\fP
Repite la última busqueda realizada
.TP
\fBCtrl-F Ctrl-O\fP
Abre un fichero para editar, reemplazando el actual
.TP
\fBCtrl-F Ctrl-S\fP
Grabar la caché a disco
.TP
\fBCtrl-F Ctrl-A\fP
Guardar a disco con un nombre de fichero diferente
.TP
\fBCtrl-F Ctrl-R\fP
Leer un fichero e insertarlo en la posición del cursor
.TP
\fBCtrl-F Ctrl-E\fP
Indicar que la caché no ha sido modificada
.TP
\fBCtrl-B Ctrl-S\fP
Seleccionar el modo de edición
.TP
\fBCtrl-B Ctrl-T\fP
Intercambiar entre tabulados fuertes y tabulados suaves
.TP
\fBCtrl-B Ctrl-A\fP
Activar o Desactivar el sangrado automático
.TP
\fBCtrl-G Ctrl-A\fP
Ir a la primera linea de la caché
.TP
\fBCtrl-G Ctrl-S\fP
Ir a la última linea de la caché
.TP
\fBCtrl-G Ctrl-G\fP
Ir a una linea específica
.TP
\fBCtrl-N Ctrl-R\fP
Introducir un valor para el repetidor de comandos
.TP
\fBCtrl-N Ctrl-T\fP
Multiplicar el repetidor de comandor por cuatro
.TP
\fBCtrl-N Ctrl-O\fP
Iniciar o parar la grabación de un macro
.TP
\fBCtrl-N Ctrl-P\fP
Reproducir la última macro grabada
.TP
\fBCtrl-V Ctrl-V\fP
Pasar toda la caché a través de un comando shell
.TP
\fBCtrl-V Ctrl-A\fP
Pasar toda la caché a través de un script awk
.TP
\fBCtrl-V Ctrl-S\fP
Pasar toda la caché a través de un script sed
.TP
\fBCtrl-V Ctrl-B\fP
Pasar un grupo de lineas a través de un comando shell
.TP
\fBCtrl-V Ctrl-D\fP
Pasar un grupo de lineas a través de un script awk
.TP
\fBCtrl-V Ctrl-F\fP
Pasar un grupo de lineas a través de un script sed
.TP
\fBCtrl-D\fP
Realizar un depurado interno
.TP
\fBCtrl-X\fP
Escribe la caché a disco y termina
.TP
\fB<interrupt>\fP
Termina sin guardar la caché en disco
.TP
\fBCtrl-Z\fP
Detiene la edición y sale al intérprete de comandos
.TP
\fBCtrl-L\fP
Borra y refresca la pantalla completa

\fB<interrupt>\fP se refiere a la tecla de interrupción de tu terminal.
Normalmente suele ser Ctrl-C, pero puede variar según el tipo de terminal.
Por otro lado, Ctrl-Z está fijada, a no ser que hayas definido otra tecla
con el mismo propósito en tu terminal.

.SH MODULOS

A pesar que \fBlpe\fP es pequeño, tiene la capacidad de realizar acciones
más avanzadas gracias a una característica llamada modos de caché. Los modos
de caché permiten a \fBlpe\fP actuar de diferente manera según en el
lenguaje que estés programando. Están implementado a traves de módulos de
lenguaje, que son cargados en tiempo de ejecución por \fBlpe\fP para manejar
la caché a la que son aplicados.

Todos los módulos de lenguaje deben de situarse en el directorio de módulos.
Este directorio está definido en la variable de entorno
\fBLPE_MODULE_PATH\fP, y contiene una lista de directorios separados por el
signo punto y coma (;). Si esta variable no está definida, \fBlpe\fP
buscará los módulos es los siguientes directorios, y por este orden

\fB$HOME/.lpe\fP
\fB/usr/local/lib/lpe\fP
\fB/usr/lib/lpe\fP
\fB/etc/lpe\fP

Recuerda que el directorio \fB/etc/lpe\fP es antiguo, obsoleto y no es estándar.
No debe ser usado para poner los módulos. Como dichos módulos son binarios y
específicos a la arquitectura de la máquina que los usa, deben estar
situados en los directorios \fBlib\fP mencionados arriba.

Estos directorios son procesados en tiempo de ejecución, y cualquier fichero
normal que se encuentre en ese directorio son interpretados como potenciales
módulos de lenguajes a usar en \fBlpe\fP. Los subdirectorios no son
procesados. Un fichero será asignado al primer módulo de lenguaje encontrado que
concuerde con ese fichero. Esto quiere decir que un módulo encontrado en el
directorio HOME de un usuario tiene preferencia sobre los módulos de los
directorios lib del sistema.

.SH "Ver también"
emacs(1), pico(1)
.SH AUTOR
Chris Smith, cd_smith@ou.edu
.SH BUGS
Muchos de ellos -- puedes ver una lista parcial en el fichero BUGS de la
distribución. No estoy pensando en actualizar esta página de manual tan a
menudo como para mantenerla igualada con la lista de fallos, así que no
trataré de listarlos aqui.

