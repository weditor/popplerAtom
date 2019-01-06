import sys
import os
# sys.path.append(os.path.join(os.path.abspath(os.path.dirname(__file__)), '../'))
from pdfatom import *
from typing import TextIO
import json
# import jinja2


def _render_items(items):
    for item in items:
        yield {
            'left': item.left,
            'right': item.right,
            'top': item.top,
            'bottom': item.bottom,
            'text': item.text.decode(),
            'children': list(_render_items(PdfIterator(item.children, item.children_len))),
        }


def _render_graphs(pathes):
    for path in pathes:
        for line in path.line_list:
            yield {
                'x0': line.x0,
                'y0': line.y0,
                'x1': line.x1,
                'y1': line.y1,
            }


def _render_lines(lines):
    for line in lines:
        yield {
            'x0': line.x0,
            'y0': line.y0,
            'x1': line.x1,
            'y1': line.y1,
        }


def render_page_info(page_info: PageInfo):
    return {
        'items': list(_render_items(page_info.items)),
        'lines': list(_render_lines(page_info.lines)),
        'graphs': list(_render_graphs(page_info.graphs)),
    }


# dir_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../")
dir_path = os.path.dirname(os.path.abspath(__file__))


PdfParser.init_global_params()
parser = PdfParser(os.path.join(dir_path, "dahua.pdf").encode())

js_data = []
for page_idx in range(1, parser.get_number_pages()+1):
    pageinfos = parser.get_page_info(page_idx, 1.0)
    js_data.append(render_page_info(pageinfos))

with open("test.json", 'w', encoding='utf-8') as fp:
    fp.write(json.dumps(js_data, ensure_ascii=False, indent=4))

PdfParser.destroy_global_params()


