// BrowseForFolder.h

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef BrowseForFolderh
#define BrowseForFolderh

/* Usage:
MFC does not provide access to the directory-picking dialog as it does the other common dialogs.
You can use the ShBrowseForFolder function;
The following two functions demonstrate the simplest way to use it:

void CMyDlg::OnButton1() {
  char Dest[MAX_PATH];
  PickDir("Hello", Dest);
}

bool CMyDlg::PickDir(const char *Prompt, char * Dest) {
// Dest is assumed to be _MAX_PATH characters in length
  BROWSEINFO bi;
  ITEMIDLIST* pItemIDList;
  char Folder[_MAX_PATH];
  memset(&bi, 0, sizeof(bi));
  bi.hwndOwner=m_hWnd;
  bi.pszDisplayName=Folder;
  bi.lpszTitle=Prompt;
  if(pItemIDList=SHBrowseForFolder(&bi)) {
     SHGetPathFromIDList(pItemIDList, Dest);
     return true;
  }
  return false;
}

There is also Callback functionality available - but most people find that confusing, so I've encapsulated it in a class (CBrowseForFolder) that should help and provided a couple of useful example classes.
CBrowseForDirectory demonstrates the usage of CBrowseForFolder and gets the user to pick a valid Directory (ie not the "Recycle Bin" or "My Computer" etc).
CBrowseForFile also demonstrates the usage of CBrowseForFolder and gets the user to tell us where a particular file is by only enabling the [OK] button when a folder containing a particular file is selected.
The classes return paths that always have a back-slash at the end.

Look at the CBrowseForFolder class to see the virtual functions you can override to receive information,
and the functions you can use to control the Dialog Box (the ones that use SendMessage).

This file also contains a horrible little class that encapsulates the revolting code necessary to find the PIDL of a path...
You shouldn't need CItemIDList because there is already a SelectItem function that takes a path, but I've included it to show how to do it.
*/

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> CBrowseForFolder <<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CBrowseForFolder {
  HWND hWnd;
  static int CALLBACK CallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
    CBrowseForFolder* Me=(CBrowseForFolder*)lpData;
    Me->hWnd=hwnd;
    switch(uMsg) {
      case BFFM_INITIALIZED:    Me->OnInit(); return TRUE; //Indicates the browse dialog box has finished initializing.
      case BFFM_SELCHANGED:     Me->OnSelChanged((ITEMIDLIST*)lParam); return TRUE; //Indicates the selection has changed.
      case BFFM_VALIDATEFAILED: return Me->OnBadAddress((const char*)lParam); //Indicates the user typed an invalid name into the edit box of the browse dialog
      default:                  return TRUE;
  } }
protected:
  char Folder [MAX_PATH+1]; //+1 because we append a back-slash
  char Display[MAX_PATH];
//To alter the behaviour, override any of the following four functions:
  virtual void OnInit() {}
  virtual void OnSelChanged(ITEMIDLIST* pItemIDList) { // override if you want a PIDL.
    bool DIR=SHGetPathFromIDList(pItemIDList, Folder) && *Folder;
    EnableOK(DIR); // Default behaviour only allows you to choose Directory File Objects.
    if(DIR) OnSelChanged(); //This may be changed during OnSelChanged()
  }
  virtual void OnSelChanged() {} // only called when a valid Directory is selected.
  virtual bool OnBadAddress(const char* Path) {return true;}  // return false to cancel the dialog box.
// Use the next four functions to control the Dialog Box:
  void EnableOK  (bool Enable=true)        {                                 SendMessage(hWnd, BFFM_ENABLEOK     ,     0, (LPARAM)Enable     );}
  bool SelectItem(ITEMIDLIST* pItemIDList) {*Folder=0;                       SendMessage(hWnd, BFFM_SETSELECTION , FALSE, (LPARAM)pItemIDList); return *Folder!=0;}
  bool SelectItem(CString Path)            {*Folder=0; Path.TrimRight('\\'); SendMessage(hWnd, BFFM_SETSELECTION ,  TRUE, (LPARAM)&*Path     ); return *Path==*Folder;}
  void Say       (const char* Msg)         {           SendMessage(hWnd, BFFM_SETSTATUSTEXT,     0, (LPARAM)Msg        );}
public:
  CBrowseForFolder() : hWnd(0) {*Display=*Folder=0;}
  bool Browse(const char* Prompt) {
    BROWSEINFO bi;
    ITEMIDLIST* pItemIDList;
    memset(&bi, 0, sizeof(bi));
    bi.hwndOwner=0;
    bi.pszDisplayName=Display;
    bi.lpszTitle=Prompt;
    bi.lpfn=CallbackProc;
    bi.lParam=(LPARAM)this; //Pass the address of this instance so that the static callback function can get back to this instance's OnEvent function.
    if(pItemIDList=SHBrowseForFolder(&bi)) {
      SHGetPathFromIDList(pItemIDList, Folder);
      char* ptr=Folder;
      while(*++ptr); //Go to last char
      if(*--ptr!='\\') { //Append a back-slash if necessary
         *++ptr ='\\';
         *++ptr =0;
      }
    }else *Folder=0;
    return *Folder!=0;
  }
  CString GetFolderPath () const {return Folder ;}
  CString GetDisplayName() const {return Display;}
};

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> CItemIDList <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
Find the PIDL of a path (you shouldn't ever need this)
Usage:
  CItemIDList  IIDL("C:\\");
  CString Path(IIDL.GetPIDL());
*/

class CItemIDList {
  LPITEMIDLIST pItemIDList;
  bool GetItemIDListFromPath(const char* _Path, LPITEMIDLIST* pItemIDList) {
    bool OK=true;
    ULONG Length=MultiByteToWideChar(CP_ACP, MB_USEGLYPHCHARS|MB_PRECOMPOSED, _Path, -1, 0, 0);
    LPWSTR Path=new WCHAR[Length];
    MultiByteToWideChar(CP_ACP, MB_USEGLYPHCHARS|MB_PRECOMPOSED, _Path, -1, Path, Length);
    LPSHELLFOLDER pShellFolder=0;
    if(SHGetDesktopFolder(&pShellFolder)!=NOERROR) return false; // Get Desktop IShellFolder interface
     if(FAILED(pShellFolder->ParseDisplayName (0,0, Path, &Length, pItemIDList, 0))) {
       *pItemIDList=0;
       OK=false;
     }
     pShellFolder->Release();
     delete Path;
     return OK;
  }
public:
  CItemIDList(const char* _Path) {GetItemIDListFromPath(_Path, &pItemIDList);}
  virtual ~CItemIDList() {
    LPMALLOC Malloc;
    SHGetMalloc(&Malloc);
    Malloc->Free(pItemIDList);
  }
  LPITEMIDLIST GetPIDL() const {return pItemIDList;}
};

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> CBrowseForDirectory <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
Demonstrates the usage of CBrowseForFolder. Gets the user to pick a valid Directory (ie not the "Recycle Bin" or "My Computer" etc):
Usage:
  CBrowseForDirectory BrowseForDirectory;
  if(BrowseForDirectory.Browse("Select Local Folder")) {
    CString Folder(BrowseForDirectory.GetFolderPath ());
    CString Name  (BrowseForDirectory.GetDisplayName());
  }
*/

class CBrowseForDirectory : public CBrowseForFolder {
  CString InitPath;
  void OnInit() {SelectItem(InitPath);}
public:
  CBrowseForDirectory() : InitPath("C:\\") {} //Assumes most people will want to start on the C: drive
  CBrowseForDirectory(CString InitPath) : InitPath(InitPath) {}
  bool Browse(const char* Prompt) {return CBrowseForFolder::Browse(Prompt) && *Folder;} 
};

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> CBrowseForFile <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
Demonstrates the usage of CBrowseForFolder. Gets the user to tell us where a particular file is:
Usage:
  CBrowseForFile BrowseForFile;
  if(BrowseForFile.Browse("AutoExec.*")) {
    CString Folder(BrowseForFile.GetFolderPath ());
    CString Name  (BrowseForFile.GetDisplayName());
  }
*/

class CBrowseForFile : public CBrowseForFolder {
  CString InitPath;
  CString File; // The file we're looking for.
  void OnInit() {SelectItem(InitPath);}
  void OnSelChanged() {EnableOK(GetFileAttributes(CString(Folder)+File)!=-1);}
public:
  CBrowseForFile() : InitPath("C:\\") {} //Assumes most people will want to start on the C: drive
  CBrowseForFile(CString InitPath) : InitPath(InitPath) {}
  bool Browse(CString FileName) {File='\\'+FileName; return CBrowseForFolder::Browse("Please locate "+File);} 
};

#endif //ndef BrowseForFolderh


