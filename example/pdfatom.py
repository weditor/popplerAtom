# -*- encoding: utf-8 -*-

from ctypes import *
import os

__all__ = [
    'CAtomBox',
    'CPdfFont',
    'CPdfImage',
    'CPdfItem',
    'CPdfLine',
    'CPdfPath',
    'CPdfShape',
    'CPageInfos',

    'initGlobalParams',
    'destroyGlobalParams',

    'createAtomParser',
    'get_page_number',
    'isParserOk',

    'renderHtml',
    'deletePageInfos',

    'PdfParser',
    'PageInfo',
    'PdfIterator',
]
# os.chdir('/home/weditor/work/popplerAtom/poppler-0.66.0/cmake-build-debug/utils/')
# os.environ['LD_LIBRARY_PATH'] = './:../'

dir_path = os.path.abspath(os.path.dirname(__file__))


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
        ("line_height", c_int),
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


class CPdfItem(Structure):
    pass


CPdfItem._fields_ = [
    ("id", c_uint),
    ("mcid", c_int),
    ("type", c_int),
    ("text", c_char_p),

    ("left", c_int),
    ("top", c_int),
    ("right", c_int),
    ("bottom", c_int),

    ("font", c_int),
    ("style", c_int),
    ("children", POINTER(CPdfItem)),
    ("children_len", c_ulong),
]


class CPdfLine(Structure):
    _fields_ = [
        ('x0', c_int),
        ('y0', c_int),
        ('x1', c_int),
        ('y1', c_int),
        # ('cx', c_int),
        # ('cy', c_int),
        ('type', c_int),
    ]


class CPdfPath(Structure):
    _fields_ = [
        ('type', c_int),
        ("lines", POINTER(CPdfLine)),
        ("line_len", c_ulong),
    ]

    @property
    def line_list(self):
        return PdfIterator(self.lines, self.line_len)


class CPdfShape(Structure):
    _fields_ = [
        ('type', c_int),
        ("pathes", POINTER(CPdfPath)),
        ("path_len", c_ulong),
    ]

    @property
    def path_list(self):
        return PdfIterator(self.pathes, self.path_len)


class CPageInfos(Structure):
    _fields_ = [
        ('page_num', c_int),
        ('width', c_int),
        ('height', c_int),

        ("fonts", POINTER(CPdfFont)),
        ("font_len", c_ulong),

        ("images", POINTER(CPdfImage)),
        ("image_len", c_ulong),

        ("items", POINTER(CPdfItem)),
        ("item_len", c_ulong),

        ("lines", POINTER(CPdfLine)),
        ("line_len", c_ulong),

        ("graphs", POINTER(CPdfPath)),
        ("graph_len", c_ulong),
    ]


# pdfparser = cdll.LoadLibrary(os.path.join(dir_path, "libpdfatom_glibc.so"))
# print(os.path.join(dir_path, "libpdfatom.dll"))
# pdfparser = cdll.LoadLibrary(os.path.join(dir_path, "libpdfatom.so"))
pdfparser = cdll.LoadLibrary("./libpdfatom.so")

initGlobalParams = pdfparser.c_initGlobalParams
initGlobalParams.argtypes = [c_char_p]

destroyGlobalParams = pdfparser.c_destroyGlobalParams

createAtomParser = pdfparser.createAtomParser
createAtomParser.argtypes = [c_char_p, c_char_p, c_char_p]
createAtomParser.restype = c_void_p

get_page_number = pdfparser.getNumPages
get_page_number.argtypes = [c_void_p]
get_page_number.restype = c_int

isParserOk = pdfparser.isParserOk
isParserOk.argtypes = [c_void_p]
isParserOk.restype = c_bool

renderHtml = pdfparser.renderHtml
renderHtml.argtypes = [c_void_p, c_uint, c_float]
renderHtml.restype = POINTER(CPageInfos)

deletePageInfos = pdfparser.deletePageInfos
deletePageInfos.argtypes = [POINTER(CPageInfos)]

crop_image = pdfparser.cropImage
crop_image.argtypes = [c_void_p, c_uint, POINTER(POINTER(c_char)), POINTER(c_ulong),
    POINTER(c_ulong), POINTER(c_ulong),
    c_int, c_int, c_int, c_int, c_float]

free_image = pdfparser.freeImage
free_image.argtypes = [POINTER(POINTER(c_char))]


class PdfIterator:
    def __init__(self, pointer, size):
        self._pointer = pointer
        self._size = size
        self._pos = -1

    def __iter__(self):
        self._pos = -1
        return self

    def __next__(self):
        if self._pos+1 >= self._size:
            raise StopIteration()
        self._pos += 1
        return self._pointer[self._pos]


class PageInfo:
    def __init__(self, page_info: CPageInfos):
        self.page_info = page_info

    @property
    def height(self):
        return self.page_info.height

    @property
    def width(self):
        return self.page_info.width

    @property
    def items(self):
        return PdfIterator(self.page_info.items, self.page_info.item_len)

    @property
    def lines(self):
        return PdfIterator(self.page_info.lines, self.page_info.line_len)

    @property
    def graphs(self):
        return PdfIterator(self.page_info.graphs, self.page_info.graph_len)

    def __del__(self):
        deletePageInfos(pointer(self.page_info))


class PdfParser:
    @staticmethod
    def init_global_params(poppler_data: str=None):
        initGlobalParams(poppler_data)

    @staticmethod
    def destroy_global_params():
        destroyGlobalParams()

    def __init__(self, pdf_path, owner_pw=None, user_pw=None):
        self.parser = createAtomParser(pdf_path, owner_pw, user_pw)

    def is_ok(self):
        return isParserOk(self.parser)

    def get_number_pages(self):
        return get_page_number(self.parser)

    def get_page_info(self, page_num, scale=1.0):
        return PageInfo(renderHtml(self.parser, page_num, scale).contents)

    def crop_image(self, page_num, x=-1, y=-1, w=-1, h=-1, scale=1.0):
        p_data = POINTER(c_char)()
        size = c_ulong(0)
        out_w = c_ulong(0)
        out_h = c_ulong(0)

        crop_image(c_void_p(self.parser), page_num, pointer(p_data), pointer(size), out_w, out_h,
                   x, y, w, h, c_float(scale))
        ret = p_data[:size.value]
        free_image(pointer(p_data))
        return ret, (out_h.value, out_w.value)


if __name__ == '__main__':
    initGlobalParams(None)

    parser = createAtomParser(os.path.join(dir_path, "dahua.pdf").encode(), None, None)
    # print(parser, type(parser))
    print(isParserOk(parser))
    print(get_page_number(parser))
    pageinfos = renderHtml(parser, 5, 1.8)

    deletePageInfos(pageinfos)

    destroyGlobalParams()
