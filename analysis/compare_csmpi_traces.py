import argparse
import glob
import os
import re
import pprint
import shelve 
import subprocess as sp 
import time

def clean_callstack(callstack):
    ### Strip out everything but function names    
    callstack = [ call.split("(")[1] for call in callstack ]
    ### Discard main etc.
    callstack = filter(lambda x: "main" not in x and
                                 "__libc_start_main" not in x and
                                 "_start" not in x,
                                 callstack)
    return tuple(callstack)

def search_source(cleaned_callstack, src_dir):
    ### For each call in the cleaned callstack
    ### grep in the source directory to find the
    ### source files containing 
    return 0

def main():
    parser = argparse.ArgumentParser(description="A script to compare CSMPI traces")
    parser.add_argument("-d", "--data_dir", nargs=1,
                        help="Directory containing one directory of CSMPI traces for each run")
    parser.add_argument("-s", "--src_dir", nargs=1,
                        help="Directory containing source code for the application that was traced.")
    args = parser.parse_args()
    data_dir = args.data_dir[0]

    root, dirs, files = os.walk(data_dir)
    runs = [ root[0]+d for d in root[1] ]
    isend_regex = re.compile("^call=isend")
    callstack_end_regex = re.compile("^end callstack$")
    run_to_isend_set = {}
    shelf_name = data_dir + ".shelf"
    if os.path.isfile(shelf_name):
        print "loading from shelf"
        start_time = time.time()
        ### read from shelf
        shelf = shelve.open(shelf_name)
        run_to_isend_set = shelf["run_to_isend_set"]
        print "done loading from shelf. elapsed = " + str(time.time() - start_time)
    else:
        ### Parse logs, collect isend data, store to shelf
        for r in runs:
            run_to_isend_set[r] = {"isends":None, "callstacks":None}
            logfiles = glob.glob(r+"/*.log")
            rank_to_isends = {}
            rank_to_callstacks = {}
            for lf in logfiles:
                with open(lf, "rb") as log_file:    
                    rank = int(lf.split(".")[0].split("_")[-1])
                    isends = []
                    isend_to_callstack = {}
                    for line in log_file:
                        if isend_regex.match(line):
                            ### get isend signature
                            features = tuple([ int(f.split("=")[1].strip()) for f in line.split(",")[1:] ])
                            isends.append(features)
                            ### get callstack data
                            callstack_data = []
                            line = log_file.next()
                            while not callstack_end_regex.match(line):
                                line = log_file.next()
                                callstack_data.append(line)
                            callstack_data = callstack_data[:-1]
                            isend_to_callstack[features] = callstack_data
                rank_to_isends[rank] = isends
                rank_to_callstacks[rank] = isend_to_callstack
            run_to_isend_set[r]["isends"] = rank_to_isends
            run_to_isend_set[r]["callstacks"] = rank_to_callstacks
        ### Persist to shelf
        shelf = shelve.open(shelf_name)
        shelf["run_to_isend_set"] = run_to_isend_set
        shelf.close()

    ### Proof of concept
    ### Check to see if rank 0's sends differed run to run
    run1_flows = {}
    run2_flows = {}
    run1_rank0_send_set = run_to_isend_set[data_dir+"run1"]["isends"][0]
    run2_rank0_send_set = run_to_isend_set[data_dir+"run2"]["isends"][0]
    run1_rank0_callstacks = run_to_isend_set[data_dir+"run1"]["callstacks"][0]
    run2_rank0_callstacks = run_to_isend_set[data_dir+"run2"]["callstacks"][0]
    for i in xrange(min(len(run1_rank0_send_set),len(run2_rank0_send_set))):
        if run1_rank0_send_set[i] == run2_rank0_send_set[i]:
            #print str(run1_rank0_send_set[i]) + " matches " + str(run2_rank0_send_set[i])
            continue
        else:
            if run1_rank0_send_set[i][4] != run2_rank0_send_set[i][4]:
                #print "diff dest: " + str(run1_rank0_send_set[i]) + " vs " + str(run2_rank0_send_set[i])
                #print "Run 1 callstack"
                #pprint.pprint(run1_rank0_callstacks[run1_rank0_send_set[i]])
                #print "Run 2 callstack"
                #pprint.pprint(run2_rank0_callstacks[run2_rank0_send_set[i]])
                run1_callstack = clean_callstack(run1_rank0_callstacks[run1_rank0_send_set[i]])
                run2_callstack = clean_callstack(run2_rank0_callstacks[run2_rank0_send_set[i]])
                if run1_callstack not in run1_flows:
                    run1_flows[run1_callstack] = 1
                else:
                    run1_flows[run1_callstack] += 1
                if run2_callstack not in run2_flows:
                    run2_flows[run2_callstack] = 1
                else:
                    run2_flows[run2_callstack] += 1
    
    print "Run 1 flows"
    pprint.pprint(run1_flows)
    print "Run 2 flows"
    pprint.pprint(run2_flows)
            




if __name__ == "__main__":
    main()
