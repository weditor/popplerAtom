//
// Created by weditor on 18-7-31.
//

#include <cmath>
#include <goo/PNGWriter.h>
#include "AtomOutputDev.h"
#include "GfxState.h"
#include "GfxFont.h"
#include "HtmlUtils.h"

#ifdef ENABLE_LIBPNG
#include <png.h>
#endif


///////////////////////////////////
/// AtomString
///////////////////////////////////
AtomString::AtomString(GfxState *state, double fontSize, HtmlFontAccu* _fonts) : fonts(_fonts) {
    GfxFont *font;
    double x, y;

    state->transform(state->getCurX(), state->getCurY(), &x, &y);
    if ((font = state->getFont())) {
        double ascent = font->getAscent();
        double descent = font->getDescent();
        if( ascent > 1.05 ){
            //printf( "ascent=%.15g is too high, descent=%.15g\n", ascent, descent );
            ascent = 1.05;
        }
        if( descent < -0.4 ){
            //printf( "descent %.15g is too low, ascent=%.15g\n", descent, ascent );
            descent = -0.4;
        }
        yMin = y - ascent * fontSize;
        yMax = y - descent * fontSize;
        GfxRGB rgb;
        state->getFillRGB(&rgb);
        HtmlFont hfont=HtmlFont(font, static_cast<int>(fontSize-1), rgb);
        if (isMatRotOrSkew(state->getTextMat())) {
            double normalizedMatrix[4];
            memcpy(normalizedMatrix, state->getTextMat(), sizeof(normalizedMatrix));
            // browser rotates the opposite way
            // so flip the sign of the angle -> sin() components change sign
            normalizedMatrix[1] *= -1;
            normalizedMatrix[2] *= -1;
            normalizeRotMat(normalizedMatrix);
            hfont.setRotMat(normalizedMatrix);
        }
        fontpos = fonts->AddFont(hfont);
    } else {
        // this means that the PDF file draws text without a current font,
        // which should never happen
        yMin = y - 0.95 * fontSize;
        yMax = y + 0.35 * fontSize;
        fontpos=0;
    }
    if (yMin == yMax) {
        // this is a sanity check for a case that shouldn't happen -- but
        // if it does happen, we want to avoid dividing by zero later
        yMin = y;
        yMax = y + 1;
    }
    col = 0;
    text = nullptr;
    xRight = nullptr;
    link = nullptr;
    len = size = 0;
    yxNext = nullptr;
    xyNext = nullptr;
    htext=new GooString();
    dir = textDirUnknown;
}


AtomString::~AtomString() {
    gfree(text);
    delete htext;
    gfree(xRight);
}

void AtomString::addChar(GfxState *state, double x, double y,
                         double dx, double dy, Unicode u) {
    if (dir == textDirUnknown) {
        //dir = UnicodeMap::getDirection(u);
        dir = textDirLeftRight;
    }

    if (len == size) {
        size += 16;
        text = (Unicode *)grealloc(text, size * sizeof(Unicode));
        xRight = (double *)grealloc(xRight, size * sizeof(double));
    }
    text[len] = u;
    if (len == 0) {
        xMin = x;
    }
    xMax = xRight[len] = x + dx;
//printf("added char: %f %f xright = %f\n", x, dx, x+dx);
    ++len;
}

void AtomString::endString()
{
    if( dir == textDirRightLeft && len > 1 )
    {
        //printf("will reverse!\n");
        for (int i = 0; i < len / 2; i++)
        {
            Unicode ch = text[i];
            text[i] = text[len - i - 1];
            text[len - i - 1] = ch;
        }
    }
}

//////////////////////
/// AtomPage
//////////////////////

AtomPage::AtomPage() {
//todo: init all members;
}

AtomPage::~AtomPage() {

}

void AtomPage::clear() {
    AtomString *p1, *p2;

    if (m_curStr) {
        delete m_curStr;
        m_curStr = nullptr;
    }
    for (p1 = m_yxStrings; p1; p1 = p2) {
        p2 = p1->yxNext;
        delete p1;
    }
    m_yxStrings = nullptr;
    m_yxTail = nullptr;
}

void AtomPage::updateFont(GfxState *state) {
    GfxFont *font;
    double *fm;
    char *name;
    int code;
    double w;

    // adjust the font size
    m_fontSize = state->getTransformedFontSize();
    if ((font = state->getFont()) && font->getType() == fontType3) {
        // This is a hack which makes it possible to deal with some Type 3
        // fonts.  The problem is that it's impossible to know what the
        // base coordinate system used in the font is without actually
        // rendering the font.  This code tries to guess by looking at the
        // width of the character 'm' (which breaks if the font is a
        // subset that doesn't contain 'm').
        for (code = 0; code < 256; ++code) {
            if ((name = ((Gfx8BitFont *)font)->getCharName(code)) &&
                name[0] == 'm' && name[1] == '\0') {
                break;
            }
        }
        if (code < 256) {
            w = ((Gfx8BitFont *)font)->getWidth(code);
            if (w != 0) {
                // 600 is a generic average 'm' width -- yes, this is a hack
                m_fontSize *= w / 0.6;
            }
        }
        fm = font->getFontMatrix();
        if (fm[0] != 0) {
            m_fontSize *= fabs(fm[3] / fm[0]);
        }
    }
}

void AtomPage::beginString(GfxState *state, const GooString *s) {
    // todo: remember to free it.
    m_curStr = new AtomString(state, m_fontSize, m_fonts);
}

void AtomPage::endString() {
    // throw away zero-length strings -- they don't have valid xMin/xMax
    // values, and they're useless anyway
    if (m_curStr->len == 0) {
        delete m_curStr;
        m_curStr = nullptr;
        return;
    }

    m_curStr->endString();

    // insert string in y-major list
    m_yxTail = m_curStr;
    if (m_yxTail)
        m_yxTail->yxNext = m_curStr;
    else
        m_yxStrings = m_curStr;
    m_yxTail = m_curStr;
    m_curStr->yxNext = nullptr;
    m_curStr = nullptr;
}

void AtomPage::addChar(GfxState *state, double x, double y, double dx, double dy, double ox, double oy, Unicode *u,
                       int uLen) {
    double x1, y1, w1, h1, dx2, dy2;
    state->transform(x, y, &x1, &y1);
    if (m_curStr && m_curStr->len){
        endString();
    }
    beginString(state, nullptr);
    state->textTransformDelta(state->getCharSpace() * state->getHorizScaling(),
                              0, &dx2, &dy2);
    dx -= dx2;
    dy -= dy2;
    state->transformDelta(dx, dy, &w1, &h1);
    if (uLen != 0) {
        w1 /= uLen;
        h1 /= uLen;
    }
    for (int i = 0; i < uLen; ++i) {
        m_curStr->addChar(state, x1 + i*w1, y1 + i*h1, w1, h1, u[i]);
    }
}

void AtomPage::conv() {
    AtomString *tmp;

    HtmlFont* h;
    for(tmp=m_yxStrings;tmp;tmp=tmp->yxNext){
        int pos=tmp->fontpos;
        //  printf("%d\n",pos);
        h=m_fonts->Get(pos);

        if (tmp->htext) delete tmp->htext;
        tmp->htext=HtmlFont::simple(h,tmp->text,tmp->len);
    }
}

void AtomPage::coalesce() {
    // todo: duplicated word
}

void AtomPage::dump(int pageNum) {
    // todo: dump
}

///////////////////////////
/// AtomOutputDev
///////////////////////////
AtomOutputDev::AtomOutputDev() {
    m_docPage = nullptr;
    m_pages = new AtomPage();
}

AtomOutputDev::~AtomOutputDev() {
    delete(m_pages);
}

void AtomOutputDev::startPage(int pageNum, GfxState *state, XRef *xref) {
//    OutputDev::startPage(pageNum, state, xref);
    this->m_pageNum = pageNum;
    m_pages->clear();
    m_pages->m_pageWidth=static_cast<int>(state->getPageWidth());
    m_pages->m_pageHeight=static_cast<int>(state->getPageHeight());
}

void AtomOutputDev::endPage() {
//    OutputDev::endPage();

    m_pages->conv();
    m_pages->coalesce();
    m_pages->dump(m_pageNum);

    // I don't yet know what to do in the case when there are pages of different
    // sizes and we want complex output: running ghostscript many times
    // seems very inefficient. So for now I'll just use last page's size
    // todo: delete these width and height?
    m_maxPageWidth = m_pages->m_pageWidth;
    m_maxPageHeight = m_pages->m_pageHeight;

    //if(!noframes&&!xml) fputs("<br/>\n", fContentsFrame);
    // todo: add some print
//    if(!stout && !globalParams->getErrQuiet()) printf("Page-%d\n",(pageNum));
}

void AtomOutputDev::updateFont(GfxState *state) {
//    OutputDev::updateFont(state);
    m_pages->updateFont(state);
}

void AtomOutputDev::beginString(GfxState *state, const GooString *s) {
    m_pages->beginString(state, s);
}

void AtomOutputDev::endString(GfxState *state) {
    m_pages->endString();
}

void AtomOutputDev::drawChar(GfxState *state, double x, double y,
                             double dx, double dy,
                             double originX, double originY,
                             CharCode code, int /*nBytes*/, Unicode *u, int uLen)
{
    // todo:
    GBool showHidden = false;
    if ( !showHidden && (state->getRender() & 3) == 3) {
        return;
    }
    m_pages->addChar(state, x, y, dx, dy, originX, originY, u, uLen);
}



void AtomOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
                                  int width, int height, GBool invert,
                                  GBool interpolate, GBool inlineImg) {
    // dump JPEG file
    if (str->getKind() == strDCT) {
        drawJpegImage(state, str);
    }
    else {
#ifdef ENABLE_LIBPNG
        drawPngImage(state, str, width, height, nullptr, gTrue);
#else
        OutputDev::drawImageMask(state, ref, str, width, height, invert, interpolate, inlineImg);
#endif
    }
}

void AtomOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
                              int width, int height, GfxImageColorMap *colorMap,
                              GBool interpolate, int *maskColors, GBool inlineImg) {
    // dump JPEG file
    if (str->getKind() == strDCT && (colorMap->getNumPixelComps() == 1 ||
                                                 colorMap->getNumPixelComps() == 3) && !inlineImg) {
        drawJpegImage(state, str);
    }
    else {
#ifdef ENABLE_LIBPNG
        drawPngImage(state, str, width, height, colorMap);
#else
        OutputDev::drawImage(state, ref, str, width, height, colorMap, interpolate,
                         maskColors, inlineImg);
#endif
    }
}

void AtomOutputDev::drawJpegImage(GfxState *state, Stream *str)
{
    // todo: add image
    FILE *f1;
    int c;

    // open the image file
    auto *fName = new GooString("test.jpg");
    if (!(f1 = fopen(fName->getCString(), "wb"))) {
        error(errIO, -1, "Couldn't open image file '{0:t}'", fName);
        delete fName;
        return;
    }

    // initialize stream
    str = str->getNextStream();
    str->reset();

    // copy the stream
    while ((c = str->getChar()) != EOF)
        fputc(c, f1);

    fclose(f1);

    if (fName) {
//        m_pages->addImage(fName, state);
    }
    delete(fName);
}

void AtomOutputDev::drawPngImage(GfxState *state, Stream *str, int width, int height,
                                 GfxImageColorMap *colorMap, GBool isMask)
{
#ifdef ENABLE_LIBPNG
    FILE *f1;

    if (!colorMap && !isMask) {
        error(errInternal, -1, "Can't have color image without a color map");
        return;
    }

    // open the image file
    auto fName=new GooString("test.png");
    if (!(f1 = fopen(fName->getCString(), "wb"))) {
        error(errIO, -1, "Couldn't open image file '{0:t}'", fName);
        delete fName;
        return;
    }

    auto *writer = new PNGWriter( isMask ? PNGWriter::MONOCHROME : PNGWriter::RGB );
    // TODO can we calculate the resolution of the image?
    if (!writer->init(f1, width, height, 72, 72)) {
        error(errInternal, -1, "Can't init PNG for image '{0:t}'", fName);
        delete writer;
        fclose(f1);
        delete fName;
        return;
    }

    if (!isMask) {
        Guchar *p;
        GfxRGB rgb;
        auto *row = (png_byte *) gmalloc(3 * width);   // 3 bytes/pixel: RGB
        png_bytep *row_pointer= &row;

        // Initialize the image stream
        auto *imgStr = new ImageStream(str, width,
                                              colorMap->getNumPixelComps(), colorMap->getBits());
        imgStr->reset();

        // For each line...
        for (int y = 0; y < height; y++) {

            // Convert into a PNG row
            p = imgStr->getLine();
            if (!p) {
                error(errIO, -1, "Failed to read PNG. '{0:t}' will be incorrect", fName);
                delete fName;
                gfree(row);
                delete writer;
                delete imgStr;
                fclose(f1);
                return;
            }
            for (int x = 0; x < width; x++) {
                colorMap->getRGB(p, &rgb);
                // Write the RGB pixels into the row
                row[3*x]= colToByte(rgb.r);
                row[3*x+1]= colToByte(rgb.g);
                row[3*x+2]= colToByte(rgb.b);
                p += colorMap->getNumPixelComps();
            }

            if (!writer->writeRow(row_pointer)) {
                error(errIO, -1, "Failed to write into PNG '{0:t}'", fName);
                delete writer;
                delete imgStr;
                delete fName;
                fclose(f1);
                return;
            }
        }
        gfree(row);
        imgStr->close();
        delete imgStr;
    }
    else { // isMask == true
        int size = (width + 7)/8;

        // PDF masks use 0 = draw current color, 1 = leave unchanged.
        // We invert this to provide the standard interpretation of alpha
        // (0 = transparent, 1 = opaque). If the colorMap already inverts
        // the mask we leave the data unchanged.
        int invert_bits = 0xff;
        if (colorMap) {
            GfxGray gray;
            Guchar zero[gfxColorMaxComps];
            memset(zero, 0, sizeof(zero));
            colorMap->getGray(zero, &gray);
            if (colToByte(gray) == 0)
                invert_bits = 0x00;
        }

        str->reset();
        auto *png_row = (Guchar *)gmalloc(size);

        for (int ri = 0; ri < height; ++ri)
        {
            for(int i = 0; i < size; i++) {
                png_row[i] = str->getChar() ^ invert_bits;
            }

            if (!writer->writeRow( &png_row ))
            {
                error(errIO, -1, "Failed to write into PNG '{0:t}'", fName);
                delete writer;
                delete fName;
                fclose(f1);
                gfree(png_row);
                return;
            }
        }
        str->close();
        gfree(png_row);
    }

    str->close();

    writer->close();
    delete writer;
    fclose(f1);
    delete fName;

//    pages->addImage(fName, state);
#endif
}
