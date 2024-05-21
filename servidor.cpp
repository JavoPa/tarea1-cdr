#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <ctime>
#include <mutex>
using namespace std;

const int FILAS = 6;
const int COLUMNAS = 7;
const char VACIO = '.';
const char JUGADOR_SERVIDOR = 'S'; // Ficha del servidor
const char JUGADOR_CLIENTE = 'C';  // Ficha del cliente
// Estructura que representa el juego
struct Juego {
    vector<vector<char>> tablero;
    char turno;
    bool terminado;
    Juego() : tablero(FILAS, vector<char>(COLUMNAS, VACIO)), turno(JUGADOR_CLIENTE), terminado(false) {}
};

vector<Juego> juegos;
mutex juegosMutex; // Para proteger el acceso a la lista de juegos
// Función que valida si se puede realizar un movimiento
bool realizarMovimiento(Juego &juego, int columna, char jugador) {
    if (columna < 0 || columna >= COLUMNAS || juego.tablero[0][columna] != VACIO) {
        return false;
    }
    for (int fila = FILAS - 1; fila >= 0; --fila) {
        if (juego.tablero[fila][columna] == VACIO) {
            juego.tablero[fila][columna] = jugador;
            return true;
        }
    }
    return false;
}
// Función que verifica si hay un ganador
bool verificarGanador(Juego &juego, char jugador) {
    for (int f = 0; f < FILAS; ++f) {
        for (int c = 0; c < COLUMNAS; ++c) {
            if (c + 3 < COLUMNAS && juego.tablero[f][c] == jugador && juego.tablero[f][c + 1] == jugador && juego.tablero[f][c + 2] == jugador && juego.tablero[f][c + 3] == jugador) {
                return true;
            }
            if (f + 3 < FILAS && juego.tablero[f][c] == jugador && juego.tablero[f + 1][c] == jugador && juego.tablero[f + 2][c] == jugador && juego.tablero[f + 3][c] == jugador) {
                return true;
            }
            if (c + 3 < COLUMNAS && f + 3 < FILAS && juego.tablero[f][c] == jugador && juego.tablero[f + 1][c + 1] == jugador && juego.tablero[f + 2][c + 2] == jugador && juego.tablero[f + 3][c + 3] == jugador) {
                return true;
            }
            if (c - 3 >= 0 && f + 3 < FILAS && juego.tablero[f][c] == jugador && juego.tablero[f + 1][c - 1] == jugador && juego.tablero[f + 2][c - 2] == jugador && juego.tablero[f + 3][c - 3] == jugador) {
                return true;
            }
        }
    }
    return false;
}
// Función que elige una columna aleatoria para el servidor
int elegirColumnaAleatoria(Juego &juego) {
    vector<int> columnasDisponibles;
    for (int c = 0; c < COLUMNAS; ++c) {
        if (juego.tablero[0][c] == VACIO) {
            columnasDisponibles.push_back(c);
        }
    }
    if (columnasDisponibles.empty()) {
        return -1; // No hay columnas disponibles
    }
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distr(0, columnasDisponibles.size() - 1);
    return columnasDisponibles[distr(gen)];
}
// Función que imprime el tablero en consola
void imprimirTablero(const vector<vector<char>> &tablero) {
    cout << "Tablero:" << endl;
    for (int f = 0; f < FILAS; ++f) {
        cout << f + 1 << " ";
        for (int c = 0; c < COLUMNAS; ++c) {
            cout << tablero[f][c] << ' ';
        }
        cout << endl;
    }
    cout << "  -------------" << endl;
    cout << "  1 2 3 4 5 6 7" << endl;
}
// Función que envía el tablero al cliente
void enviarTablero(int socket_cliente, const vector<vector<char>> &tablero) {
    string mensaje = "Tablero:\n";
    for (int f = 0; f < FILAS; ++f) {
        mensaje += to_string(f + 1) + " ";
        for (int c = 0; c < COLUMNAS; ++c) {
            mensaje += tablero[f][c];
            mensaje += ' ';
        }
        mensaje += '\n';
    }
    mensaje += "  -------------\n";
    mensaje += "  1 2 3 4 5 6 7\n";
    send(socket_cliente, mensaje.c_str(), mensaje.length(), 0);
}
// Función que implementa la lógica del juego
void jugar(int socket_cliente, struct sockaddr_in direccionCliente) {
    char buffer[1024];
    memset(buffer, '\0', sizeof(buffer));
    int n_bytes = 0;

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(direccionCliente.sin_addr), ip, INET_ADDRSTRLEN);
    cout << "[" << ip << ":" << ntohs(direccionCliente.sin_port) << "] Nuevo jugador." << endl;

    {
        lock_guard<mutex> lock(juegosMutex);
        juegos.emplace_back();
    }
    int juegoId = juegos.size() - 1;
    // Iniciar juego
    while ((n_bytes = recv(socket_cliente, buffer, 1024, 0)) > 0) {
        buffer[n_bytes] = '\0';
        if (buffer[0] == 'Q') {
            cout << "[" << ip << ":" << ntohs(direccionCliente.sin_port) << "] Sale del juego." << endl;
            close(socket_cliente);
            break;
        }

        lock_guard<mutex> lock(juegosMutex);
        Juego &juego = juegos[juegoId];
        if (juego.terminado) {
            send(socket_cliente, "Juego terminado\n", 16, 0);
            continue;
        }

        if (buffer[0] == 'C') {
            int columna = atoi(&buffer[2]) - 1;
            if (realizarMovimiento(juego, columna, JUGADOR_CLIENTE)) {
                if (verificarGanador(juego, JUGADOR_CLIENTE)) {
                    juego.terminado = true;
                    snprintf(buffer, sizeof(buffer), "Ganador: Cliente\n");
                } else {
                    juego.turno = JUGADOR_SERVIDOR;
                    int columnaServidor = elegirColumnaAleatoria(juego);
                    realizarMovimiento(juego, columnaServidor, JUGADOR_SERVIDOR);
                    if (verificarGanador(juego, JUGADOR_SERVIDOR)) {
                        juego.terminado = true;
                        snprintf(buffer, sizeof(buffer), "Ganador: Servidor\n");
                    } else {
                        juego.turno = JUGADOR_CLIENTE;
                        snprintf(buffer, sizeof(buffer), "Turno: Cliente\n");
                    }
                }
                enviarTablero(socket_cliente, juego.tablero);
            } else {
                snprintf(buffer, sizeof(buffer), "Movimiento inválido\n");
            }
            send(socket_cliente, buffer, strlen(buffer), 0);
            imprimirTablero(juego.tablero);
        } else {
            send(socket_cliente, "error\n", 6, 0);
        }
    }
}
// Función que se ejecutará en un hilo para atender a un cliente
void threadFunction(int socket_cliente, sockaddr_in direccionCliente) {
    jugar(socket_cliente, direccionCliente);
}

int main(int argc, char **argv) {
    // Verificar argumentos
    if (argc != 2) {
        cerr << "Uso: " << argv[0] << " <puerto>\n";
        return 1;
    }
    // Crear socket de escucha
    int port = atoi(argv[1]);
    int socket_server = 0;
    struct sockaddr_in direccionServidor, direccionCliente;

    cout << "Creando socket de escucha ...\n";
    if ((socket_server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Error al crear el socket de escucha\n";
        exit(EXIT_FAILURE);
    }
    // Configurar estructura o atributos de dirección del socket
    cout << "Configurando estructura de dirección del socket ...\n";
    memset(&direccionServidor, 0, sizeof(direccionServidor));
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_addr.s_addr = htonl(INADDR_ANY);
    direccionServidor.sin_port = htons(port);
    // Enlazar socket
    cout << "Enlazando socket ...\n";
    if (bind(socket_server, (struct sockaddr *)&direccionServidor, sizeof(direccionServidor)) < 0) {
        cerr << "Error al llamar a bind()\n";
        exit(EXIT_FAILURE);
    }
    // Escuchar
    cout << "Llamando a listen ...\n";
    if (listen(socket_server, 1024) < 0) {
        cerr << "Error al llamar a listen()\n";
        exit(EXIT_FAILURE);
    }
    // Aceptar conexiones
    socklen_t addr_size = sizeof(struct sockaddr_in);
    cout << "Esperando solicitudes de clientes ...\n";
    while (true) {
        int socket_cliente;
        if ((socket_cliente = accept(socket_server, (struct sockaddr *)&direccionCliente, &addr_size)) < 0) {
            cerr << "Error al llamar a accept()\n";
            exit(EXIT_FAILURE);
        }

        thread t(threadFunction, socket_cliente, direccionCliente);
        t.detach();
    }

    return 0;
}
