#ifndef __POPPLER_ATOM_TYPES__H__
#define __POPPLER_ATOM_TYPES__H__

#include <string>
#include <cmath>
#include <vector>
#include <map>
#include <goo/GooList.h>

struct AtomBox{
    double x1;
    double y1;
    double x2;
    double y2;

    AtomBox(double x1, double y1, double x2, double y2)
         : x1(x1), y1(y1), x2(x2), y2(y2) {}
    AtomBox(): x1(0), y1(0), x2(0), y2(0) {}

    bool operator==(const AtomBox &other) const 
    {
        if (&other == this) 
        {  
            return true;
        }

        const double ps = 1.0;
        return !(std::fabs(x1 - other.x1) > ps
                 || std::fabs(x2-other.x2) > ps
                 || std::fabs(y1-other.y1) > ps
                 || std::fabs(y2-other.y2) > ps);
    }

    bool operator != (const AtomBox &other) const 
    {
        return !(*this == other);
    }
};


class PdfFont{
public:
    int id;
    std::string name;
    float size;
    short weight;
    std::string color;
    bool is_bold;
    bool is_italic;
    int line_height;
    int type;
    int render;

    PdfFont(int id, std::string name, int type, float size, short weight, std::string color, bool is_bold, bool is_italic, int line_height, int render)
    {
        this->id = id;
        this->name = name;
        this->type = type;
        this->size = size;
        this->weight = weight;
        this->color = color;
        this->is_bold = is_bold;
        this->is_italic = is_italic;
        this->line_height = line_height;
        this->render = render;
    }
    PdfFont(): size(0), weight(0), is_bold(false), is_italic(false), line_height(0), type(0), render(0) {}
};


class PdfImage
{
public:
    int left;
    int top;
    int right;
    int bottom;

    PdfImage(int left, int top, int right, int bottom) 
    {
        this->left = left;
        this->top = top;
        this->right = right;
        this->bottom = bottom;
    }
    PdfImage(): left(-1), top(-1), right(-1), bottom(-1) {}

    bool is_valid()
    {
        return this->left >= 0 && this->top >= 0 && this->right >= 0 && this->bottom >= 0;
    }

    PdfImage zoom(float ratio)
    {
        return PdfImage(ratio * left, ratio * top, ratio * right, ratio * bottom);
    }
};


enum ItemType
{
    TEXT,
    CELL,
};


class PdfItem
{
public:
    unsigned int id;
    int mcid;
    ItemType type;
    std::string text;

    int left;
    int top;
    int right;
    int bottom;

    int font;
    int style;
    std::vector<PdfItem> children;

    PdfItem(int mcid, ItemType type, std::string text, int left, int top, int right, int bottom, int font=-1, int style=-1)
    {
        this->mcid = mcid;
        this->type = type;
        this->text = text;
        this->left = left;
        this->top = top;
        this->right = right;
        this->bottom = bottom;
        this->font = font;
        this->style = style;
    }
    PdfItem(): mcid(-1), type(TEXT), left(-1), top(-1), right(-1), bottom(-1), font(-1), style(-1) {

    }
    ~PdfItem(){}

    int width() const {return std::abs(right-left);}
    int height() const {return std::abs(bottom-top);}
    int xcenter() const {return (right+left)/2;}
    int ycenter() const {return (top+bottom)/2;}
};


/*class PdfLine {
public:
    int x0;
    int y0;
    int x1;
    int y1;
    int type;

    PdfLine(int x0, int y0, int x1, int y1, int type=1): x0(x0), y0(y0), x1(x1), y1(y1), type(type){}
    PdfLine(): x0(-1), y0(-1), x1(-1), y1(-1), type(-1){}
};*/


class PdfLine {
public:
    int x0;
    int y0;
    int x1;
    int y1;
    int cx;
    int cy;
    int type;
    PdfLine(int x0, int y0, int x1, int y1)
        : x0(x0), y0(y0), x1(x1), y1(y1), cx(-1), cy(-1), type(0){}
    PdfLine(int x0, int y0, int x1, int y1, int cx, int cy)
        : x0(x0), y0(y0), x1(x1), y1(y1), cx(cx), cy(cy), type(1){}
    PdfLine()
        : x0(-1), y0(-1), x1(-1), y1(-1), cx(-1), cy(-1), type(-1){}
};


class PdfPath {
public:
    std::vector<PdfLine> lines;
    int m_type;

    PdfPath(int type): m_type(type) {}
};

class PdfShape {
public:
    int type;
    std::vector<PdfPath> pathes;

    PdfShape(int type):type(type){}
};


class PageInfos {
public:
    int m_page_num;
    int m_width;
    int m_height;
    std::vector<PdfFont> m_fonts;
    std::vector<PdfImage> m_images;
    std::vector<PdfItem> m_items;
    std::vector<PdfPath> m_lines;
    std::vector<PdfPath> m_graphs;

    PageInfos(): m_page_num(0), m_width(0), m_height(0), m_item_seq(0){}

    unsigned long addItem(PdfItem &item, int parent=-1){
        item.id = m_item_seq++;
        if(parent < 0) {
            m_items.push_back(item);
            return m_items.size();
        }
        else
        {
            m_items[parent].children.push_back(item);
            return m_items[parent].children.size();
        }
    }

    const PdfItem * getLastItem(int parent=-1) const {
        if(parent < 0) {
            return m_items.empty()?nullptr:&m_items.back();
        }
        else
        {
            return m_items[parent].children.empty()?nullptr:&m_items[parent].children.back();
        }
    }
private:
    unsigned int m_item_seq;
};


class PdfStructInfo {
public:
    std::string type;
    int mcid;
    int page;
    std::map<std::string, std::string> attribute;
    std::vector<PdfStructInfo> children;

    PdfStructInfo(): mcid(-1), page(0) {}
};

#endif

