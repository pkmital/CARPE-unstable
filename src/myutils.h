
int rgb_colormap[256*3] = {0,0,0,0,0,135,0,0,139,0,0,143,0,0,147,0,0,151,0,0,155,0,0,159,0,0,163,0,0,167,0,0,171,0,0,175,0,0,179,0,0,183,0,0,187,0,0,191,0,0,195,0,0,199,0,0,203,0,0,207,0,0,211,0,0,215,0,0,219,0,0,223,0,0,227,0,0,231,0,0,235,0,0,239,0,0,243,0,0,247,0,0,251,0,0,255,0,4,255,0,8,255,0,12,255,0,16,255,0,20,255,0,24,255,0,28,255,0,32,255,0,36,255,0,40,255,0,44,255,0,48,255,0,52,255,0,56,255,0,60,255,0,64,255,0,68,255,0,72,255,0,76,255,0,80,255,0,84,255,0,88,255,0,92,255,0,96,255,0,100,255,0,104,255,0,108,255,0,112,255,0,116,255,0,120,255,0,124,255,0,128,255,0,131,255,0,135,255,0,139,255,0,143,255,0,147,255,0,151,255,0,155,255,0,159,255,0,163,255,0,167,255,0,171,255,0,175,255,0,179,255,0,183,255,0,187,255,0,191,255,0,195,255,0,199,255,0,203,255,0,207,255,0,211,255,0,215,255,0,219,255,0,223,255,0,227,255,0,231,255,0,235,255,0,239,255,0,243,255,0,247,255,0,251,255,0,255,255,4,255,251,8,255,247,12,255,243,16,255,239,20,255,235,24,255,231,28,255,227,32,255,223,36,255,219,40,255,215,44,255,211,48,255,207,52,255,203,56,255,199,60,255,195,64,255,191,68,255,187,72,255,183,76,255,179,80,255,175,84,255,171,88,255,167,92,255,163,96,255,159,100,255,155,104,255,151,108,255,147,112,255,143,116,255,139,120,255,135,124,255,131,128,255,128,131,255,124,135,255,120,139,255,116,143,255,112,147,255,108,151,255,104,155,255,100,159,255,96,163,255,92,167,255,88,171,255,84,175,255,80,179,255,76,183,255,72,187,255,68,191,255,64,195,255,60,199,255,56,203,255,52,207,255,48,211,255,44,215,255,40,219,255,36,223,255,32,227,255,28,231,255,24,235,255,20,239,255,16,243,255,12,247,255,8,251,255,4,255,255,0,255,251,0,255,247,0,255,243,0,255,239,0,255,235,0,255,231,0,255,227,0,255,223,0,255,219,0,255,215,0,255,211,0,255,207,0,255,203,0,255,199,0,255,195,0,255,191,0,255,187,0,255,183,0,255,179,0,255,175,0,255,171,0,255,167,0,255,163,0,255,159,0,255,155,0,255,151,0,255,147,0,255,143,0,255,139,0,255,135,0,255,131,0,255,128,0,255,124,0,255,120,0,255,116,0,255,112,0,255,108,0,255,104,0,255,100,0,255,96,0,255,92,0,255,88,0,255,84,0,255,80,0,255,76,0,255,72,0,255,68,0,255,64,0,255,60,0,255,56,0,255,52,0,255,48,0,255,44,0,255,40,0,255,36,0,255,32,0,255,28,0,255,24,0,255,20,0,255,16,0,255,12,0,255,8,0,255,4,0,255,0,0,251,0,0,247,0,0,243,0,0,239,0,0,235,0,0,231,0,0,227,0,0,223,0,0,219,0,0,215,0,0,211,0,0,207,0,0,203,0,0,199,0,0,195,0,0,191,0,0,187,0,0,183,0,0,179,0,0,175,0,0,171,0,0,167,0,0,163,0,0,159,0,0,155,0,0,151,0,0,147,0,0,143,0,0,139,0,0,135,0,0,131,0,0,128,0,0	};

#include <windows.h>
#include <WinBase.h>
#include <shlobj.h>
#include <commdlg.h>
#include <shlobj.h>

WINBASEAPI
DWORD
WINAPI
GetCurrentDirectoryA(
    __in DWORD nBufferLength,
    __out_ecount_part_opt(nBufferLength, return + 1) LPSTR lpBuffer
    );
WINBASEAPI
DWORD
WINAPI
GetCurrentDirectoryW(
    __in DWORD nBufferLength,
    __out_ecount_part_opt(nBufferLength, return + 1) LPWSTR lpBuffer
    );
#ifdef UNICODE
#define GetCurrentDirectory  GetCurrentDirectoryW
#else
#define GetCurrentDirectory  GetCurrentDirectoryA
#endif // !UNICODE


WINBASEAPI
BOOL
WINAPI
SetCurrentDirectoryA(
    __in LPCSTR lpPathName
    );
WINBASEAPI
BOOL
WINAPI
SetCurrentDirectoryW(
    __in LPCWSTR lpPathName
    );
#ifdef UNICODE
#define SetCurrentDirectory  SetCurrentDirectoryW
#else
#define SetCurrentDirectory  SetCurrentDirectoryA
#endif // !UNICODE

string getApplicationDirectory()
{
	char originalDirectory[ MAX_PATH ];
	
	GetCurrentDirectory( MAX_PATH, originalDirectory );
	return originalDirectory;
}

string myOpenFolderDialog()
{
	//http://msdn.microsoft.com/en-us/library/bb773205(VS.85).aspx
	char szPath[MAX_PATH]; 
	LPMALLOC pMalloc = NULL;
	LPITEMIDLIST pidl = NULL;
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(BROWSEINFO));

	bi.hwndOwner = 0;
	bi.lpszTitle = _T("Select the Folder with the Eye-Tracking Data");
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT | BIF_NONEWFOLDERBUTTON | BIF_BROWSEINCLUDEFILES;
	//bi.lpfn = BrowseCallbackProc;
	pidl = SHBrowseForFolder(&bi);
	if(pidl != NULL)
	{
		SHGetPathFromIDList(pidl, szPath);
	
		if(SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc)
		{
			pMalloc->Free(pidl);  
			pMalloc->Release(); 
			return string(szPath);
		}
	}
	return string("data/eye");
}

string myOpenIASFileDialog()
{
	//char originalDirectory[ MAX_PATH ];
	//GetCurrentDirectory( MAX_PATH, originalDirectory );
	char szFileName[MAX_PATH] = "";
	
	// http://msdn.microsoft.com/en-us/library/ms646839.aspx
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = 0;
	ofn.lpstrFilter = "Interest Area Files\0*.ias\0All Files\0*.*\0\0";
	ofn.lpstrFile = szFileName;
	ofn.lpstrTitle = "Open the Interest Area File";
	ofn.nFileOffset = 0;
	ofn.nMaxFile = MAX_PATH;
	ofn.lCustData = 0;
	//ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = 0;
	
	if(GetOpenFileName(&ofn)) {
		//loadfile(szFileName);
		//GetCurrentDirectory( MAX_PATH, originalDirectory );
		//SetCurrentDirectory( originalDirectory );
		return string(szFileName);
	}

	//SetCurrentDirectory( originalDirectory );
	return string("data/event_data/diems01.ias");
}

string myOpenMovieDialog()
{
	//char originalDirectory[ MAX_PATH ];
	//GetCurrentDirectory( MAX_PATH, originalDirectory );
	char szFileName[MAX_PATH] = "";
	
	// http://msdn.microsoft.com/en-us/library/ms646839.aspx
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = 0;
	ofn.lpstrFilter = "Movie Files\0*.mov;*.avi;*.xvd\0All Files\0*.*\0\0";
	ofn.lpstrFile = szFileName;
	ofn.lpstrTitle = "Open The Eye-Tracking Movie File";
	ofn.nFileOffset = 0;
	ofn.nMaxFile = MAX_PATH;
	ofn.lCustData = 0;
	//ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = 0;
	
	if(GetOpenFileName(&ofn)) {
		//loadfile(szFileName);
		//GetCurrentDirectory( MAX_PATH, originalDirectory );
		//SetCurrentDirectory( originalDirectory );
		return string(szFileName);
	}

	//SetCurrentDirectory( originalDirectory );
	return string("data/movie.avi");
	/*
#ifdef TARGET_OSX
	short fRefNumOut;
	FSRef output_file;
	OSStatus err;
	
	NavDialogCreationOptions options;
	NavGetDefaultDialogCreationOptions( &options );
	options.modality = kWindowModalityAppModal;
	//options.windowTitle = CFSTR(msg.c_str());
	//options.message = CFSTR("Save Movie File As... (*.mov)");
	
	
	NavDialogRef dialog;
	//err = NavCreateChooseFileDialog(&options, NULL, NULL, NULL, NULL, NULL, &dialog);
	err = NavCreatePutFileDialog(&options, '.mov', 'Moov', NULL, NULL, &dialog);
	printf("NavCreatePutFileDialog returned %i\n", err );
	
	err = NavDialogRun(dialog);
	printf("NavDialogRun returned %i\n", err );
	
	NavUserAction action;
	action = NavDialogGetUserAction( dialog );
	printf("got action %i\n", action);
	if (action == kNavUserActionNone || action == kNavUserActionCancel) {
		
		printf("Encountered Error During Get User Action");
	}
	
	// get dialog reply
	NavReplyRecord reply;
	err = NavDialogGetReply(dialog, &reply);
	if ( err != noErr )	{
		
		printf("Encountered Error During Reply");
	}
	if ( reply.replacing )
	{
		printf("Need to replace\n");
	}
	
	AEKeyword keyword;
	DescType actual_type;
	Size actual_size;
	FSRef output_dir;
	err = AEGetNthPtr(&(reply.selection), 1, typeFSRef, &keyword, &actual_type,
					  &output_dir, sizeof(output_file), &actual_size);
	
	//printf("AEGetNthPtr returned %i\n", err );
	
	
	CFURLRef cfUrl = CFURLCreateFromFSRef( kCFAllocatorDefault, &output_dir );
	CFStringRef cfString = NULL;
	if ( cfUrl != NULL )
	{
		cfString = CFURLCopyFileSystemPath( cfUrl, kCFURLPOSIXPathStyle );
		CFRelease( cfUrl );
	}
	
    // copy from a CFString into a local c string (http://www.carbondev.com/site/?page=CStrings+)
	const int kBufferSize = 255;
	
	char folderURL[kBufferSize];
	Boolean bool1 = CFStringGetCString(cfString,folderURL,kBufferSize,kCFStringEncodingMacRoman);
	
	char fileName[kBufferSize];
	Boolean bool2 = CFStringGetCString(reply.saveFileName,fileName,kBufferSize,kCFStringEncodingMacRoman);
	
	// append strings together
	
	string url1 = folderURL;
	string url2 = fileName;
	string finalURL = url1 + "/" + url2;
	
	printf("url %s\n", finalURL.c_str());
	
	// cleanup dialog
	NavDialogDispose(dialog);
	//	saver.setup(mov.width,mov.height,finalURL.c_str());
	return finalURL;
#endif
	
#ifdef TARGET_WIN32
	//	saver.setup(mov.width,mov.height,"output.mov");
#endif
return false;*/

}
/*
string myDialog()
{
	static DWORD nFilterIndex=0;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_DONTADDTORECENT | OFN_NOCHANGEDIR, NULL);

	char sCurDir[_MAX_PATH];
	::GetCurrentDirectory(_MAX_PATH,sCurDir);
	dlg.m_ofn.lpstrTitle=IDS_H_KULISSE;
	dlg.m_ofn.lpstrInitialDir=theApp.m_pfadKulissen;
	dlg.m_ofn.nFilterIndex=nFilterIndex;
	if(dlg.DoModal()==IDOK)
	{

	}
	::SetCurrentDirectory(sCurDir);
}*/

void find_and_replace( string &source, const string find, string replace ) 
{
	size_t j;
	for ( ; (j = source.find( find )) != string::npos ; ) 
	{
		source.replace( j, find.length(), replace );
	}
}
   
void jetColorMap(unsigned char *rgb,float value,float min,float max)
{
	float max4=(max-min)/4;
	int c1 = 0;
	value-=min;
	if(value==HUGE_VAL)
    {rgb[0]=rgb[1]=rgb[2]=255;}
	else if(value<=0)
    {rgb[0]=rgb[1]=rgb[2]=0;}
	else if(value<max4)
    {rgb[0]=0;rgb[1]=0;rgb[2]=c1+(unsigned char)((255-c1)*value/max4);}
	else if(value<2*max4)
    {rgb[0]=0;rgb[1]=(unsigned char)(255*(value-max4)/max4);rgb[2]=255;}
	else if(value<3*max4)
    {rgb[0]=(unsigned char)(255*(value-2*max4)/max4);rgb[1]=255;rgb[2]=255-rgb[0];}
	else if(value<max)
    {rgb[0]=255;rgb[1]=(unsigned char)(255-255*(value-3*max4)/max4);rgb[2]=0;}
	else {rgb[0]=255;rgb[1]=rgb[2]=0;}
}
