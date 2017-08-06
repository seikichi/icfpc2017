#pragma once

#include <functional>

void DoSetup(OfflineClientProtocol* protocol);

void RunAiWithTimeoutAndDie(
    OfflineClientProtocol* protocol,
    std::function<Move (const Game& game, const MapState& map_state, int my_rounds)> decide,
    int timeout_millis);

void DoScoring(OfflineClientProtocol* protocol);
