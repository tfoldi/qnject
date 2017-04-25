import requests
from collections import namedtuple


Config = namedtuple("QNjectConfig", "baseUrl")


# CONFIG ----------------------------------------


def url(config, path):
    return config.baseUrl + path

defaultConfig = Config(baseUrl="http://localhost:8000/api")

# GET MENU ----------------------------------------


def get_menu(config):
    print( "--> Getting menu data from server")
    r = requests.get(url(config, "/qwidgets/main-menu"))
    print( "<-- Got reply from server {}".format(r.status_code))
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


def trigger_action_at(config, address):
    reqUrl = url(config, "/qwidgets/by-address-unsafe/trigger/" + address)
    print("--> Trying to send to {}".format(reqUrl))
    r = requests.get(reqUrl)
    print("<-- Got {}".format(r.status_code))
    if r.status_code > 299:
        raise "Response with status code: " + str(r.status_code)

    return r.json()


def trigger_actions(config, actions):
    o = []
    for action in actions:
        address = action["object"]["address"]
        print( "--> Triggering action: {} @ {}".format(action["action"]["text"], address ))
        o.append(trigger_action_at(config, address))

    return o

# UI --------------------------------------


def get_needed_actions(config, menus):
    # both may return multiple actions for now. So we trigger all of them
    optimizes = find_menu_action(is_action_optimize, menus)
    saves = find_menu_action(is_action_save, menus)

    return {"optimize": trigger_actions(config, optimizes),
            "save": trigger_actions(config, saves)
            }


    # return {"save": saves, "optimize": optimizes}


print( get_needed_actions(defaultConfig, get_menu(defaultConfig)))
