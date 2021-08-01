from typing import Tuple

import torch
from const import (
    CARDS_NUM,
    PLAYERS_NUM,
    SQUARES_NUM,
    PIECE_TYPES_NUM,
    CARD_POSITIONS_NUM,
)
from torch import nn

# https://github.com/glinscott/nnue-pytorch/blob/master/docs/nnue.md#training-a-net-with-pytorch

NUM_FEATURES: int = (
    SQUARES_NUM * PIECE_TYPES_NUM * PLAYERS_NUM * CARDS_NUM * CARD_POSITIONS_NUM
)

# LAYER_SIZE: Tuple = (NUM_FEATURES, 64, 32)
LAYER_SIZE: Tuple = (NUM_FEATURES, 32)


class NNUE(nn.Module):
    def __init__(self):
        super(NNUE, self).__init__()
        # self.l0 = nn.Linear(LAYER_SIZE[0], LAYER_SIZE[1])
        # self.l1 = nn.Linear(LAYER_SIZE[1], LAYER_SIZE[2])
        # self.l2 = nn.Linear(LAYER_SIZE[2], 1)

        self.l0 = nn.Linear(LAYER_SIZE[0], LAYER_SIZE[1])
        self.l1 = nn.Linear(LAYER_SIZE[1], 1)

    def forward(self, x):
        # for layer in (
        #     self.l0,
        #     self.l1,
        # ):
        #     x = layer(x)
        #     x = torch.clamp(x, 0.0, 1.0)
        # x = self.l2(x)
        # return x

        x = self.l0(x)
        x = torch.clamp(x, 0.0, 1.0)
        x = self.l1(x)
        return x
