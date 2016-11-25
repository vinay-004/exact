#ifndef CNN_NEAT_NODE_H
#define CNN_NEAT_NODE_H

#include <fstream>
using std::ofstream;
using std::ifstream;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::istream;

#include <random>
using std::mt19937;


#include <string>
using std::string;

#include <vector>
using std::vector;


#include "image_tools/image_set.hxx"

#define RELU_MIN 0
#define RELU_MIN_LEAK 0.005

#define RELU_MAX 5
#define RELU_MAX_LEAK 0.00

#define INPUT_NODE 0
#define HIDDEN_NODE 1
#define OUTPUT_NODE 2
#define SOFTMAX_NODE 3


class CNN_Node {
    private:
        int innovation_number;
        double depth;

        int size_x, size_y;

        //int stride;
        //int max_pool;
        //int output_size_x, output_size_y;

        double **values;
        double **errors;
        double **bias;
        double **bias_velocity;

        int type;

        int total_inputs;
        int inputs_fired;

    public:
        CNN_Node();

        CNN_Node(int _innovation_number, double _depth, int _size_x, int _size_y, int type);

        CNN_Node* copy() const;

        int get_size_x() const;
        int get_size_y() const;

        int get_innovation_number() const;

        double get_depth() const;

        bool is_fixed() const;
        bool is_hidden() const;
        bool is_input() const;
        bool is_output() const;
        bool is_softmax() const;

        void initialize_bias(mt19937 &generator);
        void reinitialize_bias(mt19937 &generator);
        
        void propagate_bias(double mu);

        void set_values(const Image &image, int rows, int cols);
        double** get_values();

        double get_error(int y, int x);
        void set_error(int y, int x, double value);
        double** get_errors();

        void print(ostream &out);

        void reset();

        double get_value(int y, int x);

        double set_value(int y, int x, double value);

        bool modify_size_x(int change, mt19937 &generator);
        bool modify_size_y(int change, mt19937 &generator);

        void add_input();
        void disable_input();
        int get_number_inputs() const;

        void input_fired();

        friend ostream &operator<<(ostream &os, const CNN_Node* node);
        friend istream &operator>>(istream &is, CNN_Node* node);
};

struct sort_CNN_Nodes_by_depth {
    bool operator()(const CNN_Node *n1, const CNN_Node *n2) {
        return n1->get_depth() < n2->get_depth();
    }
};

#endif