.TH LPE 1 "12 grudnia 1998"
.SH NAZWA
lpe \- Ma�y edytor programisty
.SH SK�ADNIA
\fBlpe\fP [ --help | --version ]
.br
\fBlpe\fP [ -- ] \fIplik\fP
.SH OPIS
\fILpe\fP jest ma�ym, pe�noekranowym edytorem tekstowym zaprojektowanym do
prostej i �atwej w obs�udze edycji kodu. Edytor zawiera mo�liwo�ci przeszukiwania tekstu,
wycinania i wklejania zaznaczonych blok�w, a tak�e inne standardowe mo�liwo�ci
edytora, kt�re mo�na spotka� w takich programach, jak emacs(1) i pico(1).
.SS OPCJE
.TP
\fB--help\fP
Wy�wietla kr�tki opis sposobu u�ycia programu. Opcja ta musi by� podana jako
pierwsza. Pozosta�e opcje s� wtedy ignorowane.
.TP
\fB--version\fP
Wy�wietla numer wersji programu. Opcja ta musi by� podana jako pierwsza.
Pozosta�e opcje s� wtedy ignorowane.
.TP
\fB--\fP
Podanie dw�ch minus�w spowoduje, �e nast�pne argumenty b�d� traktowane jako
nazwy plik�w, a nie opcje. To pozwala u�ywa� \fBlpe\fP do edycji plik�w o nazwach
zaczynaj�cych si� od minusa (\-).
.SH INNE
Tak jak wspomniano wy�ej lpe jest ma�ym i efektywnym edytorem, lecz nie
oferuje za wielu nadzwyczajnych udogodnie�.  Dlatego nie powinno by� trudno
nauczy� si� go obs�ugiwa�, zak�adaj�c, �e u�ytkownik b�dzie chcia� pozna�
tych kilka prostych komend.
.SS Klawisze - Komendy
.TP
\fBUp\fP lub \fBAlt-K\fP
Id� do poprzedniej linii tekstu
.TP
\fBDown\fP lub \fBAlt-J\fP
Id� do nast�pnej linii tekstu
.TP
\fBLeft\fP lub \fBAlt-H\fP
Przesu� kursor w lewo o jedn� kolumn�
.TP
\fBRight\fP lub \fBAlt-L\fP
Przesu� kursor w prawo o jedn� kolumn�
.TP
\fBHome\fP lub \fBAlt-0\fP
Przesu� kursor na pocz�tek linii
.TP
\fBEnd\fP lub \fBAlt-$\fP
Przesu� kursor na koniec linii
.TP
\fBPageUp\fP lub \fBAlt-B\fP
Przesu� ekran o jedn� stron� dalej
.TP
\fBPageDn\fP lub \fBAlt-F\fP
Przesu� ekran o jedn� stron� wstecz
.TP
\fBAlt-A\fP
Przesu� kursor na pocz�tek bufora
.TP
\fBAlt-S\fP
Przesu� kursor na koniec bufora
.TP
\fBCtrl-S\fP
Szukaj okre�lonego ci�gu znak�w w pliku
.TP
\fBCtrl-K\fP
Skasuj bie��c� lini�
.TP
\fBCtrl-Y\fP lub \fBCtrl-U\fP
Wstaw ostatnio kasowany blok linii
.TP
\fBCtrl-T\fP
Prze��cz mi�dzy kr�tkimi (4-znakowymi) i d�ugimi (8-znakowymi) tabulacjami
.TP
\fBCtrl-O\fP
Otw�rz nowy plik w edytorze
.TP
\fBCtrl-W\fP
Zapisz bufor na dysk
.TP
\fBCtrl-Q\fP
Zapisz jako inny plik
.TP
\fBCtrl-E\fP
Pomi� zmiany bufora
.TP
\fBCtrl-X\fP
Zapisz bufor na dysk i zako�cz dzia�anie
.TP
\fBCtrl-D\fP
Uruchom komend� debuggera.
.TP
\fB<interrupt>\fP
Zako�cz bez zapami�tywania niczego na dysk
.TP
\fBCtrl-Z\fP
Zawie� dzia�anie edytora i przejd� do linii komend

\fB<interrupt>\fP odnosi si� do wci�ni�cia klawisza powoduj�cego
natychmiastowe przerwanie dzia�ania programu. To jest zwykle
Ctrl-C, ale mo�e si� zmienia� w zale�no�ci od ustawie� terminala.
Nie dotyczy to klawisza Ctlr-Z, kt�ry jest sta�y, niezale�nie
od ustawie� terminalowego klawisza wstrzymuj�cego program.

.SH "PATRZ TAK�E"
emacs(1), pico(1)
.SH AUTOR
Chris Smith, cd_smith@ou.edu
.SH B��DY
Jest ich sporo - ich cz�� znajdziesz w pliku BUGS w katalogu dystrybucji.
Nie planuje si� cz�stej zmiany tej�e strony podr�cznika, wi�c b��dy w
programie w og�le nie b�d� tutaj wpisywane.
