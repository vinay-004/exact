#include <fstream>
using std::ofstream;
using std::ifstream;
using std::ios;

#include <iomanip>
using std::setw;
using std::setprecision;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::istream;

#include <random>
using std::mt19937;
using std::normal_distribution;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include "image_tools/image_set.hxx"
#include "cnn_edge.hxx"
#include "cnn_node.hxx"

CNN_Edge::CNN_Edge() {
    innovation_number = -1;

    input_node_innovation_number = -1;
    output_node_innovation_number = -1;

    input_node = NULL;
    output_node = NULL;
}

CNN_Edge::CNN_Edge(CNN_Node *_input_node, CNN_Node *_output_node, bool _fixed, int _innovation_number) {
    fixed = _fixed;
    innovation_number = _innovation_number;
    disabled = false;
    reverse_filter_x = false;
    reverse_filter_y = false;

    input_node = _input_node;
    output_node = _output_node;

    input_node_innovation_number = input_node->get_innovation_number();
    output_node_innovation_number = output_node->get_innovation_number();

    if (!disabled) output_node->add_input();

    if (output_node->get_size_x() <= input_node->get_size_x()) {
        filter_x = (input_node->get_size_x() - output_node->get_size_x()) + 1;
    } else {
        reverse_filter_x = true;
        filter_x = (output_node->get_size_x() - input_node->get_size_x()) + 1;
    }

    if (output_node->get_size_y() <= input_node->get_size_y()) {
        filter_y = (input_node->get_size_y() - output_node->get_size_y()) + 1;
    } else {
        reverse_filter_y = true;
        filter_y = (output_node->get_size_y() - input_node->get_size_y()) + 1;
    }

    cout << "\t\tcreated edge " << innovation_number << " (node " << input_node_innovation_number << " to " << output_node_innovation_number << ") with filter_x: " << filter_x << " (input: " << input_node->get_size_x() << ", output: " << output_node->get_size_x() << ") and filter_y: " << filter_y << " (input: " << input_node->get_size_y() << ", output: " << output_node->get_size_y() << "), reverse filter: " << reverse_filter_x << ", reverse_filter_y: " << reverse_filter_y << endl;

    weights = vector< vector<double> >(filter_y, vector<double>(filter_x, 0.0));
    previous_velocity = vector< vector<double> >(filter_y, vector<double>(filter_x, 0.0));
}

void CNN_Edge::initialize_weights(mt19937 &generator) {
    if (disabled) return;

    int edge_size = filter_x * filter_y;
    if (edge_size == 1) edge_size = 10;
    normal_distribution<double> distribution(0.0, sqrt(2.0 / edge_size) );

    for (uint32_t i = 0; i < weights.size(); i++) {
        for (uint32_t j = 0; j < weights[i].size(); j++) {
            weights[i][j] = distribution(generator);
        }
    }
}

void CNN_Edge::reinitialize(mt19937 &generator) {
    filter_x = (input_node->get_size_x() - output_node->get_size_x()) + 1;
    filter_y = (input_node->get_size_y() - output_node->get_size_y()) + 1;

    weights = vector< vector<double> >(filter_y, vector<double>(filter_x, 0.0));
    previous_velocity = vector< vector<double> >(filter_y, vector<double>(filter_x, 0.0));

    initialize_weights(generator);
}

CNN_Edge* CNN_Edge::copy() const {
    CNN_Edge* copy = new CNN_Edge();

    copy->fixed = fixed;
    copy->innovation_number = innovation_number;
    copy->disabled = disabled;

    copy->input_node = input_node;
    copy->output_node = output_node;

    copy->input_node_innovation_number = input_node->get_innovation_number();
    copy->output_node_innovation_number = output_node->get_innovation_number();

    copy->filter_x = filter_x;
    copy->filter_y = filter_y;

    copy->reverse_filter_x = reverse_filter_x;
    copy->reverse_filter_y = reverse_filter_y;

    copy->weights = vector< vector<double> >(filter_y, vector<double>(filter_x, 0.0));
    copy->previous_velocity = vector< vector<double> >(filter_y, vector<double>(filter_x, 0.0));

    for (uint32_t y = 0; y < weights.size(); y++) {
        for (uint32_t x = 0; x < weights[y].size(); x++) {
            copy->weights[y][x] = weights[y][x];
            copy->previous_velocity[y][x] = previous_velocity[y][x];
        }
    }

    return copy;
}

bool CNN_Edge::set_nodes(const vector<CNN_Node*> nodes) {
    //cout << "nodes.size(): " << nodes.size() << endl;
    //cout << "setting input node: " << input_node_innovation_number << endl;
    //cout << "setting output node: " << output_node_innovation_number << endl;

    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->get_innovation_number() == input_node_innovation_number) {
            input_node = nodes[i];
        }

        if (nodes[i]->get_innovation_number() == output_node_innovation_number) {
            output_node = nodes[i];
            if (!disabled) output_node->add_input();
        }
    }

    if (input_node == NULL) {
        cerr << "ERROR! Could not find node with input node innovation number " << input_node_innovation_number << endl;
        cerr << "This should never happen!" << endl;
        exit(1);
    }

    if (output_node == NULL) {
        cerr << "ERROR! Could not find node with output node innovation number " << output_node_innovation_number << endl;
        cerr << "This should never happen!" << endl;
        exit(1);
    }

    if (output_node == input_node) {
        cerr << "ERROR! Setting nodes and output_node == input_node!" << endl;
        cerr << "input node innovation number: " << input_node_innovation_number << endl;
        cerr << "output node innovation number: " << output_node_innovation_number << endl;
        cerr << "This should never happen!" << endl;
        exit(1);
    }

    if (!is_filter_correct()) {
        return false;
    }

    return true;
}

bool CNN_Edge::is_filter_correct() const {
    cout << "\t\tchecking filter correctness on edge: " << innovation_number << endl;
    cout << "\t\t\tdisabled? " << disabled << endl;
    cout << "\t\t\treverse_filter_x? " << reverse_filter_x << ", reverse_filter_y: " << reverse_filter_y << endl;
    cout << "\t\t\tbetween node " << input_node_innovation_number << " and " << output_node_innovation_number << endl;

    bool is_correct = true;
    if (reverse_filter_x) {
        cout << "\t\t\tfilter_x: " << filter_x << ", should be: " << (output_node->get_size_x() - input_node->get_size_x()) + 1 << " (output_x: " << output_node->get_size_x() << " - input_x: " << input_node->get_size_x() << " + 1) " << endl;

        is_correct = is_correct && (filter_x == (output_node->get_size_x() - input_node->get_size_x()) + 1);
    } else {
        cout << "\t\t\tfilter_x: " << filter_x << ", should be: " << (input_node->get_size_x() - output_node->get_size_x()) + 1 << " (input_x: " << input_node->get_size_x() << " - output_x: " << output_node->get_size_x() << " + 1) " << endl;

        is_correct = is_correct && (filter_x == (input_node->get_size_x() - output_node->get_size_x()) + 1);
    }

    if (reverse_filter_y) {
        cout << "\t\t\tfilter_y: " << filter_y << ", should be: " << (output_node->get_size_y() - input_node->get_size_y()) + 1 << " (output_y: " << output_node->get_size_y() << " - input_y: " << input_node->get_size_y() << " + 1) " << endl;

        is_correct = is_correct && (filter_y == (output_node->get_size_y() - input_node->get_size_y()) + 1);
    } else {
        cout << "\t\t\tfilter_y: " << filter_y << ", should be: " << (input_node->get_size_y() - output_node->get_size_y()) + 1 << " (input_y: " << input_node->get_size_y() << " - output_y: " << output_node->get_size_y() << " + 1) " << endl;

        is_correct = is_correct && (filter_y == (input_node->get_size_y() - output_node->get_size_y()) + 1);
    }

    return is_correct;
}

void CNN_Edge::disable() {
    if (!disabled) {
        disabled = true;
        output_node->disable_input();
    }
}

bool CNN_Edge::is_disabled() const {
    return disabled;
}

int CNN_Edge::get_number_weights() const {
    return filter_x * filter_y;
}

int CNN_Edge::get_innovation_number() const {
    return innovation_number;
}

int CNN_Edge::get_input_innovation_number() const {
    return input_node_innovation_number;
}

int CNN_Edge::get_output_innovation_number() const {
    return output_node_innovation_number;
}


CNN_Node* CNN_Edge::get_input_node() {
    return input_node;
}

CNN_Node* CNN_Edge::get_output_node() {
    return output_node;
}

bool CNN_Edge::connects(int n1, int n2) const {
    return (input_node_innovation_number == n1) && (output_node_innovation_number == n2);
}

void CNN_Edge::print(ostream &out) {
    out << "CNN_Edge " << innovation_number << " of from node " << input_node->get_innovation_number() << " to node " << output_node->get_innovation_number() << " with filter x: " << filter_x << ", y: " << filter_y << endl;

    for (uint32_t i = 0; i < weights.size(); i++) {
        out << "    ";
        for (uint32_t j = 0; j < weights[i].size(); j++) {
            out << setw(9) << setprecision(3) << weights[i][j];
        }
        out << endl;
    }
}

void CNN_Edge::propagate_forward() {
    if (disabled) return;

    double **input = input_node->get_values();
    double **output = output_node->get_values();

    /*
    if (!is_filter_correct()) {
        cerr << "ERROR: filter_x != input_node->get_size_x: " << input_node->get_size_x() << " - output_node->get_size_x: " << output_node->get_size_x() << " + 1" << endl;
        exit(1);
    }
    */

    /*
    cout << "propagating forward!" << endl;
    cout << "\tinput_x: " << input_node->get_size_x() << endl;
    cout << "\tinput_y: " << input_node->get_size_y() << endl;
    cout << "\toutput_x: " << output_node->get_size_x() << endl;
    cout << "\toutput_y: " << output_node->get_size_y() << endl;
    cout << "\tfilter_x: " << filter_x << endl;
    cout << "\tfilter_y: " << filter_y << endl;
    */

    if (reverse_filter_x && reverse_filter_y) {
        for (uint32_t fy = 0; fy < filter_y; fy++) {
            for (uint32_t fx = 0; fx < filter_x; fx++) {
                double weight = weights[fy][fx];

                for (uint32_t y = 0; y < input_node->get_size_y(); y++) {
                    for (uint32_t x = 0; x < input_node->get_size_x(); x++) {
                        double value = weight * input[y][x];
                        output[y + fy][x + fx] += value;
                    }
                }
            }
        }

    } else if (reverse_filter_x) {
        for (uint32_t fy = 0; fy < filter_y; fy++) {
            for (uint32_t fx = 0; fx < filter_x; fx++) {
                double weight = weights[fy][fx];

                for (uint32_t y = 0; y < output_node->get_size_y(); y++) {
                    for (uint32_t x = 0; x < input_node->get_size_x(); x++) {
                        double value = weight * input[y + fy][x];
                        output[y][x + fx] += value;
                    }
                }
            }
        }

    } else if (reverse_filter_y) {
        for (uint32_t fy = 0; fy < filter_y; fy++) {
            for (uint32_t fx = 0; fx < filter_x; fx++) {
                double weight = weights[fy][fx];

                for (uint32_t y = 0; y < input_node->get_size_y(); y++) {
                    for (uint32_t x = 0; x < output_node->get_size_x(); x++) {
                        double value = weight * input[y][x + fx];
                        output[y + fy][x] += value;
                    }
                }
            }
        }

    } else {
        for (uint32_t fy = 0; fy < filter_y; fy++) {
            for (uint32_t fx = 0; fx < filter_x; fx++) {
                double weight = weights[fy][fx];

                for (uint32_t y = 0; y < output_node->get_size_y(); y++) {
                    for (uint32_t x = 0; x < output_node->get_size_x(); x++) {
                        double value = weight * input[y + fy][x + fx];
                        output[y][x] += value;
                    }
                }
            }
        }
    }

    output_node->input_fired();
}

void CNN_Edge::propagate_backward(double mu) {
    if (disabled) return;

    double **input = input_node->get_values();
    double **output_errors = output_node->get_errors();
    double **input_errors = input_node->get_errors();

    double weight, weight_update, error, update, dx, velocity, pv;

    if (reverse_filter_x && reverse_filter_y) {
        //cout << "reverse filter x and y!" << endl;

        for (uint32_t fy = 0; fy < filter_y; fy++) {
            for (uint32_t fx = 0; fx < filter_x; fx++) {
                weight_update = 0;
                weight = weights[fy][fx];

                for (uint32_t y = 0; y < input_node->get_size_y(); y++) {
                    for (uint32_t x = 0; x < input_node->get_size_x(); x++) {
                        error = output_errors[y + fy][x + fx];

                        update = input[y][x] * error;
                        weight_update -= update;

                        input_errors[y][x] += error * weight;
                    }
                }

                //double dx = LEARNING_RATE * (weight_update[k][l] / (filter_x * filter_y) + (weights[k][l] * WEIGHT_DECAY));
                //L2 regularization

                dx = LEARNING_RATE * (weight_update / (filter_x * filter_y) - (weight * WEIGHT_DECAY));
                //double dx = LEARNING_RATE * (weight_update[k][l] / (filter_x * filter_y));

                //no momemntum
                //weights[k][l] += dx;

                //momentum
                pv = previous_velocity[fy][fx];
                velocity = (mu * pv) - dx;
                weights[fy][fx] -= -mu * pv + (1 + mu) * velocity;
                previous_velocity[fy][fx] = velocity;
            }
        }

    } else if (reverse_filter_x) {
        //cout << "reverse filter x!" << endl;

        for (uint32_t fy = 0; fy < filter_y; fy++) {
            for (uint32_t fx = 0; fx < filter_x; fx++) {
                weight_update = 0;
                weight = weights[fy][fx];

                for (uint32_t y = 0; y < output_node->get_size_y(); y++) {
                    for (uint32_t x = 0; x < input_node->get_size_x(); x++) {
                        error = output_errors[y][x + fx];

                        update = input[y + fy][x] * error;
                        weight_update -= update;

                        input_errors[y + fy][x] += error * weight;
                    }
                }

                //double dx = LEARNING_RATE * (weight_update[k][l] / (filter_x * filter_y) + (weights[k][l] * WEIGHT_DECAY));
                //L2 regularization

                dx = LEARNING_RATE * (weight_update / (filter_x * filter_y) - (weight * WEIGHT_DECAY));
                //double dx = LEARNING_RATE * (weight_update[k][l] / (filter_x * filter_y));

                //no momemntum
                //weights[k][l] += dx;

                //momentum
                pv = previous_velocity[fy][fx];
                velocity = (mu * pv) - dx;
                weights[fy][fx] -= -mu * pv + (1 + mu) * velocity;
                previous_velocity[fy][fx] = velocity;
            }
        }

    } else if (reverse_filter_y) {
        //cout << "reverse filter y!" << endl;

        for (uint32_t fy = 0; fy < filter_y; fy++) {
            for (uint32_t fx = 0; fx < filter_x; fx++) {
                weight_update = 0;
                weight = weights[fy][fx];

                for (uint32_t y = 0; y < input_node->get_size_y(); y++) {
                    for (uint32_t x = 0; x < output_node->get_size_x(); x++) {
                        error = output_errors[y + fy][x];

                        update = input[y][x + fx] * error;
                        weight_update -= update;

                        input_errors[y][x + fx] += error * weight;
                    }
                }

                //double dx = LEARNING_RATE * (weight_update[k][l] / (filter_x * filter_y) + (weights[k][l] * WEIGHT_DECAY));
                //L2 regularization

                dx = LEARNING_RATE * (weight_update / (filter_x * filter_y) - (weight * WEIGHT_DECAY));
                //double dx = LEARNING_RATE * (weight_update[k][l] / (filter_x * filter_y));

                //no momemntum
                //weights[k][l] += dx;

                //momentum
                pv = previous_velocity[fy][fx];
                velocity = (mu * pv) - dx;
                weights[fy][fx] -= -mu * pv + (1 + mu) * velocity;
                previous_velocity[fy][fx] = velocity;
            }
        }

    } else {
        //cout << "no reverse filter!" << endl;

        for (uint32_t fy = 0; fy < filter_y; fy++) {
            for (uint32_t fx = 0; fx < filter_x; fx++) {
                weight_update = 0;
                weight = weights[fy][fx];

                for (uint32_t y = 0; y < output_node->get_size_y(); y++) {
                    for (uint32_t x = 0; x < output_node->get_size_x(); x++) {
                        error = output_errors[y][x];

                        update = input[y + fy][x + fx] * error;
                        weight_update -= update;

                        input_errors[y + fy][x + fx] += error * weight;
                    }
                }

                //double dx = LEARNING_RATE * (weight_update[k][l] / (filter_x * filter_y) + (weights[k][l] * WEIGHT_DECAY));
                //L2 regularization

                dx = LEARNING_RATE * (weight_update / (filter_x * filter_y) - (weight * WEIGHT_DECAY));
                //double dx = LEARNING_RATE * (weight_update[k][l] / (filter_x * filter_y));

                //no momemntum
                //weights[k][l] += dx;

                //momentum
                pv = previous_velocity[fy][fx];
                velocity = (mu * pv) - dx;
                weights[fy][fx] -= -mu * pv + (1 + mu) * velocity;
                previous_velocity[fy][fx] = velocity;
            }
        }
    }
}

ostream &operator<<(ostream &os, const CNN_Edge* edge) {
    os << edge->innovation_number << " ";
    os << edge->input_node_innovation_number << " " << edge->output_node_innovation_number << " ";
    os << edge->filter_x << " " << edge->filter_y << " ";
    os << edge->fixed << " ";
    os << edge->reverse_filter_x << " ";
    os << edge->reverse_filter_y << " ";
    os << edge->disabled << endl;

    for (uint32_t y = 0; y < edge->filter_y; y++) {
        for (uint32_t x = 0; x < edge->filter_x; x++) {
            if (y > 0 || x > 0) os << " ";
            os << setprecision(15) << edge->weights[y][x];
        }
    }
    os << endl;

    for (uint32_t y = 0; y < edge->filter_y; y++) {
        for (uint32_t x = 0; x < edge->filter_x; x++) {
            if (y > 0 || x > 0) os << " ";
            os << setprecision(15) << edge->previous_velocity[y][x];
        }
    }

    return os;
}

istream &operator>>(istream &is, CNN_Edge* edge) {
    is >> edge->innovation_number;
    is >> edge->input_node_innovation_number;
    is >> edge->output_node_innovation_number;
    is >> edge->filter_x;
    is >> edge->filter_y;
    is >> edge->fixed;
    is >> edge->reverse_filter_x;
    is >> edge->reverse_filter_y;
    is >> edge->disabled;

    edge->weights = vector< vector<double> >(edge->filter_y, vector<double>(edge->filter_x, 0.0));
    edge->previous_velocity = vector< vector<double> >(edge->filter_y, vector<double>(edge->filter_x, 0.0));

    for (uint32_t y = 0; y < edge->filter_y; y++) {
        for (uint32_t x = 0; x < edge->filter_x; x++) {
            is >> edge->weights[y][x];
        }
    }

    for (uint32_t y = 0; y < edge->filter_y; y++) {
        for (uint32_t x = 0; x < edge->filter_x; x++) {
            is >> edge->previous_velocity[y][x];
        }
    }

    return is;
}


