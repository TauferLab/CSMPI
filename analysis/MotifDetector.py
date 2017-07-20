import ast
import subprocess
import glob
import pprint
import time
import argparse
import difflib
import shelve
import os

def clean_callstack(callstack):
    ### Strip out everything but function names    
    callstack = [ call.split("(")[1].strip() for call in callstack ]
    ### Discard main etc.
    callstack = filter(lambda x: "main" not in x and
                                 "__libc_start_main" not in x and
                                 "_start" not in x,
                                 callstack)
    return tuple(callstack)

def main():
    parser = argparse.ArgumentParser(description="A script to compare CSMPI traces")
    parser.add_argument("-d", "--data_dir", nargs=1,
                        help="Directory containing one directory of CSMPI traces for each run")
    args = parser.parse_args()
    data_dir = args.data_dir[0]

    ### Read in all CSMPI data 
    run_to_data = {}
    for run_dir in glob.glob(data_dir+"/run_*"):
        print "Loading in run: " + str(run_dir)
        start_time = time.time()
        run_num = int(run_dir.split("/")[-1].split("_")[-1])
        run_to_data[run_num] = {}
        csmpi_dir = run_dir + "/csmpi/"
        for logfile in glob.glob(csmpi_dir+"/*.log"):
            rank = int(logfile.split("_")[-1].split(".")[0])
            with open(logfile, "rb") as log:
                run_to_data[run_num][rank] = log.readlines()
        print "Elapsed time = " + str(time.time() - start_time)

    ### For each rank, find diffs in send inits across runs
    rank_to_isend_seqs = {}
    #for rank in sorted(run_to_data[run_to_data.keys()[0]].keys())[:1]:  ### Only do one rank, debug
    for rank in sorted(run_to_data[run_to_data.keys()[0]].keys()):
        run_to_isend_sig_seq = {}
        run_to_isend_callstack_seq = {}
        run_to_testsome_sig_seq = {}
        testsome_sig_to_callstack = {}
        run_to_waitany_sig_seq = {}
        waitany_sig_to_callstack = {}
        ### Collect every event for this rank from all runs
        for run in run_to_data.keys():
            rank_to_data = run_to_data[run]
            log = rank_to_data[rank]
            for i in range(len(log)):
                ### Detect isend 
                if "call=isend" in log[i]:
                    isend_sig = log[i]
                    ### Clean isend sig
                    isend_sig = [ x.split("=")[-1].strip() for x in isend_sig.split(",")[1:] ]
                    isend_sig = [ isend_sig[5], isend_sig[4], isend_sig[2] ]
                    ### Get associated callstack
                    callstack = []
                    while "end callstack" not in log[i+1]:
                        i +=1
                        callstack.append(log[i])
                    callstack = clean_callstack(callstack)
                    ### Add isend signature to sequence for this run
                    if run not in run_to_isend_sig_seq:
                        run_to_isend_sig_seq[run] = [isend_sig]
                    else:
                        run_to_isend_sig_seq[run].append(isend_sig)
                ### Detect testsome
                elif "call=testsome" in log[i]:
                    testsome_sig = log[i]
                    ### Clean testsome sig
                    testsome_sig = [ int(x.split("=")[-1].strip()) for x in testsome_sig.split(",")[1:] ]
                    matched = int(testsome_sig[0])
                    if matched:
                        ### Get associated completions
                        completions = []
                        while "Done testsome" not in log[i+1]:
                            i += 1
                            completions.append(log[i])
                        ### Filter completions for the 0-tagged ones that are apparently errors
                        completions = [ x.split(",") for x in completions ]
                        completions = [ [ int(y.split("=")[-1].strip()) for y in x ] for x in completions ]
                        completions = filter(lambda x: x[-1] != 0, completions)
                        completions = [ tuple(x) for x in completions ]
                        ### Get associated callstack
                        callstack = []
                        while "end callstack" not in log[i+1]:
                            i +=1
                            callstack.append(log[i])
                        callstack = clean_callstack(callstack[1:])
                        ### Only add to sequence if associated with real completions
                        if len(completions) > 0:
                            if run not in run_to_testsome_sig_seq:
                                run_to_testsome_sig_seq[run] = [(tuple(testsome_sig[2:]), tuple(completions))]
                                key = (tuple(testsome_sig[2:]), tuple(completions))
                                testsome_sig_to_callstack[key] = callstack
                            else:
                                run_to_testsome_sig_seq[run].append((tuple(testsome_sig[2:]), tuple(completions)))
                                key = (tuple(testsome_sig[2:]), tuple(completions))
                                testsome_sig_to_callstack[key] = callstack
                ### Detect waitany
                elif "call=waitany" in log[i]:
                    waitany_sig = log[i]
                    ### Clean waitany sig
                    waitany_sig = [ int(x.split("=")[-1].strip()) for x in waitany_sig.split(",")[1:] ]
                    waitany_sig = waitany_sig[2:]
                    ### Get associated callstack
                    callstack = []
                    while "end callstack" not in log[i+1]:
                        i += 1
                        callstack.append(log[i])
                    callstack = clean_callstack(callstack)
                    if run not in run_to_waitany_sig_seq:
                        run_to_waitany_sig_seq[run] = [tuple(waitany_sig)]
                        waitany_sig_to_callstack[tuple(waitany_sig)] = callstack
                    else:
                        run_to_waitany_sig_seq[run].append(tuple(waitany_sig))
                        waitany_sig_to_callstack[tuple(waitany_sig)] = callstack

                                
        ### Detect differences in MPI_Waitany calls across runs
        runs = run_to_data.keys()
        ### Write out temporary files for diffing
        for run in runs:
            with open("waitany_seqfile_"+str(run)+".txt", "wb") as seqfile:
                for line in run_to_waitany_sig_seq[run]:
                    seqfile.write(str(line)+"\n")
        ### Do diffing and collect signatures of mismatches across runsc
        candidates = []
        for i in range(len(runs)):
            for j in range(len(runs))[i+1:]:
                curr_run = runs[i]
                comp_run = runs[j]
                #print "Diffing runs " + str(i) + " and " + str(j)
                start_time = time.time()
                try:
                    subprocess.check_output(["diff", "waitany_seqfile_"+str(curr_run)+".txt", "waitany_seqfile_"+str(comp_run)+".txt"])
                except subprocess.CalledProcessError, e:
                    edits = e.output.split("\n")
                    edits = filter(lambda x: len(x) > 0, edits)
                    edits = filter(lambda x: x[0] == ">" or x[0] == "<", edits)
                    edits = [ ast.literal_eval(x[2:]) for x in edits ]
                    candidates += edits
                #print "Elapsed = " + str(time.time() - start_time)


        ### Get counts of associated callstacks
        callstack_to_count = {}
        for c in candidates:
            try:
                cs = waitany_sig_to_callstack[c]
                if cs not in callstack_to_count:
                    callstack_to_count[cs] = 1
                else:
                    callstack_to_count[cs] += 1
            except:
                pass
        
        print "RANK: " + str(rank)
        pprint.pprint(callstack_to_count)
        print "\n"
            


        #### Detect differences in MPI_Testsome calls across runs
        #runs = run_to_data.keys()
        #### Write out temporary files for diffing
        #for run in runs:
        #    with open("testsome_seqfile_"+str(run)+".txt", "wb") as seqfile:
        #        for line in run_to_testsome_sig_seq[run]:
        #            seqfile.write(str(line)+"\n")
        #### Do diffing and collect signatures of mismatches across runsc
        #candidates = []
        #for curr_run in runs:
        #    for comp_run in runs:
        #        try:
        #            subprocess.check_output(["diff", "testsome_seqfile_"+str(curr_run)+".txt", "testsome_seqfile_"+str(comp_run)+".txt"])
        #        except subprocess.CalledProcessError, e:
        #            edits = e.output.split("\n")
        #            edits = filter(lambda x: len(x) > 0, edits)
        #            edits = filter(lambda x: x[0] == ">" or x[0] == "<", edits)
        #            edits = [ ast.literal_eval(x[2:]) for x in edits ]
        #            candidates += edits
        #### Get counts of associated callstacks
        #callstack_to_count = {}
        #for c in candidates:
        #    cs = testsome_sig_to_callstack[c]
        #    if cs not in callstack_to_count:
        #        callstack_to_count[cs] = 1
        #    else:
        #        callstack_to_count[cs] += 1
        #
        #print "RANK: " + str(rank)
        #pprint.pprint(callstack_to_count)
        #print "\n"

        """
        #testsome_seq_1 = run_to_testsome_sig_seq[runs[0]] 
        #testsome_seq_2 = run_to_testsome_sig_seq[runs[1]] 

        #exit()

        #### First, just show lengths of isend seqs for all runs
        #for run in runs:
        #    seq_len = len(run_to_isend_sig_seq[run])
        #    print "Run: " + str(run) + " isend seq len = " + str(seq_len)

        #exit()

        #seq1 = run_to_isend_sig_seq[runs[0]]
        #seq2 = run_to_isend_sig_seq[runs[18]]

        #for i in range(max(len(seq1), len(seq2))):
        #    try:
        #        print str(seq1[i]) + "\t" + str(seq2[i]) 
        #    except Exception:
        #        if i >= len(seq1):
        #            print "NONE" + "\t" + str(seq2[i])
        #        else:
        #            print str(seq1[i]) + "\t" + "NONE" 
        #exit()

        #with open("seqfile1", "wb") as seqfile1:
        #    for line in seq1:
        #        for i in range(len(line)):
        #            if i != len(line)-1:
        #                seqfile1.write(line[i] + ", ")
        #            else:
        #                seqfile1.write(line[i] + "\n")
        #with open("seqfile2", "wb") as seqfile2:
        #    for line in seq2:
        #        for i in range(len(line)):
        #            if i != len(line)-1:
        #                seqfile2.write(line[i] + ", ")
        #            else:
        #                seqfile2.write(line[i] + "\n")
        #try:
        #    p = subprocess.check_output(["diff", "seqfile1", "seqfile2"]) 
        #except subprocess.CalledProcessError, e:
        #    diff_output = e.output
        #    pprint.pprint(diff_output)
        #    exit()
        """ 
                        
                        
    
if __name__ == "__main__":
    main()
