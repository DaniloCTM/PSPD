# -*- coding: utf-8 -*-
from pyspark.sql import SparkSession
from pyspark.sql.functions import col, lit, sum as spark_sum, udf, when
from pyspark.sql.types import IntegerType, StructType, StructField
from datetime import datetime

POWMIN = 3
POWMAX = 10

def get_board_size(pow_val):
    return 1 << pow_val

board_schema = StructType([
    StructField("row", IntegerType(), False),
    StructField("col", IntegerType(), False),
    StructField("state", IntegerType(), False)
])

def is_alive(board_dict, r, c, tam_val):
    if 0 <= r < tam_val and 0 <= c < tam_val:
        return board_dict.get((r, c), 0)
    return 0

def calculate_live_neighbors_udf(r, c, current_board_map_broadcast, tam_val):
    current_board_map = current_board_map_broadcast.value
    vizviv = 0
    for dr in [-1, 0, 1]:
        for dc in [-1, 0, 1]:
            if dr == 0 and dc == 0:
                continue
            nr, nc = r + dr, c + dc
            if 1 <= nr <= tam_val and 1 <= nc <= tam_val:
                if (nr, nc) in current_board_map and current_board_map[(nr, nc)] == 1:
                    vizviv += 1
    return vizviv

def uma_vida_spark(current_df, tam_val, spark_context):
    current_board_map = {((row), (col)): state for row, col, state in current_df.collect()}
    current_board_map_broadcast = spark_context.broadcast(current_board_map)

    calculate_live_neighbors_udf_broadcasted = udf(
        lambda r, c: calculate_live_neighbors_udf(r, c, current_board_map_broadcast, tam_val), IntegerType()
    )

    df_with_neighbors = current_df.withColumn(
        "live_neighbors",
        calculate_live_neighbors_udf_broadcasted(col("row"), col("col"))
    )

    next_generation_df = df_with_neighbors.withColumn(
        "next_state",
        when(
            (col("state") == 1) & (col("live_neighbors") < 2), lit(0)
        ).when(
            (col("state") == 1) & (col("live_neighbors") > 3), lit(0)
        ).when(
            (col("state") == 0) & (col("live_neighbors") == 3), lit(1)
        ).otherwise(col("state"))
    ).select("row", "col", col("next_state").alias("state"))

    return next_generation_df

def init_tabul_spark(tam_val, spark_session):
    initial_live_cells = [
        (1, 2),
        (2, 3),
        (3, 1),
        (3, 2),
        (3, 3)
    ]

    all_cells = []
    for r in range(1, tam_val + 1):
        for c in range(1, tam_val + 1):
            all_cells.append((r, c, 0))

    initial_df = spark_session.createDataFrame(all_cells, schema=board_schema)

    live_cells_df = spark_session.createDataFrame(
        [(r, c, 1) for r, c in initial_live_cells], schema=board_schema
    )

    initial_board_states = {}
    for r in range(1, tam_val + 1):
        for c in range(1, tam_val + 1):
            initial_board_states[(r, c)] = 0

    for r, c in initial_live_cells:
        initial_board_states[(r, c)] = 1

    initial_board_data = [(r, c, state) for (r, c), state in initial_board_states.items()]

    return spark_session.createDataFrame(initial_board_data, schema=board_schema)

def correto_spark(final_df, tam_val):
    live_cells = final_df.filter(col("state") == 1).collect()
    live_coords = set([(row, col) for row, col, _ in live_cells])

    total_live_cells = len(live_cells)
    if total_live_cells != 5:
        return False

    expected_coords = set([
        (tam_val - 2, tam_val - 1),
        (tam_val - 1, tam_val),
        (tam_val, tam_val - 2),
        (tam_val, tam_val - 1),
        (tam_val, tam_val)
    ])

    return live_coords == expected_coords

def main_spark_game_of_life():
    spark = SparkSession.builder \
        .appName("JogoDaVidaSpark") \
        .getOrCreate()

    sc = spark.sparkContext

    for pow_val in range(POWMIN, POWMAX + 1):
        tam = get_board_size(pow_val)
        print(f"\n--- Processando tabuleiro de tamanho: {tam}x{tam} ---")

        start_init_time = datetime.now()
        current_board_df = init_tabul_spark(tam, spark)
        end_init_time = datetime.now()
        init_duration = (end_init_time - start_init_time).total_seconds()

        num_generations = 2 * (tam - 3)

        start_comp_time = datetime.now()
        for i in range(num_generations):
            current_board_df = uma_vida_spark(current_board_df, tam, sc)
        end_comp_time = datetime.now()
        comp_duration = (end_comp_time - start_comp_time).total_seconds()

        start_final_check_time = datetime.now()
        if correto_spark(current_board_df, tam):
            print(f"**Ok, RESULTADO CORRETO para {tam}x{tam}**")
        else:
            print(f"**Nok, RESULTADO ERRADO para {tam}x{tam}**")
        end_final_check_time = datetime.now()
        final_check_duration = (end_final_check_time - start_final_check_time).total_seconds()

        total_duration = init_duration + comp_duration + final_check_duration
        print(f"tam={tam}; tempos: init={init_duration:.7f}, comp={comp_duration:.7f}, fim={final_check_duration:.7f} tot={total_duration:.7f}")

    spark.stop()

def process_with_spark(powmin, powmax):
    spark = SparkSession.builder \
        .appName("JogoDaVidaSpark") \
        .getOrCreate()

    sc = spark.sparkContext

    resultados = []

    for pow_val in range(powmin, powmax + 1):
        tam = get_board_size(pow_val)
        print(f"\n--- Processando tabuleiro de tamanho: {tam}x{tam} ---")

        start_init_time = datetime.now()
        current_board_df = init_tabul_spark(tam, spark)
        end_init_time = datetime.now()
        init_duration = (end_init_time - start_init_time).total_seconds()

        num_generations = 2 * (tam - 3)

        start_comp_time = datetime.now()
        for i in range(num_generations):
            current_board_df = uma_vida_spark(current_board_df, tam, sc)
        end_comp_time = datetime.now()
        comp_duration = (end_comp_time - start_comp_time).total_seconds()

        start_final_check_time = datetime.now()
        valid = correto_spark(current_board_df, tam)
        end_final_check_time = datetime.now()
        final_check_duration = (end_final_check_time - start_final_check_time).total_seconds()

        total_duration = init_duration + comp_duration + final_check_duration
        resultado = {
            "tam": tam,
            "valid": valid,
            "init_time": init_duration,
            "comp_time": comp_duration,
            "final_check_time": final_check_duration,
            "total_time": total_duration
        }
        resultados.append(resultado)

    spark.stop()
    return resultados