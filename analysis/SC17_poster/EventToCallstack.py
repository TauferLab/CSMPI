import argparse 
import glob
import os
import cPickle as pkl

import pprint

def clean_callstack(callstack):
    ### Strip out everything but function names    
    callstack = [ call.split("(")[1] for call in callstack ]
    ### Discard main etc.
    callstack = filter(lambda x: "main" not in x and
                                 "__libc_start_main" not in x and
                                 "_start" not in x,
                                 callstack)
    return tuple(callstack)

def build_event_to_callstack(experiment_dir):
    pkl_file_name = "event2callstack.pkl"
    if os.path.isfile(pkl_file_name):
        print "event2callstack already exists"
        with open(pkl_file_name, "rb") as pklfile:
            event_to_callstack = pkl.load(pklfile)
        return event_to_callstack
    else:
        run_dirs = sorted(glob.glob(experiment_dir+"/run_*"))
        event_to_callstack = {}
        for run_dir in run_dirs:
            csmpi_dir = run_dir + "/csmpi/"
            for logfile in sorted(glob.glob(csmpi_dir+"/*.log")):
                run_num = int(run_dir.split("_")[-1])
                rank = int(logfile.split("_")[-1].split(".")[0])
                with open(logfile, "rb") as log:
                    lines = log.readlines()
                    for i in range(len(lines)):
                        if "call=isend" in lines[i] or "call=send" in lines[i]:
                            sig = lines[i]
                            key = str(run_num)+"_"+str(rank)+"_"+str(sig)
                            callstack = []
                            while "end callstack" not in lines[i+1]:
                                i += 1
                                callstack.append(lines[i])
                            callstack = clean_callstack(callstack)
                            if key not in event_to_callstack:
                                event_to_callstack[key] = callstack
                            else:
                                print "attempted to insert duplicate event"
                                exit()
        with open(pkl_file_name, "wb") as pklfile:
            pkl.dump(event_to_callstack, pklfile)
        return event_to_callstack

def main():
    parser = argparse.ArgumentParser(description="")
    parser.add_argument("-d", "--data_dir", nargs=1,
                        help="")
    args = parser.parse_args()
    build_event_to_callstack(args.data_dir[0])

                         



if __name__ == "__main__":
    main()
