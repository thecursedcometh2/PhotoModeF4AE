#pragma once

namespace PhotoMode
{
	inline StringMap<RE::TESIdleForm*> cachedIdles;
	
	void InstallHooks();
    void LogViewRollStatus(float degrees);
}

namespace Screenshot
{
	void InstallHooks();
}

namespace Hooks
{
	void Install();
}
