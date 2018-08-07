#!/bin/sh

if [ $# -lt 1 ]; then echo "usage: $0 [pack|unpack]"; exit 1 ; fi

if [ $1 == pack ]; then
    echo start pack ... ;
    if [ ! -d poppler-0.66.0 ]; then
        echo "directory 'poppler-0.66.0' not exits! run [unpack] first."
        exit 3;
    fi
    cd poppler-0.66.0/utils/ && cp -t ../.. PdfAtomInterface.cpp PdfAtomInterface.h AtomOutputDev.cpp AtomOutputDev.h \
        poppler_atom_types.h PdfAtomCApi.cpp PdfAtomCApi.h ../pdfatom.py
    echo "finish!"
    exit 0
elif [ $1 == unpack ]; then
    echo start unpack ... ;
    if [ -d "poppler-0.66.0" ]; then
        echo "WARNING: directory 'poppler-0.66.0' already exits!";
    else
        unzip poppler-0.66.0.zip
    fi
    cp -t poppler-0.66.0/utils/ PdfAtomInterface.cpp PdfAtomInterface.h AtomOutputDev.cpp AtomOutputDev.h \
        poppler_atom_types.h PdfAtomCApi.cpp PdfAtomCApi.h
    cp pdfatom.py poppler-0.66.0/
    echo "finish!"
    exit 0;
else
    echo "invalid option $1"
    echo "usage: $0 [pack|unpack]"
    exit 2;
fi
