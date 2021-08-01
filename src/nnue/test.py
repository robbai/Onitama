from math import atanh
from typing import List

import torch
from model import NNUE, NUM_FEATURES
from train import MODEL_FILE
from pos_set import to_features

if __name__ == "__main__":
    model: NNUE = NNUE()
    model.load_state_dict(torch.load(MODEL_FILE))

    i: List[int] = to_features([14, 2, 6, 10, 5, 2, 0, 1, 3, 4, 22, 20, 21, 23, 24])
    v: List[float] = [1.0] * len(i)
    tensor = torch.sparse_coo_tensor([i], v, (NUM_FEATURES,))
    result = model(tensor).item()
    print(result)
    # print(round(100 * atanh(result), 1))
    print(round(2500 * result, 1))
