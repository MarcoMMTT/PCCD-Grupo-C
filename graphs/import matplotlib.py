import matplotlib.pyplot as plt

# Escenarios en el orden esperado
escenarios = [
    "1 Consulta 1 Nodo",
    "N consultas 1 Nodo",
    "1 Consulta N nodos",
    "N consultas N nodos"
]

# Leer archivo
usec_escritor_vals = []

with open("salida.txt", "r") as f:
    for line in f:
        line = line.strip()
        if "Pagos" in line:
            partes = line.split(",")
            
            # Extraer usecEscritor (último campo)
            usec_escritor = int(partes[4])
            usec_escritor_vals.append(usec_escritor)

# Validación básica
if len(usec_escritor_vals) != 4:
    raise ValueError("Se esperaban exactamente 4 entradas con 'Pagos'")

# ---- DIAGRAMA DE BARRAS ----
plt.figure()
plt.bar(escenarios, usec_escritor_vals)
plt.yscale('log')
plt.title("Tiempo usecEscritor por escenario")
plt.xlabel("Escenario")
plt.ylabel("Tiempo (us) - Log")
plt.xticks(rotation=20)
plt.tight_layout()
plt.show()

# ---- DIAGRAMA DE TARTA ----
plt.figure()
plt.pie(usec_escritor_vals, labels=escenarios, autopct='%1.1f%%')
plt.title("Distribución de usecEscritor")
plt.show()