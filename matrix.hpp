/**
 * Esta classe deve auxiliar em operações envolvendo
 * matrizes e operações relacionadas a transformações geométricas.
 **/

#include <vector>

using namespace std;

struct Matrix {
private:
	vector<vector<int>> matrix;
public:
	Matrix(vector<vector<int>> _matrix) : matrix(_matrix) {}; 
	Matrix() : matrix({
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1}
	}) {};

	// sobrecarga de operador
	pair<int, int> operator *(pair<int, int> rhs) {
		return { 0, 0 };
	}
};