import re
import os


GIT_DIR = '\\'.join(os.getcwd().split('\\')[:-1])
C_CODE_FOLDER = "bms-link-FW"
C_CODE_ROOT_DIR = os.path.join(GIT_DIR, C_CODE_FOLDER)



def find_h_files(root_folder):
    h_files = []
    FOLDERS2IGNORE = ["source", ".settings", "Debug", ".launches", "Release", "Test", "targetConfigs"]
    
    # os.walk yields a 3-tuple: (current_dir_path, subdirectories, filenames)
    for dirpath, dirnames, filenames in os.walk(root_folder):
        #skip some folders
        if(dirnames in FOLDERS2IGNORE):
            continue
        for filename in filenames:
            # Check if the file ends with the .h extension
            if filename.endswith('.h'):
                # Combine the folder path and filename to get the full absolute path
                full_path = os.path.join(dirpath, filename)
                h_files.append(full_path)
                
    return h_files
def find_h_from_baseName(AllHFiles, target_hFile:str):
    for i,hfile in enumerate(AllHFiles):
        if target_hFile == os.path.basename(hfile):
            return i
    return -1
def searchDic(Dic, tar_key):
    for key, val in Dic.items():
        if key == tar_key:
            return val
    return None

def getConstFromFile(hFile, idx, deep):
    unProcessed.remove(hFile)
    # AllHFile_Unread.pop(idx)
    Processed.add(hFile)
    print(os.path.basename(hFile), f": {idx=} at {deep=}")
    if deep >20:
        raise ValueError
    
    fileConstants = {}
    dependentHFiles = set()

    # relPath = os.path.relpath(hFile, C_CODE_ROOT_DIR)
    # fileConents = hFile.read()

    inCommentBlock = False
    NotIfDepth = [0]
    InIf = True
    with open(hFile,'r') as hf:
        for line in hf:
            key,val = next(REGEX.items())
            if inCommentBlock:
                if val.match(line):
                    line = val.sub("", line)
                    inCommentBlock = False
                else:
                    continue

            if REGEX["comment_block"].match(line):
                line = REGEX["comment_block_start"].sub("", line)
            if REGEX["comment_block_start"].match(line):
                inCommentBlock = True
                line = REGEX["comment_block_start"].sub("", line)
            if REGEX["comment"].match(line):
                line = REGEX["comment_block_start"].sub("", line)
            if REGEX["only new line"].match(line):
                continue



            m_if = REGEX["if"].match(line) 
            m_defif = REGEX["defif"].match(line)
            m_defnif = REGEX["defnif"].match(line)
            if m_if | m_defif | m_defnif:
                if NotIfDepth > 0:
                    NotIfDepth+=1
                    continue
                TrueFalse =      if_handeler(m_if, fileConstants, dependentHFiles)      if m_if \
                            else defif_handeler(m_if, fileConstants, dependentHFiles)   if m_defif \
                            else not defif_handeler(m_if, fileConstants, dependentHFiles)
                if m_if:
                    TrueFalse = if_handeler(m_if, fileConstants, dependentHFiles)
                if m_defif:
                    TrueFalse = defif_handeler(m_if, fileConstants, dependentHFiles)
                if m_defnif:
                    TrueFalse = not defif_handeler(m_if, fileConstants, dependentHFiles)

                NotIfDepth += 0 if TrueFalse else 1
                continue         

            if REGEX["else"].match(line):
                NotIfDepth = 0 if NotIfDepth == 1 else \
                             1 if NotIfDepth == 0 else NotIfDepth
                continue
            if REGEX["endif"].match(line):
                NotIfDepth -= 1 if NotIfDepth > 0 else 0
                continue

            if NotIfDepth>0:
                continue

            m = REGEX["include"].match(line)
            if m:
                dhf = m.group(1)
                handle_includes(dhf, deep, dependentHFiles)

            

    AllConstaint[hFile] = fileConstants


def handle_includes(dhf, deep, dependentHFiles):
    
    i=find_h_from_baseName(unProcessed, dhf)
    dependentHFiles.add(unProcessed[i])

    if i == -1:
        return
    
    getConstFromFile(unProcessed[i], i, deep+1)

def defif_handeler(m, fileConstants, dependentHFiles):
    define = m.group(0)
    for val in 



unProcessed = set(find_h_files(C_CODE_ROOT_DIR))
Processed = set()
Processing = set()
AllConstaint = {}

for hFile in unProcessed:
    print(os.path.basename(hFile))


REGEX = {   "comment block end" : re.compile(r'^.*\*/'),
            "comment block" : re.compile(r'/\*.*\*/'),
            "comment block start" : re.compile(r'/\*.*$'),
            "comment" : re.compile(r'//.*$'),
            "only new line" : re.compile(r'^\n$'),

            "endif" : re.compile(r'//.*$'),

            
            "else" : re.compile(r'//.*$'),
            "ifdef" : re.compile(r'//.*$'),
            "ifndef" : re.compile(r'//.*$'),
            "include" : re.compile(r'//.*$'),
            "define" : re.compile(r'^\s*#\s*define\s+(\w+)\s+(.+)$'),

}
Deep = 0
while(len(unProcessed) != 0):
    hfile = unProcessed[0]
    FILE_NAME = os.path.basename(hfile)
    getConstFromFile(hfile, 0, 0)
