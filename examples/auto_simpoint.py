#!/bin/bash
import os
import sys
import subprocess 
import shlex
from threading import Thread

### CHANGE
dd = '/media/pravesh/A/snipersim/pin_kit'
SLICE_SIZE = 200000000

SIMPOINT = dd + '/extras/pinplay/PinPoints/bin/simpoint'

def run():
    folder_path = sys.argv[1]
    path_split = folder_path.split('/')
    bb_base = path_split[len(path_split) - 1].split('.')[0] + '.' + path_split[len(path_split) - 1].split('.')[1]
    print("[Base] {}".format(bb_base))
    
    ##-loadFVFile bfs_g20.graph_g20_178135.T.6.bb -coveragePct 1.0 -maxK 20 -saveSimpoints ./t6.simpoints -saveSimpointWeights ./t.weights -saveLabels t.labels
    
    simpoint_out = folder_path + '/simpoint_for_tracer.txt' 
    simpoint_out_fs = open(simpoint_out, 'w')

    files = os.listdir(folder_path)

    count_th = 0
    for f in files:
        if bb_base in f and '.bb' in f:
            count_th =  count_th + 1
        
    simpoint_out_fs.write(str(count_th)+'\n')

    for f in files:
        if bb_base in f and '.bb' in f:
            tag = f.split('.')[-2]
            print(tag)
            file_path = folder_path + '/' + f
            print("[BBV] " + file_path)
            output_file = folder_path + '/T' + str(tag) + '.simpoints' 
            SIMPOINT_CMD = '{} -loadFVFile {} -coveragePct 1.0 -maxK 20 -saveSimpoints {}'.format(SIMPOINT, file_path, output_file)
            print("[SIMPOINT] " + SIMPOINT_CMD)
            subprocess.run(shlex.split(SIMPOINT_CMD), shell=False)
            
            inof = open(output_file)
            ss = []
            for line in inof:
                line = line.lstrip().rstrip()
                ss.append(int(line[0]))

            ss.sort()
            
            # thread number
            simpoint_out_fs.write(str(tag)+'\n')

            # number of simpoints
            simpoint_out_fs.write(str(len(ss))+'\n')

            # range of simpoints
            for i in ss:
                s = i * SLICE_SIZE
                e = s + SLICE_SIZE
                simpoint_out_fs.write(str(s) + ' ' + str(e) + '\n')
                

run()
