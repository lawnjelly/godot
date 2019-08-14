#!/usr/bin/python
import os

from os import listdir
from os.path import isfile, join, isdir

def process_folder(depth, szBaseDir, szSubDir, szExtension, bRecursive, files, IgnoreList):

    szFullPath = os.path.abspath("../../" + szBaseDir + szSubDir)
    print ("FullPath : " + szFullPath)

    # all files and directorys in directory
    filenames_orig = sorted(listdir(szFullPath))
    
    # remove those on ignore list
    filenames = []
    for filename in filenames_orig:
        bOK = True
        for ignore in IgnoreList:
            #print ("is " + ignore + " in " + filename)
            if (filename.startswith(ignore)):
            #if ignore in filename:
                bOK = False
                break
        
        if bOK:
            filenames.append(filename)
    
    # only those files we are interested in
    filenames_ext = [ filename for filename in filenames if filename.endswith("." + szExtension) ]
    
    # add them to the file list
    for filename in filenames_ext:
        files.append(os.path.join(szSubDir, filename))
        sz = ""
        for t in range (depth+1):
            sz += "\t"
        print (sz + filename)
        
    # if recursive, go through sub directorys
    if bRecursive:
        for filename in filenames:
            sub_path = os.path.join(szFullPath, filename)
            
            if isdir(sub_path):
                process_folder(depth + 1, szBaseDir, os.path.join(szSubDir, filename), szExtension, bRecursive, files, IgnoreList)
            

def process(szDir, szExtension, szOutput, IgnoreList = [], bRecursive = False):
    szOutput = "SCU_" + szOutput
    print ("Process " + szDir + ", " + szExtension + " > " + szOutput)
    
    files = []
    
    process_folder(0, szDir, "", szExtension, bRecursive, files, IgnoreList)
    
    write_files(szDir, files, szOutput)
    #print (files)
    
def write_files(szDir, files, szOutput):
    f = open(szOutput, "w+")

    f.write("// Single Compilation Unit\n")
    f.write("#define SCU_IDENT(x) x\n")
    f.write("#define SCU_XSTR(x) #x\n")
    f.write("#define SCU_STR(x) SCU_XSTR(x)\n")

    f.write("#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))\n")
    f.write("#define SCU_DIR " + szDir + "\n\n")
    
    for fi in files:
        f.write("#include SCU_PATH(SCU_DIR," + fi + ")\n")
        
    f.close()

def process_ignore(szDir, szExtension, szOutput, szIgnore, bRecursive = False):
    ignore_list = []
    ignore_list.append(szIgnore)
    process(szDir, szExtension, szOutput, ignore_list, bRecursive)

# fire off creation of the unity build files
process("main/", "cpp", "main.cc")
process("editor/", "cpp", "editor.cc")
process("editor/doc/", "cpp", "editor_doc.cc")
process("editor/fileserver/", "cpp", "editor_fileserver.cc")
process("editor/import/", "cpp", "editor_import.cc")
process_ignore("editor/plugins/", "cpp", "editor_plugins.cc", "script_text_editor.cpp")
process("modules/bullet/", "cpp", "modules_bullet.cc")
process_ignore("thirdparty/assimp/code/", "cpp", "thirdparty_assimp.cc", "FBX", True)

