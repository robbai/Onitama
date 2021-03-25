from typing import Dict, List, Tuple

# https://uploads.johnhqld.com/uploads/20180926171936/Onitama-Cards-1024x576.jpg

CARD_NAMES: List[str] = [
    "Rabbit",
    "Monkey",
    "Boar",
    "Goose",
    "Cobra",
    "Crab",
    "Horse",
    "Dragon",
    "Rooster",
    "Crane",
    "Elephant",
    "Mantis",
    "Tiger",
    "Frog",
    "Ox",
    "Eel",
]

CARD_MOVES: List[Tuple[int, int]] = [
    [(-1, -1), (1, 1), (2, 0)],
    [(-1, -1), (-1, 1), (1, -1), (1, 1)],
    [(1, 0), (-1, 0), (0, 1)],
    [(-1, 0), (-1, 1), (1, 0), (1, -1)],
    [(-1, 0), (1, 1), (1, -1)],
    [(0, 1), (-2, 0), (2, 0)],
    [(0, 1), (-1, 0), (0, -1)],
    [(-1, -1), (1, -1), (-2, 1), (2, 1)],
    [(-1, 0), (1, 0), (1, 1), (-1, -1)],
    [(-1, -1), (1, -1), (0, 1)],
    [(-1, 0), (1, 0), (-1, 1), (1, 1)],
    [(-1, 1), (1, 1), (0, -1)],
    [(0, 2), (0, -1)],
    [(-1, 1), (1, -1), (-2, 0)],
    [(0, 1), (0, -1), (1, 0)],
    [(1, 0), (-1, 1), (-1, -1)],
]

NUM_CARDS: int = len(CARD_NAMES)
assert NUM_CARDS == len(CARD_MOVES)

CARD_INDEXES: Dict[str, int] = {CARD_NAMES[index]: index for index in range(NUM_CARDS)}
