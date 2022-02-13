import logging
import multiprocessing as mp
from sys import argv
from random import shuffle
from typing import List, Tuple, Optional
from itertools import permutations

from cards import NUM_CARDS
from match import Match
from history import write_result, load_previous_result

MOVE_TIME: float = 0.2

logging.basicConfig(
    level=logging.INFO, format="%(asctime)s -> %(message)s", datefmt="%I:%M:%S"
)


def cards_gen():
    all_cards: List[Tuple[int]] = list(permutations(range(NUM_CARDS), 5))
    shuffle(all_cards)
    for cards in all_cards:
        if cards[0] < cards[1] and cards[2] < cards[3]:
            yield list(cards)


def process(engine_paths, queue: mp.Queue, score_queue: mp.Queue):
    match: Match = Match(engine_paths, MOVE_TIME)
    try:
        for cards in iter(queue.get, None):
            score_queue.put(match.run_match(cards))
    except KeyboardInterrupt:
        pass
    match.quit_engines()


def main():
    if not (3 <= len(argv) <= 4):
        logging.error("Incorrect arguments: " + str(argv[1:]))
        return

    # Setup pool.
    engine_paths: List[str] = argv[1:3]
    try:
        cores: int = int(argv[3])
    except (IndexError, ValueError):
        cores: int = 1
    queue: mp.Queue = mp.Queue(maxsize=cores)
    score_queue: mp.Queue = mp.Queue()
    pool: mp.Pool = mp.Pool(cores, process, (engine_paths, queue, score_queue))

    # Run matches.
    score, dec_pair_score = load_previous_result(engine_paths, MOVE_TIME)
    try:
        for cards in cards_gen():
            queue.put(cards)

            if not score_queue.empty():
                new_score: List[float] = score_queue.get()
                score[0] += new_score[0]
                score[1] += new_score[1]
                if new_score[0] > new_score[1]:
                    dec_pair_score[0] += 1
                elif new_score[0] < new_score[1]:
                    dec_pair_score[1] += 1

                los: float = Match.get_los(dec_pair_score)
                info: str = f"{score}, {dec_pair_score}: {los:.2%}"
                elo_range: Optional[Tuple[float, float]] = Match.get_elo_range(score)
                if elo_range:
                    elo: float = sum(elo_range) / len(elo_range)
                    total_range: float = abs(elo_range[0] - elo_range[1]) / 2
                    logging.info(info + f" ({elo:.2f} ± {total_range:.2f})")
                else:
                    logging.info(info)

                if sum(score) >= 50 and max(los, 1 - los) >= 0.95:
                    logging.warning("Finished, shutting down processes")
                    break

        [queue.put(None) for _ in range(cores)]
        pool.close()
        pool.join()

    # Quit.
    except KeyboardInterrupt:
        logging.warning("Keyboard interrupted")
    finally:
        write_result(engine_paths, score, dec_pair_score, MOVE_TIME)


if __name__ == "__main__":
    main()
