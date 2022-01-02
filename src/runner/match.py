import logging
from math import erf, sqrt, log10
from typing import Set, List, Tuple, Optional
from subprocess import PIPE, Popen

from game import Game


class Match:
    def __init__(self, engine_paths: List[str], move_time: float):
        self.engines: List[Popen] = [
            Popen(
                [path, "runner"], stdin=PIPE, stdout=PIPE, stderr=PIPE, encoding="UTF8"
            )
            for path in engine_paths
        ]
        self.engine_names: List[str] = [
            self.ask(i, "name").title() for i in range(len(engine_paths))
        ]
        self.move_time: float = move_time

    @staticmethod
    def get_elo_range(
        score: List[float], stdevs: float = 2
    ) -> Optional[Tuple[float, float]]:
        wins: float = score[0]
        losses: float = score[1]
        games: float = score[0] + score[1]
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
    def get_los(dec_pair_score: Tuple[float, float]) -> bool:
        return (
            0.5
            * (
                1
                + erf(
                    (dec_pair_score[0] - dec_pair_score[1])
                    / sqrt(2 * sum(dec_pair_score))
                )
            )
            if any(dec_pair_score)
            else 0.5
        )

    def quit_engines(self):
        for engine in self.engines:
            engine.stdin.write("quit\n")

    def run_match(self, cards: List[int] = None) -> List[float]:
        score: List[float] = [0, 0]

        game1: Game = Game(cards)
        game2: Game = game1.copy()

        for i, game in enumerate(
            (
                game1,
                game2,
            )
        ):
            new_setup: str = "new " + " ".join(str(c) for c in game.cards)
            self.send(new_setup)
            # if not i:
            #     self.send("tb 2")
            history: Set[Game] = set()
            while True:
                moving: bool = game.turn ^ i
                move: str = self.ask(moving, "get " + str(self.move_time))
                if not move or not game.move(move):
                    score[not moving] += 1
                    logging.warning("Illegal move")
                    break
                if game.game_over():
                    score[moving] += 1
                    break
                if hash(game) in history:
                    score[0] += 0.5
                    score[1] += 0.5
                    break
                history.add(hash(game))
                self.send("give " + move)

        return score

    def send(self, message: str):
        for engine in self.engines:
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
