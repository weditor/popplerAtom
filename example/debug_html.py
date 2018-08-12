import sys
import os
sys.path.append('../')
from pdfatom import *
from typing import TextIO
import jinja2


def _render_items(items, out: TextIO, offset: float, ratio: float):
    for item in items:
        # item = items[item_idx]
        out.write(f'<div style="position:absolute;left:{item.left*ratio}px;top:{(item.top+offset)*ratio}px;'
                  f'width:{abs(item.left-item.right)*ratio}; height: {abs(item.bottom-item.top)*ratio};'
                  f'">{item.text.decode("utf-8")}</div>\n')
        _render_items(PdfIterator(item.children, item.children_len), out, offset, ratio)


def _render_lines(lines, out: TextIO, offset: float, ratio: float):
    for shape in lines:  # type: CPdfShape
        out.write("<div></div>\n")
        if shape.type != 2:
            continue
        for path in shape.path_list:  # type: CPdfPath
            for line in path.line_list:  # type: CPdfLine
                out.write(f'<div style="position:absolute;left:{min(line.x0, line.x1)*ratio}px;'
                          f'top:{(min(line.y0, line.y1)+offset)*ratio}px;'
                          f'width:{(abs(line.x1-line.x0)+1)*ratio}px;'
                          f'height:{(abs(line.y1-line.y0)+1)*ratio}px;'
                          f'border: red solid 1px"></div>\n')


def render_page_info(page_info: PageInfo, out: TextIO, offset: float=0.0, ratio: float=1.0):
    _render_items(page_info.items, out, offset, ratio)
    _render_lines(page_info.lines, out, offset, ratio)


dir_path = '/home/weditor/work/popplerAtom/poppler-0.66.0/cmake-build-debug/utils/'


PdfParser.init_global_params()

parser = PdfParser(os.path.join(dir_path, "dahua.pdf").encode())

page_offset = 0
with open("test.html", 'w', encoding='utf-8') as fp:
    fp.write("<html><head><meta charset=\"utf-8\"></head><body>\n")

    pageinfos = parser.get_page_info(5, 1.0)
    render_page_info(pageinfos, fp, offset=page_offset, ratio=1.8)

    page_offset += pageinfos.height
    fp.write("\n</body></html>\n")

PdfParser.destroy_global_params()


