
TARGET = libpdfatom.so

POPPLER_PREFIX = "/usr/local"

POPPLER_INCLUDE = -I$(POPPLER_PREFIX)/include \
	-I$(POPPLER_PREFIX)/include/poppler \
	-I$(POPPLER_PREFIX)/include/poppler/goo

POPPLER_LIB = -L"/usr/local/lib64" -lpoppler -lpoppler-cpp

SRC = AtomOutputDev.cpp  AtomPath.cpp  PdfAtomCApi.cpp  PdfAtomInterface.cpp

$(TARGET): $(SRC)
	g++ -std=c++11 -shared -fPIC $(SRC) -o $(TARGET) $(POPPLER_INCLUDE) $(POPPLER_LIB)

test: $(TARGET) pdftohtmlsomain.cc
	g++ -std=c++11 pdftohtmlsomain.cc -o test -lpdfatom $(POPPLER_INCLUDE) -L./ -O0

clean:
	rm $(TARGET) test -f
