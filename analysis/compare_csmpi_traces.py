import argparse
import glob
import os
import re
import pprint
import shelve 
import subprocess as sp 
import time

#from MotifDetector import get_recv_completion_motifs

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

def extract_comm_event(line, logfile):
    ### Necessary regexes
    callstack_end_regex = re.compile("^end callstack$")
    ### get function call signature
    signature = tuple([ int(f.split("=")[-1].strip()) for f in line.split(",")[1:] ])
    ### get callstack data
    callstack_data = []
    line = logfile.next()
    while not callstack_end_regex.match(line):
        line = logfile.next()
        callstack_data.append(line)
    callstack_data = callstack_data[:-1]
    return (signature, callstack_data)

def parse_multirecv_completion(line, logfile):
    ### get function call signature
    signature = tuple([ int(f.split("=")[-1].strip()) for f in line.split(",")[1:] ])
    matched = signature[0]
    ### get completions
    if matched == 1:
        completions = []
        while ("Done testsome" not in line):
            line = logfile.next()
            completions.append(line)
        ### get callstack
        callstack = []
        while ("end callstack" not in line):
            line = logfile.next()
            callstack.append(line)
        return (signature, completions[:-1], callstack[:-1])
    else:
        return None
        

def extract_csmpi_data(csmpi_dir):
    ### regexes for event types
    isend_regex = re.compile("^call=isend")
    send_regex = re.compile("^call=send")
    waitany_regex = re.compile("^call=waitany")
    test_regex = re.compile("^call=test,")
    testsome_regex = re.compile("^call=testsome,")
    rank_to_csmpi_data = {}
    ### Loop over all CSMPI logs and extract communication events
    for log in glob.glob(csmpi_dir+"/*.log"):
        event_type_to_data = {}
        ### Extract MPI rank for this logfile
        rank = int(log.split(".")[0].split("_")[-1])
        ### Open log file and get lines
        with open(log, "rb") as logfile:
            for l in logfile:
                if isend_regex.match(l):
                    if "isend" not in event_type_to_data:
                        signature, callstack = extract_comm_event(l, logfile)
                        event_type_to_data["isend"] = {"signatures":[signature], "callstacks":[callstack]}
                    else:
                        signature, callstack = extract_comm_event(l, logfile)
                        event_type_to_data["isend"]["signatures"].append(signature)
                        event_type_to_data["isend"]["callstacks"].append(callstack)
                #elif send_regex.match(l):
                #    if "send" not in event_type_to_data:
                #        signature, callstack = extract_comm_event(l, logfile)
                #        event_type_to_data["send"] = {"signatures":[signature], "callstacks":[callstack]}
                #    else:
                #        signature, callstack = extract_comm_event(l, logfile)
                #        event_type_to_data["send"]["signatures"].append(signature)
                #        event_type_to_data["send"]["callstacks"].append(callstack)
                #elif waitany_regex.match(l):
                #    if "waitany" not in event_type_to_data:
                #        signature, callstack = extract_comm_event(l, logfile)
                #        event_type_to_data["waitany"] = {"signatures":[signature], "callstacks":[callstack]}
                #    else:
                #        signature, callstack = extract_comm_event(l, logfile)
                #        event_type_to_data["waitany"]["signatures"].append(signature)
                #        event_type_to_data["waitany"]["callstacks"].append(callstack)
                #elif testsome_regex.match(l):
                #    if "testsome" not in event_type_to_data:
                #        try:
                #            signature, completions, callstack = parse_multirecv_completion(l, logfile)
                #            event_type_to_data["testsome"] = {"signatures":[signature],
                #                                              "completions":[completions],
                #                                              "callstacks":[callstack]}
                #        except Exception:
                #            pass
                #    else:
                #        try:
                #            signature, completions, callstack = parse_multirecv_completion(l, logfile)
                #            event_type_to_data["testsome"]["signatures"].append(signature)
                #            event_type_to_data["testsome"]["completions"].append(completions)
                #            event_type_to_data["testsome"]["callstacks"].append(callstack)
                #        except Exception:
                #            pass
                #elif test_regex.match(l):
                #    if "test" not in event_type_to_data:
                #        signature, callstack = extract_comm_event(l, logfile)
                #        event_type_to_data["test"] = {"signatures":[signature], "callstacks":[callstack]}
                #    else:
                #        signature, callstack = extract_comm_event(l, logfile)
                #        event_type_to_data["test"]["signatures"].append(signature)
                #        event_type_to_data["test"]["callstacks"].append(callstack)
        rank_to_csmpi_data[rank] = event_type_to_data
    return rank_to_csmpi_data
                    
            
    


def main():
    parser = argparse.ArgumentParser(description="A script to compare CSMPI traces")
    parser.add_argument("-d", "--data_dir", nargs=1,
                        help="Directory containing one directory of CSMPI traces for each run")
    parser.add_argument("-s", "--src_dir", nargs=1,
                        help="Directory containing source code for the application that was traced.")
    args = parser.parse_args()
    data_dir = args.data_dir[0]

    run_to_csmpi_data = {}
    ### Loop over all run dirs and extract run data
    for run_dir in glob.glob(data_dir+"/run_*"):
        print "Working on run: " + str(run_dir)
        start_time = time.time()
        csmpi_dir = run_dir + "/csmpi/"
        csmpi_data = extract_csmpi_data(csmpi_dir)
        run_to_csmpi_data[run_dir] = csmpi_data
        print "Elapsed time = " + str(time.time() - start_time)

    #get_recv_completion_motifs(run_to_csmpi_data)
    #exit()

    ### Look at sequence of isends for each rank across all runs
    #print "Isend record format -- [ tag, dest, count ]"
    rank_to_sig_to_callstack = {}
    #for rank in range(len(run_to_csmpi_data[run_to_csmpi_data.keys()[0]].keys()))[:1]:
    for rank in range(len(run_to_csmpi_data[run_to_csmpi_data.keys()[0]].keys())):
        print "Rank: " + str(rank)
        run_to_isend_set = {}
        #for run in sorted(run_to_csmpi_data.keys())[:7]:
        for run in sorted(run_to_csmpi_data.keys()):
            event_type_to_data = run_to_csmpi_data[run][rank]
            isends_signatures = event_type_to_data["isend"]["signatures"]
            isends_callstacks = event_type_to_data["isend"]["callstacks"]
            run_to_isend_set[run] = {"signatures":isends_signatures, "callstacks":isends_callstacks}
            #run_to_isend_set[run] = isends_signatures

        ### Find maximum length of send sequences across all runs
        isend_counts = []
        for run in sorted(run_to_isend_set.keys()):
            isend_counts.append(len(run_to_isend_set[run]["signatures"]))
        max_num_isends = max(isend_counts)
        
        ### Write send sequences for analysis
        all_sigs = []
        for run in sorted(run_to_isend_set.keys()):
            sigstr_to_callstack = {}
            run_number = run.split("_")[-1].split(".")[0]
            with open("send_seq_"+str(run_number)+".txt", "wb") as seq_file:
                for i in range(max_num_isends):
                    try:
                        sig = run_to_isend_set[run]["signatures"][i]
                        #sigstr = str([sig[5],sig[4],sig[2]])+"\n"
                        sigstr = str(sig[2:])+"\n"
                        all_sigs.append(sigstr)
                        if sigstr not in sigstr_to_callstack:
                            sigstr_to_callstack[sigstr[:-1]] = [clean_callstack(run_to_isend_set[run]["callstacks"][i])]
                        else:
                            sigstr_to_callstack[sigstr[:-1]].append(clean_callstack(run_to_isend_set[run]["callstacks"][i]))
                    except IndexError:
                        sigstr = "NONE\n"
                    seq_file.write(sigstr)

        ### Get ND-send candidates
        candidates = []
        for curr_run in sorted(run_to_isend_set.keys())[:-1]:
            curr_run_number = curr_run.split("_")[-1].split(".")[0]
            for comp_run in sorted(run_to_isend_set.keys())[1:]:
                comp_run_number = comp_run.split("_")[-1].split(".")[0]
                try:
                    p = sp.check_output(["diff", "send_seq_"+str(curr_run_number)+".txt", "send_seq_"+str(comp_run_number)+".txt"])
                ### Why is the exceptional case where I can retrieve my data? 
                except sp.CalledProcessError, e:
                    edits = e.output.split("\n")
                    edits = filter(lambda x: len(x) > 0, edits)
                    edits = filter(lambda x: x[0] == ">" or x[0] == "<", edits)
                    edits = filter(lambda x: "NONE" not in x, edits)
                    candidates += [ x[2:] for x in edits ]
        
        #print "All sigs"
        #pprint.pprint(set(all_sigs))
        #print "Candidate sigs"
        #pprint.pprint(set(candidates))
        #exit()

        #pprint.pprint(candidates)
        #pprint.pprint(sigstr_to_callstack.keys())
        ### Get callstack stats from candidates
        callstack_to_count = {}
        for sig in candidates:
            if sig in sigstr_to_callstack:
                callstacks = sigstr_to_callstack[sig]
                for cs in callstacks:
                    if cs not in callstack_to_count:
                        callstack_to_count[cs] = 1
                    else:
                        callstack_to_count[cs] += 1
        
        ### Callstack stats
        pprint.pprint(callstack_to_count)
        print "\n"

    #### Look at sequence of irecv completions for each rank across all runs
    #print "Irecv completion via Test record format -- [ tag, src, count ]"
    #for rank in range(len(run_to_csmpi_data[run_to_csmpi_data.keys()[0]].keys()))[1:]:
    #    print "Rank: " + str(rank)
    #    run_to_test_set = {}
    #    for run in run_to_csmpi_data.keys()[:4]:
    #        event_type_to_data = run_to_csmpi_data[run][rank]
    #        tests_signatures = event_type_to_data["test"]["signatures"]
    #        tests_callstacks = event_type_to_data["test"]["callstacks"]
    #        #run_to_isend_set[run] = {"signatures":isends_signatures, "callstacks":isends_callstacks}
    #        run_to_test_set[run] = tests_signatures

    #    ### Display test sequences for this rank
    #    num_tests = max({k:len(v) for k,v in run_to_test_set.items()}.values())
    #    format_str = "{:<6} "
    #    for run in sorted(run_to_test_set.keys()):
    #        format_str += "{:<15} "
    #    header = ["Test "]
    #    for run in sorted(run_to_test_set.keys()):
    #        header.append(run.split("/")[-1])
    #    output = format_str.format(*header)
    #    print output
    #    for i in range(num_tests):
    #        #tests_str = str(i) + ": "
    #        tests = [str(i)]
    #        for run in sorted(run_to_test_set.keys()):
    #            try:
    #                sig = run_to_test_set[run][i]
    #                if len(sig) < 5:
    #                    tests.append("no msg")
    #                else:
    #                    tests.append(str([sig[4], sig[3]]))
    #                #tests_str += str([sig[5],sig[4],sig[2]]) + "\t"
    #                #tests.append(str([sig[5],sig[4],sig[2]]))
    #            except IndexError:
    #                #tests_str += "NONE\t"
    #                tests.append("NONE")
    #        #print (format_str % tuple(tests))
    #        #print tests_str.expandtabs()
    #        #output = tests[0].ljust(3)
    #        #for s in tests[1:]:
    #        #    output += s.ljust(10)
    #        #    output += "\t"
    #        #output = "{:<3} {:<15} {:<15} {:<15} {:<15}".format(*tests)
    #        output = format_str.format(*tests)
    #        print output
    #
    #### Look at sequence of irecv completions for each rank across all runs
    #print "Irecv completion via Testsome record format -- [ tag, src, count ]"
    #for rank in range(len(run_to_csmpi_data[run_to_csmpi_data.keys()[0]].keys()))[1:]:
    #    print "Rank: " + str(rank)
    #    run_to_test_set = {}
    #    for run in run_to_csmpi_data.keys()[:4]:
    #        event_type_to_data = run_to_csmpi_data[run][rank]
    #        tests_signatures = event_type_to_data["testsome"]["signatures"]
    #        tests_callstacks = event_type_to_data["testsome"]["callstacks"]
    #        #run_to_isend_set[run] = {"signatures":isends_signatures, "callstacks":isends_callstacks}
    #        run_to_test_set[run] = tests_signatures

    #    ### Display test sequences for this rank
    #    num_tests = max({k:len(v) for k,v in run_to_test_set.items()}.values())
    #    format_str = "{:<6} "
    #    for run in sorted(run_to_test_set.keys()):
    #        format_str += "{:<15} "
    #    header = ["Testsome"]
    #    for run in sorted(run_to_test_set.keys()):
    #        header.append(run.split("/")[-1])
    #    output = format_str.format(*header)
    #    print output
    #    for i in range(num_tests):
    #        #tests_str = str(i) + ": "
    #        tests = [str(i)]
    #        for run in sorted(run_to_test_set.keys()):
    #            try:
    #                sig = run_to_test_set[run][i]
    #                if len(sig) < 5:
    #                    tests.append("no msg")
    #                else:
    #                    tests.append(str([sig[4], sig[3]]))
    #                #tests_str += str([sig[5],sig[4],sig[2]]) + "\t"
    #                #tests.append(str([sig[5],sig[4],sig[2]]))
    #            except IndexError:
    #                #tests_str += "NONE\t"
    #                tests.append("NONE")
    #        #print (format_str % tuple(tests))
    #        #print tests_str.expandtabs()
    #        #output = tests[0].ljust(3)
    #        #for s in tests[1:]:
    #        #    output += s.ljust(10)
    #        #    output += "\t"
    #        #output = "{:<3} {:<15} {:<15} {:<15} {:<15}".format(*tests)
    #        output = format_str.format(*tests)
    #        print output
"""
        ### Display isend sequences for this rank
        format_str = "{:<6} "
        for run in sorted(run_to_isend_set.keys()):
            format_str += "{:<15} "
        header = ["Isend "]
        for run in sorted(run_to_isend_set.keys()):
            header.append(run.split("/")[-1])
        output = format_str.format(*header)
        print output
        sig_to_callstack = {}
        for i in range(num_isends):
            #isends_str = str(i) + ": "
            isends = [str(i)]
            for run in sorted(run_to_isend_set.keys()):
                try:
                    sig = run_to_isend_set[run]["signatures"][i]
                    sig_to_callstack[sig] = run_to_isend_set[run]["callstacks"][i]
                    #isends_str += str([sig[5],sig[4],sig[2]]) + "\t"
                    isends.append(str([sig[5],sig[4],sig[2]]))
                except Exception:
                    #isends_str += "NONE\t"
                    isends.append("NONE")
            #print (format_str % tuple(isends))
            #print isends_str.expandtabs()
            #output = isends[0].ljust(3)
            #for s in isends[1:]:
            #    output += s.ljust(10)
            #    output += "\t"
            #output = "{:<3} {:<15} {:<15} {:<15} {:<15}".format(*isends)
            output = format_str.format(*isends)
            print output
        rank_to_sig_to_callstack[rank] = sig_to_callstack
    
    print "\n\n\n"

    rank_to_nd_causing_callstacks = {}
    rank_to_det_send_callstacks = {}
    for rank in rank_to_sig_to_callstack.keys():
        rank_to_nd_causing_callstacks[rank] = {}
        rank_to_det_send_callstacks[rank] = {}
        for sig in rank_to_sig_to_callstack[rank].keys():
            if sig[5] == 55:
                callstack = tuple(clean_callstack(rank_to_sig_to_callstack[rank][sig]))
                if callstack not in rank_to_nd_causing_callstacks[rank]:
                    rank_to_nd_causing_callstacks[rank][callstack] = 1
                else:
                    rank_to_nd_causing_callstacks[rank][callstack] += 1
            else:
                callstack = tuple(clean_callstack(rank_to_sig_to_callstack[rank][sig]))
                if callstack not in rank_to_det_send_callstacks[rank]:
                    rank_to_det_send_callstacks[rank][callstack] = 1
                else:
                    rank_to_det_send_callstacks[rank][callstack] += 1
    print "Nondeterminism-causing callstacks"
    pprint.pprint(rank_to_nd_causing_callstacks)
    print "\n"
    print "Deterministic callstacks"
    pprint.pprint(rank_to_det_send_callstacks)

    exit()
    ### Look at sequence of irecv completions for each rank across all runs
    print "Irecv completion via Test record format -- [ tag, src, count ]"
    for rank in range(len(run_to_csmpi_data[run_to_csmpi_data.keys()[0]].keys()))[1:]:
        print "Rank: " + str(rank)
        run_to_test_set = {}
        for run in run_to_csmpi_data.keys()[:4]:
            event_type_to_data = run_to_csmpi_data[run][rank]
            tests_signatures = event_type_to_data["test"]["signatures"]
            tests_callstacks = event_type_to_data["test"]["callstacks"]
            #run_to_isend_set[run] = {"signatures":isends_signatures, "callstacks":isends_callstacks}
            run_to_test_set[run] = tests_signatures

        ### Display test sequences for this rank
        num_tests = max({k:len(v) for k,v in run_to_test_set.items()}.values())
        format_str = "{:<6} "
        for run in sorted(run_to_test_set.keys()):
            format_str += "{:<15} "
        header = ["Test "]
        for run in sorted(run_to_test_set.keys()):
            header.append(run.split("/")[-1])
        output = format_str.format(*header)
        print output
        for i in range(num_tests):
            #tests_str = str(i) + ": "
            tests = [str(i)]
            for run in sorted(run_to_test_set.keys()):
                try:
                    sig = run_to_test_set[run][i]
                    if len(sig) < 5:
                        tests.append("no msg")
                    else:
                        tests.append(str([sig[4], sig[3]]))
                    #tests_str += str([sig[5],sig[4],sig[2]]) + "\t"
                    #tests.append(str([sig[5],sig[4],sig[2]]))
                except IndexError:
                    #tests_str += "NONE\t"
                    tests.append("NONE")
            #print (format_str % tuple(tests))
            #print tests_str.expandtabs()
            #output = tests[0].ljust(3)
            #for s in tests[1:]:
            #    output += s.ljust(10)
            #    output += "\t"
            #output = "{:<3} {:<15} {:<15} {:<15} {:<15}".format(*tests)
            output = format_str.format(*tests)
            print output
    
    exit()

    ### Check that each rank issued same number of isends each run
    rank_to_tag_to_counts = {}
    for run in run_to_csmpi_data.keys():
        rank_to_num_isends = {}
        rank_to_num_isends_of_tag = {}
        for rank in run_to_csmpi_data[run].keys():
            tag_to_isends = {}
            for isend in run_to_csmpi_data[run][rank]["isend"]["signatures"]:
                tag = isend[-2]
                if tag not in tag_to_isends:
                    tag_to_isends[tag] = [isend]
                else:
                    tag_to_isends[tag].append(isend)
            num_isends = len(run_to_csmpi_data[run][rank]["isend"]["signatures"])
            num_isends_of_tag = {k:len(v) for k,v in tag_to_isends.items()}
            rank_to_num_isends[rank] = num_isends
            rank_to_num_isends_of_tag[rank] = num_isends_of_tag
        for rank in rank_to_num_isends_of_tag.keys():
            if rank not in rank_to_tag_to_counts:
                rank_to_tag_to_counts[rank] = {}
                for tag in rank_to_num_isends_of_tag[rank].keys():
                    if tag not in rank_to_tag_to_counts[rank]:
                        rank_to_tag_to_counts[rank][tag] = [rank_to_num_isends_of_tag[rank][tag]]
                    else:
                        rank_to_tag_to_counts[rank][tag].append(rank_to_num_isends_of_tag[rank][tag])
            else:
                for tag in rank_to_num_isends_of_tag[rank].keys():
                    if tag not in rank_to_tag_to_counts[rank]:
                        rank_to_tag_to_counts[rank][tag] = [rank_to_num_isends_of_tag[rank][tag]]
                    else:
                        rank_to_tag_to_counts[rank][tag].append(rank_to_num_isends_of_tag[rank][tag])

        #print str(run) + " --> " + str(rank_to_num_isends_of_tag)
    #pprint.pprint(rank_to_tag_to_counts)
    import numpy as np
    rank_to_tag_to_std = {}
    for rank in rank_to_tag_to_counts.keys():
        rank_to_tag_to_std[rank] = {}
        for tag in rank_to_tag_to_counts[rank].keys():
            counts = rank_to_tag_to_counts[rank][tag]
            rank_to_tag_to_std[rank][tag] = {"std":round(np.std(counts)), "min":min(counts), "max":max(counts), "med":np.median(counts)}
    pprint.pprint(rank_to_tag_to_std)

    

    exit()
"""
"""
   runs = [ root[0]+d for d in root[1] ]
   isend_regex = re.compile("^call=isend")
   callstack_end_regex = re.compile("^end callstack$")
   run_to_isend_set = {}
   shelf_name = data_dir + "csmpi.shelf"
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
            print "Working on run: " + str(r)
            run_start_time = time.time()
            run_to_isend_set[r] = {"isends":None, "callstacks":None}
            logfiles = glob.glob(r+"/*.log")
            rank_to_isends = {}
            rank_to_callstacks = {}
            for lf in logfiles:
                print "Extracting from logfile: " + str(lf)
                logfile_start_time = time.time()
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
                print "Done with logfile elapsed = " + str(time.time() - logfile_start_time)
            run_to_isend_set[r]["isends"] = rank_to_isends
            run_to_isend_set[r]["callstacks"] = rank_to_callstacks
            print "Done elapsed time = " + str(time.time() - run_start_time)
        ### Persist to shelf
        shelf = shelve.open(shelf_name)
        shelf["run_to_isend_set"] = run_to_isend_set
        shelf.close()

    ### Proof of concept
    ### Check to see if rank 0's sends differed run to run
    for rank in range(8):
        run1_flows = {}
        run2_flows = {}
        run1_rank0_send_set = run_to_isend_set[data_dir+"run_00"]["isends"][rank]
        run2_rank0_send_set = run_to_isend_set[data_dir+"run_01"]["isends"][rank]
        run1_rank0_callstacks = run_to_isend_set[data_dir+"run_00"]["callstacks"][rank]
        run2_rank0_callstacks = run_to_isend_set[data_dir+"run_01"]["callstacks"][rank]
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
        print "Rank " + str(rank) 
        print "Run 1 flows"
        pprint.pprint(run1_flows)
        print "Run 2 flows"
        pprint.pprint(run2_flows)
"""         




if __name__ == "__main__":
    main()
