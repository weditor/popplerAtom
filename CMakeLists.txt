# cmake script to build popplerAtom
# build and install poppler first:
#   cmake .. -G "MSYS Makefiles" -DENABLE_XPDF_HEADERS=ON -DENABLE_UTILS=OFF -DENABLE_QT5=OFF -DCMAKE_INSTALL_PREFIX=/mingw64
#   make ; make install

PROJECT(PopplerAtom)

cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(Poppler REQUIRED)

add_library(PopplerAtom SHARED AtomOutputDev.cpp AtomPath.cpp PdfAtomCApi.cpp PdfAtomInterface.cpp)
include_directories(${Poppler_Core_INCLUDE_DIR} ${Poppler_Core_INCLUDE_DIR}/goo ${Poppler_Cpp_INCLUDE_DIR})
target_link_libraries(PopplerAtom ${Poppler_Core_LIBRARY} ${Poppler_Cpp_LIBRARY})

