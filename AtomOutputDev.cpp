//
// Created by weditor on 18-7-31.
//

#include <cmath>
#include <iostream>
#include <goo/PNGWriter.h>
#include "AtomOutputDev.h"
#include "GfxState.h"
#include "GfxFont.h"
#include "HtmlUtils.h"
#include <goo/GooList.h>
#include "HtmlFonts.h"

#include "SplashOutputDev.h"
#include "splash/SplashPath.h"


#ifdef ENABLE_LIBPNG
#include <png.h>
#include <poppler/UnicodeMap.h>
#include <poppler/GlobalParams.h>

#endif


// todo: 重叠的文字.
// todo: 字体的render
GooString* textFilter(const Unicode* u, int uLen) {
    auto *tmp = new GooString();
    UnicodeMap *uMap;
    char buf[8];
    int n;

    // get the output encoding
    if (!(uMap = globalParams->getTextEncoding())) {
        return tmp;
    }

    for (int i = 0; i < uLen; ++i) {
        // skip control characters.  W3C disallows them and they cause a warning
        // with PHP.
        if (u[i] <= 31)
            continue;
        // convert unicode to string
        if ((n = uMap->mapUnicode(u[i], buf, sizeof(buf))) > 0) {
            tmp->append(buf, n);
        }
    }

    uMap->decRefCnt();
    return tmp;
}

AtomImage::AtomImage(GfxState *state) {
    state->transform(0, 0, &xMin, &yMax);
    state->transform(1, 1, &xMax, &yMin);
}


//////////////////////
/// AtomPage
//////////////////////

AtomPage::AtomPage() {
    m_fontSize = 0;		// current font size
    m_fonts = new HtmlFontAccu();
    m_lineList = new GooList();
    m_lastBoxId = -1;
}

AtomPage::~AtomPage() {
    this->clear();
    delete m_fonts;
    delete m_lineList;
}

void AtomPage::clear() {
    while (m_lineList->getLength()) {
        int last_idx = m_lineList->getLength()-1;
        delete((AtomLine*)m_lineList->get(last_idx));
        m_lineList->del(last_idx);
    }
    m_pageInfos = PageInfos();
    m_lastBox = AtomBox();
    m_pageBox = AtomBox();
}

void AtomPage::updateFont(GfxState *state) {
    GfxFont *font;
    double *fm;
    char *name;
    int code;
    double w;

    // todo: update font here
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
}

void AtomPage::endString() {
}

void AtomPage::addChar(GfxState *state, double x, double y, double dx, double dy, double ox, double oy, Unicode *u,
                       int uLen, int mcid) {
    double x1, y1, w1, h1, dx2, dy2;
    state->transform(x, y, &x1, &y1);
    beginString(state, nullptr);
    state->textTransformDelta(state->getCharSpace() * state->getHorizScaling(), 0, &dx2, &dy2);
    dx -= dx2;
    dy -= dy2;
    state->transformDelta(dx, dy, &w1, &h1);
    if (uLen != 0) {
        w1 /= uLen;
        h1 /= uLen;
    }
    double xMin, yMin, xMax, yMax;
    state->getClipBBox(&xMin, &yMin, &xMax, &yMax);
    AtomBox box(xMin, yMin, xMax, yMax);

    if (box != m_lastBox ){
        if(box != m_pageBox){
            PdfItem item(-1, CELL, "", xMin, yMin, xMax, yMax);
            m_lastBoxId = m_pageInfos.addItem(item, -1) - 1;
        }
        else {
            m_lastBoxId = -1;
        }
        m_lastBox = box;
    }

    int fontpos = -1;
    if(GfxFont *font = state->getFont()) {
        GfxRGB rgb;
        state->getFillRGB(&rgb);
        // todo: 增加render, type.
        HtmlFont hfont = HtmlFont(font, static_cast<int>(state->getFontSize()), rgb);
        if (isMatRotOrSkew(state->getTextMat())) {
            double normalizedMatrix[4];
            memcpy(normalizedMatrix, state->getTextMat(), sizeof(normalizedMatrix));
            normalizedMatrix[1] *= -1;
            normalizedMatrix[2] *= -1;
            normalizeRotMat(normalizedMatrix);
            hfont.setRotMat(normalizedMatrix);
        }
        fontpos = m_fonts->AddFont(hfont);
    }

    for (int i = 0; i < uLen; ++i) {
        GooString *s = textFilter (u+i, 1);
        PdfItem item(mcid, TEXT, s->getCString(), x1 + i*w1, y1 + i*h1, x1 + (i+1)*w1, h1, fontpos);
        m_pageInfos.addItem(item, m_lastBoxId);
        delete s;
    }
}

void AtomPage::conv() {

}

void AtomPage::addImage(AtomImage img) {
    PdfImage pdfImage(img.xMin, img.yMin, img.xMax, img.yMax);
    m_pageInfos.m_images.push_back(pdfImage);
}

void AtomPage::addLine(PdfShape shape) {
    m_pageInfos.m_lines.push_back(shape);
}


void AtomPage::setPageBoarder(const double width, const double height){
    m_pageBox = AtomBox(0, 0, width, height);
}

void AtomPage::coalesce() {
    // todo: duplicated word
}

void AtomPage::dump(unsigned int pageNum, PageInfos &pageInfos) {
    pageInfos = m_pageInfos;
    for (int i = 0; i < m_fonts->size(); ++i) {
        // todo: add render, type, weight
        auto font = m_fonts->Get(0);
        GooString *color = font->getColor().toString();
        GooString *fontName = font->getFullName();
        PdfFont pdfFont(i, fontName->getCString(), 1, font->getSize(), 100, color->getCString(),
                font->isBold(), font->isItalic(), font->getLineSize(), 0);
        delete(fontName);
        delete(color);
        pageInfos.m_fonts.push_back(pdfFont);
    }
    pageInfos.m_page_num = pageNum;
    pageInfos.m_width = int(m_pageBox.x2);
    pageInfos.m_height = int(m_pageBox.y2);
}

///////////////////////////
/// AtomOutputDev
///////////////////////////
AtomOutputDev::AtomOutputDev() {
    m_pages = new AtomPage();
    m_ok = gTrue;
    memset(m_matrix, 0, sizeof(m_matrix));
}

AtomOutputDev::~AtomOutputDev() {
    delete(m_pages);
}

void AtomOutputDev::startPage(int pageNum, GfxState *state, XRef *xref) {
//    todo:
    if (state) {
        const double * const ctm = state->getCTM();
        m_matrix[0] = ctm[0];
        m_matrix[1] = ctm[1];
        m_matrix[2] = ctm[2];
        m_matrix[3] = ctm[3];
        m_matrix[4] = ctm[4];
        m_matrix[5] = ctm[5];
    }
    setFlatness(1);

    this->m_pageNum = pageNum;
    m_pages->clear();
    m_pages->setPageBoarder(state->getPageWidth(), state->getPageHeight());
}

void AtomOutputDev::endPage() {
    m_pages->conv();
    m_pages->coalesce();
    memset(m_matrix, 0, sizeof(m_matrix));
}

void AtomOutputDev::updateCTM(GfxState *state, double m11, double m12,
                                double m21, double m22,
                                double m31, double m32) {
    const double * ctm = state->getCTM();
    m_matrix[0] = ctm[0];
    m_matrix[1] = ctm[1];
    m_matrix[2] = ctm[2];
    m_matrix[3] = ctm[3];
    m_matrix[4] = ctm[4];
    m_matrix[5] = ctm[5];
}

void AtomOutputDev::updateFont(GfxState *state) {
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
    m_pages->addChar(state, x, y, dx, dy, originX, originY, u, uLen, getMcid());
}

void AtomOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
                                  int width, int height, GBool invert,
                                  GBool interpolate, GBool inlineImg) {
    m_pages->addImage(AtomImage(state));
    OutputDev::drawImageMask(state, ref, str, width, height, invert, interpolate, inlineImg);
}

void AtomOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
                              int width, int height, GfxImageColorMap *colorMap,
                              GBool interpolate, int *maskColors, GBool inlineImg) {
    m_pages->addImage(AtomImage(state));
    OutputDev::drawImage(state, ref, str, width, height, colorMap, interpolate,
                         maskColors, inlineImg);
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
        auto *imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
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


void AtomOutputDev::stroke(GfxState *state) {
    if (state->getStrokeColorSpace()->isNonMarking()) {
        return;
    }
    convertPath(state, state->getPath(), gFalse);
}

void AtomOutputDev::fill(GfxState *state) {
    if (state->getFillColorSpace()->isNonMarking()) {
        return;
    }
    convertPath(state, state->getPath(), gTrue);
}

void AtomOutputDev::eoFill(GfxState *state) {
    if (state->getFillColorSpace()->isNonMarking()) {
        return;
    }
    // todo: complete it.
    SplashPath *path = convertPath(state, state->getPath(), gTrue);
    SplashXPath *xpath = getSplashXPath(path);
}

void AtomOutputDev::clip(GfxState *state) {
//    double xmin, ymin, xmax, ymax;
//    state->getClipBBox(&xmin, &ymin, &xmax, &ymax);
//    std::cout<<"clip:"<<xmin<<", "<<ymin<<", "<<xmax<<", "<<ymax<<std::endl;
//    convertPath(state, state->getPath(), gTrue, 3);
}

void AtomOutputDev::eoClip(GfxState *state) {
//    double xmin, ymin, xmax, ymax;
//    state->getClipBBox(&xmin, &ymin, &xmax, &ymax);
//    std::cout<<"eoclip:"<<xmin<<", "<<ymin<<", "<<xmax<<", "<<ymax<<std::endl;
//    convertPath(state, state->getPath(), gTrue, 4);
}

void AtomOutputDev::beginMarkedContent(const char * name, Dict * properties){
    int id = -1;
    if(properties) {
        properties->lookupInt("MCID", nullptr, &id);
    }

    if (id == -1){
        return;
    }
    m_mcidStack.push_back(id);
}
void AtomOutputDev::endMarkedContent(GfxState * state){
    if(inMarkedContent()) {
        m_mcidStack.pop_back();
    }
}


void AtomOutputDev::convertPath2(GfxState *state, GfxPath *path, GBool dropEmptySubpaths, int type) {
    const int n = dropEmptySubpaths ? 1 : 0;
    PdfShape shape(type);
//    const int width = m_pages->m_pageInfos.m_width;
    const double height = m_pages->m_pageBox.y2 - m_pages->m_pageBox.y1;
    for (int i = 0; i < path->getNumSubpaths(); ++i) {
        GfxSubpath *subpath = path->getSubpath(i);
        if (subpath->getNumPoints() <= n) {
            continue;
        }
        PdfPath pdfPath;
        double lastX=subpath->getX(0), lastY=height-subpath->getY(0);
        int j = 1;
        while (j < subpath->getNumPoints()) {
            if (subpath->getCurve(j)) {
                // 绘制圆弧
                PdfLine pdfLine(
                    subpath->getX(j), height-subpath->getY(j),
                    subpath->getX(j + 1), height-subpath->getY(j + 1),
                    subpath->getX(j + 2), height-subpath->getY(j + 2)
                );
                pdfPath.lines.push_back(pdfLine);
                lastX = subpath->getX(j + 2);
                lastY = height-subpath->getY(j + 2);
                j += 3;
            } else {
                // 绘制直线.
                PdfLine pdfLine(
                    lastX, lastY,
                    subpath->getX(j), height-subpath->getY(j)
                );
                pdfPath.lines.push_back(pdfLine);
                lastX = subpath->getX(j);
                lastY = height-subpath->getY(j);
                ++j;
            }
        }
        shape.pathes.push_back(pdfPath);
    }
    m_pages->addLine(shape);
}

void AtomOutputDev::getInfo(unsigned int pageNum, PageInfos &pageInfos) {
    m_pages->dump(pageNum, pageInfos);
}

SplashPath *AtomOutputDev::convertPath(GfxState *state, GfxPath *path, GBool dropEmptySubpaths) {
    SplashPath *sPath;
    GfxSubpath *subpath;
    int n, i, j;

    n = dropEmptySubpaths ? 1 : 0;
    sPath = new SplashPath();
    for (i = 0; i < path->getNumSubpaths(); ++i) {
        subpath = path->getSubpath(i);
        if (subpath->getNumPoints() > n) {
            sPath->reserve(subpath->getNumPoints() + 1);
            sPath->moveTo((SplashCoord)subpath->getX(0),
                          (SplashCoord)subpath->getY(0));
            j = 1;
            while (j < subpath->getNumPoints()) {
                if (subpath->getCurve(j)) {
                    sPath->curveTo((SplashCoord)subpath->getX(j),
                                   (SplashCoord)subpath->getY(j),
                                   (SplashCoord)subpath->getX(j+1),
                                   (SplashCoord)subpath->getY(j+1),
                                   (SplashCoord)subpath->getX(j+2),
                                   (SplashCoord)subpath->getY(j+2));
                    j += 3;
                } else {
                    sPath->lineTo((SplashCoord)subpath->getX(j),
                                  (SplashCoord)subpath->getY(j));
                    ++j;
                }
            }
            if (subpath->isClosed()) {
                sPath->close();
            }
        }
    }
    return sPath;
}

void AtomOutputDev::updateFlatness(GfxState *state) {
#if 0 // Acrobat ignores the flatness setting, and always renders curves
    // with a fairly small flatness value
   splash->setFlatness(state->getFlatness());
#endif
}

void AtomOutputDev::setFlatness(const double flatness) {
    if (flatness < 1) {
        m_flatness = 1;
    } else {
        m_flatness = flatness;
    }
}


SplashXPath* AtomOutputDev::getSplashXPath(SplashPath *path) {
    GBool adjustLine = gFalse;
    int linePosI = 0;
    return new SplashXPath(path, m_matrix, m_flatness, gTrue,
                            adjustLine, linePosI);
}
