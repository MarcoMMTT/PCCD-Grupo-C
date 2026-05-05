import pandas as pd
import matplotlib.pyplot as plt

CSV = "metricas_rafagas.csv"

df = pd.read_csv(CSV).dropna()

# Separar "sin ráfagas" si tamRafaga >= totalProcesos
df["sin_rafagas"] = df["tamRafaga"] >= df["totalProcesos"]

df_con = df[~df["sin_rafagas"]].copy()
df_sin = df[df["sin_rafagas"]].copy()

# =========================
# 1. Impacto del tamaño de ráfaga
# =========================

plt.figure(figsize=(9, 5))

for pausa in sorted(df_con["tiempoEntreRafagas_ms"].unique()):
    sub = df_con[df_con["tiempoEntreRafagas_ms"] == pausa].sort_values("tamRafaga")
    plt.plot(
        sub["tamRafaga"],
        sub["tiempoReal_s"],
        marker="o",
        label=f"{pausa} ms pausa"
    )

if not df_sin.empty:
    tiempo_sin = df_sin["tiempoReal_s"].mean()
    plt.axhline(
        tiempo_sin,
        linestyle="--",
        label=f"Sin ráfagas ≈ {tiempo_sin:.2f}s"
    )

plt.xscale("log")
plt.xlabel("Tamaño de ráfaga (escala log)")
plt.ylabel("Tiempo real de simulación (s)")
plt.title("Impacto del tamaño de ráfaga")
plt.legend()
plt.grid(True, which="both")
plt.tight_layout()
plt.savefig("grafica_rafagas_tamano.png", dpi=200)
plt.show()


# =========================
# 2. Impacto de la pausa entre ráfagas
# =========================

plt.figure(figsize=(9, 5))

sub = df[df["tamRafaga"] == 500].sort_values("tiempoEntreRafagas_ms")

plt.plot(
    sub["tiempoEntreRafagas_ms"],
    sub["tiempoReal_s"],
    marker="o",
    label="Tiempo real"
)

plt.plot(
    sub["tiempoEntreRafagas_ms"],
    sub["idealRafagas_s"],
    marker="o",
    linestyle="--",
    label="Ideal con ráfagas"
)

plt.xlabel("Tiempo entre ráfagas (ms)")
plt.ylabel("Tiempo (s)")
plt.title("Impacto de la pausa entre ráfagas")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("grafica_rafagas_pausa.png", dpi=200)
plt.show()


# =========================
# 3. Sobrecoste sobre el ideal con ráfagas
# =========================

plt.figure(figsize=(9, 5))

for pausa in sorted(df_con["tiempoEntreRafagas_ms"].unique()):
    sub = df_con[df_con["tiempoEntreRafagas_ms"] == pausa].sort_values("tamRafaga")
    plt.plot(
        sub["tamRafaga"],
        sub["sobrecosteRafagas_s"],
        marker="o",
        label=f"{pausa} ms pausa"
    )

plt.xscale("log")
plt.axhline(0, linestyle="--")
plt.xlabel("Tamaño de ráfaga (escala log)")
plt.ylabel("Desviación respecto al ideal con ráfagas (s)")
plt.title("Sobrecoste del algoritmo usando ráfagas")
plt.legend()
plt.grid(True, which="both")
plt.tight_layout()
plt.savefig("grafica_sobrecoste_rafagas.png", dpi=200)
plt.show()


# =========================
# 4. Comparativa resumen
# =========================

resumen = df.sort_values("tiempoReal_s")[
    [
        "numNodos",
        "numPagos",
        "totalProcesos",
        "tamRafaga",
        "tiempoEntreRafagas_ms",
        "idealServicio_s",
        "idealRafagas_s",
        "tiempoReal_s",
        "sobrecosteRafagas_s",
    ]
]

print("\n=== Resultados ordenados por mejor tiempo real ===")
print(resumen.to_string(index=False))

mejor = resumen.iloc[0]
print("\nMejor configuración:")
print(
    f"tamRafaga={int(mejor['tamRafaga'])}, "
    f"pausa={int(mejor['tiempoEntreRafagas_ms'])} ms, "
    f"tiempoReal={mejor['tiempoReal_s']:.3f}s"
)