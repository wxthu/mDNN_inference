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
        if (value != param2):
            param1 = param1 + ',' + value
        cmd.append(param1)
            
    results = " ".join(tuple(cmd))
    
    if output is None:
        output = "command.sh"
        
    with open(output, "w") as cmd_file:
        cmd_file.write(results)