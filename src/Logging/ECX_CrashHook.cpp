#include "ECX_CrashHook.h"

#if defined(_WIN32) && defined(_DEBUG) && defined(_M_X64)

#include "ECX_Logging.h"
#include <windows.h>
#include <dbghelp.h>
#include <crtdbg.h>
#include <mutex>
#include <sstream>
#include <string>

#pragma comment(lib, "dbghelp.lib")

namespace
{
	// DbgHelp is single-threaded; two threads crashing at once just take turns.
	std::mutex g_symbolLock;

	// Walks `context`'s stack on the calling thread and formats one line per frame:
	// function name and file:line when the PDB (next to the exe) has them, else the raw
	// address. Capped so a runaway/recursive stack can't produce an enormous log entry.
	std::string captureStack(CONTEXT context)
	{
		std::lock_guard<std::mutex> lock(g_symbolLock);

		HANDLE process = GetCurrentProcess();
		HANDLE thread = GetCurrentThread();

		static bool symbolsReady = false;
		if (!symbolsReady)
		{
			SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
			SymInitialize(process, nullptr, TRUE);
			symbolsReady = true;
		}

		STACKFRAME64 frame = {};
		frame.AddrPC.Offset = context.Rip;
		frame.AddrPC.Mode = AddrModeFlat;
		frame.AddrFrame.Offset = context.Rbp;
		frame.AddrFrame.Mode = AddrModeFlat;
		frame.AddrStack.Offset = context.Rsp;
		frame.AddrStack.Mode = AddrModeFlat;

		std::ostringstream out;
		for (int i = 0; i < 48; ++i)
		{
			if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, &context, nullptr,
				SymFunctionTableAccess64, SymGetModuleBase64, nullptr) || frame.AddrPC.Offset == 0)
				break;

			alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + 255] = {};
			SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = 255;

			out << "  #" << i << " ";
			DWORD64 displacement = 0;
			if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
				out << symbol->Name;
			else
				out << "0x" << std::hex << frame.AddrPC.Offset << std::dec;

			IMAGEHLP_LINE64 line = {};
			line.SizeOfStruct = sizeof(line);
			DWORD lineDisplacement = 0;
			if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line))
				out << "  (" << line.FileName << ":" << line.LineNumber << ")";
			out << "\n";
		}
		return out.str();
	}

	// Per-thread, not global: two threads asserting at once should both get logged (the
	// logger copes with contention); only a thread re-entering the hook while it is already
	// inside it (e.g. an assertion raised by the reporting code itself) must be skipped.
	thread_local bool t_reporting = false;

	void reportCrtProblem(const char* kind, const std::string& message)
	{
		if (t_reporting) return;
		t_reporting = true;

		CONTEXT context;
		RtlCaptureContext(&context);

		// The first few frames are this hook and the CRT's own reporting machinery, not the
		// code that failed - drop them so the stack starts at the real culprit.
		std::istringstream frames(captureStack(context));
		std::string stack, frameLine;
		bool inReportingFrames = true;
		while (std::getline(frames, frameLine))
		{
			if (inReportingFrames && (frameLine.find("reportCrtProblem") != std::string::npos ||
				frameLine.find("wideReportHook") != std::string::npos ||
				frameLine.find("CrtDbgReport") != std::string::npos))
				continue;
			inReportingFrames = false;
			stack += frameLine + "\n";
		}

		std::ostringstream text;
		text << kind << " on thread " << GetCurrentThreadId() << ":\n"
			<< message << "\nStack (most recent call first):\n" << stack;
		LOGGING::ECX_Logger::GetInstance()->logCrashAndFlush(text.str());

		t_reporting = false;
	}

	// Only the wide hook is installed: assert() reports through the wide CRT path, and the
	// narrow one (_RPTF, which is how the STL's "vector iterators incompatible" arrives)
	// runs the wide hooks as well - installing both logged every report twice.
	// Returning FALSE tells the CRT to carry on with its normal reporting - the
	// Abort/Retry/Ignore dialog (and Retry -> break into the debugger) still appears exactly
	// as before; this only makes sure the log is already on disk when it does.
	int __cdecl wideReportHook(int reportType, wchar_t* message, int* /*returnValue*/)
	{
		if (reportType == _CRT_ASSERT || reportType == _CRT_ERROR)
		{
			std::string narrow;
			if (message)
			{
				int size = WideCharToMultiByte(CP_UTF8, 0, message, -1, nullptr, 0, nullptr, nullptr);
				if (size > 1)
				{
					narrow.resize(static_cast<size_t>(size) - 1);
					WideCharToMultiByte(CP_UTF8, 0, message, -1, narrow.data(), size, nullptr, nullptr);
				}
			}
			reportCrtProblem(reportType == _CRT_ASSERT ? "CRT ASSERTION" : "CRT ERROR", narrow);
		}
		return FALSE;
	}

	LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* info)
	{
		thread_local bool reporting = false;
		if (reporting || !info || !info->ExceptionRecord || !info->ContextRecord)
			return EXCEPTION_CONTINUE_SEARCH;
		reporting = true;

		const EXCEPTION_RECORD& record = *info->ExceptionRecord;
		std::ostringstream text;
		text << "UNHANDLED EXCEPTION 0x" << std::hex << record.ExceptionCode << std::dec
			<< " on thread " << GetCurrentThreadId();
		if (record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record.NumberParameters >= 2)
		{
			const ULONG_PTR kind = record.ExceptionInformation[0];
			text << " - access violation " << (kind == 0 ? "reading" : kind == 1 ? "writing" : "executing")
				<< " address 0x" << std::hex << record.ExceptionInformation[1] << std::dec;
		}
		text << "\nStack (most recent call first):\n" << captureStack(*info->ContextRecord);
		LOGGING::ECX_Logger::GetInstance()->logCrashAndFlush(text.str());

		reporting = false;
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

namespace LOGGING
{
	void installCrashHook()
	{
		_CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, wideReportHook);
		SetUnhandledExceptionFilter(unhandledExceptionFilter);
	}
}

#else

namespace LOGGING
{
	void installCrashHook() {}
}

#endif
