from typing import List

import torch
from const import (
    CARDS_NUM,
    PLAYERS_NUM,
    GAME_CARDS_NUM,
    PIECE_TYPES_NUM,
    PLAYER_PIECES_NUM,
    CARD_POSITIONS_NUM,
)
from model import NUM_FEATURES
from torch.utils.data import Dataset


def to_feature(square, master, player, card, card_pos) -> int:
    return (
        ((square * PIECE_TYPES_NUM + master) * PLAYERS_NUM + player) * CARDS_NUM + card
    ) * CARD_POSITIONS_NUM + card_pos


def to_features(input: List[int]) -> int:
    features: List[int] = []
    for i_card in range(GAME_CARDS_NUM):
        for i_square in range(GAME_CARDS_NUM, len(input)):
            if input[i_square] == -1:
                continue
            student: bool = i_square % PLAYER_PIECES_NUM
            player: bool = i_square >= PLAYER_PIECES_NUM * 2
            features.append(
                to_feature(
                    input[i_square], not student, player, input[i_card], i_card // 2
                )
            )
    return features


class PositionDataset(Dataset):
    def __init__(self, file_name: str, lambda_: float = 1):
        self.lines: List[str] = open(file_name, "r").readlines()[:-1]
        self.lambda_: float = lambda_

    def __len__(self):
        return len(self.lines)

    def __getitem__(self, idx):
        line: List[str] = self.lines[idx].split()

        # Input features.
        i: List[int] = to_features([int(s) for s in line[:-2]])
        tensor = torch.sparse_coo_tensor([i], [1.0] * len(i), (NUM_FEATURES,))

        # Output value.
        value: float = float(line[-1])
        value += self.lambda_ * (float(line[-2]) - value)

        return tensor, torch.tensor([value]).float()
