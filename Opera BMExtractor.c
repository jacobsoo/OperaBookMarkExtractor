#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commctrl.h>
#include <io.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tchar.h>
#include <fcntl.h>
#include "resource.h"

#pragma comment(lib, "COMCTL32")
#pragma comment(lib, "SHELL32")
#pragma comment(lib, "shlwapi")

// g (Global optimization), s (Favor small code), y (No frame pointers).
#pragma optimize("gsy", on)
#pragma comment(linker, "/OPT:NOWIN98")		// Make section alignment really small.
#define WIN32_LEAN_AND_MEAN

#define BSIZE	2048

LRESULT			CALLBACK Main(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
int				searchOperaBookmarks(TCHAR* AppData);
int				ReadFileData(TCHAR *filePath, TCHAR **buffer, DWORD *bufferSize);
unsigned long	FindString(TCHAR *buffer, unsigned long bufferlen, TCHAR *string, unsigned long start);
void			ExportBookmarks(void);
BOOL			IsVista(void);

HINSTANCE		hInst;
HWND			hwndList;
LVITEM			lvItem;
DWORD			dwCount = 0;

int WINAPI WinMain(HINSTANCE hinstCurrent, HINSTANCE hinstPrevious, LPSTR lpszCmdLine, int nCmdShow){
	INITCOMMONCONTROLSEX icc;
	icc.dwICC = ICC_LISTVIEW_CLASSES;
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);

	InitCommonControlsEx(&icc);

	hInst= hinstCurrent;
	DialogBox(hinstCurrent, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)Main);
	return 0;
}

LRESULT CALLBACK Main(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam){
	HDC					hdc;
	PAINTSTRUCT			ps;
	LVCOLUMN			lvCol;
	TCHAR				*szPath, *szUserName, *szColumn[]= {"URL"};
	TCHAR				AppData[MAX_PATH];
	int					width[]= {280, 250};
	DWORD				dwCount= BSIZE;
	BOOL				bReturn;

	switch( uMsg ){
		case WM_INITDIALOG:
			SendMessageA(hDlg,WM_SETICON,ICON_SMALL, (LPARAM) LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
			SendMessageA(hDlg,WM_SETICON, ICON_BIG,(LPARAM) LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
			hwndList= GetDlgItem(hDlg, IDC_LIST);
			ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_GRIDLINES);
			ZeroMemory(&lvCol, sizeof(LVCOLUMN));
			szPath= (TCHAR*)calloc(BSIZE, sizeof(TCHAR));
			szUserName= (TCHAR*)calloc(BSIZE, sizeof(TCHAR));

			lvCol.mask= LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
			lvCol.fmt= LVCFMT_LEFT;
			lvCol.iSubItem= 0;
			lvCol.cx= width[0];
			lvCol.pszText= szColumn[0];
			ListView_InsertColumn(hwndList, 0, &lvCol);

			if( IsVista() ){
				// In Vista and above, the folders C:\Users\[Username]\AppData\Roaming
				SHGetSpecialFolderPath(hDlg, AppData, CSIDL_APPDATA, 0);
				searchOperaBookmarks(AppData);
			}else{
				bReturn= GetUserName(szUserName, &dwCount);
				_stprintf_s(szPath, 1024, "C:\\Documents and Settings\\%s\\Application Data", szUserName);
				searchOperaBookmarks(szPath);
			}
		return TRUE;
		case WM_COMMAND:
			if( LOWORD(wParam)==IDC_EXPORT ){
				if( HIWORD(wParam)==BN_CLICKED ){
					ExportBookmarks();
				}
			}else if( LOWORD(wParam)==IDC_EXIT ){
				if( HIWORD(wParam)==BN_CLICKED ){
					EndDialog(hDlg,wParam);
				}
			}
		break;
		case WM_PAINT:
			hdc= BeginPaint(hDlg, &ps);
			InvalidateRect(hDlg, NULL, TRUE);
			EndPaint(hDlg, &ps);
		break;
		case WM_CLOSE:
			EndDialog(hDlg,wParam);
			DestroyWindow(hDlg);
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
		break;
	}
	return FALSE;
}

BOOL IsVista(void){
	OSVERSIONINFO	osvi;
	BOOL			bIsVista;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize= sizeof(OSVERSIONINFO);

	GetVersionEx(&osvi);

	if( osvi.dwPlatformId==VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion>=6 ){
		bIsVista= TRUE;
	}else{
		bIsVista= FALSE;
	}
	return bIsVista;
}

/*-----------------------------------------------------------------------------
    func:   searchOperaBookmarks
    desc:   List Opera bookmarks
    pass:   
    rtrn:   NOTHING
-----------------------------------------------------------------------------*/
int searchOperaBookmarks(TCHAR * AppData){
	TCHAR	*szBuffer;
	DWORD	dwBytesWritten= 0;
	TCHAR	*szData = NULL, *dataBuffer = NULL, *szURL;
	DWORD	dataLen, pIni, pFin = 0;

	szBuffer = (TCHAR*)calloc(1024, sizeof(TCHAR));
	szURL = (TCHAR*)calloc(2048, sizeof(TCHAR));
	_stprintf_s(szBuffer, 1024, "%s\\Opera\\Opera\\bookmarks.adr", AppData);

	if( !ReadFileData(szBuffer, &dataBuffer, &dataLen) )
		return 0;
	while( TRUE ){
		// URL
		pIni= FindString(dataBuffer, dataLen, "URL=", pFin);
		if( pIni==0 )
			break;
		pIni += 4;
		pFin = FindString(dataBuffer, dataLen, "\r\n", pIni);
		strncpy_s(szURL, 1024, &dataBuffer[pIni], pFin - pIni);
		szURL[pFin - pIni] = 0;
		dataBuffer += pFin;
		dataLen-= pFin;
		pFin = 0;

		lvItem.mask = LVIF_TEXT;
		lvItem.cchTextMax = MAX_PATH;
		lvItem.iItem = dwCount;
		lvItem.iSubItem = 0;
		lvItem.pszText = szURL;
		ListView_InsertItem(hwndList, &lvItem);
		dwCount++;
	}
	dataBuffer= NULL;
	free(dataBuffer);
	return 1;
}

/*-----------------------------------------------------------------------------
    func:   ReadFileData
    desc:   Reads in data from filePath and store it in buffer.
    pass:   Nothing
    rtrn:   1 if success
			else 0   
-----------------------------------------------------------------------------*/
int ReadFileData(TCHAR *filePath, TCHAR **buffer, DWORD *bufferSize){
	FILE * hFile;
	errno_t err;
	DWORD fSize;

	err = fopen_s(&hFile, filePath, "rb");
	if( hFile==NULL )
		return 0;

	fseek(hFile, 0, SEEK_END);
	fSize = ftell(hFile);
	fseek(hFile, 0, SEEK_SET);
	*buffer = (TCHAR*)malloc(fSize);
	if( *buffer==NULL )
		return 0;

	fread(*buffer, fSize, 1, hFile);
	fclose(hFile);
	*bufferSize = fSize;

	return 1;
}

/*-----------------------------------------------------------------------------
    func:   FindString
    desc:   
    pass:   Nothing
    rtrn:   Nothing   
-----------------------------------------------------------------------------*/
unsigned long FindString(TCHAR *buffer, unsigned long bufferlen, TCHAR *string, unsigned long start){
	unsigned long i, stringlen;
	stringlen = strlen(string);
    for (i=start; i<bufferlen-stringlen; i++) { 
        if (memcmp(&buffer[i], string, stringlen) == 0)
			return i; 
    }
    return 0;
}

void ExportBookmarks(void){
	FILE	*Export;
	int		i, iRet;
	errno_t err;
	TCHAR	*szTemporary, *szTemp2;

	szTemporary= (TCHAR*)calloc(1024, sizeof(TCHAR));
	szTemp2= (TCHAR*)calloc(1024, sizeof(TCHAR));

	if( (err= fopen_s(&Export, "bookmark_export.html", "a+" ))!=0 ){
		MessageBox(NULL, "The file could not be appended.", "File Append Error", MB_OK);
	}else{
		iRet= ListView_GetItemCount(hwndList);
		for( i=0; i<iRet; i++ ){
			ListView_GetItemText(hwndList,i, 0, szTemporary, 512);
			fprintf_s(Export, "<a href=\"%s\">%s</a><br>\r\n", szTemporary, szTemporary);
		}
	}
	fclose(Export);
	GetCurrentDirectory(BSIZE, szTemporary);
	_stprintf_s(szTemp2, 1024, "Bookmarks exported to %s\\bookmark_export.html", szTemporary);
	MessageBox(NULL, szTemp2, "Export Successful", MB_OK);
	free(szTemporary);
	free(szTemp2);
	szTemp2= NULL;
	szTemporary= NULL;
}