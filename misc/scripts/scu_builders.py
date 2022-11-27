"""Functions used to generate scu build source files during build time

"""
# from platform_methods import subprocess_main
# import re
import glob, os
import math
from pathlib import Path

base_dir = os.path.abspath("../../") + "/"


def find_files_in_directory(dir_filename, subdir_filename, mylist, extension, exceptions):
    abs_dir_filename = base_dir + dir_filename + "/" + subdir_filename

    if not os.path.isdir(abs_dir_filename):
        print("ERROR " + abs_dir_filename + " not found.")
        return mylist

    os.chdir(abs_dir_filename)

    subdir_filename_slashed = ""
    if subdir_filename != "":
        subdir_filename_slashed = subdir_filename + "/"

    for file in glob.glob("*." + extension):

        simple_name = Path(file).stem
        if not simple_name in exceptions:

            li = '#include "../' + subdir_filename_slashed + file + '"'
            mylist += [li]

    return mylist


def write_outfile(file_count, mylist, start_line, end_line, out_directory, out_filename_prefix, extension):

    out_directory = os.path.abspath(out_directory)

    if not os.path.isdir(out_directory):
        # create
        os.mkdir(out_directory)
        if not os.path.isdir(out_directory):
            print("ERROR " + out_directory + " could not be created.")
            return
        print("CREATING directory " + out_directory)

    file_text = ""

    for l in range(start_line, end_line):
        if l < len(mylist):
            line = mylist[l]
            li = line + "\n"
            file_text += li

    # print(file_text)

    num_string = ""
    if file_count > 0:
        num_string = "_" + str(file_count)

    out_filename = out_directory + "/" + out_filename_prefix + num_string + ".gen." + extension
    print("generating: " + out_filename)

    # return
    out_file = open(out_filename, "w")
    if not out_file.closed:
        out_file.write(file_text)
        out_file.close()


def process_directory(directories, out_filename, num_output_files=1, exceptions=[], extension="cpp"):
    if len(directories) == 0:
        return

    mylist = []

    main_dir = directories[0]
    dir_filename = base_dir + main_dir

    # main directory (first)
    mylist = find_files_in_directory(main_dir, "", mylist, extension, exceptions)

    # sub directories
    for d in range(1, len(directories)):
        mylist = find_files_in_directory(main_dir, directories[d], mylist, extension, exceptions)

    mylist = sorted(mylist)

    # calculate how many lines to write in each file
    total_lines = len(mylist)
    lines_per_file = math.floor(total_lines / num_output_files)
    lines_per_file = max(lines_per_file, 1)

    start_line = 0
    file_number = 0

    for file_count in range(0, num_output_files):
        end_line = start_line + lines_per_file

        # special case to cover rounding error in final file
        if file_count == (num_output_files - 1):
            end_line = len(mylist)

        out_directory = dir_filename + "/scu/"
        out_filename_prefix = "scu_" + out_filename
        write_outfile(file_count, mylist, start_line, end_line, out_directory, out_filename_prefix, extension)

        start_line = end_line


print("Generating SCU build files")
process_directory(["editor"], "editor", 1)
process_directory(["editor/import"], "editor_import")
process_directory(["editor/plugins"], "editor_plugins")
process_directory(["scene"], "scene")
process_directory(["scene/2d"], "scene_2d")
process_directory(["scene/3d"], "scene_3d", 1)
process_directory(["scene/animation"], "scene_animation")
process_directory(["scene/gui"], "scene_gui", 1)
process_directory(["scene/main"], "scene_main")
process_directory(["scene/resources"], "scene_resources")
process_directory(["core"], "core")
process_directory(["core/math"], "core_math")
process_directory(["core/os"], "core_os")
process_directory(["core/io"], "core_io")
process_directory(["core/crypto"], "core_crypto")
process_directory(["servers", "arvr", "camera", "visual", "visual/portals", "audio", "audio/effects"], "servers")
# process_directory(["servers/visual", "portals"], "servers_visual")
# process_directory(["servers/visual/portals"], "servers_visual_portals")
process_directory(["servers/physics_2d"], "servers_physics_2d")
process_directory(["servers/physics", "joints"], "servers_physics")
# process_directory(["servers/audio"], "servers_audio")
# process_directory(["servers/audio/effects"], "servers_audio_effects")

process_directory(["drivers/gles2"], "drivers_gles2")
process_directory(["drivers/gles3"], "drivers_gles3")
process_directory(["drivers/unix"], "drivers_unix")
process_directory(["drivers/png"], "drivers_png")

process_directory(["main"], "main")
process_directory(["main/tests"], "main_tests")


process_directory(["modules/bullet"], "modules_bullet")
process_directory(["modules/gltf"], "modules_gltf")
process_directory(["modules/navigation"], "modules_navigation")
process_directory(["modules/visual_script"], "modules_visual_script")
process_directory(["modules/webrtc"], "modules_webrtc")
process_directory(["modules/websocket"], "modules_websocket")
process_directory(["modules/mbedtls"], "modules_mbedtls")
process_directory(["modules/gridmap"], "modules_gridmap")


process_directory(["modules/csg"], "modules_csg")
process_directory(["modules/gdscript"], "modules_gdscript")
process_directory(["modules/gdscript/language_server"], "modules_gdscript_language_server")
process_directory(["modules/fbx", "tools/", "data", "fbx_parser"], "modules_fbx")
process_directory(["modules/gdnative", "android", "gdnative", "nativescript"], "modules_gdnative")
process_directory(["modules/gdnative/arvr"], "modules_gdnative_arvr")
process_directory(["modules/gdnative/pluginscript"], "modules_gdnative_pluginscript")
process_directory(["modules/gdnative/net"], "modules_gdnative_net")

process_directory(
    ["thirdparty/embree/common", "sys", "math", "simd", "lexers", "tasking"],
    "thirdparty_embree_common",
    1,
    ["tokenstream"],
)
process_directory(
    ["thirdparty/embree/kernels", "common", "geometry", "builders"], "thirdparty_embree_kernels", 1, ["rtcore"]
)

process_directory(["thirdparty/oidn", "common", "core", "mkl-dnn/src/common", "weights"], "thirdparty_oidn")
process_directory(
    ["thirdparty/bullet/BulletSoftBody", "BulletReducedDeformableBody"],
    "thirdparty_bullet_bulletsoftbody",
    1,
    "btSoftRigidDynamicsWorld",
)
process_directory(["thirdparty/brotli", "common", "dec"], "thirdparty_brotli", 1, [], "c")
process_directory(
    ["thirdparty/mbedtls", "library"],
    "thirdparty_mbedtls",
    1,
    ["chacha20", "chachapoly", "ctr_drbg", "camellia", "gcm", "sha512", "ssl_ciphersuites", "ssl_srv"],
    "c",
)
process_directory(
    [
        "thirdparty/libwebp",
        "src/dec",
        "src/demux",
        "src/dsp",
        "src/enc",
        "src/mux",
        "src/utils",
        "src/webp",
        "sharpyuv",
    ],
    "thirdparty_webp",
    1,
    [
        "sharpyuv",
        "demux",
        "enc",
        "enc_sse2",
        "quant_dec",
        "lossless_enc",
        "lossless_sse2",
        "ssim_sse2",
        "analysis_enc",
        "filter_enc",
        "picture_tools_enc",
        "predictor_enc",
        "quant_enc",
        "picture_psnr_enc",
        "tree_enc",
        "webp_enc",
        "anim_encode",
        "quant_levels_dec_utils",
    ],
    "c",
)
process_directory(
    ["thirdparty/recastnavigation/Recast", "Source"], "thirdparty_recast", 1, ["RecastMesh", "RecastContour"]
)


# The following contain namespace pollution and are not straightforward

# These must be handled manually

# These contain too much bad code to convert
# process_directory("thirdparty/recastnavigation/Recast/Source", "thirdparty_recast")
# process_directory(["thirdparty/oidn/", "common/", "core/", "weights/", "mkl-dnn/src/common/", "mkl-dnn/src/cpu/"], "thirdparty_oidn")
# process_directory("platform/x11", "platform_x11", 1, ["godot_x11", "tts_linux", "key_mapping_x11"])
