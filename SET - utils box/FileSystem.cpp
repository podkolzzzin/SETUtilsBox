#include <Windows.h>
#include <string>
#include <vector>
struct MyFile
{
    WIN32_FIND_DATAA fd;
    std::string fullPath;
    __int64 fileSize()
    {
        return __int64(fd.nFileSizeHigh)*(__int64(MAXDWORD)+1)+fd.nFileSizeLow;
    }
};
class FileSystem
{
public:
    static std::vector<WIN32_FIND_DATAA>* getDirectoryContentEx(const char* dir) //возвращает всю информацию о файлах. Принимает dir в формате "d:\\ololo\\*.olo"
    {
        WIN32_FIND_DATAA fd;
        std::vector<WIN32_FIND_DATAA>* content=new std::vector<WIN32_FIND_DATAA>();
        HANDLE hFind=FindFirstFileA(dir, &fd);
        if(hFind != INVALID_HANDLE_VALUE)
        {
            do{
                content->push_back(fd);
            }while(FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
        return content;
    }   
    static std::vector<WIN32_FIND_DATAA>* getDirectoryContentEx(const std::string& dir)
    {
        return getDirectoryContentEx(dir.c_str());
    }
    static std::vector<std::string>* getDirectoryContent(const char* dir) //возвращает только имена файлов
    {
        WIN32_FIND_DATAA fd;
        std::vector<std::string>* content=new std::vector<std::string>();
        HANDLE hFind=FindFirstFileA(dir, &fd);
        if(hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if(isFile(fd))
                    content->push_back(std::string(fd.cFileName));
            }
            while(FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
        return content;
    } 
    static std::vector<std::string>* findDirectoryContent(const std::string& dir,std::vector<std::string>* content=NULL)
    {
        if(content==NULL)
        {
            content=new std::vector<std::string>();
        }
        std::string prefix,file,nDir;
        WIN32_FIND_DATAA fd;
        int pos=dir.find_last_of('\\');
        prefix=dir.substr(0,pos+1);
        file=dir.substr(pos);
        
        HANDLE hFile=FindFirstFileA(dir.c_str(),&fd);
        if(hFile!=INVALID_HANDLE_VALUE)
        {
            do
            {
                if(strcmp(fd.cFileName,".")!=0 && strcmp(fd.cFileName,"..")!=0)
                {
                    nDir=prefix;
                    nDir+=fd.cFileName;
                    if(isDir(fd))
                    {
                        nDir+=file;
                        findDirectoryContent(nDir,content);
                    }
                    else
                    {
                        content->push_back(nDir);
                    }
                }
            }
            while(FindNextFileA(hFile, &fd));

        }
        return content;
    }
    static std::vector<MyFile>* findDirectoryContentEx(const std::string& dir,std::vector<MyFile>* content=NULL)
    {
        if(content==NULL)
        {
            content=new std::vector<MyFile>();
        }
        std::string prefix,file,nDir;
        WIN32_FIND_DATAA fd;
        int pos=dir.find_last_of('\\');
        prefix=dir.substr(0,pos+1);
        file=dir.substr(pos);
        
        HANDLE hFile=FindFirstFileA(dir.c_str(),&fd);
        MyFile buffer;
        if(hFile!=INVALID_HANDLE_VALUE)
        {
            do
            {
                if(strcmp(fd.cFileName,".")!=0 && strcmp(fd.cFileName,"..")!=0)
                {
                    nDir=prefix;
                    nDir+=fd.cFileName;
                    if(isDir(fd))
                    {
                        nDir+=file;
                        findDirectoryContentEx(nDir,content);
                    }
                    else
                    {
                        buffer.fd=fd;
                        buffer.fullPath=nDir;
                        content->push_back(buffer);
                    }
                }
            }
            while(FindNextFileA(hFile, &fd));

        }
        return content;
    }
    static std::vector<std::string>* getDirectoryContent(const std::string& dir)
    {
        return getDirectoryContent(dir.c_str());
    }
    static bool isDir(WIN32_FIND_DATAA &item)
    {
        return bool(item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }
    static bool isFile(WIN32_FIND_DATAA &item)
    {
        return !isDir(item);
    }
};