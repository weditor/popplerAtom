#!/usr/bin/env python
from __future__ import print_function, unicode_literals
import os
import re


os.chdir(os.path.dirname(__file__))


def find_poppler_zip():
    for fname in os.listdir('./'):
        if re.fullmatch(r'poppler.*\.zip', fname, re.IGNORECASE):
            return fname
    return ""


poppler, ext = os.path.splitext(find_poppler_zip())

