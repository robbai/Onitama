import torch
from model import NNUE
from train import MODEL_FILE

if __name__ == "__main__":
    model: NNUE = NNUE()
    model.load_state_dict(torch.load(MODEL_FILE))

    file = open(MODEL_FILE[:-3] + "txt", "w")

    for i, layer in enumerate(
        (
            model.l0,
            model.l1,
        )
    ):
        for attribute in (
            "weight",
            "bias",
        ):
            list = getattr(layer, attribute).data.flatten().tolist()
            name: str = "l" + str(i) + "_" + attribute
            data: str = (
                "    auto "
                + name
                + " = "
                + str(list).replace("[", "{").replace("]", "}")
                + ";\n"
            )
            data += (
                "    std::copy(std::begin("
                + name
                + "), std::end("
                + name
                + "), L"
                + str(i)
                + "."
                + attribute
                + ")"
                + ";\n"
            )
            file.write(data)
