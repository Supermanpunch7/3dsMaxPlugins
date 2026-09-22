#pragma once
#include <cmath>

// Return a state only for parallel incident parents with unanimous state.
// This is a conservative geometric hypothesis, not generation-time provenance.
inline int CrossingDirectionState(const double* direction, const double* rays,
                                  const int* states, int count) {
    double n = 0;
    for (int j=0; j<3; ++j) n += direction[j]*direction[j];
    if (!std::isfinite(n) || n <= 1.e-20) return -1;
    int result = -1;
    for (int i=0; i<count; ++i) {
        double length=0, dot=0;
        for (int j=0; j<3; ++j) {
            length += rays[3*i+j]*rays[3*i+j];
            dot += direction[j]*rays[3*i+j];
        }
        if (!std::isfinite(length) || !std::isfinite(dot) || length <= 1.e-20) continue;
        if (dot*dot < n*length*(1.0-1.e-8)) continue;
        if (states[i] != 0 && states[i] != 1) return -1;
        if (result >= 0 && result != states[i]) return -1;
        result = states[i];
    }
    return result;
}