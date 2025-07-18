import subprocess

def process_with_mpi(powmin, powmax):
    try:
        cmd = ["mpirun", "-np", "4", "./engines/mpi_engine", str(powmin), str(powmax)]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=30)

        if result.returncode != 0:
            return f"[ERRO MPI]: {result.stderr}"

        return result.stdout
    except subprocess.TimeoutExpired:
        return "[ERRO MPI]: Tempo de execução excedido"
    except Exception as e:
        return f"[ERRO MPI]: {str(e)}"