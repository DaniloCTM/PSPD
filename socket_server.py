import socket
import threading
import json
import time
from engines.spark_engine import process_with_spark
from engines.mpi_engine import process_with_mpi

HOST = '0.0.0.0'
PORT = 5000

def handle_client(conn, addr):
    print(f"[+] Conexão recebida de {addr}")

    try:
        data = conn.recv(1024).decode()
        if not data:
            print("[-] Nenhum dado recebido.")
            return

        print(f"[>] Dados recebidos: {data}")
        req = json.loads(data)

        id_cliente = req.get("id_cliente")
        powmin = req.get("powmin")
        powmax = req.get("powmax")
        engine_type = req.get("engine", "spark")

        start = time.time()
        if engine_type == "spark":
            result = process_with_spark(powmin, powmax)
        else:
            result = process_with_mpi(powmin, powmax)
        end = time.time()

        response = {
            "id_cliente": id_cliente,
            "resultado": result,
            "tempo_execucao": round(end - start, 4)
        }

        print("Reposta gerada:", response)

        conn.sendall(json.dumps(response).encode())
        print(f"[✓] Resposta enviada a {addr}")

    except Exception as e:
        print(f"[!] Erro: {e}")
    finally:
        conn.close()
        print(f"[x] Conexão encerrada com {addr}")

def start_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.bind((HOST, PORT))
        server.listen()
        print(f"[🔌] Servidor socket escutando em {HOST}:{PORT}")

        while True:
            conn, addr = server.accept()
            thread = threading.Thread(target=handle_client, args=(conn, addr))
            thread.start()

if __name__ == "__main__":
    start_server()
