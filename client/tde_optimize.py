import requests
from collections import namedtuple
import click

Config = namedtuple("QNjectConfig", "baseUrl")


# CONFIG ----------------------------------------


def url(config, path):
    return config.baseUrl + path

defaultConfig = Config(baseUrl="http://localhost:8000/api")

# GET MENU ----------------------------------------


# Tries to get the main menuBars for the application from the provided server
def get_menu(config):
    print( "--> Getting menu data from server")
    r = requests.get(url(config, "/qwidgets/main-menu"))
    print( "<-- Got {}".format(r.status_code))
    if r.status_code > 299:
        raise "Response with status code: " + str(r.status_code)

    return r.json()


# Returns all actions from the menus that match pred
def find_menu_action(pred, menus):
    acts = []

    for menu in menus:
        for action in menu["actions"]:
            if pred(action):
                acts.append(action)


        acts = acts + find_menu_action(pred, menu["childMenus"])

    return acts

# PREDICATES --------------------------------------

# Returns true if the selected action is "Optimize"
def is_action_optimize(action):
    return action["action"]["text"] == "&Optimize"

# Returns true if the selected action is "Optimize"
def is_action_save(action):
    return action["action"]["text"] == "&Save"

# TRIGGER --------------------------------------

# Triggers a QAction by address
def trigger_action_at(config, address):
    reqUrl = url(config, "/qwidgets/by-address-unsafe/trigger/" + address)
    print("--> Trying to send to {}".format(reqUrl))
    r = requests.get(reqUrl)
    print("<-- Got {}".format(r.status_code))
    if r.status_code > 299:
        raise "Response with status code: " + str(r.status_code)

    return r.json()


# Triggers a list of actions
def trigger_actions(config, actions):
    o = []
    for action in actions:
        address = action["object"]["address"]
        text = action["action"]["text"]
        print( "--> Triggering action: {} @ {}".format(text, address ))
        o.append({"text": text, "address": address, "result":trigger_action_at(config, address)})

    return o

# UI --------------------------------------


def find_and_trigger_actions(config, actionNames, menus):
    o = []
    for actionName in actionNames:
        actionsFound = find_menu_action(lambda a: a["action"]["text"] == actionName, menus)
        o.append(actionsFound)
        print( "~~+ Added >>{}<< to triggered actions".format(
            map(lambda a: a["action"]["text"], actionsFound )))

    return map(lambda a: trigger_actions(config, a), o)



if __name__ == '__main__':

    @click.command()
    @click.option('--server', default="http://localhost:8000/api", help='The QNject server to connect to')
    @click.argument('actions', nargs=-1)
    def main(actions, server):
        """ Connect to a QNjects server injected into Tableau Desktop and runs Optimize then Save on a workbook 
        
        The 'ACTIONS' are the actions to search for by text (they will be executed in the same order).
        
        Examples:

        ./env/bin/python tde_optimize.py --help
        
        ./env/bin/python tde_optimize.py
        
        ./env/bin/python tde_optimize.py "&Save" --server http://localhost:12345
                
        """
        config = Config(baseUrl=server)

        if len(actions) == 0:
            actions = ["&Optimize", "&Save"]

            # Call the actions
        res = find_and_trigger_actions(config, actions, get_menu(config))

        # Print results
        for act in res:
            for v in act:
                print("'{}' @ {} = {}".format(v["text"], v["address"], v["result"]))


    main()


