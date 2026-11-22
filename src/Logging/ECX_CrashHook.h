#pragma once

namespace LOGGING
{
	// Debug/Windows only. Installs a CRT report hook (asserts and STL debug checks) and an
	// unhandled-exception filter that write the message and a symbolised stack into the engine log and
	// flush log.html. The normal dialog and crash handling still follow. Call first in main().
	void installCrashHook();
}
