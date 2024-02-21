#ifdef _WIN32
#include "CommonFile.h"
#include "MiniDumpMgr.h"

#pragma comment(lib, "dbghelp.lib")


CMiniDumpMgr::CMiniDumpMgr()
	: m_dumpMode(DM_MINI)
{
	getInstances().insert(this);
}

CMiniDumpMgr::~CMiniDumpMgr()
{
	getInstances().erase(this);

	if(getInstances().empty())
		UninstallCrashHook();
}

void CMiniDumpMgr::SetDumpMode(DumpMode mode)
{
	m_dumpMode = mode;
}

std::set<CMiniDumpMgr*>& CMiniDumpMgr::getInstances()
{
	static std::set<CMiniDumpMgr*>	instances;
	return instances;
}

CString CMiniDumpMgr::getDumpFileName()
{
	CString		name;
	GetModuleFileName(NULL, name.GetBuffer(_MAX_PATH), _MAX_PATH);
	name.ReleaseBuffer();

	SYSTEMTIME	now;
	GetLocalTime(&now);

	name.AppendFormat(_T("_%d%02d%02d_%d%02d%02d.dmp"),
		now.wYear, now.wMonth, now.wDay,
		now.wHour, now.wMinute, now.wSecond);
	return name;
}

bool CMiniDumpMgr::CreateDump(EXCEPTION_POINTERS *pep)
{
	HANDLE	hFile = CreateFile(getDumpFileName(), GENERIC_READ | GENERIC_WRITE, 
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hFile == NULL || hFile == INVALID_HANDLE_VALUE)
		return false;

	MINIDUMP_TYPE	mdt;
	
	switch(m_dumpMode)
	{
	case DM_MINI:
		mdt = (MINIDUMP_TYPE)(
			MiniDumpWithIndirectlyReferencedMemory |
			MiniDumpScanMemory);
		break;
	case DM_MIDI:
		mdt = (MINIDUMP_TYPE)(
			MiniDumpWithPrivateReadWriteMemory |
			MiniDumpWithDataSegs |
			MiniDumpWithHandleData |
			MiniDumpWithFullMemoryInfo |
			MiniDumpWithThreadInfo |
			MiniDumpWithUnloadedModules);
		break;
	case DM_FULL:
		mdt = (MINIDUMP_TYPE)(
			MiniDumpWithFullMemory |
			MiniDumpWithFullMemoryInfo |
			MiniDumpWithHandleData |
			MiniDumpWithThreadInfo |
			MiniDumpWithUnloadedModules);
		break;
	default:
		mdt = MiniDumpNormal;
	}

	MINIDUMP_EXCEPTION_INFORMATION	mdei;
	mdei.ThreadId = GetCurrentThreadId();
	mdei.ExceptionPointers = pep;
	mdei.ClientPointers = FALSE;

	//MINIDUMP_CALLBACK_INFORMATION	mci;
	//mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)MyMiniDumpCallback;
	//mci.CallbackParam = 0;

	BOOL rv = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
		hFile, mdt, (pep != 0) ? &mdei : 0, 0, NULL/*&mci*/);

	CloseHandle(hFile);
	return rv != FALSE;
}

void CMiniDumpMgr::InstallCrashHook()
{
	SetUnhandledExceptionFilter(myExceptionFilter);
}

void CMiniDumpMgr::UninstallCrashHook()
{
	SetUnhandledExceptionFilter(NULL);
}

LONG WINAPI CMiniDumpMgr::myExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
	for(auto it=getInstances().begin(); it!=getInstances().end(); ++it)
		(*it)->CreateDump(ExceptionInfo);

	//MessageBox(NULL, _T("出现无法恢复的错误，请点确定退出程序。"),
	//	_T("错误"), MB_ICONERROR | MB_OK);
	return EXCEPTION_EXECUTE_HANDLER;
}

#endif
