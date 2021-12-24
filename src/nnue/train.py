import logging
from time import time
from random import shuffle
from typing import List, Optional
from os.path import exists
from datetime import timedelta

import torch
import torch.optim as optim
from model import NNUE
from torch import nn
from pos_set import PositionDataset
from torch.utils.data import DataLoader

EPOCHS: Optional[int] = None
BATCH_SIZE: int = 4096

MODEL_FILE: str = "models/model6.pth"
LAST_MODEL_FILE: Optional[str] = None
DATASET_FILE: str = "../../cmake-build-release/example.txt"

logging.basicConfig(
    format="%(asctime)s: %(message)s",
    datefmt="%H:%M:%S",
    level=logging.DEBUG,
)

if __name__ == "__main__":
    device: str = "cuda" if torch.cuda.is_available() else "cpu"
    logging.info("Device: " + device)

    # Initialise model.
    model: NNUE = NNUE().to(device)
    if LAST_MODEL_FILE and exists(LAST_MODEL_FILE):
        logging.info("Starting from previous model.")
        logging.info("Loading from: " + LAST_MODEL_FILE)
        model.load_state_dict(torch.load(LAST_MODEL_FILE))
    else:
        logging.info("Starting from scratch.")
    logging.info(model)

    test_frac: float = 0.05
    datalines: List[str] = open(DATASET_FILE, "r").readlines()[:-1]
    shuffle(datalines)
    dataset: PositionDataset = PositionDataset(
        datalines[: -int(len(datalines) * test_frac)], lambda_=0.9
    )
    testset: PositionDataset = PositionDataset(
        datalines[-int(len(datalines) * test_frac) :], lambda_=0.9
    )
    dataloader: DataLoader = DataLoader(dataset, batch_size=BATCH_SIZE)
    testloader: DataLoader = DataLoader(testset, batch_size=BATCH_SIZE)

    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=0.003)

    # from torch_lr_finder import LRFinder
    # lr_finder = LRFinder(model, optimizer, criterion, device=device)
    # lr_finder.range_test(dataloader, start_lr=0.00001, end_lr=100)
    # lr_finder.plot()
    # lr_finder.reset()
    # exit()

    # Epoch loop.
    start_time: float = time()
    batch_losses: List[float] = []
    for epoch in range(1, EPOCHS + 1 if EPOCHS else 10 ** 10):
        epoch_start_batch: int = len(batch_losses)
        if not EPOCHS:
            start_time = time()

        # Batch loop.
        try:
            for in_batch, out_batch in iter(dataloader):
                in_batch, out_batch = in_batch.to(device), out_batch.to(device)

                optimizer.zero_grad()

                # Forward.
                out = model(in_batch)

                # Backward.
                loss = criterion(out, out_batch)
                loss.backward()

                optimizer.step()

            # Test.
            for in_batch, out_batch in iter(testloader):
                in_batch, out_batch = in_batch.to(device), out_batch.to(device)

                optimizer.zero_grad()

                # Forward.
                out = model(in_batch)

                # Backward.
                loss = criterion(out, out_batch)
                batch_losses.append(loss.item())
        except KeyboardInterrupt:
            logging.info("Interrupted by keyboard.")
            break

        # Logging.
        loss: float = sum(batch_losses[epoch_start_batch:]) / (
            len(batch_losses) - epoch_start_batch
        )
        error: float = (890 * loss ** 1.5 + 859 * loss ** 0.5) / 10
        lr: float = optimizer.param_groups[0]["lr"]
        time_display: float = (time() - start_time) * (
            (EPOCHS - epoch) / epoch if EPOCHS else 1
        )
        time_str: str = str(timedelta(seconds=int(time_display)))
        logging.info(
            f"Epoch: {epoch:>3}, Loss: {loss:>8.6f}, Error: {error:>6.2f}, LR: {lr:>8.6f}, Time: {time_str}"
        )

        batch_losses.clear()

    logging.info("Finished training.")
    logging.info("Saving at: " + MODEL_FILE)
    torch.save(model.state_dict(), MODEL_FILE)
