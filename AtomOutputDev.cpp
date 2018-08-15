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
#include <algorithm>
//#include <splash/SplashMath.h>

#endif


static inline int splashRound(double x) {
    x = x+0.5;
    if (x > 0) return (int)x;
    else return (int)floor(x);
}

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
    GfxFont *font = state->getFont();
    if(font) {
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
    double ascent = 1;
    double descent = 0;
    if (font) {
        ascent = font->getAscent();
        descent = font->getDescent();
        if( ascent > 1.05 ){
            //printf( "ascent=%.15g is too high, descent=%.15g\n", ascent, descent );
            ascent = 1.05;
        }
        if( descent < -0.4 ){
            //printf( "descent %.15g is too low, ascent=%.15g\n", descent, ascent );
            descent = -0.4;
        }
    }

    yMin = y1 - ascent * m_fontSize;
    yMax = y1 - descent * m_fontSize;

    for (int i = 0; i < uLen; ++i) {
        GooString *s = textFilter (u+i, 1);
        PdfItem item(mcid, TEXT, s->getCString(), x1 + i*w1, yMin, x1 + (i+1)*w1, yMax, fontpos);
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

void AtomPage::addLine(PdfPath shape) {
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
    SplashPath *path = convertPath(state, state->getPath(), gTrue);
    SplashXPath *xpath = getSplashXPath(path);

    PdfPath pdfPath(1);
    for (int i = 0; i < xpath->getSegLength(); ++i) {
        SplashXPathSeg *seg = xpath->getSeg(i);
        pdfPath.lines.emplace_back(seg->x0, seg->y0, seg->x1, seg->y1);
    }

    m_pages->addLine(pdfPath);
    delete path;
    delete xpath;
}

void AtomOutputDev::eoFill(GfxState *state) {
    if (state->getFillColorSpace()->isNonMarking()) {
        return;
    }
    // todo: complete it.
    SplashPath *path = convertPath(state, state->getPath(), gTrue);
    SplashXPath *xpath = getSplashXPath(path);

    PdfPath pdfPath(2);
    for (int i = 0; i < xpath->getSegLength(); ++i) {
        SplashXPathSeg *seg = xpath->getSeg(i);
        pdfPath.lines.emplace_back(seg->x0, seg->y0, seg->x1, seg->y1);
    }

    m_pages->addLine(pdfPath);
    delete path;
    delete xpath;
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
//    PdfShape shape(type);
//    const int width = m_pages->m_pageInfos.m_width;
    const double height = m_pages->m_pageBox.y2 - m_pages->m_pageBox.y1;
    for (int i = 0; i < path->getNumSubpaths(); ++i) {
        GfxSubpath *subpath = path->getSubpath(i);
        if (subpath->getNumPoints() <= n) {
            continue;
        }
        PdfPath pdfPath(type);
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
//        shape.pathes.push_back(pdfPath);
    }
//    m_pages->addLine(shape);
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
            sPath->moveTo((double)subpath->getX(0),
                          (double)subpath->getY(0));
            j = 1;
            while (j < subpath->getNumPoints()) {
                if (subpath->getCurve(j)) {
                    sPath->curveTo((double)subpath->getX(j),
                                   (double)subpath->getY(j),
                                   (double)subpath->getX(j+1),
                                   (double)subpath->getY(j+1),
                                   (double)subpath->getX(j+2),
                                   (double)subpath->getY(j+2));
                    j += 3;
                } else {
                    sPath->lineTo((double)subpath->getX(j),
                                  (double)subpath->getY(j));
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



//------------------------------------------------------------------------
// SplashXPath
//------------------------------------------------------------------------

// Transform a point from user space to device space.
inline void SplashXPath::transform(double *matrix,
                                   double xi, double yi,
                                   double *xo, double *yo) {
    //                          [ m[0] m[1] 0 ]
    // [xo yo 1] = [xi yi 1] *  [ m[2] m[3] 0 ]
    //                          [ m[4] m[5] 1 ]
    *xo = xi * matrix[0] + yi * matrix[2] + matrix[4];
    *yo = xi * matrix[1] + yi * matrix[3] + matrix[5];
}

SplashXPath::SplashXPath(SplashPath *path, double *matrix,
                         double flatness, GBool closeSubpaths,
                         GBool adjustLines, int linePosI) {
    SplashPathHint *hint;
    SplashXPathPoint *pts;
    SplashXPathAdjust *adjusts, *adjust;
    double x0, y0, x1, y1, x2, y2, x3, y3, xsp, ysp;
    double adj0, adj1;
    int curSubpath, i, j;

    // transform the points
    pts = (SplashXPathPoint *)gmallocn(path->length, sizeof(SplashXPathPoint));
    for (i = 0; i < path->length; ++i) {
        transform(matrix, path->pts[i].x, path->pts[i].y, &pts[i].x, &pts[i].y);
    }

    // set up the stroke adjustment hints
    if (path->hints) {
        adjusts = (SplashXPathAdjust *) gmallocn(path->hintsLength, sizeof(SplashXPathAdjust));
        for (i = 0; i < path->hintsLength; ++i) {
            hint = &path->hints[i];
            if (hint->ctrl0 + 1 >= path->length || hint->ctrl1 + 1 >= path->length) {
                gfree(adjusts);
                adjusts = nullptr;
                break;
            }
            x0 = pts[hint->ctrl0].x;
            y0 = pts[hint->ctrl0].y;
            x1 = pts[hint->ctrl0 + 1].x;
            y1 = pts[hint->ctrl0 + 1].y;
            x2 = pts[hint->ctrl1].x;
            y2 = pts[hint->ctrl1].y;
            x3 = pts[hint->ctrl1 + 1].x;
            y3 = pts[hint->ctrl1 + 1].y;
            if (x0 == x1 && x2 == x3) {
                adjusts[i].vert = gTrue;
                adj0 = x0;
                adj1 = x2;
            } else if (y0 == y1 && y2 == y3) {
                adjusts[i].vert = gFalse;
                adj0 = y0;
                adj1 = y2;
            } else {
                gfree(adjusts);
                adjusts = nullptr;
                break;
            }
            if (adj0 > adj1) {
                x0 = adj0;
                adj0 = adj1;
                adj1 = x0;
            }
            adjusts[i].x0a = adj0 - 0.01;
            adjusts[i].x0b = adj0 + 0.01;
            adjusts[i].xma = (double) 0.5 * (adj0 + adj1) - 0.01;
            adjusts[i].xmb = (double) 0.5 * (adj0 + adj1) + 0.01;
            adjusts[i].x1a = adj1 - 0.01;
            adjusts[i].x1b = adj1 + 0.01;
            // rounding both edge coordinates can result in lines of
            // different widths (e.g., adj=10.1, adj1=11.3 --> x0=10, x1=11;
            // adj0=10.4, adj1=11.6 --> x0=10, x1=12), but it has the
            // benefit of making adjacent strokes/fills line up without any
            // gaps between them
            x0 = splashRound(adj0);
            x1 = splashRound(adj1);
            if (x1 == x0) {
                if (adjustLines) {
                    // the adjustment moves thin lines (clip rectangle with
                    // empty width or height) out of clip area, here we need
                    // a special adjustment:
                    x0 = linePosI;
                    x1 = x0 + 1;
                } else {
                    x1 = x1 + 1;
                }
            }
            adjusts[i].x0 = (double) x0;
            adjusts[i].x1 = (double) x1 - 0.01;
            adjusts[i].xm = (double) 0.5 * (adjusts[i].x0 + adjusts[i].x1);
            adjusts[i].firstPt = hint->firstPt;
            adjusts[i].lastPt = hint->lastPt;
        }

    } else {
        adjusts = nullptr;
    }

    // perform stroke adjustment
    if (adjusts) {
        for (i = 0, adjust = adjusts; i < path->hintsLength; ++i, ++adjust) {
            for (j = adjust->firstPt; j <= adjust->lastPt; ++j) {
                strokeAdjust(adjust, &pts[j].x, &pts[j].y);
            }
        }
        gfree(adjusts);
    }

    segs = nullptr;
    length = size = 0;

    x0 = y0 = xsp = ysp = 0; // make gcc happy
    adj0 = adj1 = 0; // make gcc happy
    curSubpath = 0;
    i = 0;
    while (i < path->length) {

        // first point in subpath - skip it
        if (path->flags[i] & splashPathFirst) {
            x0 = pts[i].x;
            y0 = pts[i].y;
            xsp = x0;
            ysp = y0;
            curSubpath = i;
            ++i;

        } else {

            // curve segment
            if (path->flags[i] & splashPathCurve) {
                x1 = pts[i].x;
                y1 = pts[i].y;
                x2 = pts[i+1].x;
                y2 = pts[i+1].y;
                x3 = pts[i+2].x;
                y3 = pts[i+2].y;
                addCurve(x0, y0, x1, y1, x2, y2, x3, y3,
                         flatness,
                         (path->flags[i-1] & splashPathFirst),
                         (path->flags[i+2] & splashPathLast),
                         !closeSubpaths &&
                         (path->flags[i-1] & splashPathFirst) &&
                         !(path->flags[i-1] & splashPathClosed),
                         !closeSubpaths &&
                         (path->flags[i+2] & splashPathLast) &&
                         !(path->flags[i+2] & splashPathClosed));
                x0 = x3;
                y0 = y3;
                i += 3;

                // line segment
            } else {
                x1 = pts[i].x;
                y1 = pts[i].y;
                addSegment(x0, y0, x1, y1);
                x0 = x1;
                y0 = y1;
                ++i;
            }

            // close a subpath
            if (closeSubpaths &&
                (path->flags[i-1] & splashPathLast) &&
                (pts[i-1].x != pts[curSubpath].x ||
                 pts[i-1].y != pts[curSubpath].y)) {
                addSegment(x0, y0, xsp, ysp);
            }
        }
    }

    gfree(pts);
}

// Apply the stroke adjust hints to point <pt>: (*<xp>, *<yp>).
void SplashXPath::strokeAdjust(SplashXPathAdjust *adjust,
                               double *xp, double *yp) {
    double x, y;

    if (adjust->vert) {
        x = *xp;
        if (x > adjust->x0a && x < adjust->x0b) {
            *xp = adjust->x0;
        } else if (x > adjust->xma && x < adjust->xmb) {
            *xp = adjust->xm;
        } else if (x > adjust->x1a && x < adjust->x1b) {
            *xp = adjust->x1;
        }
    } else {
        y = *yp;
        if (y > adjust->x0a && y < adjust->x0b) {
            *yp = adjust->x0;
        } else if (y > adjust->xma && y < adjust->xmb) {
            *yp = adjust->xm;
        } else if (y > adjust->x1a && y < adjust->x1b) {
            *yp = adjust->x1;
        }
    }
}

SplashXPath::SplashXPath(SplashXPath *xPath) {
    length = xPath->length;
    size = xPath->size;
    segs = (SplashXPathSeg *)gmallocn(size, sizeof(SplashXPathSeg));
    memcpy(segs, xPath->segs, length * sizeof(SplashXPathSeg));
}

SplashXPath::~SplashXPath() {
    gfree(segs);
}

// Add space for <nSegs> more segments
void SplashXPath::grow(int nSegs) {
    if (length + nSegs > size) {
        if (size == 0) {
            size = 32;
        }
        while (size < length + nSegs) {
            size *= 2;
        }
        segs = (SplashXPathSeg *)greallocn(segs, size, sizeof(SplashXPathSeg));
    }
}

void SplashXPath::addCurve(double x0, double y0,
                           double x1, double y1,
                           double x2, double y2,
                           double x3, double y3,
                           double flatness,
                           GBool first, GBool last, GBool end0, GBool end1) {
    double *cx = new double[(splashMaxCurveSplits + 1) * 3];
    double *cy = new double[(splashMaxCurveSplits + 1) * 3];
    int *cNext = new int[splashMaxCurveSplits + 1];
    double xl0, xl1, xl2, xr0, xr1, xr2, xr3, xx1, xx2, xh;
    double yl0, yl1, yl2, yr0, yr1, yr2, yr3, yy1, yy2, yh;
    double dx, dy, mx, my, d1, d2, flatness2;
    int p1, p2, p3;

#ifdef USE_FIXEDPOINT
    flatness2 = flatness;
#else
    flatness2 = flatness * flatness;
#endif

    // initial segment
    p1 = 0;
    p2 = splashMaxCurveSplits;

    *(cx + p1 * 3 + 0) = x0;
    *(cx + p1 * 3 + 1) = x1;
    *(cx + p1 * 3 + 2) = x2;
    *(cx + p2 * 3 + 0) = x3;

    *(cy + p1 * 3 + 0) = y0;
    *(cy + p1 * 3 + 1) = y1;
    *(cy + p1 * 3 + 2) = y2;
    *(cy + p2 * 3 + 0) = y3;

    *(cNext + p1) = p2;

    while (p1 < splashMaxCurveSplits) {

        // get the next segment
        xl0 = *(cx + p1 * 3 + 0);
        xx1 = *(cx + p1 * 3 + 1);
        xx2 = *(cx + p1 * 3 + 2);

        yl0 = *(cy + p1 * 3 + 0);
        yy1 = *(cy + p1 * 3 + 1);
        yy2 = *(cy + p1 * 3 + 2);

        p2 = *(cNext + p1);

        xr3 = *(cx + p2 * 3 + 0);
        yr3 = *(cy + p2 * 3 + 0);

        // compute the distances from the control points to the
        // midpoint of the straight line (this is a bit of a hack, but
        // it's much faster than computing the actual distances to the
        // line)
        mx = (xl0 + xr3) * 0.5;
        my = (yl0 + yr3) * 0.5;
#ifdef USE_FIXEDPOINT
        d1 = splashDist(xx1, yy1, mx, my);
    d2 = splashDist(xx2, yy2, mx, my);
#else
        dx = xx1 - mx;
        dy = yy1 - my;
        d1 = dx*dx + dy*dy;
        dx = xx2 - mx;
        dy = yy2 - my;
        d2 = dx*dx + dy*dy;
#endif

        // if the curve is flat enough, or no more subdivisions are
        // allowed, add the straight line segment
        if (p2 - p1 == 1 || (d1 <= flatness2 && d2 <= flatness2)) {
            addSegment(xl0, yl0, xr3, yr3);
            p1 = p2;

            // otherwise, subdivide the curve
        } else {
            xl1 = (xl0 + xx1) * 0.5;
            yl1 = (yl0 + yy1) * 0.5;
            xh = (xx1 + xx2) * 0.5;
            yh = (yy1 + yy2) * 0.5;
            xl2 = (xl1 + xh) * 0.5;
            yl2 = (yl1 + yh) * 0.5;
            xr2 = (xx2 + xr3) * 0.5;
            yr2 = (yy2 + yr3) * 0.5;
            xr1 = (xh + xr2) * 0.5;
            yr1 = (yh + yr2) * 0.5;
            xr0 = (xl2 + xr1) * 0.5;
            yr0 = (yl2 + yr1) * 0.5;
            // add the new subdivision points
            p3 = (p1 + p2) / 2;

            *(cx + p1 * 3 + 1) = xl1;
            *(cx + p1 * 3 + 2) = xl2;

            *(cy + p1 * 3 + 1) = yl1;
            *(cy + p1 * 3 + 2) = yl2;

            *(cNext + p1) = p3;

            *(cx + p3 * 3 + 0) = xr0;
            *(cx + p3 * 3 + 1) = xr1;
            *(cx + p3 * 3 + 2) = xr2;

            *(cy + p3 * 3 + 0) = yr0;
            *(cy + p3 * 3 + 1) = yr1;
            *(cy + p3 * 3 + 2) = yr2;

            *(cNext + p3) = p2;
        }
    }

    delete [] cx;
    delete [] cy;
    delete [] cNext;
}

void SplashXPath::addSegment(double x0, double y0,
                             double x1, double y1) {
    grow(1);
    segs[length].x0 = x0;
    segs[length].y0 = y0;
    segs[length].x1 = x1;
    segs[length].y1 = y1;
    segs[length].flags = 0;
    if (y1 == y0) {
        segs[length].dxdy = segs[length].dydx = 0;
        segs[length].flags |= splashXPathHoriz;
        if (x1 == x0) {
            segs[length].flags |= splashXPathVert;
        }
    } else if (x1 == x0) {
        segs[length].dxdy = segs[length].dydx = 0;
        segs[length].flags |= splashXPathVert;
    } else {
#ifdef USE_FIXEDPOINT
        if (FixedPoint::divCheck(x1 - x0, y1 - y0, &segs[length].dxdy)) {
      segs[length].dydx = (double)1 / segs[length].dxdy;
    } else {
      segs[length].dxdy = segs[length].dydx = 0;
      if (splashAbs(x1 - x0) > splashAbs(y1 - y0)) {
	segs[length].flags |= splashXPathHoriz;
      } else {
	segs[length].flags |= splashXPathVert;
      }
    }
#else
        segs[length].dxdy = (x1 - x0) / (y1 - y0);
        segs[length].dydx = (double)1 / segs[length].dxdy;
#endif
    }
    if (y0 > y1) {
        segs[length].flags |= splashXPathFlip;
    }
    ++length;
}

struct cmpXPathSegsFunctor {
    bool operator()(const SplashXPathSeg &seg0, const SplashXPathSeg &seg1) {
        double x0, y0, x1, y1;

        if (seg0.flags & splashXPathFlip) {
            x0 = seg0.x1;
            y0 = seg0.y1;
        } else {
            x0 = seg0.x0;
            y0 = seg0.y0;
        }
        if (seg1.flags & splashXPathFlip) {
            x1 = seg1.x1;
            y1 = seg1.y1;
        } else {
            x1 = seg1.x0;
            y1 = seg1.y0;
        }
        return (y0 != y1) ? (y0 < y1) : (x0 < x1);
    }
};

void SplashXPath::aaScale() {
    // kangjuchi
    SplashXPathSeg *seg;
    int i;

    for (i = 0, seg = segs; i < length; ++i, ++seg) {
        seg->x0 *= splashAASize;
        seg->y0 *= splashAASize;
        seg->x1 *= splashAASize;
        seg->y1 *= splashAASize;
    }
}

void SplashXPath::sort() {
    std::sort(segs, segs + length, cmpXPathSegsFunctor());
}

