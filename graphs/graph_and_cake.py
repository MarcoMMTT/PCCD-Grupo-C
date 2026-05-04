import pandas as pd
import matplotlib
# Obligamos a matplotlib a trabajar en "modo silencioso" (sin ventanas)
# Esto soluciona el error de FigureCanvasAgg de raíz.
matplotlib.use('Agg') 
import matplotlib.pyplot as plt
import numpy as np
import os

# 1. Leer los datos
nombres_columnas = ['ID', 'Proceso', 'Tiempo_SC', 'Tiempo_Salida']
nombre_archivo = 'salida.txt'

if not os.path.exists(nombre_archivo):
    print(f"❌ Error: No encuentro el archivo '{nombre_archivo}' en esta carpeta.")
    print(f"Asegúrate de que estás ejecutando el script desde el mismo directorio donde está el .txt")
    exit(1)

print(f"✅ Leyendo datos de {nombre_archivo}...")
df = pd.read_csv(nombre_archivo, header=None, names=nombres_columnas)

# 2. Preparar los datos
# Barras: Promedios
df_promedios = df.groupby('Proceso')[['Tiempo_SC', 'Tiempo_Salida']].mean()

# Tarta: Totales (SC + Salida)
df['Tiempo_Total'] = df['Tiempo_SC'] + df['Tiempo_Salida']
df_totales = df.groupby('Proceso')['Tiempo_Total'].sum()

# 3. Dibujar las gráficas
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# --- GRÁFICO DE BARRAS ---
x = np.arange(len(df_promedios.index))
ancho_barra = 0.35

ax1.bar(x - ancho_barra/2, df_promedios['Tiempo_SC'], ancho_barra, label='Tiempo SC', color='#3498db')
ax1.bar(x + ancho_barra/2, df_promedios['Tiempo_Salida'], ancho_barra, label='Tiempo Salida', color='#e74c3c')

ax1.set_ylabel('Microsegundos (Promedio)')
ax1.set_title('Tiempo Promedio por Tipo de Proceso')
ax1.set_xticks(x)
ax1.set_xticklabels(df_promedios.index, rotation=45)
ax1.legend()
ax1.grid(axis='y', linestyle='--', alpha=0.7)

# --- GRÁFICO DE TARTA ---
ax2.pie(df_totales, labels=df_totales.index, autopct='%1.1f%%', startangle=90, 
        colors=plt.cm.Set3.colors, wedgeprops={'edgecolor': 'black'})
ax2.set_title('Distribución del Tiempo Total Acumulado')

plt.tight_layout()

# 4. Guardar como imagen en lugar de abrir ventana
nombre_imagen = 'graficas_resultado.png'
plt.savefig(nombre_imagen, dpi=300, bbox_inches='tight')

print(f"🎉 ¡Éxito! Se ha creado la imagen '{nombre_imagen}' en tu carpeta.")
print(f"Abre ese archivo de imagen para ver tus gráficas.")