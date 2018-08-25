#!/usr/bin/env python
from __future__ import print_function, unicode_literals
import os
import re
import argparse
import zipfile
import shutil

file_list = [
    ('example/debug_html.py', 'example/debug_html.py'),
    ('utils/CMakeLists.txt', 'CMakeLists.txt'),

    ('utils/AtomOutputDev.cpp', 'AtomOutputDev.cpp'),
    ('utils/AtomOutputDev.h', 'AtomOutputDev.h'),
    ('utils/AtomPath.cpp', 'AtomPath.cpp'),
    ('utils/AtomPath.h', 'AtomPath.h'),
    ('utils/PdfAtomCApi.cpp', 'PdfAtomCApi.cpp'),
    ('utils/PdfAtomCApi.h', 'PdfAtomCApi.h'),
    ('utils/PdfAtomInterface.cpp', 'PdfAtomInterface.cpp'),
    ('utils/PdfAtomInterface.h', 'PdfAtomInterface.h'),
    ('pdfatom.py', 'pdfatom.py'),
    ('utils/pdftohtmlsomain.cc', 'pdftohtmlsomain.cc'),
    ('utils/poppler_atom_types.h', 'poppler_atom_types.h'),
]

os.chdir(os.path.dirname(os.path.realpath(__file__)))


def find_poppler_zip():
    for fname in os.listdir('./'):
        if re.fullmatch(r'poppler.*\.zip', fname, re.IGNORECASE):
            return fname
    return ""


poppler, ext = os.path.splitext(find_poppler_zip())


def unpack():
    if os.path.exists(poppler):
        print("{path} is already exists, skip extract!".format(path=poppler))

        should_continue = input("This will overwrite files in {poppler}, Are you sure to continue? [y/N]?"
                                .format(poppler=poppler))
        if should_continue.lower() not in {"y", "yes"}:
            print("do nothing, exit!")
            return
    else:
        if ext == ".zip":
            zfp = zipfile.ZipFile(poppler+ext)
            zfp.extractall()
        else:
            print("unrecognized ext type: {ext}".format(ext=ext))

    for dst, src in file_list:
        dst = os.path.join(poppler, dst)
        print("copy [{src}] to [{dst}], ".format(**locals()), end='')
        if not os.path.exists(src):
            print("{src} not exist!".format(src=src))
            continue
        os.makedirs(os.path.dirname(os.path.realpath(dst)), exist_ok=True)
        shutil.copy(src, dst)
        print("ok!")


def pack():
    for src, dst in file_list:
        src = os.path.join(poppler, src)
        print("copy [{src}] to [{dst}], ".format(**locals()), end='')
        if not os.path.exists(src):
            print("{src} not exist!".format(src=src))
            continue
        os.makedirs(os.path.dirname(os.path.realpath(dst)), exist_ok=True)
        shutil.copy(src, dst)
        print("ok!")


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='pack or unpack popplerAtom.')
    parser.add_argument("--unpack", dest="unpack", action="store_true")
    parser.add_argument("--pack", dest="pack", action="store_true")
    args = parser.parse_args()
    if not (args.unpack ^ args.pack):
        print('should provide one (and only one) of [--pack] and [--unpack]')
        parser.print_help()
        exit(1)
    if args.unpack:
        unpack()
    elif args.pack:
        pack()
    print("finish")
