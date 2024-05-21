#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
using namespace std;
// Función que muestra el tablero e informacion del servidor
void visualizarTablero(const char* buffer) {
    cout << buffer << endl;
}
// Función principal
int main(int argc, char **argv) {
    // Verificar los argumentos de la línea de comandos
    if (argc != 3) {
        cerr << "Uso: " << argv[0] << " <dirección IP> <puerto>\n";
        return 1;
    }
    // Crear el socket
    const char* server_ip = argv[1];
    int port = atoi(argv[2]);
    int socket_cliente;
    struct sockaddr_in direccionServidor;
    // Conectar al servidor
    cout << "Conectando al servidor ...\n";
    if ((socket_cliente = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Error al crear el socket\n";
        return 1;
    }
    // Estructura con la dirección del servidor
    memset(&direccionServidor, 0, sizeof(direccionServidor));
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &direccionServidor.sin_addr) <= 0) {
        cerr << "Dirección IP inválida\n";
        return 1;
    }
    // Conectar al servidor
    if (connect(socket_cliente, (struct sockaddr *)&direccionServidor, sizeof(direccionServidor)) < 0) {
        cerr << "Error al conectar al servidor\n";
        return 1;
    }
    // Juego
    char buffer[1024];
    while (true) {
        cout << "Introduce la columna (1-7) o 'Q' para salir: ";
        // Leer entrada del usuario
        cin.getline(buffer, 1024);
        // Salir si el usuario ingresa 'Q'
        if (buffer[0] == 'Q') {
            send(socket_cliente, buffer, strlen(buffer), 0);
            break;
        }
        // Enviar movimiento al servidor
        if (buffer[0] >= '1' && buffer[0] <= '7') {
            snprintf(buffer, sizeof(buffer), "C %d\n", atoi(buffer));
            send(socket_cliente, buffer, strlen(buffer), 0);
        } else {
            cout << "Entrada inválida\n";
            continue;
        }
        // Recibir respuesta del servidor
        int n_bytes = recv(socket_cliente, buffer, 1024, 0);
        if (n_bytes > 0) {
            buffer[n_bytes] = '\0';
            visualizarTablero(buffer);
        } else {
            cout << "Servidor desconectado\n";
            break;
        }
    }

    close(socket_cliente);
    return 0;
}
