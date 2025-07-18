import socket
import json
import threading
import time

HOST = 'localhost'
PORT = 5000
NUM_CLIENTES = 10

DATA = {
    "id_cliente": None,
    "powmin": 3,
    "powmax": 6,
    "engine": "spark"
}

def cliente_thread(id_cliente):
    data = DATA.copy()
    data["id_cliente"] = id_cliente
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((HOST, PORT))
            sock.sendall(json.dumps(data).encode())
            response = sock.recv(4096).decode()
            print(f"Cliente {id_cliente} recebeu: {response}")
    except Exception as e:
        print(f"Cliente {id_cliente} erro: {e}")

threads = []
start = time.time()
for i in range(NUM_CLIENTES):
    t = threading.Thread(target=cliente_thread, args=(i+1,))
    t.start()
    threads.append(t)

for t in threads:
    t.join()
end = time.time()
print(f"Todos os clientes terminaram. Tempo total: {round(end-start, 2)}s") 