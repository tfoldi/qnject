## Get help

```
➜ ./tde_optimize.py --help
Usage: tde_optimize.py [OPTIONS] [ACTIONS]...

  Connect to a QNjects server injected into Tableau Desktop and runs
  Optimize then Save on a workbook

  The 'ACTIONS' are the actions to search for by text (they will be executed
  in the same order).

  Examples:

  ./env/bin/python tde_optimize.py --help

  ./env/bin/python tde_optimize.py

  ./env/bin/python tde_optimize.py "&Save" --server http://localhost:12345

Options:
  --server TEXT  The QNject server to connect to
  --help         Show this message and exit.

```

