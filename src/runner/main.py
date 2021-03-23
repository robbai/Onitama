from sys import argv
from typing import Tuple, Optional

from match import Match


def main():
    if len(argv) != 3:
        print("Incorrect arguments: " + str(argv[1:]))
        return

    match: Match = Match(argv[1:])

    # Run games.
    while True:
        match.run_match()
        print(
            match.engine_names[0]
            + " "
            + str(match.score[0])
            + "-"
            + str(match.score[1])
            + " "
            + match.engine_names[1]
        )
        elo_range: Optional[Tuple[float, float]] = match.get_elo_range()
        if elo_range:
            elo: float = sum(elo_range) / len(elo_range)
            range: float = abs(elo_range[0] - elo_range[1]) / 2
            print(str(round(elo, 2)) + " ± " + str(round(range, 2)))
            if Match.is_concordant(elo_range):
                break
        print()

    match.quit_engines()


if __name__ == "__main__":
    main()
