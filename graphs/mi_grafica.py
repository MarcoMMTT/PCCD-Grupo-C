import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import sys
import os

def leer_datos(fichero):
    x, y = [], []
    with open(fichero, 'r') as f:
        for linea in f:
            linea = linea.strip()
            if not linea:
                continue
            partes = linea.replace(';', ',').split(',')
            if len(partes) >= 2:
                x.append(float(partes[0]))
                y.append(float(partes[1]))
    return x, y

def graficar(fichero):
    x, y = leer_datos(fichero)

    plt.figure(figsize=(9, 5))
    plt.plot(x, y, marker='o', linewidth=2, markersize=6, color='steelblue')

    for xi, p in zip(x, y):
        plt.annotate(f'{(p/1000000):.3f}', xy=(xi, p), textcoords='offset points',
                     xytext=(0, 10), ha='center', fontsize=10)

    plt.xlabel('Número de Nodos', fontsize=12)
    plt.ylabel('Tiempo de sincronización', fontsize=12)
    plt.title('Tiempo de sincronización en funcion del numero de nodos', fontsize=14)
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.legend()

    ax = plt.gca()
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda val, _: f'{float(val / 1000000)} s'))

    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Uso: python grafica.py <fichero.txt>")
        print("Ejemplo: python grafica.py datos.txt")
        sys.exit(1)

    graficar(sys.argv[1])