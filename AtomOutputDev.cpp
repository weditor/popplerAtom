//
// Created by weditor on 18-7-31.
//

#include <cmath>
#include <climits>
#include <iostream>
#include <goo/PNGWriter.h>
// #include <goo/GooList.h>
#include "AtomOutputDev.h"
#include "GfxState.h"
#include "GfxFont.h"
#include "AtomPath.h"

#ifdef ENABLE_LIBPNG
#include <png.h>
#endif

#include <poppler/UnicodeMap.h>
#include <poppler/GlobalParams.h>
#include <algorithm>

static inline bool is_within(double a, double thresh, double b)
{
    return fabs(a - b) < thresh;
}

static inline bool rot_matrices_equal(const double *const mat0, const double *const mat1)
{
    return is_within(mat0[0], .1, mat1[0]) && is_within(mat0[1], .1, mat1[1]) &&
           is_within(mat0[2], .1, mat1[2]) && is_within(mat0[3], .1, mat1[3]);
}

// rotation is (cos q, sin q, -sin q, cos q, 0, 0)
// sin q is zero iff there is no rotation, or 180 deg. rotation;
// for 180 rotation, cos q will be negative
static inline bool isMatRotOrSkew(const double *const mat)
{
    return mat[0] < 0 || !is_within(mat[1], .1, 0);
}

static inline void normalizeRotMat(double *mat)
{
    double scale = fabs(mat[0] + mat[1]);
    if (!scale)
        return;
    for (int i = 0; i < 4; i++)
        mat[i] /= scale;
}

static std::string convtoX(unsigned int xcol)
{
    std::string xret;
    char tmp;
    unsigned int k;
    k = (xcol / 16);
    if (k < 10)
        tmp = (char)('0' + k);
    else
        tmp = (char)('a' + k - 10);
    xret.push_back(tmp);
    k = (xcol % 16);
    if (k < 10)
        tmp = (char)('0' + k);
    else
        tmp = (char)('a' + k - 10);
    xret.push_back(tmp);
    return xret;
}

static std::string color2string(GfxRGB rgb)
{
    std::string result("#");
    auto r = static_cast<unsigned int>(rgb.r / 65535.0 * 255.0);
    auto g = static_cast<unsigned int>(rgb.g / 65535.0 * 255.0);
    auto b = static_cast<unsigned int>(rgb.b / 65535.0 * 255.0);
    if (r > 255 || g > 255 || b > 255)
    {
        if (!globalParams->getErrQuiet())
            fprintf(stderr, "Error : Bad color (%d,%d,%d) reset to (0,0,0)\n", r, g, b);
        r = 0;
        g = 0;
        b = 0;
    }
    result.append(convtoX(r));
    result.append(convtoX(g));
    result.append(convtoX(b));
    return result;
}

GooString *textFilter(const Unicode *u, int uLen)
{
    auto *tmp = new GooString();
    UnicodeMap *uMap;
    char buf[8];
    int n;

    // get the output encoding
    if (!(uMap = globalParams->getTextEncoding()))
    {
        return tmp;
    }

    for (int i = 0; i < uLen; ++i)
    {
        // skip control characters.  W3C disallows them and they cause a warning
        // with PHP.
        if (u[i] <= 31)
            continue;
        // convert unicode to string
        if ((n = uMap->mapUnicode(u[i], buf, sizeof(buf))) > 0)
        {
            tmp->append(buf, n);
        }
    }

    uMap->decRefCnt();
    return tmp;
}

AtomImage::AtomImage(GfxState *state)
{
    state->transform(0, 0, &xMin, &yMax);
    state->transform(1, 1, &xMax, &yMin);
}

AtomFont::AtomFont(GfxFont *font, int _size, std::string color, int render)
{
    this->color = color;
    fontName = defaultFont;
    if (font->getName())
    {
        fontName = font->getName()->toStr();
    }

    size = (unsigned int)(_size);
    italic = false;
    bold = false;
    rotOrSkewed = false;
    weight = font->getWeight() * 100;
    type = font->getType();
    this->render = static_cast<unsigned int>(render);

    auto *fontname = new GooString(fontName.c_str());
    if (font->isBold() || font->getWeight() >= GfxFont::W700 || strstr(fontname->lowerCase()->c_str(), "bold"))
    {
        bold = true;
    }

    if (font->isItalic() || strstr(fontname->lowerCase()->c_str(), "italic") || strstr(fontname->lowerCase()->c_str(), "oblique"))
    {
        italic = true;
    }

    rotSkewMat[0] = rotSkewMat[1] = rotSkewMat[2] = rotSkewMat[3] = 0;
    delete fontname;
}

const std::string AtomFont::defaultFont = "Times";

AtomFont::AtomFont(const AtomFont &x)
{
    size = x.size;
    italic = x.italic;
    bold = x.bold;
    color = x.color;
    fontName = x.fontName;
    type = x.type;
    weight = x.weight;
    render = x.render;
    rotOrSkewed = x.rotOrSkewed;
    memcpy(rotSkewMat, x.rotSkewMat, sizeof(rotSkewMat));
}

AtomFont::~AtomFont()
{
}

AtomFont &AtomFont::operator=(const AtomFont &x)
{
    if (this == &x)
    {
        return *this;
    }
    size = x.size;
    italic = x.italic;
    bold = x.bold;
    color = x.color;
    fontName = x.fontName;
    type = x.type;
    weight = x.weight;
    render = x.render;
    rotOrSkewed = x.rotOrSkewed;
    memcpy(rotSkewMat, x.rotSkewMat, sizeof(rotSkewMat));
    return *this;
}

/*
  This function is used to compare font uniquely for insertion into
  the list of all encountered fonts
*/
bool AtomFont::isEqual(const AtomFont &x) const
{
    return (size == x.size) &&
           (bold == x.bold) &&
           (italic == x.italic) &&
           (color == x.getColor()) &&
           (type == x.type) &&
           weight == x.weight &&
           render == x.render &&
           isRotOrSkewed() == x.isRotOrSkewed() &&
           (!isRotOrSkewed() || rot_matrices_equal(getRotMat(), x.getRotMat()));
}

const std::string &AtomFont::getFontName() const
{
    return fontName;
}

AtomFontManager::AtomFontManager() {}

AtomFontManager::~AtomFontManager() {}

unsigned long AtomFontManager::AddFont(const AtomFont &font)
{
    for (auto fontIter = m_fonts.begin(); fontIter != m_fonts.end(); ++fontIter)
    {
        if (font.isEqual(*fontIter))
        {
            return static_cast<unsigned long>(fontIter - m_fonts.begin());
        }
    }

    m_fonts.push_back(font);
    return (m_fonts.size() - 1);
}

//////////////////////
/// AtomPage
//////////////////////

AtomPage::AtomPage()
{
    m_fontSize = 0; // current font size
    m_fonts = new AtomFontManager();
    // m_lineList = new GooList();
    m_lastBoxId = -1;
}

AtomPage::~AtomPage()
{
    this->clear();
    delete m_fonts;
    // delete m_lineList;
}

void AtomPage::clear()
{
    // while (m_lineList->getLength())
    // {
    //     int last_idx = m_lineList->getLength() - 1;
    //     delete ((AtomLine *)m_lineList->get(last_idx));
    //     m_lineList->del(last_idx);
    // }
    m_pageInfos = PageInfos();
    m_lastBox = AtomBox();
    m_pageBox = AtomBox();
}

void AtomPage::updateFont(GfxState *state)
{
    GfxFont *font;
    const double *fm;
    const char *name;
    int code;
    double w;

    // adjust the font size
    m_fontSize = state->getTransformedFontSize();
    if ((font = state->getFont()) && font->getType() == fontType3)
    {
        // This is a hack which makes it possible to deal with some Type 3
        // fonts.  The problem is that it's impossible to know what the
        // base coordinate system used in the font is without actually
        // rendering the font.  This code tries to guess by looking at the
        // width of the character 'm' (which breaks if the font is a
        // subset that doesn't contain 'm').
        for (code = 0; code < 256; ++code)
        {
            if ((name = ((Gfx8BitFont *)font)->getCharName(code)) &&
                name[0] == 'm' && name[1] == '\0')
            {
                break;
            }
        }
        if (code < 256)
        {
            w = ((Gfx8BitFont *)font)->getWidth(code);
            if (w != 0)
            {
                // 600 is a generic average 'm' width -- yes, this is a hack
                m_fontSize *= w / 0.6;
            }
        }
        fm = font->getFontMatrix();
        if (fm[0] != 0)
        {
            m_fontSize *= fabs(fm[3] / fm[0]);
        }
    }
}

void AtomPage::beginString(GfxState *state, const GooString *s)
{
}

void AtomPage::endString()
{
}

void AtomPage::addChar(GfxState *state, double x, double y, double dx, double dy, double ox, double oy, Unicode *u,
                       int uLen, int mcid)
{
    double x1, y1, w1, h1, dx2, dy2;
    state->transform(x, y, &x1, &y1);
    beginString(state, nullptr);
    state->textTransformDelta(state->getCharSpace() * state->getHorizScaling(), 0, &dx2, &dy2);
    dx -= dx2;
    dy -= dy2;
    state->transformDelta(dx, dy, &w1, &h1);
    if (uLen != 0)
    {
        w1 /= uLen;
        h1 /= uLen;
    }
    double xMin, yMin, xMax, yMax;
    state->getClipBBox(&xMin, &yMin, &xMax, &yMax);
    AtomBox box(xMin, yMin, xMax, yMax);

    if (box != m_lastBox)
    {
        if (box != m_pageBox)
        {
            PdfItem item(-1, CELL, "", xMin, yMin, xMax, yMax);
            m_lastBoxId = m_pageInfos.addItem(item, -1) - 1;
        }
        else
        {
            m_lastBoxId = -1;
        }
        m_lastBox = box;
    }

    int fontpos = -1;
    GfxFont *font = state->getFont();
    if (font)
    {
        GfxRGB rgb;
        state->getFillRGB(&rgb);
        AtomFont hfont = AtomFont(font, static_cast<int>(state->getFontSize()), color2string(rgb), state->getRender());
        if (isMatRotOrSkew(state->getTextMat()))
        {
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
    if (font)
    {
        ascent = font->getAscent();
        descent = font->getDescent();
        if (ascent > 1.05)
        {
            //printf( "ascent=%.15g is too high, descent=%.15g\n", ascent, descent );
            ascent = 1.05;
        }
        if (descent < -0.4)
        {
            //printf( "descent %.15g is too low, ascent=%.15g\n", descent, ascent );
            descent = -0.4;
        }
    }

    yMin = y1 - ascent * m_fontSize;
    yMax = y1 - descent * m_fontSize;

    for (int i = 0; i < uLen; ++i)
    {
        GooString *s = textFilter(u + i, 1);
        PdfItem item(mcid, TEXT, s->c_str(), x1 + i * w1, yMin, x1 + (i + 1) * w1, yMax, fontpos);
        delete s;
        s = nullptr;
        const PdfItem *lastItem = m_pageInfos.getLastItem(m_lastBoxId);

        // text overlap
        if (lastItem && lastItem->type == item.type && std::abs(lastItem->xcenter() - item.xcenter()) < (lastItem->width() + item.width()) * 0.2 && std::abs(lastItem->ycenter() - item.ycenter()) < (lastItem->height() + item.height()) * 0.2)
        {
            continue;
        }
        m_pageInfos.addItem(item, m_lastBoxId);
    }
}

void AtomPage::conv()
{
}

void AtomPage::addImage(AtomImage img)
{
    PdfImage pdfImage(img.xMin, img.yMin, img.xMax, img.yMax);
    m_pageInfos.m_images.push_back(pdfImage);
}

void AtomPage::addLine(PdfPath shape)
{
    if (shape.isLine())
    {
        int xMin = INT_MAX, yMin = INT_MAX, xMax = INT_MIN, yMax = INT_MIN;
        for (auto line : shape.lines)
        {
            xMin = std::min(xMin, std::min(line.x0, line.x1));
            yMin = std::min(yMin, std::min(line.y0, line.y1));
            xMax = std::max(xMax, std::max(line.x0, line.x1));
            yMax = std::max(yMax, std::max(line.y0, line.y1));
        }
        PdfLine line(xMin, yMin, xMax, yMax);
        m_pageInfos.m_lines.push_back(line);
    }
    else
    {
        m_pageInfos.m_graphs.push_back(shape);
    }
}

void AtomPage::setPageBoarder(const double width, const double height)
{
    m_pageBox = AtomBox(0, 0, width, height);
}

void AtomPage::coalesce()
{
}

void AtomPage::dump(unsigned int pageNum, PageInfos &pageInfos)
{
    pageInfos = m_pageInfos;
    for (int i = 0; i < m_fonts->size(); ++i)
    {
        auto font = m_fonts->Get(0);
        const std::string &color = font->getColor();
        const std::string &fontName = font->getFontName();
        PdfFont pdfFont(i, fontName, font->getType(), font->getSize(), font->getWeight(), color,
                        font->isBold(), font->isItalic(), -1, font->getRender());
        pageInfos.m_fonts.push_back(pdfFont);
    }
    pageInfos.m_page_num = pageNum;
    pageInfos.m_width = int(m_pageBox.x2);
    pageInfos.m_height = int(m_pageBox.y2);
}

///////////////////////////
/// AtomOutputDev
///////////////////////////
AtomOutputDev::AtomOutputDev()
{
    m_pages = new AtomPage();
    m_ok = true;
    memset(m_matrix, 0, sizeof(m_matrix));
}

AtomOutputDev::~AtomOutputDev()
{
    delete (m_pages);
}

void AtomOutputDev::startPage(int pageNum, GfxState *state, XRef *xref)
{
    const double *const ctm = state->getCTM();
    m_matrix[0] = ctm[0];
    m_matrix[1] = ctm[1];
    m_matrix[2] = ctm[2];
    m_matrix[3] = ctm[3];
    m_matrix[4] = ctm[4];
    m_matrix[5] = ctm[5];
    setFlatness(1);

    this->m_pageNum = pageNum;
    m_pages->clear();
    m_pages->setPageBoarder(state->getPageWidth(), state->getPageHeight());
}

void AtomOutputDev::endPage()
{
    m_pages->conv();
    m_pages->coalesce();
    memset(m_matrix, 0, sizeof(m_matrix));
}

void AtomOutputDev::updateCTM(GfxState *state, double m11, double m12,
                              double m21, double m22,
                              double m31, double m32)
{
    const double *ctm = state->getCTM();
    m_matrix[0] = ctm[0];
    m_matrix[1] = ctm[1];
    m_matrix[2] = ctm[2];
    m_matrix[3] = ctm[3];
    m_matrix[4] = ctm[4];
    m_matrix[5] = ctm[5];
}

void AtomOutputDev::updateFont(GfxState *state)
{
    m_pages->updateFont(state);
}

void AtomOutputDev::beginString(GfxState *state, const GooString *s)
{
    m_pages->beginString(state, s);
}

void AtomOutputDev::endString(GfxState *state)
{
    m_pages->endString();
}

void AtomOutputDev::drawChar(GfxState *state, double x, double y,
                             double dx, double dy,
                             double originX, double originY,
                             CharCode code, int /*nBytes*/, Unicode *u, int uLen)
{
    // todo:
    bool showHidden = false;
    if (!showHidden && (state->getRender() & 3) == 3)
    {
        return;
    }
    m_pages->addChar(state, x, y, dx, dy, originX, originY, u, uLen, getMcid());
}

void AtomOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
                                  int width, int height, bool invert,
                                  bool interpolate, bool inlineImg)
{
    m_pages->addImage(AtomImage(state));
    OutputDev::drawImageMask(state, ref, str, width, height, invert, interpolate, inlineImg);
}

void AtomOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
                              int width, int height, GfxImageColorMap *colorMap,
                              bool interpolate, int *maskColors, bool inlineImg)
{
    m_pages->addImage(AtomImage(state));
    OutputDev::drawImage(state, ref, str, width, height, colorMap, interpolate,
                         maskColors, inlineImg);
}

void AtomOutputDev::stroke(GfxState *state)
{
    if (state->getStrokeColorSpace()->isNonMarking())
    {
        return;
    }
    convertPath(state, state->getPath(), false);
}

void AtomOutputDev::fill(GfxState *state)
{
    if (state->getFillColorSpace()->isNonMarking())
    {
        return;
    }
    AtomPath *path = convertPath(state, state->getPath(), true);
    AtomXPath *xpath = getSplashXPath(path);

    PdfPath pdfPath(1);
    for (int i = 0; i < xpath->getSegLength(); ++i)
    {
        SplashXPathSeg *seg = xpath->getSeg(i);
        pdfPath.lines.emplace_back(seg->x0, seg->y0, seg->x1, seg->y1);
    }

    m_pages->addLine(pdfPath);
    delete path;
    delete xpath;
}

void AtomOutputDev::eoFill(GfxState *state)
{
    if (state->getFillColorSpace()->isNonMarking())
    {
        return;
    }
    AtomPath *path = convertPath(state, state->getPath(), true);
    AtomXPath *xpath = getSplashXPath(path);

    PdfPath pdfPath(2);
    for (int i = 0; i < xpath->getSegLength(); ++i)
    {
        SplashXPathSeg *seg = xpath->getSeg(i);
        pdfPath.lines.emplace_back(seg->x0, seg->y0, seg->x1, seg->y1);
    }

    m_pages->addLine(pdfPath);
    delete path;
    delete xpath;
}

void AtomOutputDev::clip(GfxState *state)
{
    //    double xmin, ymin, xmax, ymax;
    //    state->getClipBBox(&xmin, &ymin, &xmax, &ymax);
    //    std::cout<<"clip:"<<xmin<<", "<<ymin<<", "<<xmax<<", "<<ymax<<std::endl;
    //    convertPath(state, state->getPath(), true, 3);
}

void AtomOutputDev::eoClip(GfxState *state)
{
    //    double xmin, ymin, xmax, ymax;
    //    state->getClipBBox(&xmin, &ymin, &xmax, &ymax);
    //    std::cout<<"eoclip:"<<xmin<<", "<<ymin<<", "<<xmax<<", "<<ymax<<std::endl;
    //    convertPath(state, state->getPath(), true, 4);
}

void AtomOutputDev::beginMarkedContent(const char *name, Dict *properties)
{
    int id = -1;
    if (properties)
    {
        properties->lookupInt("MCID", nullptr, &id);
    }

    if (id == -1)
    {
        return;
    }
    m_mcidStack.push_back(id);
}
void AtomOutputDev::endMarkedContent(GfxState *state)
{
    if (inMarkedContent())
    {
        m_mcidStack.pop_back();
    }
}

void AtomOutputDev::getInfo(unsigned int pageNum, PageInfos &pageInfos)
{
    m_pages->dump(pageNum, pageInfos);
}

AtomPath *AtomOutputDev::convertPath(GfxState *state, GfxPath *path, bool dropEmptySubPaths)
{
    int i, j;

    const int n = dropEmptySubPaths ? 1 : 0;
    auto *sPath = new AtomPath();
    for (i = 0; i < path->getNumSubpaths(); ++i)
    {
        GfxSubpath *subPath = path->getSubpath(i);
        if (subPath->getNumPoints() > n)
        {
            sPath->reserve(subPath->getNumPoints() + 1);
            sPath->moveTo(subPath->getX(0),
                          subPath->getY(0));
            j = 1;
            while (j < subPath->getNumPoints())
            {
                if (subPath->getCurve(j))
                {
                    sPath->curveTo(subPath->getX(j),
                                   subPath->getY(j),
                                   subPath->getX(j + 1),
                                   subPath->getY(j + 1),
                                   subPath->getX(j + 2),
                                   subPath->getY(j + 2));
                    j += 3;
                }
                else
                {
                    sPath->lineTo(subPath->getX(j),
                                  subPath->getY(j));
                    ++j;
                }
            }
            if (subPath->isClosed())
            {
                sPath->close();
            }
        }
    }
    return sPath;
}

void AtomOutputDev::updateFlatness(GfxState *state)
{
#if 0 // Acrobat ignores the flatness setting, and always renders curves
    // with a fairly small flatness value
   splash->setFlatness(state->getFlatness());
#endif
}

void AtomOutputDev::setFlatness(const double flatness)
{
    if (flatness < 1)
    {
        m_flatness = 1;
    }
    else
    {
        m_flatness = flatness;
    }
}

AtomXPath *AtomOutputDev::getSplashXPath(AtomPath *path)
{
    bool adjustLine = false;
    int linePosI = 0;
    return new AtomXPath(path, m_matrix, m_flatness, true,
                         adjustLine, linePosI);
}
