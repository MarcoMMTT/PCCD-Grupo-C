import pandas as pd
import matplotlib.pyplot as plt

# Leer datos
df = pd.read_csv("metricas_rafagas.csv")

# Filtrar pausa fija
df = df[df["tiempoEntreRafagas_ms"] == 10]

# Ordenar por tamaño de ráfaga
df = df.sort_values("tamRafaga")

# Gráfica
plt.figure()
plt.plot(df["tamRafaga"], df["tiempoReal_s"], marker='o')

plt.xscale("log")
plt.xlabel("Tamaño de ráfaga (escala log)")
plt.ylabel("Tiempo de simulación (s)")
plt.title("Impacto del tamaño de ráfaga en el tiempo de ejecución")

# Línea ideal (opcional pero muy buena)
plt.axhline(y=16, linestyle='--', label="Tiempo ideal ≈ 16s")

plt.legend()
plt.grid(True)
plt.savefig("aaaaaa.png", dpi=200)
plt.show()
plt.show()