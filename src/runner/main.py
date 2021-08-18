from sys import argv
from random import shuffle
from typing import List, Tuple, Optional
from itertools import permutations

from cards import NUM_CARDS
from match import Match
from history import write_result


def cards_gen():
    all_cards: List[Tuple[int]] = list(permutations(range(NUM_CARDS), 5))
    shuffle(all_cards)
    for cards in all_cards:
        if cards[0] < cards[1] and cards[2] < cards[3]:
            yield list(cards)


def main():
    if len(argv) != 3:
        print("Incorrect arguments: " + str(argv[1:]))
        return

    engine_paths: List[str] = argv[1:]
    match: Match = Match(engine_paths)

    # Run matches.
    for cards in cards_gen():
        try:
            match.run_match(cards)
        except KeyboardInterrupt:
            print("\nKeyboard interrupt.")
            break
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
    write_result(engine_paths, match.move_time, match.score)


if __name__ == "__main__":
    main()
