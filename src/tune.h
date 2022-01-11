#ifndef ONITAMA_TUNE_H
#define ONITAMA_TUNE_H


class Param {
 public:
    const float minimum, maximum, start;
    float value;
    explicit Param(float min = 0, float max = 2, float start = 1)
        : minimum(min), maximum(max), start(start) {
        this->value = start;
    }
};


int run_tune(const float magnitude, const float initial_delta, const int iterations,
             const bool reinit_search = false);


#endif  // ONITAMA_TUNE_H
