import logging
from math import atanh
from time import time
from typing import List, Optional
from os.path import exists
from datetime import timedelta

import torch
import torch.optim as optim
import torch.optim.lr_scheduler as lr_scheduler
from model import NNUE
from torch import nn
from pos_set import PositionDataset
from quicktracer import trace
from torch.utils.data import DataLoader

EPOCHS: Optional[int] = None
BATCH_SIZE: int = 4096

MODEL_FILE: str = "models/model3.pth"
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

    dataset: PositionDataset = PositionDataset(DATASET_FILE, lambda_=0.9)
    dataloader: DataLoader = DataLoader(dataset, batch_size=BATCH_SIZE, shuffle=True)

    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=0.005)

    # from torch_lr_finder import LRFinder
    # lr_finder = LRFinder(model, optimizer, criterion, device=device)
    # lr_finder.range_test(dataloader, start_lr=0.00001, end_lr=100)
    # lr_finder.plot()
    # lr_finder.reset()
    # exit()

    # scheduler = lr_scheduler.CyclicLR(
    #     optimizer, 0.002, 1, step_size_up=64 * BATCH_SIZE
    # )

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
                batch_losses.append(loss.item())
                loss.backward()

                # trace_sample_size: int = 100
                # if len(batch_losses) >= trace_sample_size:
                #     trace_value: float = sum(batch_losses[-trace_sample_size:]) / min(
                #         trace_sample_size, len(batch_losses)
                #     )
                #     trace(trace_value)

                optimizer.step()
        except KeyboardInterrupt:
            logging.info("Interrupted by keyboard.")
            break
        # scheduler.step()

        # Logging.
        loss: float = sum(batch_losses[epoch_start_batch:]) / (
            len(batch_losses) - epoch_start_batch
        )
        error: float = 250 * atanh(loss ** 0.5)
        time_display: float = (time() - start_time) * (
            (EPOCHS - epoch) / epoch if EPOCHS else 1
        )
        logging.info(
            "Epoch: {}, Loss: {}, Error: {}, LR: {}, Time: {}".format(
                epoch,
                round(loss, 6),
                round(error, 2),
                round(optimizer.param_groups[0]["lr"], 8),
                timedelta(seconds=int(time_display)),
            )
        )

        batch_losses.clear()

    logging.info("Finished training.")
    logging.info("Saving at: " + MODEL_FILE)
    torch.save(model.state_dict(), MODEL_FILE)
