BINARY_NAAM=transistortester
GEBRUIKTE_COMPILER=g++
GEBRUIKTE_DEPENDENCIES=-lncurses

rpi: src/*.cpp
	mkdir -p bin
	$(GEBRUIKTE_COMPILER) -o bin/$(BINARY_NAAM) src/*.cpp $(GEBRUIKTE_DEPENDENCIES) -I include -I include/rpi

win: src/*.cpp
	mkdir -p bin
	$(GEBRUIKTE_COMPILER) -o bin/$(BINARY_NAAM) src/*.cpp $(GEBRUIKTE_DEPENDENCIES) -I include -I include/win