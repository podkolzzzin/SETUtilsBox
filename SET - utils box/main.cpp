// Файл WINDOWS.H содержит определения, макросы, и структуры 
// которые используются при написании приложений под Windows. 
#include <windows.h>
#include "resource.h"
#include <tchar.h>
#include <CommCtrl.h>
#include <Richedit.h>
#include <string>
#include <vector>
#include <fstream>
#include "FileSystem.cpp"
#include "plugins\pluginInfo.h"
#pragma comment(lib,"comctl32")
using namespace std;
#define WTF_TEXT "Эта программулина предназначена для цветной подсветки синтаксиса кода, его форматирования.\n\n Не смотря на то что на дворе ХХI век, в нашем мире еще остались форумы, блоги которые не потдерживают подсветку синтаксиса. С помощью этой жутко-проги также можно импортировать ваш код в виде HTML, или если дела совсем плохи, то картинки"

namespace Set
{
    static int lineCounter=0;
    static TextFormat* defaultFormat=new TextFormat();
    string getExtension(const string &file)
    {
        int pos=file.find_last_of('.');
        if(pos+1 && pos+1<file.length())
        {
            return file.substr(pos+1);
        }
        return string();
    }
    string getFileName(const string &path)
    {
        int pos=path.find_last_of('\\');
        if(pos+1 && pos+1<path.length())
        {
            return path.substr(pos+1);
        }
    }
    class Edit
    {
    private:
        HWND hParent,hWnd,hHScroll,hVScroll;
        HDC hDc;
        int x,y,width,height;
        vector<textInfo>* info;
        void redrawStr(textInfo& str)
        {
            DrawTextA(hDc,str.text.c_str(),0,0,0);
        }
        string replace(const string& content,char del,const string &change)
        {
            string rez;
            for(int i=0;i<content.length();i++)
                if(content[i]!=del)
                rez+=content[i];
                else
                rez+=change;
            return rez;
        }
    public:
        Edit(){}
        void init(int x,int y,int width,int height,HWND hParent,bool isReadOnly=false)
        {
            this->x=x;
            this->y=y;
            this->height=height;
            this->width=width;
            this->hParent=hParent;
            info=new vector<textInfo>();
            DWORD style=WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_SAVESEL | ES_WANTRETURN;
            if(isReadOnly) style |= ES_READONLY;
            hWnd=CreateWindowEx(0,RICHEDIT_CLASS,0,style,x,y,width,height,hParent,0,0,0);        
        }
        Edit(int x,int y,int width,int height,HWND hParent,bool isReadOnly=false)
        {
            this->init(x,y,width,height,hParent,isReadOnly);
        };
        void setWindowText(const string& str)
        {
            info->clear();
            textInfo t;
            t.hFont=Set::defaultFormat;
            t.text=str;
            info->push_back(t);
            SetWindowTextA(hWnd,str.c_str());
        }
        string getWindowText()
        {
            int n=GetWindowTextLength(hWnd);
            char* buffer=new char[n+1];
            GetWindowTextA(hWnd,buffer,n);
            return buffer;
        }
        string htmlspecialchars(string& str,bool showLineNumbers)
        {
            string rez;
            char* t=new char[25];
            if(lineCounter==0 && showLineNumbers)
            {
                rez+="<span style='color:#670505;border-right:1px solid black;width:40px;display:block;float:left;'>1</span>";lineCounter++;
            }
            for(int i=0;i<str.length();i++)
            {
                
                switch(str[i])
                {
                case '\n':
                    {
                        lineCounter++;                      
                        itoa(lineCounter,t,10);
                        rez+="\n<br />";
                        if(showLineNumbers) rez+="<span style='color:#670505;border-right:1px solid black;width:40px;display:block;float:left;'>"+string(t)+"</span>"; break;
                        
                    }
                case '<': rez+="&lt;";break;
                case '>': rez+="&gt;";break;
                case ' ': rez+="&nbsp;"; break;
                default: rez+=str[i];
                }
                
            }delete[] t;
            return rez;
        }
        string getWindowTextHTML(bool showCaption,bool showLanguage,bool showFileName,bool showLineNumbers,const string& fileName,const string &fileLanguage)
        {
            string rez="<p class='set-html-save-item'>";
            if(showCaption)
            {
                rez+="<h2 style='border-bottom:1px solid silver'>";
                if(showFileName)
                {
                    rez+="<b>"+fileName+"</b>";
                }
                if(showLanguage)
                {
                    if(fileLanguage.length()!=0) 
                        rez+="<i>"+fileLanguage+"</i> code";
                    else
                        rez+="<i>"+Set::getExtension(fileName)+"</i> code";
                }
                rez+="</h2>";
            }
            for(int i=0;i<info->size();i++)
            {
                rez+="<span style='"+info->at(i).hFont->cssFontInfo()+"'>"+htmlspecialchars(info->at(i).text,showLineNumbers)+"</span>";
            }
            rez+="</p>";
            lineCounter=0;
            return rez;
        }
        void append(const string& text,TextFormat* tf,bool updateNow=false)
        {
            textInfo temp;temp.text=text;temp.hFont=tf;
            int first=GetWindowTextLengthA(hWnd);
            int last=text.length()+first-1;
            info->push_back(temp);
            if(updateNow) this->update();
        }
        void update()
        {
            string rezStr;
            for(int i=0;i<info->size();i++)
            {
                rezStr+=info->at(i).text;
            }
            SetWindowTextA(hWnd,rezStr.c_str());
        }
    };
    typedef const vector<textInfo>* (*parserFunc) (string& str);
    typedef string (*langFunc) ();
    struct formatInfo
    {
        string format;
        int fileIndex;
    };
    string currentDirectory;
    vector<formatInfo>* formats=NULL;
    vector<string>* libFiles=NULL;
    HANDLE getImage(const char* name)
    {
        return LoadImageA(GetModuleHandle(0),name,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
    }
    char* getOpenFileName(HWND hWnd)
    {
        OPENFILENAMEA a;
        memset(&a,0,sizeof(a));
        a.lStructSize=sizeof(a);
        a.hwndOwner=hWnd;
        a.lpstrFile=new char[3000];
        a.nMaxFile=2999;        
        a.lpstrFile[0]='\0';
        if(GetOpenFileNameA(&a))
            return a.lpstrFile;
        return NULL;
    }
    char* getOpenDirName(HWND hWnd)
    {
        OPENFILENAMEA a;
        memset(&a,0,sizeof(a));
        a.lStructSize=sizeof(a);
        a.hwndOwner=hWnd;
        a.lpstrFile=new char[3000];
        a.nMaxFile=2999;        
        a.lpstrFile[0]='\0';
        if(GetOpenFileNameA(&a))
        {
            for(int i=strlen(a.lpstrFile)-1;i>=0;i--)
                if(a.lpstrFile[i]=='\\')
                {
                    a.lpstrFile[i]='\0';
                    break;
                }
            return a.lpstrFile;
        }
        return NULL;
    }
    char* getSaveFileName(HWND hWnd)
    {
        OPENFILENAMEA a;
        memset(&a,0,sizeof(a));
        a.lStructSize=sizeof(a);
        a.hwndOwner=hWnd;
        a.lpstrFile=new char[3000];
        a.nMaxFile=2999;        
        a.lpstrFile[0]='\0';
        if(GetSaveFileNameA(&a))
            return a.lpstrFile;
        return NULL;        
    }
    void loadFormatList()
    {
        FileSystem fs;
        string path=currentDirectory + "\\plugins\\*.dll";
        libFiles=fs.getDirectoryContent(path);
        string temp;
        formatInfo format;
        formats=new vector<formatInfo>();
        int pos;
        for(int i=0;i<libFiles->size();i++)
        {
            temp=libFiles->at(i).substr(0,libFiles->at(i).find_first_of('.'));
            pos=temp.find(' ');
            while(pos!=-1)
            {
                format.format=temp.substr(0,pos);
                format.fileIndex=i;
                formats->push_back(format);
                if(pos+1<temp.length())
                {
                    temp=temp.substr(pos+1);
                    pos=temp.find(' ');
                }
                else break;
            }

        }
    }
    string strlwr(const string& str)
    {
        string rez;
        rez.resize(str.length());
        for(int i=0;i<str.length();i++)
        {
            if(str[i]>='A' && str[i]<='Z') rez[i]=str[i]-('A'-'a');
            else if(str[i]>='А' && str[i]<='Я') rez[i]=str[i]-('А'-'я');
            else rez[i]=str[i];
        }
        return rez;
    }
    int isSupported(const string& format)
    {
        if(formats==NULL) loadFormatList();
        for(int i=0;i<formats->size();i++)
        {
            if(formats->at(i).format==strlwr(format)) return i;
        }
        return -1;
    }
    void unsupportedFormat(const string& format)
    {
        string message="Set doesn't support *."+format+" files.\n"+"Now the system supports only:\n";
        for(int i=0;i<formats->size();i++)
        {
            message+="*."+formats->at(i).format;
            if(i!=formats->size()-1) message+=", ";
        }
        message+=" files";
        MessageBoxA(0,message.c_str(),"Set - error",MB_OK | MB_ICONERROR);
    }
    void libraryError(const string& lib)
    {
        MessageBoxA(0,("Can not load "+lib).c_str(),"Set - error",MB_OK | MB_ICONERROR);
    }
    void functionError(const string& lib,const string& func)
    {
        MessageBoxA(0,("Can not load function "+func+" from library "+lib).c_str(),"Set - error",MB_OK | MB_ICONERROR);
    }
    void directoryError(const string& dir)
    {
        MessageBoxA(0,("Directory not found or there are no supported files in it. \nDir: "+dir).c_str(),"Set - error",MB_OK | MB_ICONERROR); 
    }
    void loadFile(const string& fileName,string& buffer)
    {
        char* cbuffer=new char[2500];
        cbuffer[0]='\0';
        ifstream file;
        file.open(fileName);
        while(!file.fail())
        {
            file.getline(cbuffer,2499);
            if(!file.fail())
                buffer+=string(cbuffer)+'\n';
        }
        delete []cbuffer;
    }
};
HWND lcBut,siBut,pnBut;
bool isIlluminatorStarted=false; 
Set::Edit output;
static HWND dirInfohWnd;
static HWND currentTab;
static char* dirInfoDir;
string fileName,htmlResult,fileLanguage;
class DirInfo
{
public:

    static void init(HWND hParent,char* dir)
    {
        dirInfoDir=dir;
        DialogBox(GetModuleHandle(0),MAKEINTRESOURCE(ILLUMINATOR),hParent,dirDlgProc);
    }
    static void initControlBox(HWND hWnd)
    {
        auto dirContent=FileSystem::findDirectoryContent(string(dirInfoDir)+"\\*.*");
        if(!dirContent)
        {
            Set::directoryError(dirInfoDir);
            EndDialog(hWnd,0);
            return;
        }
        vector<string> finalFiles;
        for(int i=0;i<dirContent->size();i++)
        {
            if(Set::isSupported(Set::getExtension(dirContent->at(i)))+1)
                finalFiles.push_back(dirContent->at(i));
        }
        if(finalFiles.size()==0)
        {
            Set::directoryError(dirInfoDir);
            return;
        }
        RECT clientRect;
        GetClientRect(hWnd,&clientRect);
        int width=clientRect.right-clientRect.left-20;
        for(int i=0;i<finalFiles.size();i++)
        {
            CreateWindowExA(0,"BUTTON",Set::getFileName(finalFiles[i]).c_str(),WS_CHILD | WS_VISIBLE,clientRect.left+i*width/finalFiles.size()+10,45,width/finalFiles.size(),25,hWnd,HMENU(5000+i),0,0);
        }
        currentTab=GetDlgItem(hWnd,5000);
        EnableWindow(currentTab,false);
    }
    static BOOL CALLBACK dirDlgProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
    {
        static HWND hEdit;
        switch(uMessage)
        {
        case WM_INITDIALOG:
            {
                EnableWindow(GetDlgItem(hWnd,IMPORT_PROJECT_HTML),true);
                EnableWindow(GetDlgItem(hWnd,IMPORT_PROJECT_PICTURE),true);
                initControlBox(hWnd);
                break;
            }
        case WM_COMMAND: 
            {
                switch(wParam)
                {
                    case WTF_BUT: MessageBoxA(hWnd,WTF_TEXT,"Set - about illuminator",MB_OK | MB_ICONINFORMATION); break;
                }
                break;
            }
        case WM_CLOSE:
            EndDialog(hWnd,0);
            return TRUE;

        }
        return FALSE;
    }     
};
class SintaxisIllumination
{
private:
    HWND hWnd;  
    HWND hParent;
public:
    void init(HWND hParent)
    {
        this->hParent=hParent;  
        ShowWindow(CreateDialog(0,MAKEINTRESOURCE(ILLUMINATOR_START_WINDOW),hParent,WindowProc),SW_SHOW);
    }
    static void ShowIlluminatedFile(const vector<textInfo> *parserRez,Set::Edit& output)
    {
        for(int i=0;i<parserRez->size();i++)
        {
            output.append(parserRez->at(i).text,parserRez->at(i).hFont);
        }
        output.update();
    }
    static void fileInfo(const char* file,HWND hParent)
    {
        string ffile=file;
        fileName=file;
        string fileNamel=ffile.substr(ffile.find_last_of('\\')+1);
        string extension=ffile.substr(ffile.find_last_of('.')+1);
        int extIndex=Set::isSupported(extension);
        if(extIndex+1)
        {
            HWND hWnd=CreateDialog(GetModuleHandle(0),MAKEINTRESOURCE(ILLUMINATOR),hParent,fileIlluminatorProc);
            ShowWindow(hWnd,SW_SHOW);
            SetWindowTextA(hWnd,(string("Set - utils box - Syntaxis Illumination - ")+fileNamel).c_str());
            output.init(10,45,666,468,hWnd,true);   
            //===============
            HMODULE hLib=LoadLibraryA((Set::currentDirectory+"\\plugins\\"+Set::libFiles->at(Set::formats->at(extIndex).fileIndex)).c_str());


            if(hLib==NULL)
            {
                Set::libraryError(Set::libFiles->at(extIndex));
                return;
            }
            Set::parserFunc parserProc=(Set::parserFunc)GetProcAddress(hLib,"parserProc");
            string fileString;
            if(parserProc==NULL)
            {
                Set::functionError(Set::libFiles->at(extIndex),"parserProc");
                return;
            }
            Set::langFunc getLang=(Set::langFunc)GetProcAddress(hLib,"getLanguage");
            if(getLang!=NULL)
            {
                fileLanguage=getLang();
            }
            Set::loadFile(ffile,fileString);
            auto parsingResult=parserProc(fileString);
            ShowIlluminatedFile(parsingResult,output);
            FreeModule(hLib);
        }
        else
        {
            Set::unsupportedFormat(extension);
        }
    }

    static void importFileHTML(HWND hWnd)
    {
        DialogBox(GetModuleHandle(0),MAKEINTRESOURCE(IMPORT_SETTINGS_BOX),hWnd,importSettingsProc);
    }
    static void importFileHTML(HWND hWnd,bool importAsFile,bool showCaption,bool showLineNumbers,bool showFileName,bool showLanguage,const string& fileName,const string& fileLang)
    {
        htmlResult=output.getWindowTextHTML(showCaption,showLanguage,showFileName,showLineNumbers,fileName,fileLanguage);
        if(importAsFile)
        {
            ofstream file(fileName);
            file<<htmlResult;
        }
        else
        {
            DialogBox(GetModuleHandle(0),MAKEINTRESOURCE(IDD_IMPORT_RESULT),hWnd,importResultProc);
        }
    }
    static BOOL CALLBACK importResultProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
    {
        static HWND hEdit;
        switch(uMessage)
        {
        case WM_INITDIALOG:
            {
                SetWindowTextA(GetDlgItem(hWnd,IDC_EDIT1),htmlResult.c_str());
                break;
            }
        case WM_COMMAND: 
            {
                if(wParam==IDC_COPY)
                {
                    HWND t=GetDlgItem(hWnd,IDC_COPY);
                    SetFocus(t);
                    SendMessage(t,EM_SETSEL,1,GetWindowTextLength(t)-1);
                    SendMessage(t,WM_COPY,0,0);
                    
                }
                break;
            }
        case WM_CLOSE:
            EndDialog(hWnd,0);
            return TRUE;

        }
        return FALSE;
    } 
    static BOOL CALLBACK importSettingsProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
    {
        static HWND hEdit;
        switch(uMessage)
        {
        case WM_INITDIALOG:
            {
                break;
            }
        case WM_COMMAND: 
            {
                static bool importAsFile=false,showCaption=false,showLineNumbers=false,showFileName=false,showLanguage=false;
                static char* saveFile=NULL;
                switch(wParam)
                {
                case IDC_IMPORT_AS_FILE:
                    {
                        if(importAsFile)
                        {
                            EnableWindow(GetDlgItem(hWnd,IDC_FILE_BUTTON),false);
                            EnableWindow(GetDlgItem(hWnd,IDC_IMPORT_BUTTON),true);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hWnd,IDC_FILE_BUTTON),true);
                            if(saveFile) EnableWindow(GetDlgItem(hWnd,IDC_IMPORT_BUTTON),true);
                            else EnableWindow(GetDlgItem(hWnd,IDC_IMPORT_BUTTON),false);
                        }
                        importAsFile=!importAsFile;
                        break;
                    }
                case IDC_SHOW_CAPTION:
                    {
                        if(showCaption)
                        {
                            EnableWindow(GetDlgItem(hWnd,IDC_SHOW_FILE_NAME),false);
                            EnableWindow(GetDlgItem(hWnd,IDC_SHOW_LANGUAGE),false);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hWnd,IDC_SHOW_FILE_NAME),true);
                            EnableWindow(GetDlgItem(hWnd,IDC_SHOW_LANGUAGE),true);                            
                        }
                        showCaption=!showCaption;
                        break;
                    }
                case IDC_IMPORT_BUTTON:
                    {
                        if(importAsFile)
                            importFileHTML(hWnd,importAsFile,showCaption,showLineNumbers,showFileName,showLanguage,saveFile,fileLanguage);
                        else
                            importFileHTML(hWnd,importAsFile,showCaption,showLineNumbers,showFileName,showLanguage,fileName,fileLanguage);
                        EndDialog(hWnd,0);
                        break;
                    }
                    //(hWnd,importAsFile,showCaption,showLineNumbers,showFileName,showLanguage,fileName);
                case IDC_FILE_BUTTON: 
                    {
                        saveFile=Set::getSaveFileName(hWnd);
                        if(saveFile)
                        {
                            EnableWindow(GetDlgItem(hWnd,IDC_IMPORT_BUTTON),true);
                        }
                        SetDlgItemTextA(hWnd,IDC_FILE_PATH,saveFile);
                        break;
                    }
                case IDC_SHOW_LINE_NUMBERS:showLineNumbers=!showLineNumbers; break;
                case IDC_SHOW_FILE_NAME:showFileName=!showFileName; break;
                case IDC_SHOW_LANGUAGE:showLanguage=!showLanguage; break;
                }
                break;
            }
        case WM_CLOSE:
            EndDialog(hWnd,0);
            return TRUE;

        }
        return FALSE;        
    }
    static void importFilePicture(HWND hWnd)
    {}

    static BOOL CALLBACK fileIlluminatorProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
    {
        static HWND hEdit;
        switch(uMessage)
        {
        case WM_INITDIALOG:
            {

                break;
            }
        case WM_COMMAND: 
            {
                switch(wParam)
                {
                    case WTF_BUT: MessageBoxA(hWnd,WTF_TEXT,"Set - about illuminator",MB_OK | MB_ICONINFORMATION); break;
                    case IMPORT_FILE_HTML: importFileHTML(hWnd);
                    case IMPORT_FILE_PICTURE: importFilePicture(hWnd);
                }
                break;
            }
        case WM_CLOSE:
            EndDialog(hWnd,0);
            return TRUE;

        }
        return FALSE;
    }     
    static BOOL CALLBACK WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
    {
        switch(uMessage)
        {
        case WM_INITDIALOG:  break;
        case WM_CLOSE: 
            isIlluminatorStarted=false;
            EndDialog(hWnd,0);
            return TRUE;
            break;
        case WM_COMMAND:
            {
                if(wParam==ILLUMINATOR_OPEN_FILE)
                {
                    char *fileName=Set::getOpenFileName(hWnd);
                    if(fileName!=NULL)
                        fileInfo(fileName,hWnd);
                }
                else if(wParam==ILLUMINATOR_OPEN_DIRECTORY)
                {
                    char *fileName1=Set::getOpenDirName(hWnd);
                    if(fileName1!=NULL)
                        DirInfo::init(hWnd,fileName1);
                }
                return TRUE;
                break;
            }
        }
        return FALSE;
    } 
};

class Application
{
private:
    HWND hWnd;
    
public:
    Application()
    {
        
    }
    void init(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nCmdShow)
    {
        WNDCLASSEX wcl;
        // 1. Определение класса окна
        wcl.cbSize = sizeof(wcl);	// размер структуры WNDCLASSEX
        // Перерисовать всё окно, если изменён размер по горизонтали или по вертикали
        wcl.style = CS_HREDRAW | CS_VREDRAW;	// CS (Class Style) - стиль класса окна
        wcl.lpfnWndProc = WindowProc;	// адрес оконной процедуры
        wcl.cbClsExtra = 0;		// используется Windows 
        wcl.cbWndExtra  = 0; 	// используется Windows 
        wcl.hInstance = hInst;	// дескриптор данного приложения
        wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION);	// загрузка стандартной иконки
        wcl.hCursor = LoadCursor(NULL, IDC_ARROW);	// загрузка стандартного курсора	
        wcl.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);	// заполнение окна белым цветом
        wcl.lpszMenuName = NULL;	// приложение не содержит меню
        wcl.lpszClassName = L"SET - utils box";	// имя класса окна
        wcl.hIconSm = NULL;	// отсутствие маленькой иконки для связи с классом окна

        // 2. Регистрация класса окна
        if (!RegisterClassEx(&wcl))
            return; // при неудачной регистрации - выход

        // 3. Создание окна
        // создается окно и  переменной hWnd присваивается дескриптор окна
        hWnd = CreateWindowEx(
            0,		// расширенный стиль окна
            L"SET - utils box",	//имя класса окна
            TEXT("SET - utils box"), // заголовок окна
            WS_OVERLAPPEDWINDOW,				// стиль окна
            /* Заголовок, рамка, позволяющая менять размеры, системное меню, кнопки развёртывания и свёртывания окна  */
            CW_USEDEFAULT,	// х-координата левого верхнего угла окна
            CW_USEDEFAULT,	// y-координата левого верхнего угла окна
            1080-355,	// ширина окна
            393,	// высота окна
            NULL,			// дескриптор родительского окна
            NULL,			// дескриптор меню окна
            hInst,		// идентификатор приложения, создавшего окно
            NULL);		// указатель на область данных приложения

        // 4. Отображение окна
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd); // перерисовка окна

        // 5. Запуск цикла обработки сообщений
        SendMessage(hWnd,WM_INITDIALOG,0,0);
    }	
    WPARAM work()
    {
        MSG lpMsg;
        while(GetMessage(&lpMsg, NULL, 0, 0)) // получение очередного сообщения из очереди сообщений
        {
            TranslateMessage(&lpMsg);	// трансляция сообщения
            DispatchMessage(&lpMsg);	// диспетчеризация сообщений
        }
        return lpMsg.wParam;        
    }
    static void initUI(HWND hWnd)
    {
        lcBut=CreateWindowExA(0,"BUTTON",0,WS_CHILD | WS_VISIBLE | BS_BITMAP,0,0,355,355,hWnd,0,0,0);
        SendMessage(lcBut,BM_SETIMAGE,WPARAM(IMAGE_BITMAP),LPARAM(Set::getImage("lc.bmp")));
        siBut=CreateWindowExA(0,"BUTTON",0,WS_CHILD | WS_VISIBLE | BS_BITMAP,355,0,355,355,hWnd,0,0,0);
        SendMessage(siBut,BM_SETIMAGE,WPARAM(IMAGE_BITMAP),LPARAM(Set::getImage("si.bmp")));
    }
    static void startIlluminator(HWND hWnd)
    {
        SintaxisIllumination* t=new SintaxisIllumination();
        t->init(hWnd);
    }
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
    {
        switch(uMessage)
        {
        case WM_INITDIALOG: initUI(hWnd); break;
        case WM_DESTROY: // сообщение о завершении программы
            PostQuitMessage(0); // посылка сообщения WM_QUIT
            break;
        case WM_COMMAND:
            {
                if((HWND)lParam==lcBut)
                {
                    ShellExecuteA(hWnd,"open","lc.exe",0,0,SW_SHOW);
                }
                else if((HWND)lParam==siBut)
                {
                    if(!isIlluminatorStarted)
                    {
                        isIlluminatorStarted=true;
                        startIlluminator(hWnd);
                    }
                    else
                    {
                        MessageBoxA(hWnd,"Illuminator is already started","SET - error",MB_OK | MB_ICONERROR);
                    }
                }
                break;
            }
        default:
            // все сообщения, которые не обрабатываются в данной оконной функции 
            // направляются обратно Windows на обработку по умолчанию
            return DefWindowProc(hWnd, uMessage, wParam, lParam);
        }
        return 0;
    }
};

// прототип оконной процедуры
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
TCHAR szClassWindow[] = TEXT("Каркасное приложение");	/* Имя класса окна */
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nCmdShow)
{
    char* buf=new char[5000];
    GetCurrentDirectoryA(5000,buf);
    Set::currentDirectory=buf;
    delete buf;
    LoadLibraryA("riched32.dll"); 
    Application* app=new Application();
    app->init(hInst,hPrevInst,lpszCmdLine,nCmdShow);
    return app->work();
}
