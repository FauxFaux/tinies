#define NTDDI_VERSION NTDDI_VISTA
#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <iomanip>
#include <imagehlp.h>

#pragma comment(lib, "imagehlp.lib")

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
		if (argc != 3)
			ErrorHandler("usage: verify|unsign filename");

		bool fixing;
		if (argv[1] == std::wstring(L"verify"))
			fixing = false;
		else if (argv[1] == std::wstring(L"unsign"))
			fixing = true;
		else
			ErrorHandler("Not an action name");

		HANDLE file = CreateFile(argv[2],
			(fixing ? GENERIC_WRITE : 0) | GENERIC_READ,
			FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (file == INVALID_HANDLE_VALUE)
			ErrorHandler("couldn't load file");

		DWORD cnt;
		const DWORD BUF_SIZE = 10;
		DWORD buf[BUF_SIZE];

		if (!ImageEnumerateCertificates(file, CERT_SECTION_TYPE_ANY, &cnt, buf, BUF_SIZE))
			std::cout << "Normal cert parsing failed: Error: " << GetLastError() << "." << std::endl;
		else
			std::cout << "Normal cert passing was fine.  You should be able to use signtool as normal." << std::endl;

		HANDLE mapping = CreateFileMapping(file, NULL, fixing ? PAGE_READWRITE : PAGE_READONLY, 0, 0, NULL);
		if (NULL == mapping)
			ErrorHandler("couldn't map file");

		void *base = MapViewOfFile(mapping, (fixing ? FILE_MAP_WRITE : 0) | FILE_MAP_READ, 0, 0, 0);
		if (NULL == base)
			ErrorHandler("couldn't map file");

		PIMAGE_NT_HEADERS headers = ImageNtHeader(base);
		if (NULL == headers)
			ErrorHandler("couldn't get header");

		#define certificates (headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY])

		if (0 == certificates.Size && 0 == certificates.VirtualAddress)
			std::cout << "No certificates block at all, nothing for me to do." << std::endl;
		else {
			std::cout << "Header thinks the certificates are at " << certificates.VirtualAddress
				<< " for " << certificates.Size << " bytes." << std::endl;

#ifdef DUMP_CERT_BLOCK
			int j = 0;
			for (size_t i = certificates.VirtualAddress; i < certificates.VirtualAddress + certificates.Size; ++i) {
				if (++j == 10) {
					j = 0;
					std::cout << std::endl;
				}
				const char c = static_cast<char*>(base)[i];
				std::cout << std::setw(3) << std::hex << static_cast<int>(static_cast<unsigned char>(c));
			}

			std::cout << std::endl;
#endif

			if (fixing) {
				certificates.Size = 0;
				certificates.VirtualAddress = 0;
				if (!FlushViewOfFile(base, 0))
					ErrorHandler("couldn't flush changes");
				std::cout << "Signature removed." << std::endl;
			}
		}

		UnmapViewOfFile(base);
		CloseHandle(mapping);
		CloseHandle(file);
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << "\n" << "Error code: " << GetLastError() << "." << std::endl;
		return 1;
	}
	return 0;
}
