from kafka import KafkaProducer, KafkaConsumer
import json
import uuid
import time

KAFKA_BROKER = 'localhost:9092'
TOPIC_REQ = 'jogodavida-req'
TOPIC_RESP = 'jogodavida-resp'

req_id = str(uuid.uuid4())
data = {
    "id_cliente": req_id,
    "powmin": 3,
    "powmax": 6,
    "engine": "spark"
}


producer = KafkaProducer(bootstrap_servers=KAFKA_BROKER,
                         value_serializer=lambda v: json.dumps(v).encode())
producer.send(TOPIC_REQ, data)
producer.flush()
print(f"Requisição enviada com id {req_id}")


consumer = KafkaConsumer(
    TOPIC_RESP,
    bootstrap_servers=KAFKA_BROKER,
    value_deserializer=lambda m: json.loads(m.decode()),
    auto_offset_reset='earliest',
    group_id=f'cliente-{req_id}'
)
print("Aguardando resposta...")
for msg in consumer:
    resp = msg.value
    if resp.get("id_cliente") == req_id:
        print("Resposta recebida:", resp)
        break
consumer.close() 