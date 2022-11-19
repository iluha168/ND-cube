#ifndef MATRIX_H
#define MATRIX_H
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <array>
#include <math.h>

typedef long double floating;

template <size_t H, size_t W>
struct Matrix {
    std::array<std::array<floating, W>, H> data;

    /// generates diagonal identity matrix
    Matrix(){
        for(size_t y = 0; y < H; y++){
            data[y] = std::array<floating, W>();
            if(y < W) data[y][y] = 1;
        }
    }

    Matrix(std::array<std::array<floating, W>, H> data): data(data){}

    friend std::array<floating, H> operator* (const std::array<floating, H>& vector, const Matrix<W, H> matrix){
        std::array<floating, H> result;
        for(size_t y = 0; y < H; y++){
            floating row_result = 0;
            for(size_t i = 0; i < H; i++){
                row_result = std::fma(matrix.data[y][i], vector[i], row_result);
            }
            result[y] = row_result;
        }
        return result;
    }

    template <size_t P>
    Matrix<H, P> operator* (const Matrix<W, P>& other){
        std::array<std::array<floating, P>, H> ans;
        for(size_t row = 0; row < H; row++) // iterate through rows of this
        for(size_t col = 0; col < P; col++){// iterate through cols of other
            floating dotProduct = 0.;
            for(size_t x = 0; x < W; x++)
                dotProduct = std::fma(this->data[row][x], other.data[x][col], dotProduct);
            ans[row][col] = dotProduct;
        }
        return Matrix<H, P>(ans);
    }

    friend std::ostream& operator<< (std::ostream& os, const Matrix<H, W>& matrix){
        for(size_t y = 0; y < H; y++){
            for(size_t x = 0; x < W; x++)
                os << std::setw(20) << matrix.data[y][x] << ' ';
            os << '\n';
        }
        return os;
    }
};

template <size_t N>
Matrix<N, N> inverseMatrix(Matrix<N, N> matrix){
    std::array<std::array<floating, N*2>, N> a;
    // Augmenting Identity Matrix of Order N
    for(size_t i = 0; i < N; i++){
        for(size_t j = 0; j < N; j++){
            a[i][j] = matrix.data[i][j];
            a[i][j+N] = i==j;
        }
    }
    // Applying Gauss Jordan Elimination
    for(size_t i = 0; i < N; i++){
        if(a[i][i] == 0.0){ // Cannot inverse matrix
            return Matrix<N,N>();
        }
        for(size_t j = 0; j < N; j++){
            if(i==j) continue;
            floating ratio = a[j][i]/a[i][i];
            for(size_t k = 0; k < 2*N; k++)
                a[j][k] -= ratio*a[i][k];
        }
    }
    // Row Operation to Make Principal Diagonal to 1
    for(size_t i = 0; i < N; i++)
    for(size_t j = N; j < 2*N; j++)
        a[i][j] /= a[i][i];
    // Translating Inverse Matrix
    Matrix<N, N> ans(std::array<std::array<floating, N>, N>({}));
    for(size_t i = 0; i<N; i++)
    for(size_t j = N; j < 2*N; j++)
        ans.data[i][j-N] = a[i][j];
    return ans;
}

/// Implementation of the Aguilera-Perez Algorithm.
/// Aguilera, Antonio, and Ricardo Pérez-Aguila. "General n-dimensional rotations." (2004).

// divided into 2 parts because it can be cached for optimisation
template <size_t N>
Matrix<N, N> rotationMatrixPart1(Matrix<N, N-2> v){
    Matrix<N, N> M;
    for(size_t c = 0; c < N-2; c++)
    for(size_t r = N-1; r > c; r--){
        floating t = std::atan2(v.data[r][c], v.data[r-1][c]);
        Matrix<N, N> R;
        R.data[r-1][r-1] = std::cos(t);
        R.data[r  ][r-1] =-std::sin(t);
        R.data[r-1][r  ] = std::sin(t);
        R.data[r  ][r  ] = std::cos(t);
        v = R*v;
        M = R*M;
    }
    return M;
}

template <size_t N>
Matrix<N, N> rotationMatrixPart2(Matrix<N, N> part1, floating theta){
    Matrix<N, N> R;
    R.data[N-2][N-2] = std::cos(theta);
    R.data[N-2][N-1] =-std::sin(theta);
    R.data[N-1][N-2] = std::sin(theta);
    R.data[N-1][N-1] = std::cos(theta);
    return inverseMatrix(part1)*(R*part1);
}

#endif // MATRIX_H
