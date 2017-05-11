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




## HTTP endpoints


Triggering save and optimize for an already injected Tableau.

```
http://localhost:5000/triggers/save
200 - [[{"text": "&Optimize", "result": ["ok"], "address": "0x0000000053469B80"}], [{"text": "&Save", "result": ["ok"], "address": "0x000000000CB2CCD0"}]]
```



Triggering injection

```
http://localhost:5000/triggers/do-inject
```



Starting Tableau

```
http://localhost:5000/triggers/do-inject
```