
TARGET = libpdfatom.dll

POPPLER_INCLUDE = -I"C:/Program Files (x86)/poppler/include" \
	-I"C:/Program Files (x86)/poppler/include/poppler" \
	-I"C:/Program Files (x86)/poppler/include/poppler/goo" \
	-I"D:\env\msys2\mingw64\include"

POPPLER_LIB = -L"C:/Program Files (x86)/poppler/lib" -lpoppler


SRC = AtomOutputDev.cpp  AtomPath.cpp  PdfAtomCApi.cpp  PdfAtomInterface.cpp
$(TARGET): $(SRC)
	g++ -std=c++11 -shared -fPIC $(SRC) -o $(TARGET) $(POPPLER_INCLUDE) $(POPPLER_LIB)

test: $(TARGET) pdftohtmlsomain.cc
	g++ -std=c++11 pdftohtmlsomain.cc -o test -lpdfatom $(POPPLER_INCLUDE) -L./ -O2

clean:
	rm $(TARGET) test -f