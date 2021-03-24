from random import sample
from typing import Dict, List

from cards import NUM_CARDS, CARD_INDEXES

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
    def __init__(self, cards: List[int] = None):
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
        return their_master not in self.pieces

    def move(self, move: str):
        card: str = move[:-5]
        from_sq: str = SQUARES[move[-4:-2]]
        to_sq: str = SQUARES[move[-2:]]
        self.pieces[to_sq] = self.pieces[from_sq]
        self.pieces[from_sq] = 0
        index: int = self.cards.index(CARD_INDEXES[card])
        self.cards[index], self.cards[-1] = self.cards[-1], self.cards[index]
        self.turn = not self.turn

    def copy(self) -> "Game":
        copy: "Game" = Game()
        copy.cards = self.cards[:]
        return copy
