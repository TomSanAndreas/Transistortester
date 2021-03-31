## Transistortester
Deze code dient gebruikt te worden met de Transistortester PCB en Raspberry Pi. Via de I²C-bus worden de verschillende componenten gebruikt. Het is ook mogelijk de code (en interface(s)) te testen op een Windows (of Linux) computer, al is de volledige functionaliteit natuurlijk niet beschikbaar.
## Functionaliteit
Zie ```transistortester -h``` voor een lijst van mogelijke functies. Afhankelijk van de configuraties zijn bepaalde functies al dan niet beschikbaar. Hier worden de functies nog extra toegelicht:
 - Shell: Via een commandline-interface en een reeks aan commando's is het mogelijk de 3 probes te besturen: het instellen van een voltage, nameten van de probe-spanning & -stroom en het instellen van shuntweerstand is hier mogelijk.
 - Standaardfunctionaliteit: Via een GTK-interface kan de standaardtoepassing van de transistortester worden gebruikt. Via de eenvoudige interface is de gebruiker in staat om de gebruikte component te herkennen en na te meten.
 - Help: Een beknopte versie van deze tekst wordt weergegeven.
## Installatie
### Raspberry Pi
Eerst wordt de repo gedownload.
```
~$ git clone https://github.ugent.be/towindel/transistortester.git
```
Via het bestand ```transistortester/include/base.hpp``` is het mogelijk bepaalde functionaliteit al dan niet mee te compileren. Dit heeft effect op de grootte van het resultaat, alsook de duur van het compileren en de benodigdheden. De verschillende mogelijkheden zijn:
 - Shell (heeft ```ncurses``` als benodigdheid; afhankelijk van het gebruikte besturingssysteem is deze anders te installeren)
 *BELANGRIJK*: De meegeleverde ```Makefile``` verondersteld dat deze wordt gebruikt. Indien ```ncurses``` niet is geïnstalleerd, en er is geen nood aan het shell, dan mag de parameter ```GEBRUIKTE_RPI_DEPENDENCIES``` in de Makefile ```-lncurses``` niet meer bevatten om geen linkerfouten te krijgen.
Functionaliteit kan veranderd worden door de betreffende lijn weg te laten (of in commentaar te veranderen).
Eerst dient ```GTK 3``` (en eventueel ```ncurses```) geinstalleerd te worden. Dit kan met volgende commando's:
```
~$ sudo apt install libgtk-3-dev
~$ sudo apt install libncurses5-dev libncursesw5-dev (optioneel)
```
Eenmaal ```base.hpp``` de gewenste configuratie bevat, volstaat onderstaand commando's om de code te compileren:
```
~$ cd transistortester
transistortester$ make rpi
```
Normaalgezien zou dan, na enkele seconden, het resultaat te vinden moeten zijn in een nieuwe map ```bin```.
### Windows (om functionaliteit van de interface(s) te testen)
Hier kan de code op verschillende manieren worden gedownload. De meest eenvoudige is waarschijnlijk de code als ZIP te downloaden, en deze ergens uit te pakken. Voor eenvoud worden de commando's getoond alsof de ZIP in de C:-schijf is uitgepakt, al maakt de werkelijke locatie weinig uit.
Juist zoals bij de Raspberry Pi, is het mogelijk de code die gecompileerd wordt aan te passen. Dit gebeurt ook in ```base.hpp```. Nadat de parameters ingesteld staan, (en de Makefile eventueel is aangepast door ```-lncurses``` bij ```GEBRUIKTE_WIN_DEPENDENCIES``` weg te pakken indien de shell niet gewenst is) kan de code met volgende commando's worden gecompileerd:
```
C:\> cd transistortester
C:\transistortester> make win
```
### Linux (om functionaliteit van de interface(s) te testen)
Juist zoals bij de Raspberry Pi, kan de code gedownload worden via 
```
~$ git clone https://github.ugent.be/towindel/transistortester.git
```
De configuratie van de te compileren functionaliteit kan gewijzigd worden in ```base.hpp```. Afhankelijk van de gebruikte distro kan het installeren van de nodige libraries (```GTK 3``` en ```ncurses```, indien de shell gewenst is) verschillen, dus dit wordt niet verder uitgelegd.

## To-Do
Momenteel is er ook een (onvolledige) grafische interface. Het GTK-venster laat al toe een aangesloten component te herkennen (weerstand, diode, BJT (NPN & PNP)) en de pinout van deze component weer te geven. Verder nameten van deze component moet nog worden toegevoegd.
## Troubleshooting
### Fout bij het compileren
Indien```make``` als output ```make: *** No targets specified and no makefile found. Stop.``` heeft, dan wordt er niet in de juiste directory gecompileerd, of is de code niet compleet gedownload. Dit kan eenvoudig worden gecontroleerd: ```pwd``` zou op het einde van het resultaat ```transistortester``` moeten zijn. Indien hier nog iets achter staat, dan kan het probleem opgelost worden door het commando ```cd ..``` te gebruiken totdat ```pwd``` wel eindigd op ```transistortester```. Indien ```pwd``` nergens ```transistortester``` heeft staan, dan kan het probleem worden opgelost door ```cd transistortester``` in te geven (indien de code daar gedownload werd).
Indien ```make``` als output compileerproblemen geeft die code-gerelateerd zijn, controleer dan of er geen "illegale" combinatie gevormd is in ```parameters.hpp``` en ```base.hpp``` (zoals bijvoorbeeld ```USING_RPI``` bij het compileren van de Windows-versie), en dat het juiste platform gespecifieerd wordt na ```make```. Indien er nog steeds een probleem optreedt, gelieve dan de ontwikkelaars te contacteren.
### De code is gecompileerd, maar de apparaten geven geen resultaat in de shell/GTK-interface
Indien er een andere versie van Raspberry Pi gebruikt wordt, is het mogelijk dat er een andere I²C-bus wordt gebruikt. In het bestand ```pi_i2c.cpp``` kan het mogelijk zijn dat de lijn ```const char* device = "/dev/i2c-1"``` moet gewijzigd worden naar ```const char* device = "/dev/i2c-0"```.
## Credits
Bachelorproef 2020-2021, UGent.
Ingo Chin & Tom Windels
