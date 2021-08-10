from os import replace
import re

def merge_files(file1, file2, output=None):
    with open(file1, 'r') as f1:
        cmd1 = f1.readline()
    
    with open(file2, 'r') as f2:
        cmd2 = f2.readline()
        
    cmd1 = cmd1.split()
    cmd2 = cmd2.split()
    
    cmd = []
    for param1, param2 in zip(cmd1, cmd2):
        value = re.sub(r'.*=', "", param2)
        
        # isdigit() only works for non-negative number
        if value != param2 and value.isdigit() is False and re.match(r'LD_LIBRARY_PATH*', param1) is None:  
            param1 = param1 + '&' + value    
            
            param1 = param1.replace("'","")
            param1 = param1.replace('"',"")
            param1 = param1.replace('[', "")
            param1 = param1.replace(']', "")   # hard code for reading operator numbers : op_nums
            param1 = param1.split('=')
            param1 = param1[0] + "='" + param1[1] + "'"  
        cmd.append(param1)
            
    results = " ".join(tuple(cmd))
    
    if output is None:
        output = "command.sh"
        
    with open(output, "w") as cmd_file:
        cmd_file.write(results)
        
def normalize_file(file, output=None):
    with open(file,'r') as f:
        strm = f.readline()
        
    strm = strm.split()
    
    cmd = []
    for elt in strm:
        elt = elt.replace('[', "")
        elt = elt.replace(']', "")  # hard code for reading operator numbers : op_nums
        cmd.append(elt)
        
    results = " ".join(tuple(cmd))
    
    if output is None:
        output = "command.sh"
        
    with open(output, 'w') as cmd_file:
        cmd_file.write(results)