# rts-dev

authors: David Klaška, Tomáš Lamser, Jiří Novotný, Vladimír Štill

## Implementace

Naše implementace je v jazyce C++, využíváme open-source platformu
ev3dev (ev3dev.org), postavenou na upraveném systému Debian 8. Samotný
systém i C++ knihovny se zdají být již v dostatečné použitelné fázi
vývoje, při jejich používání jsme nenarazili na žádné závažné nedostatky.

Samotnou implementaci lze rozdělit do několika úloh, které jsou
vykonávány v celkem 3 vláknech. V primárním vlákně probíhá inicializace
systému, ovládání motorů a ramena senzoru a načítání dat ze senzoru.
Toto vlákno se dále stará o korekce směru pro jízdu vpřed, a to za
použití PID kontroléru \footnote{Proportional-Integral-Derivative
Controller}, a o záznam dat ze senzoru pro použití v algoritmu pro
detekci křižovatky. Poslední úlohou primárního vlákna je předat data
vláknu pro detekci křižovatky v okamžiku, kdy detekuje potenciální
rozšíření čáry (což indikuje zatáčku nebo křižovatku). Primární vlákno
nepoužívá žádný plánovací algoritmus, je trvale aktivní.

Sekundární vlákno se zabývá dlouhodobým plánováním a detekcí křižovatek,
je spouštěno vždy na žádost primárního vlákna, a to za pomoci podmínkové
proměnné. Tento systém se sice nevyznačuje dobrou predikovatelností na
krátkých časových intervalech (díky nepředvídatelnosti plánovače úloh),
na druhou stranu je však jednoduchý k implementaci a pro náš případ se
zdá být dostatečným. Komunikace mezi primárním a sekundárním vláknem je
asynchronní, využívá bufferu pro jednu zprávu, a to takovým způsobem, že
primární vlákno se dotazuje na možnost přijetí/odeslání zprávy, a pokud
je buffer plný tak neblokuje, nýbrž pokračuje dále ve své práci;
sekundární vlákno je při čekání na data blokované. Pro zamezení
race-condition jsou komunikační buffery chráněny mutexem.

Poslední vlákno slouží k detekci stisknutí tlačítka nouzového zastavení,
přičemž provádí kontrolu tlačítka s periodou 100ms. V případe stisku
tlačítka je toto indikováno nastavením atomické proměnné, která indikuje
ostatním vláknům, že mají ukončit svou činnost. Stejná atomická proměnná
je používána i při reakci na signál pro zastavení z terminálu.


## Těžkosti
- Barevný senzor je schopný snímat data pouze z jednoho bodu. Pokud se data
potřebují zasadit do širšího kontextu, tak je zapotřebí senzor umístit na
otočné rameno, které se senzorem velice rychle pohybuje. Kvalita snímání je
potom značně ovlivněna vzorkovací frekvení senzoru, a poměrem rychlostí otáčení
ramena ku rychlosti samotného robota.
- Ačkoli PID kontrolér je schopný velmi dobře korigovat změny směru
jízdy robota a poradí si i s některými neočekávanými případy, jako jsou
například oblé zatáčky, je poměrně těžké nastavit jeho parametry tak,
aby korekce byly správně velké.
- Na použitém operačním systému je třeba počítat s nerovnoměrným
plánováním procesu -- čas od času je tím například negativně ovlivněna
rozlišovací schopnost hlavního senzoru a program sesbírá méně vzorků
barev podkladu.
- C++ se ukázalo být poměrně nevhodnou volbou z hlediska rychlosti
prototypování implementace, jeho kompilace na na EV3 je velice pomalá.
- Vývoje neustále komplikuje to, že EV3 se neumí napájet z USB.


To connect to device using ssh on linux:
```
ip addr add 10.42.0.1/24 dev <dev_name>
ssh ev3@10.42.0.3
```
where `<dev_name>` can be determined by use of command `ip addr`.
