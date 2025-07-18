FROM python:3.10-slim

WORKDIR /app

RUN apt-get update && \
    apt-get install -y gcc mpich && \
    rm -rf /var/lib/apt/lists/*

COPY . .

RUN pip install --no-cache-dir pyspark

RUN cd engines && mpicc -o mpi_engine mpi_engine.c

EXPOSE 5000

CMD ["python3", "socket_server.py"] 