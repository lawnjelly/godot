# config.py

def get_doc_path():
    return "doc_classes"

def get_doc_classes():
    return [
    "FastArray_2f32",
    "FastArray_4f32",
    "FastArray_4i32",
    "Vec4_i32",
    ]

def can_build(env, platform):
    return True

def configure(env):
    pass
