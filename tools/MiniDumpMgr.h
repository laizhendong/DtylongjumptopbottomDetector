#pragma once

#include <dbghelp.h>
#include <atlstr.h>
#include <set>


class CMiniDumpMgr
{
public:
	enum DumpMode { DM_TINY = 0, DM_MINI, DM_MIDI, DM_FULL };

	CMiniDumpMgr();
	~CMiniDumpMgr();

	void SetDumpMode(DumpMode mode);
	bool CreateDump(EXCEPTION_POINTERS *pep = NULL);
	static void InstallCrashHook();
	static void UninstallCrashHook();

private:
	static std::set<CMiniDumpMgr*>& getInstances();
	CString getDumpFileName();

	static LONG WINAPI myExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo);

private:
	DumpMode	m_dumpMode;
};
