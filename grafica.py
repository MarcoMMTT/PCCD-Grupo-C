import numpy as np
import matplotlib.pyplot as plt

# Cargamos sin nombres de columnas para que sea más directo
# Usamos dtype=None para que detecte strings y números automáticamente
datos = np.genfromtxt('salida.txt', 
                      delimiter=',', 
                      dtype=None, 
                      encoding='utf-8', 
                      autostrip=True)

# datos[:, 1] es la segunda columna (los tipos: Pagos, Consultas...)
tipos = datos['f1'] # genfromtxt asigna nombres f0, f1, f2... por defecto
tiempos = datos['f2'] # columna 2: t_sc

# CREA LA MÁSCARA
# Si quieres ver las CONSULTAS (que son casi todo el archivo):
mask = (tipos == "Consultas")

# Si quieres ver TODO LO QUE NO SEA Consultas:
# mask = (tipos != "Consultas")

x_filtrado = tiempos[mask]

# VERIFICACIÓN EN CONSOLA
print(f"Total filas en archivo: {len(datos)}")
print(f"Filas tras filtrar: {len(x_filtrado)}")

if len(x_filtrado) == 0:
    print("La máscara ha borrado todo. Revisa si 'Consultas' está bien escrito.")
else:
    plt.figure(figsize=(12, 5))
    plt.plot(x_filtrado, color='blue', label='Tiempo en SC')
    plt.title('Evolución de Tiempos en Sección Crítica')
    plt.xlabel('Número de llegada del proceso')
    plt.ylabel('Tiempo(us)')
    plt.grid(True)
    plt.legend()
    plt.show()