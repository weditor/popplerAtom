# -*- encoding: utf-8 -*-

from ctypes import *
import os

# os.chdir('/home/weditor/work/popplerAtom/poppler-0.66.0/cmake-build-debug/utils/')
# os.environ['LD_LIBRARY_PATH'] = './:../'

dir_path = '/home/weditor/work/popplerAtom/poppler-0.66.0/cmake-build-debug/utils/'


class CAtomBox(Structure):
    _fields_ = [
        ('x1', c_double),
        ('y1', c_double),
        ('x2', c_double),
        ('y2', c_double),
    ]


class CPdfFont(Structure):
    _fields_ = [
        ("id", c_int),
        ("name", c_char_p),
        ("size", c_float),
        ("weight", c_short),
        ("color", c_char_p),
        ("is_bold", c_bool),
        ("is_italic", c_bool),
        ("line_height", c_short),
        ("type", c_int),
        ("render", c_int),
    ]


class CPdfImage(Structure):
    _fields_ = [
        ("left", c_int),
        ("top", c_int),
        ("right", c_int),
        ("bottom", c_int),
    ]


# pdfparser = cdll.LoadLibrary("/home/weditor/work/popplerAtom/poppler-0.66.0/cmake-build-debug/utils/libpdfatom_glibc.so")
pdfparser = cdll.LoadLibrary(os.path.join(dir_path, "libpdfatom_glibc.so"))

createAtomParser = pdfparser.createAtomParser
createAtomParser.argtypes = [c_char_p, c_char_p, c_char_p]
createAtomParser.restype = c_void_p

get_page_number = pdfparser.getNumPages
get_page_number.argtypes = [c_void_p]
get_page_number.restype = c_int

isParserOk = pdfparser.isParserOk
isParserOk.argtypes = [c_void_p]
isParserOk.restype = c_bool


if __name__ == '__main__':
    parser = createAtomParser(os.path.join(dir_path, "dahua.pdf").encode(), None, None)
    # print(parser, type(parser))
    print(isParserOk(c_void_p(parser)))
    print(get_page_number(c_void_p(parser)))

