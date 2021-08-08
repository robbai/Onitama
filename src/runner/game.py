from random import sample
from typing import Dict, List, Optional

from cards import NUM_CARDS, CARD_MOVES, CARD_INDEXES

SQUARES: Dict[str, int] = {
    "a1": 0,
    "b1": 1,
    "c1": 2,
    "d1": 3,
    "e1": 4,
    "a2": 5,
    "b2": 6,
    "c2": 7,
    "d2": 8,
    "e2": 9,
    "a3": 10,
    "b3": 11,
    "c3": 12,
    "d3": 13,
    "e3": 14,
    "a4": 15,
    "b4": 16,
    "c4": 17,
    "d4": 18,
    "e4": 19,
    "a5": 20,
    "b5": 21,
    "c5": 22,
    "d5": 23,
    "e5": 24,
}


class Game:
    def __init__(self, cards: Optional[List[int]] = None):
        self.cards = cards
        if not cards:
            self.cards: List[int] = sample(range(NUM_CARDS), 5)
        self.turn: bool = False
        self.pieces: List[int] = [
            1,
            1,
            2,
            1,
            1,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            3,
            3,
            4,
            3,
            3,
        ]

    def game_over(self) -> bool:
        their_master: int = 2 if self.turn else 4
        if (
            self.pieces[22] == their_master
            if self.turn
            else self.pieces[2] == their_master
        ):
            return True
        our_master: int = 6 - their_master
        return our_master not in self.pieces

    def move(self, move: str) -> bool:
        card: str = move[:-5]
        if card not in CARD_INDEXES:
            return False
        card_index: int = CARD_INDEXES[card]

        from_sq: int = SQUARES[move[-4:-2]]
        if self.pieces[from_sq] not in (3 if self.turn else 1, 4 if self.turn else 2):
            return False

        to_sq: int = SQUARES[move[-2:]]
        if self.pieces[to_sq] in (3 if self.turn else 1, 4 if self.turn else 2):
            return False

        dir: int = -1 if self.turn else 1
        if (
            (to_sq % 5 - from_sq % 5) * dir,
            (to_sq // 5 - from_sq // 5) * dir,
        ) not in CARD_MOVES[card_index]:
            return False

        self.pieces[to_sq] = self.pieces[from_sq]
        self.pieces[from_sq] = 0
        hand_index: int = self.cards.index(card_index)
        self.cards[hand_index], self.cards[-1] = self.cards[-1], self.cards[hand_index]
        self.turn = not self.turn
        return True

    def copy(self) -> "Game":
        copy: "Game" = Game()
        copy.cards = self.cards[:]
        return copy
