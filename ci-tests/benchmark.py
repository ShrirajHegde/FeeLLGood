#!/usr/bin/python3

import os
import sys
import json
import subprocess
import timeit

from math import sin
from feellgood.meshMaker import Cylinder

#["y", "-x", "sqrt(1-x*x-y*y)"]
def makeSettings(mesh, surface_name, volume_name, nbThreads):
    settings = {
        "outputs": {
            "file_basename": "benchmark",
            "evol_time_step": 1e-12,
            "final_time": 1e-10,
            "mag_config_every": False
            },
        "mesh": {
            "filename": mesh,
            "length_unit": 1e-9,
            "volume_regions": { volume_name: {} },
            "surface_regions": { surface_name: {} }
        },
        "initial_magnetization": [0, 0, 1],
        "Bext": [1, 0, 1],
        "finite_element_solver": { "nb_threads": nbThreads },
        "demagnetizing_field_solver": { "nb_threads": nbThreads },
        "time_integration": {
            "min(dt)": 5e-18,
            "max(dt)": 1e-12,
            "max(du)": 0.1
            }
        }
    return settings

def task2test(settings):
    """ feellgood runs in a subprocess with seed=2 """
    val = subprocess.run(["../feellgood", "--seed", "2", "-"], input=json.dumps(settings), text=True)
    return val


if __name__ == '__main__':
    os.chdir(sys.path[0])
    MaxNbThreads = int(subprocess.check_output(["getconf","_NPROCESSORS_ONLN"]))
    meshFileName = "cylinder.msh"
    surface_name = "surface"
    volume_name = "volume"
    height = 32          # we use nanometers
    radius = height / 2  # so diameter = heigh
    elt_sizes = {2.5,3.0,3.5,4.0}
    
    with open('benchmark.txt','w') as f:
        for elt_size in elt_sizes:
            f.write(str(elt_size)+'\t')
            mesh = Cylinder(radius, height, elt_size, surface_name, volume_name)
            mesh.make(meshFileName)    
            for nbThreads in range(MaxNbThreads//2):
                settings = makeSettings(meshFileName,surface_name,volume_name,1+nbThreads)
                t = timeit.timeit("task2test(settings)", setup="from __main__ import task2test,settings",number=1)
                if nbThreads == (MaxNbThreads//2 - 1):
                    f.write(str(t)+'\n')
                else:
                    f.write(str(t)+'\t')
        f.close()


