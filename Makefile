BINARY_NAAM=transistortester
GEBRUIKTE_COMPILER=g++
#GEBRUIKTE_DEPENDENCIES=-lwiringPi
GEBRUIKTE_DEPENDENCIES=-lncurses

transistortester: src/*.cpp
	$(GEBRUIKTE_COMPILER) -o bin/$(BINARY_NAAM) src/*.cpp $(GEBRUIKTE_DEPENDENCIES) -I include
