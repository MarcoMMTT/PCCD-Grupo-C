import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("metricas_simulacion_solo_pagos_2.csv")
df = df.dropna()

# Calcular sobrecoste
df["sobrecoste_s"] = df["tiempoReal_s"] - df["tiempoIdeal_s"]
df["sobrecoste_pct"] = 100 * df["sobrecoste_s"] / df["tiempoIdeal_s"]

# 🔹 GRÁFICA 1: Sobrecoste absoluto
plt.figure()
for nodos in sorted(df["numNodos"].unique()):
    sub = df[df["numNodos"] == nodos].sort_values("totalProcesos")
    plt.plot(sub["totalProcesos"], sub["sobrecoste_s"], marker="o", label=f"{nodos} nodos")

plt.xlabel("Número total de procesos")
plt.ylabel("Sobrecoste (segundos)")
plt.title("Sobrecoste absoluto por sincronización")
plt.legend()
plt.grid(True)
plt.savefig("sobrecoste_segundos.png")
plt.show()


# 🔹 GRÁFICA 2: Sobrecoste porcentual (la clave)
plt.figure()
for nodos in sorted(df["numNodos"].unique()):
    sub = df[df["numNodos"] == nodos].sort_values("totalProcesos")
    plt.plot(sub["totalProcesos"], sub["sobrecoste_pct"], marker="o", label=f"{nodos} nodos")

plt.xlabel("Número total de procesos")
plt.ylabel("Sobrecoste (%)")
plt.title("Sobrecoste porcentual del algoritmo")
plt.legend()
plt.grid(True)
plt.savefig("sobrecoste_porcentaje.png")
plt.show()