import numpy as np
import matplotlib.pyplot as plt
import sys



def show(binfile, ax, n):
    x = np.fromfile(binfile, np.int, n * n).reshape((n, n))
    ax.imshow(x, cmap="hot", interpolation="bicubic")
    ax.title.set_text(binfile)

if __name__ == '__main__':

    if len(sys.argv) != 3:
        print("Usage: python3 display.py [prefix] [size of grid]")
        sys.exit(0)

    binfile = sys.argv[1]
    n = int(sys.argv[2])

    x = np.fromfile(binfile, np.int32, n * n).reshape((n, n))
    plt.imshow(x, cmap="hot", interpolation="bicubic")
    plt.title(binfile)
    plt.show()
