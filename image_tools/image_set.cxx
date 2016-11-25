#include <fstream>
using std::ifstream;

#include <iomanip>
using std::setw;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::ios;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include "image_set.hxx"

Image::Image(ifstream &infile, int size, int _rows, int _cols, int _classification) {
    rows = _rows;
    cols = _cols;
    classification = _classification;

    pixels = new double*[cols];
    for (uint32_t i = 0; i < cols; i++) {
        pixels[i] = new double[rows];
    }

    char* c_pixels = new char[size];

    infile.read( c_pixels, sizeof(char) * size);

    int current = 0;
    for (uint32_t y = 0; y < cols; y++) {
        for (uint32_t x = 0; x < rows; x++) {
            pixels[y][x] = (int)(uint8_t)c_pixels[current];
            current++;
        }
    }

    delete [] c_pixels;
}

double Image::get_pixel(int x, int y) const {
    return pixels[y][x];
}

int Image::get_classification() const {
    return classification;
}

int Image::get_rows() const {
    return rows;
}

int Image::get_cols() const {
    return cols;
}


void Image::print(ostream &out) {
    out << "Image Class: " << classification << endl;
    for (uint32_t y = 0; y < cols; y++) {
        for (uint32_t x = 0; x < rows; x++) {
            out << setw(7) << pixels[y][x];
        }
        out << endl;
    }
}

Images::Images(string binary_filename) {
    ifstream infile(binary_filename.c_str(), ios::in | ios::binary);

    if (!infile.is_open()) {
        cerr << "Could not open '" << binary_filename << "' for reading." << endl;
        return;
    }

    int initial_vals[3];
    infile.read( (char*)&initial_vals, sizeof(initial_vals) );

    number_classes = initial_vals[0];
    int rowscols = initial_vals[1];
    rows = rowscols;
    cols = rowscols;    //square for now

    vals_per_pixel = initial_vals[2];

    cout << "number_classes: " << number_classes << endl;
    cout << "rowscols: " << rowscols << endl;
    cout << "vals_per_pixel: " << vals_per_pixel << endl;

    class_sizes = vector<int>(number_classes, 0);
    infile.read( (char*)&class_sizes[0], sizeof(int) * number_classes );

    int image_size = rowscols * rowscols * vals_per_pixel;

    for (int i = 0; i < number_classes; i++) {
        cout << "reading image set with " << class_sizes[i] << " images." << endl;

        for (uint32_t j = 0; j < class_sizes[i]; j++) {
            images.push_back(Image(infile, image_size, rowscols, rowscols, i));
        }
    }
    number_images = images.size();

    infile.close();

    cout << "image_size: " << rowscols << "x" << rowscols << " = " << image_size << endl;

    cout << "read " << images.size() << " images." << endl;
    for (int i = 0; i < class_sizes.size(); i++) {
        cout << "    class " << setw(4) << i << ": " << class_sizes[i] << endl;
    }

    /*
       for (int i = 0; i < images.size(); i++) {
       images[i].print(cout);
       }
       */

}

int Images::get_class_size(int i) const {
    return class_sizes[i];
}

int Images::get_number_classes() const {
    return number_classes;
}

int Images::get_number_images() const {
    return number_images;
}

int Images::get_image_rows() const {
    return rows;
}

int Images::get_image_cols() const {
    return cols;
}

const Image& Images::get_image(int image) const {
    return images[image];
}