"""`python -m gruntz.win` -> the Windows provisioning bootstrap."""

import sys

from .bootstrap import main

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
