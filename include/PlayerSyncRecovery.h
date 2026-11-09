#pragma once

namespace PhotoMode
{
    enum class SyncRecoveryResult { refused, unavailable, alreadyClear, writeFailed, notCleared, cleared };

    template <class Graph>
    SyncRecoveryResult ClearPlayerSync(Graph& graph, bool permitted)
    {
        if (!permitted) return SyncRecoveryResult::refused;
        bool driven{}, synced{};
        if (!graph.Get("bAnimationDriven", driven) || !graph.Get("bIsSynced", synced))
            return SyncRecoveryResult::unavailable;
        if (driven) return SyncRecoveryResult::refused;
        if (!synced) return SyncRecoveryResult::alreadyClear;
        if (!graph.Set("bIsSynced", false)) return SyncRecoveryResult::writeFailed;
        if (!graph.Get("bIsSynced", synced)) return SyncRecoveryResult::unavailable;
        return synced ? SyncRecoveryResult::notCleared : SyncRecoveryResult::cleared;
    }
}
