#pragma once

namespace PhotoMode
{
	inline StringMap<RE::TESIdleForm*> cachedIdles;
	
	void InstallHooks();
    void ReleaseViewRoll();
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
