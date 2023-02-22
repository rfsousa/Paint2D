/**
 * Esta classe deve auxiliar em operações envolvendo
 * matrizes e operações relacionadas a transformações geométricas.
 **/

#include <vector>

using namespace std;

template<class T>
struct Matrix {
private:
    vector<vector<T>> matrix;
public:
    Matrix(vector<vector<T>> _matrix) : matrix(_matrix) {};
    Matrix() : matrix({
        { 1, 0, 0 },
        { 0, 1, 0 },
        { 0, 0, 1 }
    }) {};

    // sobrecarga de operador
    pair<T, T> operator *(pair<T, T> rhs) {
        if(matrix.size() == 0) return rhs;
        // não é possível multiplicar, pensando em rhs como coordenadas homogeneas.
        if(matrix[0].size() != 3) return rhs;

        T vec[] = { rhs.first, rhs.second, 1 };
        T ret[] = { 0, 0, 0 };

        for(int i = 0; i < 3; i++) {
            T c = 0;
            for(int j = 0; j < matrix[0].size(); j++) {
                c += matrix[i][j] * vec[j];
            }
            ret[i] = c;
        }

        return { ret[0] / ret[2], ret[1] / ret[2] };
    }


    Matrix operator *(Matrix rhs) {
        if(matrix.size() == 0) return *this;
        if(matrix[0].size() != rhs.matrix.size()) return *this;

        const int r = matrix.size(), c = rhs.matrix[0].size();

        vector<vector<T>> ret(r, vector<T>(c));

        for(int i = 0; i < r; i++) {
            for(int j = 0; j < c; j++) {
                T sum = 0;
                for(int k = 0; k < rhs.matrix.size(); k++) {
                    sum += matrix[i][k] * rhs.matrix[k][j];
                }
                ret[i][j] = sum;
            }
        }

        return Matrix(ret);
    }

    static Matrix translationMatrix(T x, T y) {
    	return Matrix({
    		{ 1, 0, x },
    		{ 0, 1, y },
    		{ 0, 0, 1 }
    	});
    }

    static Matrix scaleMatrix(T x, T y) {
    	return Matrix({
    		{ x, 0, 0 },
    		{ 0, y, 0 },
    		{ 0, 0, 1 }
    	});
    }

    static Matrix shearMatrix(T x, T y) {
    	return Matrix({
    		{ 1, y, 0 },
    		{ x, 1, 0 },
    		{ 0, 0, 1 }
    	});
    }

    static Matrix rotationMatrix(T rad) {
    	return Matrix({
    		{ cos(rad), -sin(rad), 0 },
    		{ sin(rad),  cos(rad), 0 },
    		{ 		 0, 		0, 1 }
    	});
    }
    
};
