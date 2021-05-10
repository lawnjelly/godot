# config.py

def get_doc_path():
    return "doc_classes"

def get_doc_classes():
    return [
    "LPortal",
    "LRoom",
    "LRoomManager",
    ]

def can_build(env, platform):
    return True

def configure(env):
    pass
