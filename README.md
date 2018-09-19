# popplerAtom
A pdf parsing library, based on poppler. output all words/images/shapes with coordinates

# install
you should install libpoppler first. download it from [https://poppler.freedesktop.org/](https://poppler.freedesktop.org/)

## install libpoppler (source)
```sh
wget https://poppler.freedesktop.org/poppler-0.68.0.tar.xz
tar -axvf poppler-0.68.0.tar.xz
cd poppler-0.68.0
mkdir build
cd build
# if you donnot add -DCMAKE_INSTALL_PREFIX=/path/to/install, then it will be install in /usr/local/.
cmake .. -DENABLE_QT5=OFF -DBUILD_QT5_TESTS=OFF -DENABLE_XPDF_HEADERS=ON -DCMAKE_INSTALL_PREFIX=/path/to/install
make && make install
```

## build popplerAtom

*To be supplemented*

# Api
*To be supplemented*

## C++ interfaces
see `PdfAtomInterface.h`

## C interfaces
see `PdfAtomCApi.h`

## python interfaces
see `pdfatom.py`

# todo:
add cropImage interface.
output image data, not only it's coordinates.

