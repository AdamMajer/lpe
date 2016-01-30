.TH LPE 1 "12 grudnia 1998"
.SH NAZWA
lpe \- Ma³y edytor programisty
.SH SK£ADNIA
\fBlpe\fP [ --help | --version ]
.br
\fBlpe\fP [ -- ] \fIplik\fP
.SH OPIS
\fILpe\fP jest ma³ym, pe³noekranowym edytorem tekstowym zaprojektowanym do
prostej i ³atwej w obs³udze edycji kodu. Edytor zawiera mo¿liwo¶ci przeszukiwania tekstu,
wycinania i wklejania zaznaczonych bloków, a tak¿e inne standardowe mo¿liwo¶ci
edytora, które mo¿na spotkaæ w takich programach, jak emacs(1) i pico(1).
.SS OPCJE
.TP
\fB--help\fP
Wy¶wietla krótki opis sposobu u¿ycia programu. Opcja ta musi byæ podana jako
pierwsza. Pozosta³e opcje s± wtedy ignorowane.
.TP
\fB--version\fP
Wy¶wietla numer wersji programu. Opcja ta musi byæ podana jako pierwsza.
Pozosta³e opcje s± wtedy ignorowane.
.TP
\fB--\fP
Podanie dwóch minusów spowoduje, ¿e nastêpne argumenty bêd± traktowane jako
nazwy plików, a nie opcje. To pozwala u¿ywaæ \fBlpe\fP do edycji plików o nazwach
zaczynaj±cych siê od minusa (\-).
.SH INNE
Tak jak wspomniano wy¿ej lpe jest ma³ym i efektywnym edytorem, lecz nie
oferuje za wielu nadzwyczajnych udogodnieñ.  Dlatego nie powinno byæ trudno
nauczyæ siê go obs³ugiwaæ, zak³adaj±c, ¿e u¿ytkownik bêdzie chcia³ poznaæ
tych kilka prostych komend.
.SS Klawisze - Komendy
.TP
\fBUp\fP lub \fBAlt-K\fP
Id¼ do poprzedniej linii tekstu
.TP
\fBDown\fP lub \fBAlt-J\fP
Id¼ do nastêpnej linii tekstu
.TP
\fBLeft\fP lub \fBAlt-H\fP
Przesuñ kursor w lewo o jedn± kolumnê
.TP
\fBRight\fP lub \fBAlt-L\fP
Przesuñ kursor w prawo o jedn± kolumnê
.TP
\fBHome\fP lub \fBAlt-0\fP
Przesuñ kursor na pocz±tek linii
.TP
\fBEnd\fP lub \fBAlt-$\fP
Przesuñ kursor na koniec linii
.TP
\fBPageUp\fP lub \fBAlt-B\fP
Przesuñ ekran o jedn± stronê dalej
.TP
\fBPageDn\fP lub \fBAlt-F\fP
Przesuñ ekran o jedn± stronê wstecz
.TP
\fBAlt-A\fP
Przesuñ kursor na pocz±tek bufora
.TP
\fBAlt-S\fP
Przesuñ kursor na koniec bufora
.TP
\fBCtrl-S\fP
Szukaj okre¶lonego ci±gu znaków w pliku
.TP
\fBCtrl-K\fP
Skasuj bie¿±c± liniê
.TP
\fBCtrl-Y\fP lub \fBCtrl-U\fP
Wstaw ostatnio kasowany blok linii
.TP
\fBCtrl-T\fP
Prze³±cz miêdzy krótkimi (4-znakowymi) i d³ugimi (8-znakowymi) tabulacjami
.TP
\fBCtrl-O\fP
Otwórz nowy plik w edytorze
.TP
\fBCtrl-W\fP
Zapisz bufor na dysk
.TP
\fBCtrl-Q\fP
Zapisz jako inny plik
.TP
\fBCtrl-E\fP
Pomiñ zmiany bufora
.TP
\fBCtrl-X\fP
Zapisz bufor na dysk i zakoñcz dzia³anie
.TP
\fBCtrl-D\fP
Uruchom komendê debuggera.
.TP
\fB<interrupt>\fP
Zakoñcz bez zapamiêtywania niczego na dysk
.TP
\fBCtrl-Z\fP
Zawie¶ dzia³anie edytora i przejd¼ do linii komend

\fB<interrupt>\fP odnosi siê do wci¶niêcia klawisza powoduj±cego
natychmiastowe przerwanie dzia³ania programu. To jest zwykle
Ctrl-C, ale mo¿e siê zmieniaæ w zale¿no¶ci od ustawieñ terminala.
Nie dotyczy to klawisza Ctlr-Z, który jest sta³y, niezale¿nie
od ustawieñ terminalowego klawisza wstrzymuj±cego program.

.SH "PATRZ TAK¯E"
emacs(1), pico(1)
.SH AUTOR
Chris Smith, cd_smith@ou.edu
.SH B£ÊDY
Jest ich sporo - ich czê¶æ znajdziesz w pliku BUGS w katalogu dystrybucji.
Nie planuje siê czêstej zmiany tej¿e strony podrêcznika, wiêc b³êdy w
programie w ogóle nie bêd± tutaj wpisywane.
