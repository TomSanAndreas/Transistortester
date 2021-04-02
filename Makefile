BINARY_NAAM=transistortester
GEBRUIKTE_COMPILER=g++

GEBRUIKTE_WIN_DEPENDENCIES=-lncurses -lgtk-3 -lgdk-3 -latk-1.0 -lgio-2.0 -lpangowin32-1.0 -lgdi32 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lglib-2.0 -lintl
GEBRUIKTE_RPI_DEPENDENCIES=-lncurses -lgtk-3 -lgdk-3 -latk-1.0 -lgio-2.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lglib-2.0 -lX11 -rdynamic -pthread
GEBRUIKTE_LINUX_DEPENDENCIES=-lncurses -lgtk-3 -lgdk-3 -latk-1.0 -lgio-2.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lglib-2.0 -lX11 -rdynamic -pthread

GEBRUIKTE_WIN_INCLUDES=-I include -I include/win -I C:/msys64/mingw64/include/gtk-3.0 -I C:/msys64/mingw64/include/gdk-pixbuf-2.0 -I C:/msys64/mingw64/include/glib-2.0 -I C:/msys64/mingw64/include/atk-1.0 -I C:/msys64/mingw64/include/pango-1.0 -I C:/msys64/mingw64/include/cairo -I C:/msys64/mingw64/lib/glib-2.0/include
GEBRUIKTE_RPI_INCLUDES=-I include -I include/rpi -I /usr/include/gtk-3.0 -I /usr/include/gdk-pixbuf-2.0 -I /usr/include/glib-2.0 -I /usr/include/atk-1.0 -I /usr/include/pango-1.0 -I /usr/include/cairo -I /usr/lib/arm-linux-gnueabihf/glib-2.0/include
GEBRUIKTE_LINUX_INCLUDES=-I include -I include/linux -I /usr/include/gtk-3.0 -I /usr/include/gdk-pixbuf-2.0 -I /usr/include/glib-2.0 -I /usr/include/atk-1.0 -I /usr/include/pango-1.0 -I /usr/include/cairo -I /usr/lib/glib-2.0/include -I /usr/include/harfbuzz #-I /usr/include/glib-2.0

rpi: src/*.cpp
	mkdir -p bin
	$(GEBRUIKTE_COMPILER) -o bin/$(BINARY_NAAM) src/*.cpp $(GEBRUIKTE_RPI_DEPENDENCIES) $(GEBRUIKTE_RPI_INCLUDES)

win: src/*.cpp
	mkdir -p bin
	$(GEBRUIKTE_COMPILER) -o bin/$(BINARY_NAAM) -Wall src/*.cpp $(GEBRUIKTE_WIN_DEPENDENCIES) $(GEBRUIKTE_WIN_INCLUDES)

linux: src/*.cpp
	mkdir -p bin
	$(GEBRUIKTE_COMPILER) -o bin/$(BINARY_NAAM) src/*.cpp $(GEBRUIKTE_LINUX_DEPENDENCIES) $(GEBRUIKTE_LINUX_INCLUDES)