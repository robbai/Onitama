from math import sqrt, log10
from typing import List, Tuple, Optional
from subprocess import PIPE, Popen

from game import Game
from cards import CARD_NAMES


class Match:
    def __init__(self, engine_paths: List[str]):
        self.engines: List[Popen] = [
            Popen(
                [path, "runner"], stdin=PIPE, stdout=PIPE, stderr=PIPE, encoding="UTF8"
            )
            for path in engine_paths
        ]
        self.engine_names: List[str] = [
            self.ask(i, "name").title() for i in range(len(engine_paths))
        ]
        self.score: List[int, int] = [0, 0]

    @property
    def games(self) -> int:
        return self.score[0] + self.score[1]

    def get_elo_range(self, stdevs: float = 2) -> Optional[Tuple[float, float]]:
        wins: float = self.score[0]
        losses: float = self.score[1]
        games: float = self.games
        if not wins or not losses or wins == losses:
            return None
        score: float = wins / games

        # Elo difference.
        def elo_diff(score: float) -> float:
            return -400 * log10(1 / score - 1)

        # Range.
        stdev: float = sqrt(
            (wins * (1 - score) ** 2 + losses * score ** 2) / (games - 1)
        )
        _min = score - stdevs * stdev / sqrt(games)
        _max = score + stdevs * stdev / sqrt(games)
        if _min < 0 or _max > 1:
            return None
        return elo_diff(_min), elo_diff(_max)

    @staticmethod
    def is_concordant(elo_range: Tuple[float, float]) -> bool:
        return elo_range[0] * elo_range[1] > 0

    def quit_engines(self):
        self.send("quit")

    def run_match(self, search_time: float = 0.1, draw_plies: int = 96):
        game1: Game = Game()
        game2: Game = game1.copy()
        print(
            "Cards: "
            + ", ".join(CARD_NAMES[c] + " (" + str(c) + ")" for c in game1.cards)
        )

        for i, game in enumerate((game1, game2,)):
            new_setup: str = "new " + " ".join(str(c) for c in game.cards)
            self.send(new_setup)
            print(self.engine_names[i] + "-" + self.engine_names[not i], end=": ")
            for _ in range(draw_plies):
                moving: bool = game.turn ^ i
                move: str = self.ask(moving, "get " + str(search_time))
                print(move, end=" ", flush=True)
                game.move(move)
                if game.game_over():
                    self.score[moving] += 1
                    print("1-0" if game.turn else "0-1")
                    break
                self.send("give " + move)
            else:
                self.score[0] += 0.5
                self.score[1] += 0.5
                print("1/2-1/2")

    def send(self, message: str):
        for index, engine in enumerate(self.engines):
            engine.stdin.write(message + "\n")
            engine.stdin.flush()

    def ask(self, index: int, message: str) -> Optional[str]:
        engine: Popen = self.engines[index]
        engine.stdin.write(message + "\n")
        engine.stdin.flush()
        message: str = message.split(" ")[0] + " "
        for output in iter(engine.stdout.readline, ""):
            if output.startswith(message):
                return output[len(message) : -1]
