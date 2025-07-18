from kafka import KafkaConsumer, KafkaProducer
import json
import time
from engines.spark_engine import process_with_spark
from engines.mpi_engine import process_with_mpi

KAFKA_BROKER = 'localhost:9092'
TOPIC_REQ = 'jogodavida-req'
TOPIC_RESP = 'jogodavida-resp'

consumer = KafkaConsumer(
    TOPIC_REQ,
    bootstrap_servers=KAFKA_BROKER,
    value_deserializer=lambda m: json.loads(m.decode()),
    auto_offset_reset='earliest',
    group_id='jogodavida-server'
)
producer = KafkaProducer(bootstrap_servers=KAFKA_BROKER,
                         value_serializer=lambda v: json.dumps(v).encode())

print("[KafkaServer] Aguardando requisições...")
for msg in consumer:
    req = msg.value
    id_cliente = req.get("id_cliente")
    powmin = req.get("powmin")
    powmax = req.get("powmax")
    engine_type = req.get("engine", "spark")
    print(f"[KafkaServer] Requisição recebida: {req}")
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
    producer.send(TOPIC_RESP, response)
    producer.flush()
    print(f"[KafkaServer] Resposta enviada: {response}") 