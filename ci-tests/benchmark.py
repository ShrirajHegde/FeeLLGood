#!/usr/bin/env python3

import os
import sys
import json
import subprocess
import timeit
from datetime import datetime
from gmsh import __version__ as gmshVersion
from math import log2,floor
from feellgood.meshMaker import Cylinder

def makeSettings(mesh, surface_name, volume_name, nbThreads, final_time):
    """ returns a dictionary of settings for feellgood input """
    settings = {
        "outputs": {
            "file_basename": "benchmark",
            "evol_time_step": 1e-12,
            "final_time": final_time,
            "mag_config_every": False
        },
        "mesh": {
            "filename": mesh,
            "length_unit": 1e-9, # we use nanometers
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

def makeListNbThreads():
    result = subprocess.run(["nproc"],text=True,capture_output=True)
    maxNbThreads = 2**floor(log2(int(result.stdout)))
    nb = 2*maxNbThreads
    listNbThreads = []
    while nb >= (maxNbThreads//4):
        listNbThreads.append(nb)
        nb = nb // 2
    return listNbThreads

def version2test(str_executable):
    feellgood_version = subprocess.run([str_executable, "--version"], text=True, capture_output=True)
    return feellgood_version.stdout

def task2test(str_executable, settings):
    """ elementary task to benchmark. feellgood executable runs in a subprocess with seed=2 for being deterministic """
    val = subprocess.run([str_executable, "--seed", "2", "-"], input=json.dumps(settings), text=True)
    return val

def bench(str_executable, outputFileName, elt_sizes, listNbThreads, final_time):
    """
    loop over mesh size and nb threads for benchmarking feellgood executable,
    mesh is a cylinder of fixed geometry, varying mesh size
    """
    meshFileName = "cylinder.msh"
    surface_name = "surface"
    volume_name = "volume"
    height = 32
    radius = 3.0 * height

    with open(outputFileName, 'w') as f:
        str_date = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
        f.write("# " + version2test(str_executable) + "# " + str_date + '\n')
        f.write("# nbThreads: " + str(listNbThreads) + '\n')
        for elt_size in elt_sizes:
            f.write(str(elt_size) + '\t')
            mesh = Cylinder(radius, height, elt_size, surface_name, volume_name)
            mesh.make(meshFileName)
            for nbThreads in listNbThreads:
                settings = makeSettings(meshFileName, surface_name, volume_name, nbThreads, final_time)
                t = timeit.timeit(lambda: task2test(str_executable,settings), number=1)
                if nbThreads == listNbThreads[-1]:
                    f.write(str(t) + '\n')
                else:
                    f.write(str(t) + '\t')
        f.close()

def get_params(default_elt_sizes, default_listNbThreads, default_final_time):
    """ command line parser """
    import argparse
    description = 'feellgood benchmark'
    epilogue = '''
    runs several feellgood simulations varying meshSize of a cylinder and thread numbers 
    '''
    parser = argparse.ArgumentParser(description=description, epilog=epilogue,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-s','--sizes',metavar='elt_sizes',type=float,nargs='+',
                        help='table of mesh sizes', default=default_elt_sizes)
    parser.add_argument('-n','--nbThreads',metavar='listNbThreads',type=int,nargs='+',
                        help='table of number of threads', default=default_listNbThreads)
    parser.add_argument('-t','--final_time',type=float,help='final physical simulation time in s',
                        default=default_final_time)
    parser.add_argument('--version',action='version',version= __version__,help='show the version number')
    parser.add_argument('-f','--fast',help='fast benchmarking',action="store_true")
    return parser.parse_args()

__version__ = '1.0.2'
if __name__ == '__main__':
    default_final_time = 2e-11
    default_elt_sizes = [4.0, 3.5, 3.0, 2.5]
    default_listNbThreads = makeListNbThreads()
    args = get_params(default_elt_sizes, default_listNbThreads, default_final_time)
    if args.fast:
        args.final_time = 3e-12
        args.sizes = [4.0]
        try:
            nb = default_listNbThreads[2]
        except:
            nb = 2
        args.nbThreads = [nb]
        print("fast benchmark with",args.nbThreads,"threads")
    else:
        print("full benchmark version "+ __version__,"using gmsh", gmshVersion)
    os.chdir(sys.path[0])

    try:
        str_exec = "../feellgood"
        str_version = version2test(str_exec)
        outputFileName = 'benchmark' + str_version[-8:-1] + '.txt'
        bench(str_exec, outputFileName, args.sizes, args.nbThreads, args.final_time)
    except KeyboardInterrupt:
        print(" benchmark interrupted")
        sys.exit()
