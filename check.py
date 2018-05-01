import numpy as np
import sys

if __name__ == '__main__':
    if len(sys.argv) < 1:
        print("Usage: python check.py Width")

    width = int(sys.argv[1])

    names = ["serial", "dynamic", "static"]

    serial = np.fromfile("out/serial", np.int, width * width)

    for xname in ("dynamic", "static"):
        x = np.fromfile("out/" + xname, np.int, width * width)
        assert (serial == x).all(), "%s does not match serial" % (xname)
        
    print("All versions mutually consistent.")
