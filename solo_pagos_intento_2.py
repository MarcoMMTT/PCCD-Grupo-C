import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("metricas_simulacion_solo_pagos_2.csv")

df["sobrecoste_s"] = df["tiempoReal_s"] - df["tiempoIdeal_s"]
df["sobrecoste_pct"] = 100 * df["sobrecoste_s"] / df["tiempoIdeal_s"]

plt.figure()
for nodos in sorted(df["numNodos"].unique()):
    sub = df[df["numNodos"] == nodos]
    plt.plot(sub["totalProcesos"], sub["tiempoReal_s"], marker="o", label=f"{nodos} nodos")

plt.xlabel("Número total de procesos")
plt.ylabel("Tiempo real de simulación (s)")
plt.title("Tiempo real según procesos totales y número de nodos")
plt.legend()
plt.grid(True)
plt.savefig("tiempo_real_por_nodos.png")
plt.show()

plt.figure()
for nodos in sorted(df["numNodos"].unique()):
    sub = df[df["numNodos"] == nodos]
    plt.plot(sub["totalProcesos"], sub["sobrecoste_pct"], marker="o", label=f"{nodos} nodos")

plt.xlabel("Número total de procesos")
plt.ylabel("Sobrecoste (%)")
plt.title("Sobrecoste de sincronización según número de nodos")
plt.legend()
plt.grid(True)
plt.savefig("sobrecoste_por_nodos.png")
plt.show()