#pragma once

namespace LOGGING
{
	// Debug-build, Windows-only crash reporting (a no-op everywhere else). Installs:
	//  - a CRT report hook, so a failed assert()/_ASSERT and the MSVC STL's debug checks
	//    ("vector iterators incompatible", "vector subscript out of range", ...) all get
	//    their message and a symbolised stack trace of the failing thread written into the
	//    engine log, and the log flushed to file, BEFORE the usual Abort/Retry/Ignore dialog
	//    appears. The dialog and its Retry-to-debug behaviour are left exactly as they were.
	//  - an unhandled-exception filter doing the same for a hard crash (access violation
	//    etc.), then passing the exception on to normal crash handling.
	// Meant for rare races that can't be reproduced on demand: whoever hits it next has the
	// full stack in log.html even if they weren't running under a debugger.
	// Call once, as early as possible in main().
	void installCrashHook();
}
