import tde_optimize
import json
import time
import subprocess
import re
from flask import Flask, request


# CONFIG --------------------------

injectorConfig = {
    # "injector-cmd": "Injector64.exe",
    "injector": "C:\\tmp\\_builds\\qnject64\\Injector64.exe",
    "dll": "c:\\tmp\\_builds\\qnject64\\vaccine\\Release\\qnject.dll",
    "process-name": "tableau.exe",
}


tableauConfig = {
    "tableau.exe": "C:\\Program Files\\Tableau\\Tableau 10.2\\bin\\tableau.exe",
}

# Pre-configure the qnject handler
baseUrl = "http://localhost:8000/api"
qnjectConfig = tde_optimize.Config(baseUrl=baseUrl)


# APP ---------------------------


app = Flask(__name__)

def getInjectorCmd(cfg):
    return [cfg["injector"],
            "--process-name", cfg["process-name"],
            "--module-name", cfg["dll"],
            "--inject"]



# Tries to inject the dll.
def try_injection(cfg):
    is_successful = re.compile(r"Successfully injected module")
    result = ""

    # if the proc gives an empty exit code, it will throw, so wrap it
    try:
        cmd = getInjectorCmd(cfg)
        result = subprocess.check_output(cmd, shell=True)
        if is_successful.search( result ):
            return {"ok": { "msg": "Succesfully injected", "output": result}}
        else:
            return {"error": {"msg": "Failed injection", "output": result}}

    except subprocess.CalledProcessError as e:
        return {"error": {"msg": "Failed injection", "rc": e.returncode}}




def start_tableau(cfg, twb):
    try:
        cmd = ["cmd.exe", "/c", "start", cfg["tableau.exe"], twb]
        print("Running command: {}".format(cmd))
        result = subprocess.check_output(cmd, shell=True)
        return {"ok": { "msg": "Tableau started", "output": result}}

    except subprocess.CalledProcessError as e:
        return {"error": { "msg": "Tableau start failed", "rc": e.returncode}}
    # cmd / c
    # start
    # "C:\Program Files\Tableau\Tableau 10.2\bin\tableau.exe" "c:\tmp\_builds\qnject64\test\packaged_tv.twbx"





# MAKE SURE WE ARE OK AND CAN BE DEBUGGED REMOTELY

@app.route("/")
def root():
    return "TDE Optimizer"


@app.route("/ping")
def ping(): return "pong"


# Get the config for debugging
@app.route("/config")
def get_config():
    return str(json.dumps({"baseUrl": qnjectConfig.baseUrl}))


# TRIGGERING --------------------------

@app.route("/triggers/save")
def trigger_save():
    actions = ["&Optimize", "&Save"]

    # Call the actions
    menu = tde_optimize.get_menu(qnjectConfig)
    return json.dumps(tde_optimize.find_and_trigger_actions(qnjectConfig, actions, menu))


@app.route("/triggers/do-inject")
def trigger_injection():
    return json.dumps(try_injection(injectorConfig))

@app.route("/triggers/start-tableau")
def trigger_open_tableau():
    fn = request.args.get('file', '')
    if fn == "":
        return json.dumps({"error": {"msg": "a 'file' url parameter must be provided."}}), 405
    else:
        return json.dumps(start_tableau(tableauConfig, fn))
# tde_optimize.find_and_trigger_actions()


def num(s, default=0):
    try:
        return int(s)
    except ValueError:
        return default


@app.route("/optimize")
def trigger_optimize():
    fn = request.args.get('file', '')
    sleepSeconds = num(request.args.get('sleep', '10'), 10)





    if fn == "":
        return json.dumps({"error": {"msg": "a 'file' url parameter must be provided."}})
    else:

        print("--> Starting tableau with: {0}".format(fn))
        res = start_tableau(tableauConfig, fn)
        # Error checking
        if "error" in res:
            return json.dumps(res), 500



        print("--> Wating for {0} seconds".format(sleepSeconds))
        time.sleep(sleepSeconds)


        print("--> injecting")
        res = try_injection(injectorConfig)
        if "error" in res:
            return json.dumps(res), 500


        print("--> Optimize, save and exit")
        actions = ["&Optimize", "&Save", "E&xit"]

        # Call the actions
        menu = tde_optimize.get_menu(qnjectConfig)
        res = tde_optimize.find_and_trigger_actions(qnjectConfig, actions, menu)
        if "error" in res:
            return json.dumps(res), 500

        # We should be OK at this point
        return json.dumps(res), 200







# MAIN --------------------------


if __name__ == "__main__":
    app.run()
