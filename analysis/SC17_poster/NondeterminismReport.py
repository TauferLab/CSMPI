import glob
import argparse
import cPickle as pkl

from EventToCallstack import build_event_to_callstack
from TraceComparison import TraceComparison
from UtilsDiff import (run_diff,
                       separate_hunks,
                       get_nd_events_from_hunk)
from UtilsPreprocessing import dict_add

from PosterPlots import (generate_callstack_to_count_pie_chart,
                         generate_d_vs_nd_pie_chart)

import pprint
import time

def main():
    parser = argparse.ArgumentParser(description="Generates a report of        \
                                     nondeterminism detected across many runs  \
                                     of an application.")
    parser.add_argument("-d", "--data_dir", nargs=1,
                        help="Directory containing all runs")
    args = parser.parse_args()

    ### Build mapping between event signatures and callstacks
    print "Making event-to-callstack dict"
    start_time = time.time()
    event_to_callstack = build_event_to_callstack(args.data_dir[0])
    print "Elapsed time = " + str(time.time() - start_time)

    print "all callstacks that terminate in sends"
    pprint.pprint(list(set(event_to_callstack.values())))

    ### Identify ND events and get callstack counts
    all_runs = sorted(glob.glob(args.data_dir[0]+"/run_*"))
    ranks = range(8)
    rank_to_report = {}
    #callstack_to_count = {k:0 for k in list(set(event_to_callstack.values()))}
    num_isends = 0
    #for rank in ranks:
    for rank in [0, 1, 3, 7, 15]:
        all_hunks = {}
        run_to_fraction_nd = {}
        run_to_events = {}
        for i in range(len(all_runs)):
            current_run = all_runs[i]+"/csmpi/"
            all_nd_isends = set()
            all_callstack_to_count = {}
            for j in range(i+1, len(all_runs)):
                callstack_to_count = {k:0 for k in list(set(event_to_callstack.values()))}
                comparison_run = all_runs[j]+"/csmpi/"
                tc = TraceComparison(current_run, comparison_run)
                print "Comparing run " + str(i) + " with run " + str(j)
                if i not in run_to_events:
                    tc.run_1_desc.build_description(i)
                    run_to_events[i] = tc.run_1_desc.rank_to_event_seqs
                    print "Run 1 # isends = " + str(tc.run_1_desc.rank_to_event_counts[rank]["isend"])
                else:
                    tc.run_1_desc.rank_to_event_seqs = run_to_events[i]
                    print "Run 1 # isends = " + str(len(run_to_events[i][rank]["isend"]))
                if j not in run_to_events:
                    tc.run_2_desc.build_description(j)
                    run_to_events[j] = tc.run_2_desc.rank_to_event_seqs
                    print "Run 2 # isends = " + str(tc.run_2_desc.rank_to_event_counts[rank]["isend"])
                else:
                    tc.run_2_desc.rank_to_event_seqs = run_to_events[j]
                    print "Run 2 # isends = " + str(len(run_to_events[j][rank]["isend"]))

                run_1_num_isends = len(tc.run_1_desc.rank_to_event_seqs[rank]["isend"])
                run_2_num_isends = len(tc.run_2_desc.rank_to_event_seqs[rank]["isend"])

                ### 
                start_time = time.time()
                print "Computing diff" 
                diff_hunks = tc.get_send_diff(rank, i, j)
                print "Elapsed time = " + str(time.time() - start_time)
                all_hunks[(i,j)] = diff_hunks
                ### Lookup callstacks for each hunk's ND events
                num_nd_curr_run = 0 
                num_nd_comp_run = 0
                for h in diff_hunks:
                    #print "Hunk: "
                    #print h
                    nd_events, curr_run_nd_count, comp_run_nd_count  = get_nd_events_from_hunk(h, rank, i, j)
                    num_nd_curr_run += curr_run_nd_count
                    num_nd_comp_run += comp_run_nd_count
                    #print "ND event keys: "
                    #print nd_events
                    for e in nd_events:
                        #print "Looking up callstack for event: " + str(e)
                        if e in event_to_callstack:
                            #print "Found callstack!"
                            #pprint.pprint(event_to_callstack[e])
                            callstack_to_count[event_to_callstack[e]] += 1
                            all_nd_isends.add(e)
                        else:
                            #print "Not found..."
                            pass
                print "Run 1 # ND isends = " + str(num_nd_curr_run)
                print "Run 2 # ND isends = " + str(num_nd_comp_run)
                #pprint.pprint(callstack_to_count)
                all_callstack_to_count = dict_add(all_callstack_to_count, callstack_to_count)
                
            #generate_d_vs_nd_pie_chart(run_1_num_isends, num_nd_curr_run, i, rank)
            #generate_pie_chart(callstack_to_count, i, rank)
            d_vs_nd = {"d":(run_1_num_isends-len(all_nd_isends)), "nd":len(all_nd_isends)}
            pklfile_name = "rank"+str(rank)+"_run"+str(i)+"_d_vs_nd.pkl"
            with open(pklfile_name, "wb") as pklfile:
                pkl.dump(d_vs_nd, pklfile)
            pklfile_name = "rank"+str(rank)+"_run"+str(i)+"_callstack2count.pkl"
            with open(pklfile_name, "wb") as pklfile:
                pkl.dump(all_callstack_to_count, pklfile)


        

if __name__ == "__main__":
    main()
                                     
                                     
