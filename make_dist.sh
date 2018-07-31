#!/bin/sh

if [ $# -lt 1 ]; then echo "usage: $0 [pack|unpack]"; exit 1 ; fi

if [ $1 == pack ]; then
    echo start pack ... ;
    if [ ! -d poppler-0.66.0 ]; then
        echo "directory 'poppler-0.66.0' not exits! run [unpack] first."
        exit 3;
    fi
    cp -t . poppler-0.66.0/utils/PdfAtomInterface.cpp poppler-0.66.0/utils/PdfAtomInterface.h poppler-0.66.0/utils/AtomOutputDev.cpp poppler-0.66.0/utils/AtomOutputDev.h
    echo "finish!"
    exit 0
elif [ $1 == unpack ]; then
    echo start unpack ... ;
    if [ ! -d poppler-0.66.0 ]; then
        echo "directory 'poppler-0.66.0' already exits! remove it first."
        exit 4;
    fi
    unzip poppler-0.66.0.zip
    echo "finish!"
    exit 0;
else
    echo "invalid option $1"
    echo "usage: $0 [pack|unpack]"
    exit 2;
fi