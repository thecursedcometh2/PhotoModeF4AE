#pragma once

namespace PhotoMode
{
	inline StringMap<RE::TESIdleForm*> cachedIdles;
	
	void InstallHooks();
    void EnsureViewRollHook();
}

namespace Screenshot
{
	void InstallHooks();
}

namespace Hooks
{
	void Install();
}
