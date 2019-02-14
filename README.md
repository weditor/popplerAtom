# popplerAtom
A pdf parsing library, based on poppler. output all words/images/shapes with coordinates

# install
Install libpoppler first. download it from [https://poppler.freedesktop.org/](https://poppler.freedesktop.org/)

## install libpoppler 

### from source
```sh

# use labest poppler relase if possible, my version is 0.68 while i write this document.
wget https://poppler.freedesktop.org/poppler-0.68.0.tar.xz
tar -axvf poppler-0.68.0.tar.xz
cd poppler-0.68.0
mkdir build
cd build

# install extra xpdf headers. the default is OFF.
cmake .. -DENABLE_XPDF_HEADERS=ON

# if you want to speedup the compile, you can disable QT/utils .
# cmake .. -DENABLE_XPDF_HEADERS=ON -DENABLE_UTILS=OFF -DENABLE_QT5=OFF

# if you compile poppler in MSYS2 on windows, add '-G "MSYS Makefiles"' and CMAKE_INSTALL_PREFIX to install in /mingw64.
# cmake .. -G "MSYS Makefiles" -DENABLE_XPDF_HEADERS=ON -DCMAKE_INSTALL_PREFIX=/mingw64

make && make install
```

## build popplerAtom

```sh
mkdir build && cd build;
cmake .. ;
make ;
```

# Api

## C++ interfaces
see `PdfAtomInterface.h`


## C interfaces
see `PdfAtomCApi.h`

## python interfaces
see `pdfatom.py`

# todo:
add cropImage interface.
output image data, not only it's coordinates.

