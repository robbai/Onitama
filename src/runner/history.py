import hashlib
from typing import List

HISTORY_FILE: str = "history.txt"
with open(HISTORY_FILE, "a") as file:
    pass  # Create file.


def sha256(file_path: str) -> str:
    """
    https://stackoverflow.com/a/44873382
    """
    sha = hashlib.sha256()
    mv = memoryview(bytearray(128 * 1024))
    with open(file_path, "rb", buffering=0) as file:
        for n in iter(lambda: file.readinto(mv), 0):
            sha.update(mv[:n])
    return sha.hexdigest()


def load_previous_result(engine_paths: List[str], move_time: float) -> List[float]:
    hashes: List[str] = [sha256(engine_path) for engine_path in engine_paths]
    for line in open(HISTORY_FILE, "r").readlines():
        tokens: List[str] = line.strip().split(",")
        if float(tokens[2]) != move_time:
            continue
        if tokens[0] == hashes[0] and tokens[1] == hashes[1]:
            return [float(tokens[3]), float(tokens[4])]
        elif tokens[0] == hashes[1] and tokens[1] == hashes[0]:
            return [float(tokens[4]), float(tokens[3])]
    return [0, 0]


def write_result(
    engine_paths: List[str], move_time: float, score: List[float]
) -> List[float]:
    hashes: List[str] = [sha256(engine_path) for engine_path in engine_paths]
    new_line: str = "{},{},{},{},{}\n".format(*hashes, move_time, *score)
    lines: List[str] = open(HISTORY_FILE, "r").readlines()
    for i, line in enumerate(lines):
        tokens: List[str] = line.strip().split(",")
        if all(sha in tokens[:2] for sha in hashes) and float(tokens[2]) == move_time:
            lines[i] = new_line
            break
    else:
        lines.append(new_line)
    open(HISTORY_FILE, "w").writelines(lines)
