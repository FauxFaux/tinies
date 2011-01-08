#define NTDDI_VERSION NTDDI_VISTA
#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <iostream>
#include <stdexcept>
#include <string>

#ifndef LOAD_LIBRARY_AS_IMAGE_RESOURCE
#define LOAD_LIBRARY_AS_IMAGE_RESOURCE 0x00000020
#endif

using std::cout; using std::endl; using std::string;

void ErrorHandler(const char* s)
{
	throw std::exception(s);
}

int wmain(int argc, wchar_t*argv[])
{
	try
	{
		HRSRC hResLoad;     // handle to loaded resource
		HMODULE hExe = NULL;        // handle to existing .EXE file
		HRSRC hRes;         // handle/ptr. to res. info. in hExe
		HANDLE hUpdateRes;  // update resource handle
		char *lpResLock;    // pointer to resource data
		BOOL result;

		if (argc != 2)
			ErrorHandler("Filename please.");

		// Load the .EXE file that contains the dialog box you want to copy.
		hExe = LoadLibraryEx(argv[1], NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
		//hExe = LoadLibrary(argv[1]);

		if (hExe == NULL)
			ErrorHandler("Could not load exe.");

		// Locate the dialog box resource in the .EXE file.
		hRes = FindResource(hExe, MAKEINTRESOURCE(1), MAKEINTRESOURCE(RT_MANIFEST));
		if (hRes == NULL)
			ErrorHandler("Could not locate resource.");

		// Load the dialog box into global memory.
		hResLoad = (HRSRC)LoadResource(hExe, hRes);
		if (hResLoad == NULL)
			ErrorHandler("Could not load dialog box.");

		// Lock the dialog box into global memory.
		lpResLock = (char *)LockResource(hResLoad);
		if (lpResLock == NULL)
			ErrorHandler("Could not lock dialog box.");

		string manifest(lpResLock, SizeofResource(hExe, hRes));

		// Clean up the loaded library.
		if (!FreeLibrary(hExe))
			ErrorHandler("Could not free executable.");

		// "Parse" the XML to terminate the trustInfo element.
		string::size_type begin = manifest.find("<trustInfo");
		if (begin == string::npos)
			ErrorHandler("Start tag not found, no operation may be necessary?");

		const string trustinfo("</trustInfo>");
		string::size_type end = manifest.find(trustinfo, begin);
		if (end == string::npos)
			ErrorHandler("End tag not found.");

		const string::size_type num_to_replace = end - begin + trustinfo.size();
		manifest.replace(begin, num_to_replace, num_to_replace, ' ');

		// Open the file to which you want to add the dialog box resource.
		hUpdateRes = BeginUpdateResource(argv[1], FALSE);
		if (hUpdateRes == NULL)
			ErrorHandler("Could not open file for writing.");

		// Overwrite the resource.
		result = UpdateResource(hUpdateRes,       // update resource handle
			 RT_MANIFEST,
			 MAKEINTRESOURCE(1),
			 MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),  // neutral language, guess/hack.
			 (LPVOID)manifest.c_str(),
			 manifest.size());

		if (result == FALSE)
			ErrorHandler("Could not add resource.");

		if (!EndUpdateResource(hUpdateRes, FALSE))
			ErrorHandler("Could not write changes to file.");
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << "\n" << "Error code: " << GetLastError() << "." << std::endl;
		return 1;
	}
	return 0;
}