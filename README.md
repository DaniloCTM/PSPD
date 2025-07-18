# Jogo da Vida Paralelo/Distribuído

## Descrição

Este projeto implementa o Jogo da Vida usando duas abordagens de paralelismo: Apache Spark (distribuído) e MPI (paralelo em C). Um servidor socket recebe requisições de clientes e executa o engine escolhido.

---

## Dependências
- Python 3.x
- PySpark (`pip install pyspark`)
- MPI (ex: `sudo apt install mpich`)
- Java (para Spark)

---

## Como rodar

### 1. Compile o código C (MPI):
```bash
cd engines
mpicc -o mpi_engine mpi_engine.c
cd ..
```

### 2. Instale as dependências Python:
```bash
pip install pyspark
```

### 3. Rode o servidor:
```bash
python3 socket_server.py
```

### 4. Em outro terminal, rode o cliente:
```bash
python3 teste.py
```

- Para testar o engine MPI, edite `teste.py` e troque `"engine": "spark"` por `"engine": "mpi"`.

---

## Métricas
- As métricas de uso (número de clientes, tempos de execução) são salvas em `metrics.json` após cada requisição.

---

## Teste de Stress
- Para simular múltiplos clientes, abra vários terminais e rode `python3 teste.py` em cada um.
- Ou use o script `multi_client_test.py` (ver abaixo).

---

## Observações para Kubernetes
- O projeto pode ser containerizado com o `Dockerfile` fornecido.
- Um exemplo de manifesto Kubernetes está em `k8s-deployment.yaml`.
- Para rodar em cluster, basta construir a imagem, subir no seu registry e aplicar o manifesto.

---

## ElasticSearch e Kibana (Métricas avançadas)

Para registrar e visualizar métricas em tempo real:

1. Suba o ElasticSearch e o Kibana (em terminais separados):
   ```bash
   docker run -d --name elasticsearch -p 9200:9200 -e "discovery.type=single-node" elasticsearch:8.13.4
   docker run -d --name kibana --link elasticsearch:elasticsearch -p 5601:5601 kibana:8.13.4
   ```
2. Acesse o Kibana em [http://localhost:5601](http://localhost:5601)
3. Crie um índice chamado `jogodavida-metricas`.
4. Visualize gráficos de tempo de execução, número de clientes, etc.

O servidor já envia métricas automaticamente para o ElasticSearch a cada requisição.

---

## Kafka (Arquitetura alternativa de mensageria)

### Como rodar o Kafka (para testes locais)

```bash
docker run -d --name zookeeper -p 2181:2181 zookeeper:3.9
docker run -d --name kafka -p 9092:9092 --link zookeeper:zookeeper \
  -e KAFKA_ZOOKEEPER_CONNECT=zookeeper:2181 \
  -e KAFKA_ADVERTISED_LISTENERS=PLAINTEXT://localhost:9092 \
  -e KAFKA_LISTENERS=PLAINTEXT://0.0.0.0:9092 \
  bitnami/kafka:latest
```

### Instale a dependência Python:
```bash
pip install kafka-python
```

### Como usar a arquitetura Kafka:
- Rode o servidor consumidor:
  ```bash
  python3 kafka_server.py
  ```
- Em outro terminal, rode o cliente produtor:
  ```bash
  python3 kafka_client.py
  ```

O cliente publica requisições no tópico `jogodavida-req` e aguarda a resposta no tópico `jogodavida-resp`. O servidor consome, processa e publica a resposta.

### Observação
- Você pode rodar a arquitetura socket tradicional **ou** a arquitetura Kafka, conforme desejar.
- Documente no relatório qual arquitetura foi usada nos testes e por quê.
