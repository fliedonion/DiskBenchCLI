#include <iostream>
#include <iomanip>
#include <windows.h>
#include <atlstr.h>




using namespace std;


static HANDLE hFile;



const int kFormatMessageBufferSize = 4096;


int WriteTest(TCHAR*, DWORD);
int ReadTest(const TCHAR*);
void print_error();

std::ios_base::fmtflags init_flags(cout.flags());



int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR module_path[MAX_PATH];
	GetModuleFileName(NULL, module_path, MAX_PATH);

	TCHAR module_fname[_MAX_FNAME];
	TCHAR module_fext[_MAX_EXT];
	// _MAX_DRIVE // _MAX_DIR 

	_tsplitpath_s(module_path, NULL, 0, NULL, 0, module_fname, _MAX_FNAME, module_fext, _MAX_EXT);

	if (argc != 2) {
		wcerr << "Usage: " << module_fname << module_fext << " \"<path to testdata file>\"" << endl;
		wcerr << "       if specified file already exists, the file will be deleted." << endl;
		return 1;
	}


	TCHAR target_path[MAX_PATH];
	_tcscpy_s(target_path, MAX_PATH, argv[1]);


	TCHAR* target_dir_only_ptr;

	TCHAR target_drive[_MAX_DRIVE];
	TCHAR target_fname[_MAX_FNAME];
	TCHAR target_fext[_MAX_EXT];

	target_dir_only_ptr = new TCHAR[MAX_PATH];

	// wcout << target_path << endl;

	if (_tsplitpath_s(target_path, target_drive, _MAX_DRIVE, target_dir_only_ptr, MAX_PATH, target_fname, _MAX_FNAME, target_fext, _MAX_EXT)) {
		wcerr << "ERROR: parsing target path. " << endl;
		wcerr << "       please specify absolute path. " << endl;

		delete[] target_dir_only_ptr;
		return 1;
	}



	// wcout << target_drive << target_dir_only << " " << target_fname << target_fext << endl;
	// wcout << argv[1] << endl;

	if (_tcsclen(target_dir_only_ptr) == 1) { // only null termination
		wcerr << "DON'T specify drive root folder as target directory." << endl;

		delete[] target_dir_only_ptr;
		return 1;
	}

	TCHAR target_directory[MAX_PATH];


	if (_tcsclen(target_drive) > 0) {
		PathCombine(target_directory, target_drive, target_dir_only_ptr);
	}
	else {
		_tcscpy_s(target_directory, MAX_PATH, target_dir_only_ptr);
	}

	delete[] target_dir_only_ptr;

	wcout << "Target Directory is... " << target_directory << endl << endl;
	if (!PathFileExists(target_directory) || !PathIsDirectory(target_directory)) {
		wcerr << target_directory << " not found or is not a directory" << endl;
		return 1;
	}


	wcout << "NOTICE: Since this speed test is very simple, there is a large variation in " << endl;
	wcout << "        the results. If possible, it is recommended to take more than " << endl;
	wcout << "        10 measurements and compare them." << endl
		<< endl;


	if (!WriteTest(target_path, CREATE_ALWAYS)) {
		if (!DeleteFile(target_path)) {
			print_error();
		}
		return 1;
	}

	wcout << "Interval 2sec." << endl;
	Sleep(2000);

	if (!WriteTest(target_path, OPEN_EXISTING)) {
		if (!DeleteFile(target_path)) {
			print_error();
		}
		return 1;
	}

	wcout << "Interval 2sec." << endl;
	Sleep(2000);

	if (!ReadTest(target_path)) {
		if (!DeleteFile(target_path)) {
			print_error();
		}
		return 1;
	}

	wcout << "* Remove work file..." << endl;
	if (!DeleteFile(target_path)) {
		print_error();
	}
	wcout << "Done." << endl;
	return 0;
}


BOOL _WriteTestCore(TCHAR* target_path, DWORD dwCreationDisposition, int datasize_mb, char* buf, int bufsize) {

	// hFile = CreateFile(target_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
	//    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	hFile = CreateFile(target_path,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		dwCreationDisposition,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		print_error();

		wcerr << "[ERR] WRITE TEST " << "Create File Error" << endl;
		return FALSE;
	}


	DWORD writtenBytes = 0;

	ULONGLONG start = GetTickCount64();

	streamsize indent = 2;


	wcout << "* Disk Write Test ";
	if (dwCreationDisposition == CREATE_ALWAYS) {
		wcout << "mode:CREATE_ALWAYS";
	}
	else if (dwCreationDisposition == OPEN_EXISTING) {
		wcout << "mode:OPEN_EXISTING";
	}
	else {
		wcout << "mode:(" << dwCreationDisposition << ")";
	}
	wcout <<" [ Q1 T1 Seq Buf=1MB ] ..." << endl;

	wcout << setw(7 + indent) << setfill(L' ') << "    0.0 %";
	wcout << fixed << setprecision(1);


	for (int i = 0; i < datasize_mb; i++)
	{
		if (WriteFile(hFile, buf, bufsize, &writtenBytes, NULL))
		{
			if (i % 100 == 0) {
				wcout << "\b\b\b\b\b\b\b\b\b";
				wcout << setw(5 + indent) << setfill(L' ') << (i * 100.0 / datasize_mb) << " %";
			}
		}
		else
		{
			print_error();

			FlushFileBuffers(hFile);
			CloseHandle(hFile);

			wcerr << "[ERR] WRITE TEST " << "Data Write Error at loop-index " << i << endl;
			wcout.flags(init_flags);
			return FALSE;
		}
	}
	wcout << "\b\b\b\b\b\b\b\b\b";
	wcout << setw(7 + indent) << setfill(L' ') << "100.0 %" << endl;


	wcout.flags(init_flags);


	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

	FlushFileBuffers(hFile);
	CloseHandle(hFile);

	ULONGLONG finish = GetTickCount64();

	wcout << fixed << setprecision(1);
	wcout << "  WRITE: " << datasize_mb / ((finish - start) * 1.0 / 1000) << " MB/s" 
		<< " ( " << datasize_mb << "MB data / " << ((finish - start) * 1.0 / 1000) << " sec )"
		<< endl;

	wcout << "Done." << endl << endl;

	wcout.flags(init_flags);

	return TRUE;
}


BOOL WriteTest(TCHAR* target_path, DWORD dwCreationDisposition) {
	// WRITE TEST

	int datasize_mb = 1024;

	char* buf = NULL;
	int BufSize = 1024 * 1024;
	buf = (char*)malloc(BufSize);
	if (buf == NULL) {
		return FALSE;
	}

	memset(buf, 0xFF, BufSize);


	BOOL result = _WriteTestCore(target_path, dwCreationDisposition, datasize_mb, buf, BufSize);

	free(buf);
	return result;
}




BOOL _ReadTestCore(const TCHAR* const target_path, int datasize_mb, char* buf, int bufsize) {


	hFile = CreateFile(target_path,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		print_error();

		wcerr << "[ERR] READ TEST " << "Open File Error" << endl;
		return FALSE;
	}


	const streamsize indent = 2;
	DWORD readBytes = 0;

	ULONGLONG start = GetTickCount64();

	wcout << "* Disk Read Test [ Q1 T1 Seq Buf 1MB ] ..." << endl;
	wcout << setw(7 + indent) << setfill(L' ') << "  0.0 %";
	wcout << fixed << setprecision(1);


	for (int i = 0; i < datasize_mb; i++)
	{
		if (ReadFile(hFile, buf, bufsize, &readBytes, NULL))
		{
			if (i % 100 == 0) {
				wcout << "\b\b\b\b\b\b\b\b\b";
				wcout << setw(5 + indent) << setfill(L' ') << (i * 100.0 / datasize_mb) << " %";
			}
		}
		else
		{
			print_error();

			CloseHandle(hFile);

			wcerr << "[ERR] READ TEST " << "Data Read Error at loop-index " << i << endl;
			wcout.flags(init_flags);

			return FALSE;
		}
	}
	wcout << "\b\b\b\b\b\b\b\b\b";
	wcout << setw(7 + indent) << setfill(L' ') << "100.0 %" << endl;

	wcout.flags(init_flags);


	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	CloseHandle(hFile);

	ULONGLONG finish = GetTickCount64();


	wcout << fixed << setprecision(1);

	wcout << "  READ " << datasize_mb / ((finish - start) * 1.0 / 1000) << " MB/s"
		<< " ( " << datasize_mb << "MB data / " << ((finish - start) * 1.0 / 1000) << " sec )"
		<< endl;

	wcout << "Done." << endl << endl;

	wcout.flags(init_flags);
	return TRUE;
}


BOOL ReadTest(const TCHAR* const target_path) {
	// READ TEST

	int datasize_mb = 1024;

	char* buf = NULL;
	int BufSize = 1024 * 1024;
	buf = (char*)malloc(BufSize);
	if (buf == NULL) {
		return FALSE;
	}

	memset(buf, 0x00, BufSize);


	BOOL result = _ReadTestCore(target_path, datasize_mb, buf, BufSize);

	free(buf);
	return result;
}



void print_error()
{
	DWORD errorCode = GetLastError();
	TCHAR szFormatBuffer[kFormatMessageBufferSize];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)szFormatBuffer,
		kFormatMessageBufferSize,
		NULL);

	wcerr << "ERR-CODE: " << errorCode << endl;
	wcerr << szFormatBuffer << endl;
}