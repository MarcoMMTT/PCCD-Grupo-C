import numpy as np
from matplotlib import pyplot as plt

id, tipo, tiempo_sc = np.loadtxt('salida.txt',skiprows=1,delimiter=',',usecols=[0,1,2],unpack=True)

mask = (tipo!=4)

x = id[mask]
y = tiempo_sc[mask]

# Configuración del gráfico
plt.figure(figsize=(10, 6))
plt.title('Tiempo hasta entrar en la SC por Proceso')
plt.plot(x, y, marker='o', linestyle='-', color='b')
plt.xlabel('ID del Proceso')
plt.ylabel('Tiempo en Sección Crítica')
plt.grid(True)
plt.show()