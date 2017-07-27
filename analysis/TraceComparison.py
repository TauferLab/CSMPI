import argparse
import difflib
import re

import time
import pprint

from TraceDescription import TraceDescription

class TraceComparison(object):
    def __init__(self, run_1_trace, run_2_trace):
        self.run_1_trace = run_1_trace
        self.run_2_trace = run_2_trace
        self.run_1_desc = TraceDescription(self.run_1_trace)
        self.run_2_desc = TraceDescription(self.run_2_trace)
        self.shared_properties = self.sanity_check()

    
    """
    """
    def sanity_check(self):
        print "Sanity checking runs"
        props = {"ranks":None}
        run_1_ranks = sorted(self.run_1_desc.run_properties["ranks"])
        run_2_ranks = sorted(self.run_2_desc.run_properties["ranks"])
        if run_1_ranks != run_2_ranks:
            print "Run 1's ranks != Run 2's ranks"
            print "Run 1: " + str(run_1_ranks)
            print "Run 2: " + str(run_2_ranks)
            exit()
        else:
            props["ranks"] = run_1_ranks
        return props
        

    """
    """
    def compare(self):
        ### Build trace descriptions
        self.run_1_desc.build_description()
        self.run_2_desc.build_description()
        ### Compare the send sequences
        rank_to_send_diff = self.get_rank_to_send_diff()
        ### Compare the recv completion sequences 

    """
    """
    def get_rank_to_send_diff(self):
        rank_to_send_diff = {}
        for rank in self.shared_properties["ranks"]:
            rank_to_send_diff[rank] = self.compare_sends(rank)
            exit()

    """
    """
    def compare_sends(self, rank):
        run_1_events = self.run_1_desc.rank_to_event_seqs[rank]
        run_2_events = self.run_2_desc.rank_to_event_seqs[rank]
        ### Get the isend sequences and make a big string out
        ### of each so that difflib stuff can be used
        run_1_isends = run_1_events["isend"] 
        run_2_isends = run_2_events["isend"] 
       
        ### Create the diff
        run_1_isends_strs = [ str(x) for x in run_1_isends ]
        run_2_isends_strs = [ str(x) for x in run_2_isends ]
        print "run 1 # isends: " + str(len(run_1_isends_strs))
        print "run 2 # isends: " + str(len(run_2_isends_strs))
        exit()

        ### Casting to list here out of convenience 
        num_context_lines = 3 ### This is default but idk what effect this has yet
        diff = list(difflib.unified_diff(run_1_isends_strs, run_2_isends_strs, n=num_context_lines))

        ### Separate the hunks of the diff for analysis
        hunks = []
        hunk_start_re = re.compile("^@@ -[\d]+,[\d]+ \+[\d]+,[\d]+ @@$")
        i = 0
        while not hunk_start_re.match(diff[i]):
            i += 1
            try:
                if hunk_start_re.match(diff[i]):
                    hunk = [diff[i]]
                    while not hunk_start_re.match(diff[i+1]) and i < len(diff)-2:
                        i += 1
                        hunk.append(diff[i])
                    hunks.append(hunk)
            except IndexError:
                break
        hunks[-1].append(diff[-1]) # ugh
        
        ### Classify the kind of sender ND present in each hunk
        hunk_to_ND_class = {}
        for h in hunks:
            hunk_info = h[0]
            plus_lines = [ y[1:] for y in filter(lambda x: x[0] == "+", h) ]
            minus_lines = [ y[1:] for y in filter(lambda x: x[0] == "-", h) ]
            if sorted(plus_lines) == sorted(minus_lines):
                hunk_to_ND_class[hunk_info] = (len(plus_lines), "shuffled")
                
                


        #with open("run_1_isends.txt", "wb") as isend_file:
        #    for s in run_1_isends_strs:
        #        isend_file.write(s)
        #        isend_file.write("\n")
        #with open("run_2_isends.txt", "wb") as isend_file:
        #    for s in run_2_isends_strs:
        #        isend_file.write(s)
        #        isend_file.write("\n")
        #with open("isends_diff.txt", "wb") as isend_diff:
        #    for line in difflib.unified_diff(run_1_isends_strs, run_2_isends_strs):
        #        isend_diff.write(line)
        #        isend_diff.write("\n")

        #run_1_isends_str = "".join([str(x) for x in run_1_isends])
        #run_2_isends_str = "".join([str(x) for x in run_2_isends])
        #print run_1_isends_str
        #print run_2_isends_str
        #sm = difflib.SequenceMatcher(None, run_1_isends_str, run_2_isends_str)
        #print "Finding matching blocks of isends:"
        #start_time = time.time()
        #matching_blocks = sm.get_matching_blocks()
        #print "Elapsed = " + str(time.time() - start_time)
        #pprint.pprint(matching_blocks)



def main():
    parser = argparse.ArgumentParser(description="Compares two trace descriptons\
                                     to extract the MPI events that are         \
                                     consistent between the two runs and those  \
                                     that differ and thus are manifestations of \
                                     MPI or application nondeterminism.")
    parser.add_argument("-t1", "--trace_dir_1", nargs=1,
                        help="The directory containing the CSMPI traces for the \
                             first run.")
    parser.add_argument("-t2", "--trace_dir_2", nargs=1,
                        help="The directory containing the CSMPI traces for the \
                             second run.")
    args = parser.parse_args()

    #td1 = TraceDescription(args.trace_dir_1[0])
    #td2 = TraceDescription(args.trace_dir_2[0])
    #td1.build_description()
    #td2.build_description()

    tc = TraceComparison(args.trace_dir_1[0], args.trace_dir_2[0])
    tc.compare()


if __name__ == "__main__":
    main()


