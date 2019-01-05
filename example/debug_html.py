import sys
import os
# sys.path.append(os.path.join(os.path.abspath(os.path.dirname(__file__)), '../'))
from pdfatom import *
from typing import TextIO
# import jinja2


def _render_items(items, out: TextIO, offset: float, ratio: float):
    for item in items:
        # item = items[item_idx]
        out.write(f'<div style="position:absolute;left:{int(item.left*ratio)}px;top:{int((item.top+offset)*ratio)}px;'
                  f'width:{int(abs(item.left-item.right)*ratio)}px; height: {int(abs(item.bottom-item.top)*ratio)}px;'
                  f'">{item.text.decode("utf-8")}</div>\n')
        _render_items(PdfIterator(item.children, item.children_len), out, offset, ratio)


def _render_graphs(pathes, out: TextIO, offset: float, ratio: float):
    for path in pathes:  # type: CPdfShape
        out.write("<div></div>\n")
        if path.type != 2:
            continue
        for line in path.line_list:  # type: CPdfLine
            out.write(f'<div style="position:absolute;left:{int(min(line.x0, line.x1)*ratio)}px;'
                      f'top:{int((min(line.y0, line.y1)+offset)*ratio)}px;'
                      f'width:{int((abs(line.x1-line.x0)+1)*ratio)}px;'
                      f'height:{int((abs(line.y1-line.y0)+1)*ratio)}px;'
                      # f'background-color: #00ff0011"></div>\n')
                      f'border: green dashed 2px"></div>\n')


def _render_lines(lines, out: TextIO, offset: float, ratio: float):
    for line in lines:  # type: CPdfLine
        out.write(f'<div style="position:absolute;left:{min(line.x0, line.x1)*ratio}px;'
                  f'top:{int((min(line.y0, line.y1)+offset)*ratio)}px;'
                  f'width:{max(int(abs(line.x1-line.x0)*ratio), 1)}px;'
                  f'height:{max(int(abs(line.y1-line.y0)*ratio), 1)}px;'
                  f'border: red solid 1px"></div>\n')


def render_page_info(page_info: PageInfo, out: TextIO, offset: float=0.0, ratio: float=1.0):
    _render_items(page_info.items, out, offset, ratio)
    _render_lines(page_info.lines, out, offset, ratio)
    _render_graphs(page_info.graphs, out, offset, ratio)


# dir_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../")
dir_path = os.path.dirname(os.path.abspath(__file__))


PdfParser.init_global_params()
parser = PdfParser(os.path.join(dir_path, "tongyu.pdf").encode())

# img = parser.crop_image(2)
# with open("test_py_2.png", 'wb') as fp:
#     fp.write(img)

# exit(0)
page_offset = 0
with open("test.html", 'w', encoding='utf-8') as fp:
    fp.write("<html><head><meta charset=\"utf-8\"></head><body>\n")

    for page_idx in range(1, parser.get_number_pages()+1):
        pageinfos = parser.get_page_info(page_idx, 1.0)
        render_page_info(pageinfos, fp, offset=page_offset, ratio=1.8)
        page_offset += pageinfos.height
    fp.write("\n</body></html>\n")

PdfParser.destroy_global_params()


