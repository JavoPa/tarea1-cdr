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
const char JUGADOR_SERVIDOR = 'S';
const char JUGADOR_CLIENTE = 'C';

// Clase del juego Conecta 4
class Juego {
public:
    vector<vector<char>> tablero;
    char turno;
    bool terminado;
    string ipCliente;  
    int puertoCliente;

    Juego(string ip, int puerto) : tablero(FILAS, vector<char>(COLUMNAS, VACIO)), turno(JUGADOR_CLIENTE), terminado(false), ipCliente(ip), puertoCliente(puerto) {}

    // Realizar un movimiento en el tablero
    bool realizarMovimiento(int columna, char jugador) {
        if (columna < 0 || columna >= COLUMNAS || tablero[0][columna] != VACIO) {
            return false;
        }
        for (int fila = FILAS - 1; fila >= 0; --fila) {
            if (tablero[fila][columna] == VACIO) {
                tablero[fila][columna] = jugador;
                return true;
            }
        }
        return false;
    }

    // Verifica si hay un ganador en el juego
    bool verificarGanador(char jugador) {
        for (int f = 0; f < FILAS; ++f) {
            for (int c = 0; c < COLUMNAS; ++c) {
                if (c + 3 < COLUMNAS && tablero[f][c] == jugador && tablero[f][c + 1] == jugador && tablero[f][c + 2] == jugador && tablero[f][c + 3] == jugador) {
                    return true;
                }
                if (f + 3 < FILAS && tablero[f][c] == jugador && tablero[f + 1][c] == jugador && tablero[f + 2][c] == jugador && tablero[f + 3][c] == jugador) {
                    return true;
                }
                if (c + 3 < COLUMNAS && f + 3 < FILAS && tablero[f][c] == jugador && tablero[f + 1][c + 1] == jugador && tablero[f + 2][c + 2] == jugador && tablero[f + 3][c + 3] == jugador) {
                    return true;
                }
                if (c - 3 >= 0 && f + 3 < FILAS && tablero[f][c] == jugador && tablero[f + 1][c - 1] == jugador && tablero[f + 2][c - 2] == jugador && tablero[f + 3][c - 3] == jugador) {
                    return true;
                }
            }
        }
        return false;
    }

    // Imprime el tablero en la consola del servidor
    void imprimirTablero() {
        cout << "Tablero (Cliente: " << ipCliente << ":" << puertoCliente << "):" << endl;
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

    // Imprime el tablero como una cadena de caracteres
    string obtenerTableroComoString() {
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
        return mensaje;
    }
};

// Clase que representa el servidor de juego
class Servidor {
private:
    int puerto;
    int socketServidor;
    struct sockaddr_in direccionServidor;
    vector<Juego> juegos;
    mutex juegosMutex;

public:
    Servidor(int p) : puerto(p), socketServidor(0) {
        memset(&direccionServidor, 0, sizeof(direccionServidor));
    }

    // Inicia el servidor
    void iniciar() {
        cout << "Creando socket de escucha ...\n";
        if ((socketServidor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            cerr << "Error al crear el socket de escucha\n";
            exit(EXIT_FAILURE);
        }

        cout << "Configurando estructura de dirección del socket ...\n";
        direccionServidor.sin_family = AF_INET;
        direccionServidor.sin_addr.s_addr = htonl(INADDR_ANY);
        direccionServidor.sin_port = htons(puerto);

        cout << "Enlazando socket ...\n";
        if (bind(socketServidor, (struct sockaddr *)&direccionServidor, sizeof(direccionServidor)) < 0) {
            cerr << "Error al llamar a bind()\n";
            exit(EXIT_FAILURE);
        }

        cout << "Llamando a listen ...\n";
        if (listen(socketServidor, 1024) < 0) {
            cerr << "Error al llamar a listen()\n";
            exit(EXIT_FAILURE);
        }

        cout << "Esperando solicitudes de clientes ...\n";
        while (true) {
            struct sockaddr_in direccionCliente;
            socklen_t addr_size = sizeof(struct sockaddr_in);
            int socketCliente;
            if ((socketCliente = accept(socketServidor, (struct sockaddr *)&direccionCliente, &addr_size)) < 0) {
                cerr << "Error al llamar a accept()\n";
                exit(EXIT_FAILURE);
            }

            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(direccionCliente.sin_addr), ip, INET_ADDRSTRLEN);
            string ipStr(ip);
            int puertoCliente = ntohs(direccionCliente.sin_port);

            thread t(&Servidor::manejarCliente, this, socketCliente, direccionCliente, ipStr, puertoCliente);
            t.detach();
        }
    }

private:
    // Maneja la comunicación con un cliente
    void manejarCliente(int socketCliente, struct sockaddr_in direccionCliente, string ipStr, int puertoCliente) {
        char buffer[1024];
        memset(buffer, '\0', sizeof(buffer));
        int n_bytes = 0;

        cout << "[" << ipStr << ":" << puertoCliente << "] Nuevo jugador." << endl;

        int juegoId;
        {
            lock_guard<mutex> lock(juegosMutex);
            juegos.emplace_back(ipStr, puertoCliente);
            juegoId = juegos.size() - 1;
        }

        while ((n_bytes = recv(socketCliente, buffer, 1024, 0)) > 0) {
            buffer[n_bytes] = '\0';
            if (buffer[0] == 'Q') {
                cout << "[" << ipStr << ":" << puertoCliente << "] Sale del juego." << endl;
                close(socketCliente);
                break;
            }

            lock_guard<mutex> lock(juegosMutex);
            Juego &juego = juegos[juegoId];
            if (juego.terminado) {
                send(socketCliente, "Juego terminado\n", 16, 0);
                continue;
            }

            if (buffer[0] == 'C') {
                int columna = atoi(&buffer[2]) - 1;
                if (juego.realizarMovimiento(columna, JUGADOR_CLIENTE)) {
                    if (juego.verificarGanador(JUGADOR_CLIENTE)) {
                        juego.terminado = true;
                        snprintf(buffer, sizeof(buffer), "Ganador: Cliente\n");
                    } else {
                        juego.turno = JUGADOR_SERVIDOR;
                        int columnaServidor = elegirColumnaAleatoria(juego);
                        juego.realizarMovimiento(columnaServidor, JUGADOR_SERVIDOR);
                        if (juego.verificarGanador(JUGADOR_SERVIDOR)) {
                            juego.terminado = true;
                            snprintf(buffer, sizeof(buffer), "Ganador: Servidor\n");
                        } else {
                            juego.turno = JUGADOR_CLIENTE;
                            snprintf(buffer, sizeof(buffer), "Turno: Cliente\n");
                        }
                    }
                    enviarTablero(socketCliente, juego);
                } else {
                    snprintf(buffer, sizeof(buffer), "Movimiento inválido\n");
                }
                send(socketCliente, buffer, strlen(buffer), 0);
                juego.imprimirTablero();
            } else {
                send(socketCliente, "error\n", 6, 0);
            }
        }
    }

    // Elige una columna aleatoria para que juegue el servidor
    int elegirColumnaAleatoria(Juego &juego) {
        vector<int> columnasDisponibles;
        for (int c = 0; c < COLUMNAS; ++c) {
            if (juego.tablero[0][c] == VACIO) {
                columnasDisponibles.push_back(c);
            }
        }
        if (columnasDisponibles.empty()) {
            return -1;
        }
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> distr(0, columnasDisponibles.size() - 1);
        return columnasDisponibles[distr(gen)];
    }

    // Envía el tablero al cliente
    void enviarTablero(int socketCliente, Juego &juego) {
        string mensaje = juego.obtenerTableroComoString();
        send(socketCliente, mensaje.c_str(), mensaje.length(), 0);
    }
};

// Función principal
int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << "Uso: " << argv[0] << " <puerto>\n";
        return 1;
    }
    int puerto = atoi(argv[1]);
    Servidor servidor(puerto);
    servidor.iniciar();
    return 0;
}