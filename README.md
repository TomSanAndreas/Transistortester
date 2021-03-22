## Transistortester
Deze code dient gebruikt te worden met de Transistortester PCB en Raspberry Pi. Via de I²C-bus worden de verschillende componenten gebruikt. Het is ook mogelijk de code (en interface(s)) te testen op een Windows (of Linux) computer, al is de volledige functionaliteit natuurlijk niet beschikbaar.
## Functionaliteit
Zie ```transistortester -h``` voor een lijst van mogelijke functies. Afhankelijk van de configuraties zijn bepaalde functies al dan niet beschikbaar. Hier worden de functies nog extra toegelicht:
 - Kalibratiescherm: Via een commandline-interface en een reeks aan commando's is het mogelijk de 3 probe's te besturen: het instellen van een voltage, nameten van de probe-spanning & -stroom, instellen van shuntweerstand en aanmaken van een kalibratiebestand is hier mogelijk. Deze wordt telkens ingelezen wanneer de transistortester wordt gestart, zodanig dat een nauwkeurige meting mogelijk is.
 - Start (hetzelfde als enkel de binary te starten): Via een GTK-interface kan de standaardtoepassing van de transistortester worden gebruikt. Via de eenvoudige interface is de gebruiker in staat om de gebruikte component (na wat basis-gegevens) te herkennen en na te meten.
 - Help: Een beknopte versie van deze tekst wordt weergegeven.
## Installatie
### Raspberry Pi
Eerst wordt de repo gedownload.
```
~$ git clone https://github.ugent.be/towindel/transistortester.git
```
Via het bestand ```transistortester/include/rpi/parameters.hpp``` is het mogelijk bepaalde functionaliteit al dan niet mee te compileren. Dit heeft effect op de grootte van het resultaat, alsook de duur van het compileren en de benodigdheden. De verschillende mogelijkheden zijn:
 - Debuggen (wordt voornamelijk gebruikt indien niet alle hardware lijkt te werken; het wordt afgeraden om bij een volledig werkende PCB deze functionaliteit mee te compileren)
 - Kalibratiescherm (heeft ```ncurses``` als benodigdheid; afhankelijk van het gebruikte besturingssysteem is deze anders te installeren)
 *BELANGRIJK*: De meegeleverde ```Makefile``` verondersteld dat deze wordt gebruikt. Indien ```ncurses``` niet is geïnstalleerd, en er is geen nood aan het kalibratiescherm, dan dient de parameter ```GEBRUIKTE_DEPENDENCIES``` in de Makefile leeg gemaakt te worden.
Functionaliteit kan veranderd worden door de betreffende lijn weg te laten (of in commentaar te veranderen).
Eenmaal ```parameters.hpp``` de gewenste configuratie bevat, volstaat onderstaand commando's om de code te compileren:
```
~$ cd transistortester
transistortester$ make rpi
```
Normaalgezien zou dan, na enkele seconden, het resultaat te vinden moeten zijn in een nieuwe map ```bin```.
### Windows (om functionaliteit van de interface(s) te testen)
Hier kan de code op verschillende manieren worden gedownload. De meest eenvoudige is waarschijnlijk de code als ZIP te downloaden, en deze ergens uit te pakken. Voor eenvoud worden de commando's getoond alsof de ZIP in de C:-schijf is uitgepakt, al maakt de werkelijke locatie weinig uit.
Juist zoals bij de Raspberry Pi, is het mogelijk de code die gecompileerd wordt aan te passen. Hiervoor is het bestand terug te vinden in ```include/win/parameters.hpp```. Nadat de parameters ingesteld staan, kan de code met volgende commando's worden gecompileerd:
```
C:\> cd transistortester
C:\transistortester> make win
```
## To-Do
Momenteel is het kalibratiescherm (bijna) klaar voor gebruik: de PCB is volledig configureerbaar via de ingebouwde commando's. Het opslaan van kalibratie-instellingen ontbreekt nog.
Momenteel is er ook nog geen grafische interface. Eenmaal kalibratie volledig succesvol kan verlopen, wordt een GTK-interface geprogrammeerd die het mogelijk maakt via enkele knoppen de volledige transistortester te gebruiken.
## Troubleshooting
### Fout bij het compileren
Indien```make``` als output ```make: *** No targets specified and no makefile found. Stop.``` heeft, dan wordt er niet in de juiste directory gecompileerd, of is de code niet compleet gedownload. Dit kan eenvoudig worden gecontroleerd: ```pwd``` zou op het einde van het resultaat ```transistortester``` moeten zijn. Indien hier nog iets achter staat, dan kan het probleem opgelost worden door het commando ```cd ..``` te gebruiken totdat ```pwd``` wel eindigd op ```transistortester```. Indien ```pwd``` nergens ```transistortester``` heeft staan, dan kan het probleem worden opgelost door ```cd transistortester``` in te geven (indien de code daar gedownload werd).
Indien ```make``` als output compileerproblemen geeft die code-gerelateerd zijn, controleer dan of er geen "illegale" combinatie gevormd is in ```parameters.hpp``` (zoals bijvoorbeeld ```USING_RPI``` bij het compileren van de Windows-versie), en dat het juiste platform gespecifieerd wordt na ```make```. Indien er nog steeds een probleem optreedt, gelieve dan de ontwikkelaars te contacteren.
### De code is gecompileerd, maar de apparaten geven geen resultaat in kalibratiescherm/GTK-interface
Indien er een andere versie van Raspberry Pi gebruikt wordt, is het mogelijk dat er een andere I²C-bus wordt gebruikt. In het bestand ```pi_i2c.cpp``` kan het mogelijk zijn dat de lijn ```const char* device = "/dev/i2c-1"``` moet gewijzigd worden naar ```const char* device = "/dev/i2c-0"```.
## Credits
Bachelorproef 2020-2021, UGent.
Ingo Chin & Tom Windels