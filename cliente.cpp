#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
using namespace std;

// Clase del cliente del juego
class Cliente {
private:
    const char* server_ip;
    int port;
    int socket_cliente;
    struct sockaddr_in direccionServidor;
    bool juegoTerminado;

public:
    Cliente(const char* ip, int p) : server_ip(ip), port(p), socket_cliente(0), juegoTerminado(false) {
        memset(&direccionServidor, 0, sizeof(direccionServidor));
    }

    // Establece la conexión con el servidor
    bool conectar() {
        cout << "Conectando al servidor ...\n";
        if ((socket_cliente = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            cerr << "Error al crear el socket\n";
            return false;
        }

        direccionServidor.sin_family = AF_INET;
        direccionServidor.sin_port = htons(port);

        if (inet_pton(AF_INET, server_ip, &direccionServidor.sin_addr) <= 0) {
            cerr << "Dirección IP inválida\n";
            return false;
        }

        if (connect(socket_cliente, (struct sockaddr *)&direccionServidor, sizeof(direccionServidor)) < 0) {
            cerr << "Error al conectar al servidor\n";
            return false;
        }
        return true;
    }

    // Inicia la interacción del cliente con el servidor
    void iniciar() {
        char buffer[1024];
        while (true) {
            cout << "Introduce la columna (1-7) o 'Q' para salir: "; //Solicita ingresar la columna
            cin.getline(buffer, 1024);

            if (buffer[0] == 'Q') {
                send(socket_cliente, buffer, strlen(buffer), 0);
                break;
            }

            if (!juegoTerminado) {
                if (buffer[0] >= '1' && buffer[0] <= '7') {
                    snprintf(buffer, sizeof(buffer), "C %d\n", atoi(buffer));
                    send(socket_cliente, buffer, strlen(buffer), 0);
                } else {
                    cout << "Entrada inválida\n"; 
                    continue;
                }

                recibirYMostrarTablero();
            } else {
                cout << "El juego ha terminado. Presiona 'Q' para salir: ";  //Cuando ya hay un ganador solo solicita salir
                cin.getline(buffer, 1024);
                if (buffer[0] == 'Q') {
                    send(socket_cliente, buffer, strlen(buffer), 0);
                    break;
                } else {
                    cout << "Entrada inválida. Presiona 'Q' para salir.\n";
                }
            }
        }

        close(socket_cliente);
    }

    // Recibe y muestra el tablero actualizado desde el servidor
    void recibirYMostrarTablero() {
        char buffer[1024];
        int n_bytes;
        string tableroCompleto;

        while ((n_bytes = recv(socket_cliente, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[n_bytes] = '\0';
            tableroCompleto += buffer;

            if (tableroCompleto.find("Turno: Cliente") != string::npos ||
                tableroCompleto.find("Ganador") != string::npos ||
                tableroCompleto.find("Movimiento inválido") != string::npos) {
                if (tableroCompleto.find("Ganador") != string::npos) {
                    juegoTerminado = true;
                }
                break;
            }
        }

    // Función estática para visualizar el tablero
        if (!tableroCompleto.empty()) {
            visualizarTablero(tableroCompleto.c_str());
        } else {
            cout << "Servidor desconectado\n";
        }
    }

    static void visualizarTablero(const char* buffer) {
        cout << buffer << endl;
    }
};

// Función principal
int main(int argc, char **argv) {
    if (argc != 3) {
        cerr << "Uso: " << argv[0] << " <dirección IP> <puerto>\n";
        return 1;
    }

    const char* server_ip = argv[1];
    int port = atoi(argv[2]);

    Cliente cliente(server_ip, port);
    if (cliente.conectar()) {
        cliente.iniciar();
    }

    return 0;
}