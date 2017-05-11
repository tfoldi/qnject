# Optimizer webservice

This is a flask-based microservice that allows triggering different steps in the optimization pipeline that need to
use Desktop and the injector.


## Basic usage

#### Set up the configuration

Currently the configuration for the steps is stored in the microservice python file itself:

```python

# Configuration for the QNject bits
injectorConfig = {
    # Path to the Injector executable for the target architecture
    "injector": "C:\\tmp\\_builds\\qnject64\\Injector64.exe",
    # Path to the QNject dll for the target architecture
    "dll": "c:\\tmp\\_builds\\qnject64\\vaccine\\Release\\qnject.dll",
    # The process name we want to hook.
    "process-name": "tableau.exe",
}

# Configuration for launching Tableau
tableauConfig = {
    # The path to the tableau.exe executable
    "tableau.exe": "C:\\Program Files\\Tableau\\Tableau 10.2\\bin\\tableau.exe",
}

# The
baseUrl = "http://localhost:8000/api"
```

TODO: move the config to a separate file


#### Start the service

```bash
python service.py
```

#### Trigger an optimize

```bash

# Open tableau, load <LOCAL_PATH>, wait for <SLEEP_SECONDS> before injection, then trigger `Optimize`, `Save`, `Exit`
# The input file is overwritten.
curl http://localhost:5000/optimize?sleep=<SLEEP_SECONDS>&file=<LOCAL_PATH>

# Sleep for 10 seconds by default
curl http://localhost:5000/optimize?file=<LOCAL_PATH>
```

Example:

```bash
curl http://localhost:5000/optimize?file=c:\tmp\packaged_tv_2.twbx

# [[{"text": "&Optimize", "result": ["ok"], "address": "0x0000000055930770"}],
#  [{"text": "&Save", "result": ["ok"], "address": "0x000000000E7F0B00"}],
#  [{"text": "E&xit", "result": ["ok"], "address": "0x000000000E7F1180"}]]
```



Note: For now the file to be optimized needs to be present on a locally accessable Windows Path (either local or via windows sharing).



### HTTP endpoints for the optimizer



#### Save & Optimize

Triggering save and optimize for an already injected Tableau.

```
http://localhost:5000/triggers/save -> 200
[[{"text": "&Optimize", "result": ["ok"], "address": "0x0000000053469B80"}], [{"text": "&Save", "result": ["ok"], "address": "0x000000000CB2CCD0"}]]
```


#### Triggering injection of the qnject DLL

```
http://localhost:5000/triggers/do-inject
```



#### Starting Tableau

```
http://localhost:5000/triggers/do-inject&file=PATH_ON_TARGET_MACHINE
```


# Command-line interface for triggering menu commands

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




