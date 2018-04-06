#ifndef GENERATE_LOOP_GT_SYMMETRIC_MATRIX_H
#define GENERATE_LOOP_GT_SYMMETRIC_MATRIX_H

#include <vector>

template<typename T>
struct SymmetricMatrix {
    SymmetricMatrix();
    void create(int rows, T value = {});
    int rows() const { return rows_; }
    int cols() const { return rows_; }
    void clear();
    int stride(int row) const;

    void set(int row, int col, T value);
    T get(int row, int col) const;

    typename std::vector<T>::const_iterator begin(int row) const;
    typename std::vector<T>::const_iterator end(int row) const;

 private:
    // stored as an upper square matrix
    size_t cell(int row, int col) const;
    int rows_;
    std::vector<T> data_;
};

template<typename T>
SymmetricMatrix<T>::SymmetricMatrix() : rows_(0) {}

template<typename T>
void SymmetricMatrix<T>::create(int rows, T value) {
    if (rows > 0) {
        // sum_{i = 1 .. rows} i
        size_t N = (static_cast<size_t>(rows) * (static_cast<size_t>(rows) + 1)) / 2;
        data_.clear();
        data_.resize(N, value);
        rows_ = rows;
    } else {
        rows_ = 0;
        data_.clear();
    }
}

template<typename T>
void SymmetricMatrix<T>::clear() {
    rows_ = 0;
    data_.clear();
}

template<typename T>
int SymmetricMatrix<T>::stride(int row) const {
    return (row >= 0 && row < rows_ ? rows_ - row : 0);
}

template<typename T>
size_t SymmetricMatrix<T>::cell(int row, int col) const {
    if (col < row) std::swap(col, row);
    // (col - row) + sum_{i in 0..row-1} (rows - i)
    return ((2 * static_cast<size_t>(rows_) - row - 1) * row) / 2 + col;
}

template<typename T>
void SymmetricMatrix<T>::set(int row, int col, T value) {
    if (row >= 0 && row < rows_ && col >= 0 && col < rows_) {
        data_[cell(row, col)] = value;
    }
}

template<typename T>
T SymmetricMatrix<T>::get(int row, int col) const {
    if (row >= 0 && row < rows_ && col >= 0 && col < rows_) {
        return data_[cell(row, col)];
    } else {
        return {};
    }
}

template<typename T>
typename std::vector<T>::const_iterator SymmetricMatrix<T>::begin(int row) const {
    return (row >= 0 && row < rows_ ? data_.begin() + cell(row, row)
                                    : data_.end());
}

template<typename T>
typename std::vector<T>::const_iterator SymmetricMatrix<T>::end(int row) const {
    return (row >= 0 && row < rows_ ? data_.begin() + cell(row, row) + (rows_ - row)
                                    : data_.end());
}

#endif  // GENERATE_LOOP_GT_SYMMETRIC_MATRIX_H
