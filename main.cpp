#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

int main(int argc, char* argv[]) {
    string input;

    // ─────────────────────────────────────────
    // LEER ENTRADA — archivo o consola
    // ─────────────────────────────────────────
    if (argc >= 2) {
        // Se pasó un archivo como argumento
        // Ejemplo: ./programa caso1.txt
        ifstream file(argv[1]);

        if (!file.is_open()) {
            cerr << "Error: no se pudo abrir el archivo: "
                 << argv[1] << endl;
            return 1;
        }

        // Leer todo el contenido del archivo en un string
        stringstream buffer;
        buffer << file.rdbuf();
        input = buffer.str();
        file.close();

    } else {
        // No se pasó archivo → leer desde consola
        // El usuario escribe o pega el texto
        // Termina cuando hace Ctrl+D (Linux/Mac) o Ctrl+Z (Windows)
        stringstream buffer;
        buffer << cin.rdbuf();
        input = buffer.str();
    }
}