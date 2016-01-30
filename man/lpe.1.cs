.TH LPE 1 "12 December 1998"
.SH JMENO
lpe \- Lightweight programmer's editor
.SH POUZITI
\fBlpe\fP [ --help | --version ]
.br
\fBlpe\fP [ -- ] \fIfile\fP
.SH POPIS
\fILpe\fP je miniaturni, visualni, real-time textovy editor vytvoreny pro 
jednodussi editaci kodu bez odcizovani uzivatele z shellu.
.SS Volby
.TP
\fB--help\fP
Vytiskne kratky popis pouziti programu a skonci.  Musi byt uvedeno jako
prvni volba na radce (vsechny ostatni jsou ignorovany).
.TP
\fB--version\fP
Vytiskne zpravu indikujici verzi \fBlpe\fP a skonci.  Musi byt uvedeno jako
prvni volba na radce (vsechny ostatni jsou ignorovany).
.TP
\fB--\fP
Bude povazovat budouci volby jako jmena souboru (ne jako volby).  Umoznuje
editaci souboru zacinajicich znakem '\-'.
.SH POZNAMKY
Jak jiz bylo uvedeno vyse, lpe je designovan jako miniaturni editor, 
coz znamena byt maly, robustni a vykonny, ale neposkytovat mnoho nadbytecnych
funkci. Z tohoto duvodu nemusi byt tezke naucit se s lpe pracovat. Jde vlastne
jen o to, naucit se par zakladnich klaves.
.SS Prikazove klavesy
.TP
\fBSipka nahoru\fP nebo \fBAlt-K\fP
Presun kurzoru na predchozi radku v textu
.TP
\fBSipka dilu\fP nebo \fBAlt-J\fP
Presun kurzoru na nasledujici radku v textu
.TP
\fBSipka vlevo\fP nebo \fBAlt-H\fP
Presun kurzoru vlevo o jeden znak
.TP
\fBSipka vpravo\fP nebo \fBAlt-L\fP
Presun kurzoru vpravo o jeden znak
.TP
\fBHome\fP nebo \fBAlt-0\fP
Presun kurzoru na zacatek radky
.TP
\fBEnd\fP nebo \fBAlt-$\fP
Presun kurzoru na konec radky
.TP
\fBPageUp\fP nebo \fBAlt-B\fP
Posun o jednu stranku nahoru
.TP
\fBPageDn\fP nebo \fBAlt-F\fP
Posun o jednu stranku dolu
.TP
\fBAlt-A\fP
Presun kurzoru na zacatek souboru
.TP
\fBAlt-S\fP
Presun kurzoru na konec souboru
.TP
\fBCtrl-S\fP
Vyhledani zadaneho retezce v souboru
.TP
\fBCtrl-A\fP
Hledani nasledujiciho vyskytu posledniho zadaneho retezce
.TP
\fBCtrl-G\fP
Prechod na specifikovanou radku v souboru
.TP
\fBCtrl-K\fP
Vymazani aktualni radky
.TP
\fBCtrl-Y\fP nebo \fBCtrl-U\fP
Obnoveni (vlozeni) posledniho bloku vymazanych radek
.TP
\fBCtrl-R\fP
Vlozeni souboru na misto, kde stoji kurzor
.TP
\fBCtrl-T\fP
Prepnuti mezi pouzivanim 'hard' a 'soft' tabulatoru
.TP
\fBCtrl-O\fP
Otevreni noveho souboru v editoru
.TP
\fBCtrl-W\fP
Zapis souboru na disk
.TP
\fBCtrl-Q\fP
Ulozeni do alternativniho souboru
.TP
\fBCtrl-E\fP
Zapomenuti modifikace souboru
.TP
\fBCtrl-L\fP
Prekresleni obrazovky
.TP
\fBCtrl-X\fP
Zapis souboru na disk a ukonceni programu
.TP
\fBCtrl-D\fP
Pokud je povoleno debugovani, provedeni interniho debug prikazu
.TP
\fB<interrupt>\fP
Ukonceni programu bez zapisu na disk
.TP
\fBCtrl-Z\fP
Suspendovani (uspani) editoru a navrat do shellu

\fB<interrupt>\fP znamena 'interrupt key' vaseho terminalu.  To je casto Ctrl-C,
ale je to zavisle na typu terminalu.  Na druhou stranu Ctrl-Z je predefinovano,
nedbajic na normalni 'stop key'.

.SH MODULY

Ackoli je \fBlpe\fP vytvareno jako miniaturni editor, ma schopnost provadeni
dokonalejsich akci skrz vlastnost zvanou 'buffer modes'. Buffer modes dovoluji
\fBlpe\fP pracovat ruzne podle programovaciho jazyka aktualniho souboru.
Jsou implementovany skrz jazykove moduly, ktere jsou nacitany za behu.

Vsechny jazykove moduly by mely byt umisteny v jednom z adresaru,
jejihz jmena jsou uvedena v sekci SOUBORY. Jsou scanovany pri startu programu
pro lokalizaci modulu pro aktualni soubor.

.SH SOUBORY
.TP
\fB/etc/lpe\fP
Umisteni defaultnich jazykovych modulu
.TP
\fB$HOME/.lpe\fP
Umisteni uzivatelsky specifickych jazykovych modulu

Oba tyto adresare jsou scanovany pri startu a jakekoli regularni soubory v nich
jsou interpretovany jako potencionalni moduly. Podadresare nejsou scanovany.
.SH "SEE ALSO"
emacs(1), pico(1)
.SH AUTOR
Chris Smith, cd_smith@ou.edu (autor programu lpe)
.br
Michal Safranek, wayne@linuxfreak.com (autor prekladu)
.SH CHYBY
Je jich mnoho -- shlednete soubor 'BUGS' v distribuci pro castecny seznam.
Neplanuji updatovat tuto manualovou stranku natolik casto, aby byla aktualni 
se soucasnym stavem, takze se ani nebudu pokouset vypisovat zde seznam chyb.
