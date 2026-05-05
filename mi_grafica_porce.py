import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import sys
import os

def leer_datos(fichero):
    x, centro, derecha = [], [], []
    with open(fichero, 'r') as f:
        for linea in f:
            linea = linea.strip()
            if not linea:
                continue
            partes = linea.replace(';', ',').split(',')
            if len(partes) >= 3:
                x.append(float(partes[0]))
                centro.append(float(partes[1]))
                derecha.append(float(partes[2]))
    return x, centro, derecha

def graficar(fichero):
    x, centro, derecha = leer_datos(fichero)

    porcentajes = [(d / c) * 100 for d, c in zip(derecha, centro)]

    plt.figure(figsize=(9, 5))
    plt.plot(x, porcentajes, marker='o', linewidth=2, markersize=6, color='steelblue')

    # Anotar cada punto con su valor
    for xi, p in zip(x, porcentajes):
        plt.annotate(f'{p:.1f}%', xy=(xi, p), textcoords='offset points',
                     xytext=(0, 10), ha='center', fontsize=10)

    plt.xlabel('Número de Nodos', fontsize=12)
    plt.ylabel('Porcentaje de Tiempo de Sincronización (%)', fontsize=12)
    plt.title('Porcentaje de Tiempo de Sincronización en función del número de nodos', fontsize=13)
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.legend()

    ax = plt.gca()
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda val, _: f'{val:.1f}%'))

    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Uso: python grafica_porcentaje.py <fichero.txt>")
        print("Ejemplo: python grafica_porcentaje.py datos.txt")
        sys.exit(1)

    graficar(sys.argv[1])