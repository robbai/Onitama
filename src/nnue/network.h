#ifndef ONITAMA_NETWORK_H
#define ONITAMA_NETWORK_H

#include "layer.h"

constexpr int LAYER_SIZE[2] = {4800, 32};

inline SparseLinearLayer<LAYER_SIZE[0], LAYER_SIZE[1]> L0;
inline LinearLayer<LAYER_SIZE[1], 1> L1;

inline CReluLayer<LAYER_SIZE[1]> CR0;

void init_network();

#endif  // ONITAMA_NETWORK_H
