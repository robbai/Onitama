#ifndef ONITAMA_LAYER_H
#define ONITAMA_LAYER_H

#include <cmath>
#include <vector>

using std::vector;

template<int in_dim, int out_dim>
class Layer {
 public:
    static constexpr int in_size = in_dim;
    static constexpr int out_size = out_dim;

    float *pass(const float *input, float *output);
};

template<int in_dim, int out_dim>
class LinearLayer : public Layer<in_dim, out_dim> {
 public:
    float bias[out_dim];
    float weight[in_dim * out_dim];

    float *pass(const float *input, float *output) {
        for (int i = 0; i < this->out_size; ++i)
            output[i] = this->bias[i];

        for (int i = 0; i < this->in_size; ++i) {
            for (int j = 0; j < this->out_size; ++j)
                output[j] += input[i] * this->weight[i + j * this->in_size];
        }

        return output + this->out_size;
    }
};

template<int in_dim, int out_dim>
class SparseLinearLayer : public LinearLayer<in_dim, out_dim> {
 public:
    float *pass(const vector<int> *input, float *output) {
        for (int j = 0; j < this->out_size; ++j) {
            output[j] = this->bias[j];
            for (int i : *input)
                output[j] += this->weight[i + j * this->in_size];
        }

        return output + this->out_size;
    }
};

template<int dims>
class CReluLayer : Layer<dims, dims> {
 public:
    float *pass(const float *input, float *output) {
        for (int i = 0; i < this->in_size; ++i)
            output[i] = fmin(fmax(input[i], 0), 1);

        return output + this->out_size;
    }
};


#endif  // ONITAMA_LAYER_H
