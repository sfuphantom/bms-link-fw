import re
import os
from dataclasses import dataclass

GIT_DIR = '\\'.join(os.getcwd().split('\\')[:-1])
C_CODE_FOLDER = "bms-link-FW"
C_CODE_ROOT_DIR = os.path.join(GIT_DIR, C_CODE_FOLDER)

FOLDERS2IGNORE = ["BMS_Master", "source", ".settings", "Debug", ".launches", "Release", "Test", "targetConfigs"]
HFILES2IGNORE = {"PhantomHelpers.h", "ltc6811_commands.h", "sci_helpers.h", "spi_helpers.h", "GIO_helpers.h"}
OUTPUT_JSON = "AllHeaderConstants.json"


BLOCK_COMMENT_RE = re.compile(r'/\*.*?\*/', re.DOTALL)
END_BLOCK_COMMENT_RE = re.compile(r'^.*\*/', re.DOTALL)
START_BLOCK_COMMENT_RE = re.compile(r'/\*.*$')
LINE_COMMENT_RE = re.compile(r'//.*')
LINE_CONTINUATION_RE = re.compile(r'\\\r?\n')
EMPTY_LINE_RE = re.compile(r"^\s*$")

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+[<"]([^">]+)[">]')
DEFINE_RE = re.compile(r'^\s*#\s*define\s+(\w+)(\([^)]*\))?\s*(.*)$')
UNDEF_RE = re.compile(r'^\s*#\s*undef\s+(\w+)')
IFDEF_RE = re.compile(r'^\s*#\s*ifdef\s+(\w+)')
IFNDEF_RE = re.compile(r'^\s*#\s*ifndef\s+(\w+)')
IF_RE = re.compile(r'^\s*#\s*if\s+(.*)$')
ELIF_RE = re.compile(r'^\s*#\s*elif\s+(.*)$')
ELSE_RE = re.compile(r'^\s*#\s*else\b')
ENDIF_RE = re.compile(r'^\s*#\s*endif\b')

DEFINED_RE = re.compile(r'defined\s*\(\s*(\w+)\s*\)|defined\s+(\w+)')
TOKEN_RE = re.compile(r'\b[A-Za-z_]\w*\b')
INT_SUFFIX_RE = re.compile(r'\b(0[xX][0-9A-Fa-f]+|\d+)[uUlL]+\b')




def slitpStrTokens(inStr: str) -> list[str]:
    allChars: list[str] = list(inStr.strip())
    tokens: list[str] = []
    currentToken: str = ""

    for char in allChars:
        if char == '"' | char == "'":
            tokens.append(currentToken)
            pre_char = char
            while (char != '"' or char != "'") and pre_char != "\\":
                currentToken += char
                pre_char = char
                
                char = next(allChars, None)

                if pre_char == "\\":
                    char += next(allChars, None)

                if not char:
                    break

            currentToken += c
            tokens.append(currentToken)
            currentToken = ""
            continue

        if char in ["", " ", "\t", "\n"]:
            tokens.append(currentToken)
            currentToken = ""
            continue

        if re.match(["!%^&*()-+=[]{}|<>/?~"],char):
            tokens.append(currentToken)
            currentToken = ""
            tokens.append(char)
            continue 


        if re.match("\w",char):
            currentToken += char

        if not currentToken:
            if char.isdigit():
                c = char
                
                while c.isdigit():
                    currentToken += c
                    c += next(allChars, None)
                    if not c:
                        break

                tokens.append(currentToken)
                currentToken = c
                continue

        if char.isalpha():
            currentToken += char
            continue
        
        

@dataclass(frozen=True, kw_only=True)
class defines():
    Val: float|int|str|bool|None
    type: str



def find_h_files(root_folder):
    h_files = []
    
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
def FindFromBaseName(AllFiles: list[str], target_File:str) -> str:
    for file in AllFiles:
        if target_File == os.path.basename(file):
            return file
    return None
def searchDic(Dic, tar_key):
    for key, val in Dic.items():
        if key == tar_key:
            return val
    return None

def strip_comments_and_join_continuations(text):
    text = BLOCK_COMMENT_RE.sub(' ', text)
    text = LINE_COMMENT_RE.sub('', text)
    # text = LINE_CONTINUATION_RE.sub(' ', text)
    return text

class Progress():

    def __init__(self, All_hFiles: list[str], max_deep: int = 20, maxFileSize_KB: int=100):
        self.unProcessed:   set[str] = set(All_hFiles)
        self.Processed:     set[str] = set()
        self.Processing:    set[str] = set()
        self.AllConstaint:  dict[str, dict[str, defines]] = dict()

        self.keyWords: dict[str, float|int|str|bool|None] = {
            "True":True,
            "False":False,

        }

        self.deep: int = 0
        self.MAXDEEP: int = max_deep
        self.MAXFILESIZE: int = maxFileSize_KB


    def pickFile(self, idx: int):
        targetFile = self.unProcessed.pop(idx)
        self.Processed.add(targetFile)

        return targetFile

    def findConstantFromDependentFiles(self, dependentHFiles: set[str], target: str):
        for dhf in dependentHFiles:
            if target in self.AllConstaint[dhf].keys():
                for key, val in self.AllConstaint[dhf].items():
                    if target == key:
                        return True, val
        return False, None

    @staticmethod
    def sliceLineTokens(inStr: str):
        for char in inStr:
            match char:
                case "'" | '"':
                    ...
                    
    def getValFromExecStr(self, inStr: str, dependentHFiles) -> defines:

        ...

    @staticmethod
    def stripFile(file):

        fileText: str = "\n".join(file)

        fileText_striped: str = strip_comments_and_join_continuations(fileText)

        fileLines_striped: list[str] = fileText_striped.split("\n")
        fileLines_striped = [line.strip() for line in fileLines_striped]
        fileLines_striped = [line for line in fileLines_striped if line]

        return fileLines_striped

    @staticmethod
    def handle_endif(line: str, IfDepth: list[bool]) -> bool:
        endIfLine = ENDIF_RE.match(line)
        if not endIfLine:
            return False

        IfDepth.pop(-1)

        return True

    @staticmethod
    def handle_else(line: str, IfDepth: list[bool]) -> bool:
        elseLine = ELSE_RE.match(line)
        if not elseLine:
            # return line
            return False

        if all(IfDepth[:-1]):
            IfDepth[-1] = not IfDepth[-1]

        return True

    def handle_ifdef(self, line: str, IfDepth: list[bool], dependentHFiles: set[str]) -> bool:
        ifDefLine = IFDEF_RE.match(line)
        ifNDefLine = IFNDEF_RE.match(line)

        if not ifDefLine and not ifNDefLine:
            return False


        if ifDefLine :
            ShouldNotExist = False
            target = ifDefLine.groups()[0]

        else:
            target = ifNDefLine.groups()[0]
            ShouldNotExist = True

        Exist, _ = self.findConstantFromDependentFiles(dependentHFiles, target)

        IfDepth.append(Exist ^ ShouldNotExist)

        return True


    def handle_if(self, line: str, IfDepth: list[bool], dependentHFiles: set[str]) -> bool:
        ifLine = IF_RE.match(line)

        if not (ifLine):
            return False

        IFTrue = self.checkIfTrue(ifLine)
        IfDepth.append(IFTrue)
        
        return True

    def handle_includes(self, line:str, dependentHFiles: set[str]) -> bool:
        includeLine = INCLUDE_RE.match(line)

        if not includeLine:
            return False
        
        dhf:str = includeLine.groups()[0]
        hFile = FindFromBaseName(self.unProcessed, dhf)

        if not hFile:
            hFile = FindFromBaseName(self.Processed, dhf)
            dependentHFiles.add(hFile)
            return True

        dependentHFiles.add(hFile)
        
        self.deep += 1
        self.processFile(hFile)
        self.deep -= 1

        return True


    def processFile(self, dhf: str):
        if(self.deep > self.MAXDEEP):
            raise ValueError("include too deep")
        
        self.unProcessed.remove(dhf)
        self.Processed.add(dhf)

        hFile_stats = os.stat(hFile)
        if hFile_stats.st_size > self.MAXFILESIZE:
            raise ValueError("hFile might be too large")
        

        with open(hFile,'r') as hf:
            hfLines_striped = self.stripFile(hf)


        dependentHFiles: set[str] = {dhf}
        IfDepth: list[bool]= []

        fileConstants: dict[str, defines] = {}
        self.AllConstaint[dhf] = fileConstants

        for line in hfLines_striped:
            if self.handle_endif(line, IfDepth):
                continue
            if self.handle_else(line, IfDepth):
                continue
            if self.handle_ifdef(line, IfDepth, dependentHFiles):
                continue
            if self.handle_if(line, IfDepth, dependentHFiles):
                continue

            if self.handle_includes(line, dependentHFiles):
                continue

        
            


        

def processAllFiles(Progress: Progress):
    while(len(Progress.unProcessed) != 0):
        hfile = list(Progress.unProcessed)[0]
        # hfile = Progress.pickFile(0)
        # FILE_NAME = os.path.basename(hfile)
        Progress.processFile(hfile)




            

    






AllProgress = Progress(find_h_files(C_CODE_ROOT_DIR), 20, 100000)

for hFile in AllProgress.unProcessed:
    print(os.path.basename(hFile))



processAllFiles(AllProgress)
