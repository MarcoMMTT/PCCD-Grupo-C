import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("metricas_simulacion_solo_pagos.csv")

df["totalProcesos"] = df["numNodos"] * df["numPagos"]
df["sobrecoste_s"] = df["tiempoReal_s"] - df["tiempoIdeal_s"]

plt.figure()
plt.plot(df["totalProcesos"], df["tiempoIdeal_s"], marker="o", label="Tiempo ideal")
plt.plot(df["totalProcesos"], df["tiempoReal_s"], marker="o", label="Tiempo real")
plt.xlabel("Número total de pagos")
plt.ylabel("Tiempo de simulación (s)")
plt.title("Tiempo ideal vs tiempo real")
plt.legend()
plt.grid(True)
plt.savefig("tiempo_simulacion.png")
plt.show()

plt.figure()
plt.plot(df["totalProcesos"], df["sobrecoste_s"], marker="o")
plt.xlabel("Número total de pagos")
plt.ylabel("Sobrecoste (s)")
plt.title("Sobrecoste por sincronización")
plt.grid(True)
plt.savefig("sobrecoste_simulacion.png")
plt.show()