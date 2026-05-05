import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("metricas_rafagas_2.csv")
df = df[df["tiempoEntreRafagas_ms"] == 10].copy()

# Quitar caso "sin ráfagas" de la línea principal si molesta
# df = df[df["tamRafaga"] < 1000000]

plt.figure(figsize=(9, 5))

for pagos in sorted(df["numPagos"].unique()):
    sub = df[df["numPagos"] == pagos].sort_values("tamRafaga")
    plt.plot(
        sub["tamRafaga"],
        sub["tiempoReal_s"],
        marker="o",
        label=f"{pagos} pagos/nodo"
    )

plt.xscale("log")
plt.xlabel("Tamaño de ráfaga")
plt.ylabel("Tiempo real de simulación (s)")
plt.title("Impacto del tamaño de ráfaga según pagos por nodo")
plt.legend()
plt.grid(True, which="both")
plt.tight_layout()
plt.savefig("rafaga_vs_pagos_por_nodo.png", dpi=200)
plt.show()