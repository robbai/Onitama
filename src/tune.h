#ifndef ONITAMA_TUNE_H
#define ONITAMA_TUNE_H


class Param {
 public:
    const float minimum, maximum, start;
    float value;
    explicit Param(float min, float max, float start)
        : minimum(min), maximum(max), start(start) {
        this->value = start;
    }
};


int run_tune(const float magnitude, const float initial_delta, const float A = 100,
             const float c = 1);


#endif  // ONITAMA_TUNE_H
