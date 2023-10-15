#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

template <int N>
class Vertices {
public:
    // Constructor por defecto para inicializar todos los elementos a cero
    Vertices() : data(glm::vec<N, float>()) {}

    // Constructor para inicializar el vértice con valores específicos
    explicit Vertices(glm::vec<N, float> vec) : data(vec) {}

    // Operador de acceso para acceder a los elementos del vértice
    float& operator[](int index) {
        return data[index];
    }

    // Operador de acceso constante para acceder a los elementos del vértice en modo de solo lectura
    const float& operator[](int index) const {
        return data[index];
    }

    // Función para obtener el tamaño del vértice
    int size() const {
        return N;
    }

    // Función para obtener la suma de dos vértices
    Vertices<N> operator+(const Vertices<N>& other) const {
        return Vertices<N>(data + other.data);
    }

    // Función para obtener la multiplicación de un vértice por una constante
    Vertices<N> operator*(float scalar) const {
        return Vertices<N>(data * scalar);
    }

    // Función para obtener la multiplicación de un vértice por una matriz
    template <int M>
    Vertices<N> operator*(const glm::mat<M, N, float>& matrix) const {
        return Vertices<N>(matrix * data);
    }

private:
    glm::vec<N, float> data;
};