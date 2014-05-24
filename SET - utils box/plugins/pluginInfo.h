//еякх дюммши тюик ондйкчвем  РН 
#ifdef ISDLL
//дкъ опнейрю дкк
#define TYPEDLL extern "C" _declspec(dllexport)
#else
//дкъ нйнммнцн опхкнфемхъ
#define TYPEDLL extern "C" _declspec(dllimport)
#endif
#include <vector>
#include <string>
#include <Windows.h>
using namespace std;
class TextFormat
{
    string cssFont;
public:
    string fontFamily;
    int size;
    long color;
    bool isBold,isUnderline,isItalic;
    int align;
    HFONT hFont;

    TextFormat(const string &fontFamily="Times",int size=12,long color=0,bool isBold=false,bool isItalic=false,bool isUnderline=false) 
    {

        this->fontFamily=fontFamily;
        this->size=size;
        this->color=color;
        this->isBold=isBold;
        this->isItalic=isItalic;
        this->isUnderline=isUnderline;
        this->align=align;
        LOGFONTA font={0};
        strcpy(font.lfFaceName,fontFamily.c_str());
        font.lfHeight=size;
        font.lfUnderline=isUnderline;
        font.lfItalic=isItalic;
        font.lfUnderline=isUnderline;
        hFont=CreateFontIndirectA(&font);
    }
    string cssFontInfo()
    {
        if(cssFont.length()==0)
        {
            char* temp=new char[25];
            itoa(size,temp,10);
            cssFont="font-size:"+string(temp)+";font-family:"+fontFamily+";";
            itoa(color,temp,16);
            cssFont+="color:#"+string(temp)+";";
            if(isBold) cssFont+="font-weight:bold;";
            if(isUnderline) cssFont+="text-decoration:underline;";
            if(isItalic) cssFont+"font-style:italic;";
            delete[] temp;           
        }
        return cssFont;
    }
};
struct textInfo
{
    std::string text;
    TextFormat* hFont;
};
TYPEDLL std::vector<textInfo>* parserProc(const string& str);