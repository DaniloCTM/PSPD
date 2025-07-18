import socket
import json

HOST = 'localhost'
PORT = 5000

data = {
    "id_cliente": 1,
    "powmin": 3,
    "powmax": 6,
    "engine": "spark"
}

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    sock.connect((HOST, PORT))
    sock.sendall(json.dumps(data).encode())

    response = sock.recv(4096).decode()
    print("Resposta do servidor:", response)
