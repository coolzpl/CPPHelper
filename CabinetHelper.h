#pragma once

#include <fdi.h>
#include <Fci.h>

#pragma comment(lib,"cabinet.lib")


#include <io.h>
#include <fcntl.h>

#include <comdef.h>



#include <atlstr.h>
#include <Strsafe.h>
#include "Base.h"
#include "StringHelper.h"

//#define _IsDots(FileName) (((FileName)[0] == L'.' &&((FileName)[1] == NULL ||((FileName)[1] == L'.'&&(FileName)[2] == NULL))))

static FNOPEN(fnFileOpen)
{
	DWORD dwDesiredAccess = 0;
	DWORD dwCreationDisposition = 0;

	UNREFERENCED_PARAMETER(pmode);

	if (oflag & _O_RDWR)
	{
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	}
	else if (oflag & _O_WRONLY)
	{
		dwDesiredAccess = GENERIC_WRITE;
	}
	else
	{
		dwDesiredAccess = GENERIC_READ;
	}

	if (oflag & _O_CREAT)
	{
		dwCreationDisposition = CREATE_ALWAYS;
	}
	else
	{
		dwCreationDisposition = OPEN_EXISTING;
	}

	return (INT_PTR)CreateFile(UTF8ToUnicode( pszFile),
		dwDesiredAccess,
		FILE_SHARE_READ,
		NULL,
		dwCreationDisposition,
		FILE_ATTRIBUTE_NORMAL| FILE_OPTION,
		NULL);
}

static FNREAD(fnFileRead)
{
	DWORD dwBytesRead = 0;

	return ReadFile((HANDLE)hf, pv, cb, &dwBytesRead, NULL) ? dwBytesRead : -1;
}

static FNWRITE(fnFileWrite)
{
	DWORD dwBytesWritten = 0;

	if (WriteFile((HANDLE)hf, pv, cb, &dwBytesWritten, NULL) == FALSE)
	{
		dwBytesWritten = (DWORD)-1;
	}

	return dwBytesWritten;
}

static FNCLOSE(fnFileClose)
{
	return CloseHandle((HANDLE)hf) ? 0 : -1;
}

static FNSEEK(fnFileSeek)
{
	return SetFilePointer((HANDLE)hf, dist, NULL, seektype);
}


#define FDICreate2(pErf) FDICreate((PFNALLOC)malloc,(PFNFREE)free,(PFNOPEN)fnFileOpen,(PFNREAD)fnFileRead,(PFNWRITE)fnFileWrite,(PFNCLOSE)fnFileClose,(PFNSEEK)fnFileSeek,cpuUNKNOWN,pErf)

static FNFCIFILEPLACED(fnFCIFilePlaced)
{
	UNREFERENCED_PARAMETER(pv);

	return 0;
}

static FNFCIOPEN(fnFCIFileOpen)
{
	UNREFERENCED_PARAMETER(pv);

	auto hFile=fnFileOpen(pszFile, oflag, pmode);

	if (hFile == -1)
	{
		*err = GetLastError();
	}

	return (INT_PTR)hFile;
}


static FNFCIREAD(fnFCIFileRead)
{
	DWORD dwBytesRead = 0;

	UNREFERENCED_PARAMETER(pv);

	if (!ReadFile((HANDLE)hf, memory, cb, &dwBytesRead, NULL))
	{
		dwBytesRead = (DWORD)-1;
		*err = GetLastError();
	}

	return dwBytesRead;
}


static FNFCIWRITE(fnFCIFileWrite)
{
	DWORD dwBytesWritten = 0;

	UNREFERENCED_PARAMETER(pv);

	if (!WriteFile((HANDLE)hf, memory, cb, &dwBytesWritten, NULL))
	{
		dwBytesWritten = (DWORD)-1;
		*err = GetLastError();
	}

	return dwBytesWritten;
}

static FNFCIDELETE(fnFCIFileDelete)
{
	INT iResult = 0;

	UNREFERENCED_PARAMETER(pv);

	if (DeleteFileW(UTF8ToUnicode( pszFile)) == FALSE)
	{
		*err = GetLastError();
		iResult = -1;
	}

	return iResult;
}

static FNFCIGETTEMPFILE(fnFCIGetTempFileName)
{
	BOOL bSucceeded = FALSE;
	CHAR pszTempPath[MAX_PATH];
	CHAR pszTempFile[MAX_PATH];

	UNREFERENCED_PARAMETER(pv);
	UNREFERENCED_PARAMETER(cbTempName);

	if (GetTempPathA(MAX_PATH, pszTempPath) != 0)
	{
		if (GetTempFileNameA(pszTempPath, "CABINET", 0, pszTempFile) != 0)
		{
			DeleteFileA(pszTempFile);

		
			bSucceeded = SUCCEEDED(StringCbCopyA(pszTempName, cbTempName, pszTempFile));
		}
	}

	return bSucceeded;
}


static FNFCICLOSE(fnFCIFileClose)
{
	INT iResult = 0;

	UNREFERENCED_PARAMETER(pv);

	if (CloseHandle((HANDLE)hf) == FALSE)
	{
		*err = GetLastError();
		iResult = -1;
	}

	return iResult;
}

static FNFCISEEK(fnFCIFileSeek)
{
	INT iResult = 0;

	UNREFERENCED_PARAMETER(pv);

	iResult = SetFilePointer((HANDLE)hf, dist, NULL, seektype);

	if (iResult == -1)
	{
		*err = GetLastError();
	}

	return iResult;
}

static FNFCIGETNEXTCABINET(fnFDIGetNextCabinet)
{
	return FALSE;
}

static FNFCISTATUS(fnFCIStatus)
{
	return ERROR_SUCCESS;
}

static FNFCIGETOPENINFO(fnFCIGetOpenInfo)
{
	FILETIME fileTime;
	BY_HANDLE_FILE_INFORMATION fileInfo;

	auto hFile = (HANDLE)fnFCIFileOpen(pszName, _O_RDONLY, 0, err, pv);

	if (hFile != (HANDLE)-1)
	{
		if (GetFileInformationByHandle(hFile, &fileInfo)
			&& FileTimeToLocalFileTime(&fileInfo.ftCreationTime, &fileTime)
			&& FileTimeToDosDateTime(&fileTime, pdate, ptime))
		{
			*pattribs = (USHORT)fileInfo.dwFileAttributes;
			*pattribs &= (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
		}
		else
		{
			fnFCIFileClose((INT_PTR)hFile, err, pv);
			hFile = (HANDLE)-1;
		}
	}

	return (INT_PTR)hFile;
}


#define FCICreate2(pErf,ccab) FCICreate(pErf,fnFCIFilePlaced,(PFNALLOC)malloc,(PFNFREE)free,(PFNFCIOPEN)fnFCIFileOpen,(PFNFCIREAD)fnFCIFileRead,(PFNFCIWRITE)fnFCIFileWrite,(PFNFCICLOSE)fnFCIFileClose,(PFNFCISEEK)fnFCIFileSeek,fnFCIFileDelete,fnFCIGetTempFileName,&ccab,NULL)


static HRESULT CabExtractFile(LPCWSTR CabFilePath, LPCWSTR ExtractPath)
{
	ERF  erf;

	auto hfdi= FDICreate2(&erf);

	if (!hfdi)
		return E_FAIL;

	HRESULT hr = S_OK;

	if (!FDICopy(hfdi, (LPSTR)Unicode2UTF8( CabFilePath).GetString(), "", 0, [](FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION    pfdin)->INT_PTR
	{
		INT_PTR iResult = 0;


		switch (fdint)
		{
		case fdintCOPY_FILE:
		{
			auto FileName = StrRChrA(pfdin->psz1, NULL, '\\');

			CStringA FilePath = (LPCSTR)pfdin->pv;

			if (FileName)
			{
				FilePath.Append(pfdin->psz1, FileName - pfdin->psz1);

				CreateDirectoryW(UTF8ToUnicode(FilePath, FilePath.GetLength()), NULL);

				FilePath += FileName;
			}
			else
			{
				FilePath += pfdin->psz1;
			}

			iResult = fnFileOpen(FilePath.GetBuffer(), _O_WRONLY | _O_CREAT, 0);

			break;
		}
		case fdintCLOSE_FILE_INFO:
		{
			//pfdin->hf

			//auto pData = (vector<BYTE>*) ((void**)pfdin->pv)[1];

			//pData->_Mylast = pData->_Myfirst + pfdin->cb;

			//pfdin->hf = NULL;
			iResult = !fnFileClose(pfdin->hf);
			break;
		}
		case fdintNEXT_CABINET:
			break;
		case fdintPARTIAL_FILE:
			iResult = 0;
			break;
		case fdintCABINET_INFO:
			iResult = 0;
			break;
		case fdintENUMERATE:
			iResult = 0;
			break;
		default:
			iResult = -1;
			break;
		}

		return iResult;

	}, NULL, (void*)Unicode2UTF8(ExtractPath).GetString()))
	{
		hr = E_FAIL;
	}


	FDIDestroy(hfdi);

	return hr;
}


static HRESULT CabExtractFile(LPCSTR CabFilePath, LPCSTR ExtractPath)
{
	return CabExtractFile(CStringW(CabFilePath), CStringW(ExtractPath));
}

//此接口仅内部调用
static HRESULT CabCreateFileInternal_U(HFCI hfci,int chRoot, CStringA FilePath, TCOMP typeCompress)
{
	WIN32_FIND_DATAW FindData;

	HANDLE hFileFind = FindFirstFileW(UTF8ToUnicode( FilePath) + L'*', &FindData);

	if (hFileFind == INVALID_HANDLE_VALUE)
		return GetLastError();
	HRESULT hr = S_OK;
	do
	{
		auto TempFileName = FilePath + Unicode2UTF8( FindData.cFileName);

		if (FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			if (_IsDots(FindData.cFileName))
				continue;
			TempFileName += '\\';
			hr = CabCreateFileInternal_U(hfci, chRoot, TempFileName, typeCompress);
			if (hr!=S_OK)
				break;
		}
		else
		{
			if (!FCIAddFile(hfci, TempFileName.GetBuffer(), TempFileName.GetBuffer() + chRoot, FALSE, NULL, fnFCIStatus, fnFCIGetOpenInfo, typeCompress))
			{
				hr = E_FAIL;
				break;
			}
		}


	} while (FindNextFileW(hFileFind,&FindData));

	FindClose(hFileFind);

	return hr;
}

//将一个文件夹压缩为cab
static HRESULT CabCreateDirectory_U(LPCSTR CabFilePath, CStringA RootPath, TCOMP typeCompress= TCOMPfromLZXWindow(21))
{
	if (RootPath.IsEmpty())
		return 87;

	if (RootPath[RootPath.GetLength() - 1] != '\\')
		RootPath += '\\';

	if (GetFileType(CabFilePath))
	{
		if (!DeleteFileW( UTF8ToUnicode( CabFilePath)))
			return GetLastError();
	}

	ERF erf;            //FCI error structure

	CCAB ccab = { 0 ,900000 };           //cabinet information structure
	ccab.setID = 12345;
	ccab.iCab = 1;
	ccab.iDisk = 0;

	auto FileName = PathFindFileNameA(CabFilePath);
	StrCpyA(ccab.szCab, FileName);
	StrCpyNA(ccab.szCabPath, CabFilePath, FileName - CabFilePath +1);

	HFCI hfci = FCICreate2(&erf, ccab);

	if (hfci == NULL)
	{
		return erf.erfOper;
	}

	HRESULT hr = CabCreateFileInternal_U(hfci, RootPath.GetLength(), RootPath, typeCompress);

	if (hr == S_OK)
	{
		if (!FCIFlushCabinet(hfci, FALSE, NULL, fnFCIStatus))
		{
			hr=erf.erfOper;
		}
	}

	FCIDestroy(hfci);



	return hr;
}

static HRESULT CabCreateDirectory(LPCWSTR CabFilePath, LPCWSTR RootPath, TCOMP typeCompress = TCOMPfromLZXWindow(21))
{
	return CabCreateDirectory_U(Unicode2UTF8(CabFilePath), Unicode2UTF8(RootPath), typeCompress);
}
static HRESULT CabCreateDirectory(LPCSTR CabFilePath, LPCSTR RootPath, TCOMP typeCompress = TCOMPfromLZXWindow(21))
{
	return CabCreateDirectory(CStringW(CabFilePath), CStringW(RootPath), typeCompress);
}

static HRESULT CabCreateFile(LPCWSTR CabFilePath, LPCWSTR SourceFile, LPCWSTR FileName, TCOMP typeCompress = TCOMPfromLZXWindow(21))
{
	if (GetFileType(CabFilePath))
	{
		if (!DeleteFileW(CabFilePath))
			return GetLastError();
	}

	ERF erf;            //FCI error structure

	CCAB ccab = { 0 ,900000 };           //cabinet information structure
	ccab.setID = 12345;
	ccab.iCab = 1;
	ccab.iDisk = 0;

	auto CabFilePath_U = Unicode2UTF8(CabFilePath);

	auto CabFileName = PathFindFileNameA(CabFilePath_U);
	StrCpyA(ccab.szCab, CabFileName);
	StrCpyNA(ccab.szCabPath, CabFilePath_U, CabFileName - CabFilePath_U + 1);

	HFCI hfci = FCICreate2(&erf, ccab);

	if (hfci == NULL)
	{
		return erf.erfOper;
	}

	//HRESULT hr = CabCreateFileInternal(hfci, RootPath.GetLength(), RootPath, typeCompress);
	if (FCIAddFile(hfci, (LPSTR)Unicode2UTF8(SourceFile).GetString(), (LPSTR)Unicode2UTF8(FileName).GetString(), FALSE, NULL, fnFCIStatus, fnFCIGetOpenInfo, typeCompress))
	{
		if (FCIFlushCabinet(hfci, FALSE, NULL, fnFCIStatus))
		{
			erf.erfOper = S_OK;
		}
	}

	FCIDestroy(hfci);

	return erf.erfOper;
}

static HRESULT CabCreateFile(LPCSTR CabFilePath, LPCSTR SourceFile, LPCSTR FileName, TCOMP typeCompress = TCOMPfromLZXWindow(21))
{
	return CabCreateFile(CStringW(CabFilePath), CStringW(SourceFile), CStringW(FileName), typeCompress);
}

static HRESULT CabCreateFile(LPCSTR CabFilePath, LPCSTR SourceFile, TCOMP typeCompress = TCOMPfromLZXWindow(21))
{
	return CabCreateFile(CabFilePath, SourceFile,PathFindFileNameA(SourceFile), typeCompress);
}

static HRESULT CabCreateFile(LPCWSTR CabFilePath, LPCWSTR SourceFile, TCOMP typeCompress = TCOMPfromLZXWindow(21))
{
	return CabCreateFile(CabFilePath,SourceFile, PathFindFileNameW(SourceFile),typeCompress);
}
