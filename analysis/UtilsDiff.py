import subprocess as sp
import re
import argparse 

from UtilsPreprocessing import only_numeric, list_to_percentages

import pprint

"""
Returns the unified-format diff 
"""
def run_diff(file_1, file_2, diff_file):
    cmd = ["diff", "-u", file_1, file_2]
    try:
        p = sp.check_output(cmd)
    except sp.CalledProcessError, e:
        return e.output.split("\n")

"""
Returns a list of the diff hunks
"""
def separate_hunks(diff):
    hunks = []
    hunk_start = re.compile("^@@ -[\d]+,[\d]+ \+[\d]+,[\d]+ @@$")
    i = 0
    while not hunk_start.match(diff[i]):
        i += 1
        try:
            if hunk_start.match(diff[i]):
                hunk = [diff[i]]
                while not hunk_start.match(diff[i+1]) and i < len(diff)-2:
                    i += 1
                    hunk.append(diff[i])
                hunks.append(hunk)
        except IndexError:
            break
    return hunks

"""
Attempt to provide a quantitative description of the indicators of 
nondeterminism (possibly) present in this hunk. 
"""
def describe_hunk(hunk):
    header = hunk[0]
    plus_lines = [ x[1:] for x in filter(lambda x: x[0] == "+", hunk)]
    plus_lines = [ [x.strip() for x in line.split(",")] for line in plus_lines ]
    plus_lines = [ [int(only_numeric(x)) for x in line] for line in plus_lines ]
    minus_lines = [ x[1:] for x in filter(lambda x: x[0] == "-", hunk)]
    minus_lines = [ [x.strip() for x in line.split(",")] for line in minus_lines ]
    minus_lines = [ [int(only_numeric(x)) for x in line] for line in minus_lines ]
    descr = {}
    ### Case: the sends are the same set, just in different order
    if plus_lines == minus_lines:
        dsts = []
        tags = []
        for line in plus_lines:
            dsts.append(line[1])
            tags.append(line[2])
        descr["type"] = "shuffle"
        descr["count"] = len(dsts)
        descr["dsts"] = dsts
        descr["dsts_stats"] = list_to_percentages(dsts)
        descr["tags"] = tags
        descr["tags_stats"] = list_to_percentages(tags)
    else:
        ### Case: Same number of sends issued, but destinations, tags, or
        ### payloads differed
        if len(plus_lines) == len(minus_lines):
            p_cnts = []
            p_dsts = []
            p_tags = []
            m_cnts = []
            m_dsts = []
            m_tags = []
            for line in plus_lines:
                p_cnts.append(line[0])
                p_dsts.append(line[1])
                p_tags.append(line[2])
            for line in minus_lines:
                m_cnts.append(line[0])
                m_dsts.append(line[1])
                m_tags.append(line[2])
            ### Subcase: same order of destinations
            if p_dsts == m_dsts:
                ### Subcase: only payloads differ
                if p_tags == m_tags and p_cnts != m_cnts:
                    descr["type"] = "diff payloads"
                    descr["count"] = len(p_dsts)
                    descr["dsts"] = p_dsts
                    descr["dsts_stats"] = list_to_percentages(p_dsts)
                    descr["tags"] = p_tags
                    descr["tags_stats"] = list_to_percentages(p_tags)
                ### Subcase: only tags differ
                if p_tags != m_tags and p_cnts == m_cnts:
                    descr["type"] = "diff tags"
                    descr["count"] = len(p_dsts)
                    descr["dsts"] = p_dsts
                    descr["dsts_stats"] = list_to_percentages(p_dsts)
                    descr["cnts"] = p_cnts
                    descr["cnts_stats"] = list_to_percentages(p_cnts)
                ### Subcase: tags and payloads differ
                    descr["type"] = "diff tags and payloads"
                    descr["count"] = len(p_dsts)
                    
                
    return descr
        
        

def main():
    parser = argparse.ArgumentParser(description="Utility functions for using  \
                                     unix diff to compare CSMPI traces.")
    parser.add_argument("-t", "--test", action="store_true",
                        help="Run tests")
    args = parser.parse_args()
    if args.test:
        diff = run_diff("run_1_isends.txt", "run_2_isends.txt", "test_diff.txt")
        hunks = separate_hunks(diff)
        nd_count = 0
        for h in hunks:
            #print "Raw hunk"
            #pprint.pprint(h)
            #print "Description"
            descr = describe_hunk(h)
            #pprint.pprint(descr)
            try:
                nd_count += descr["count"]
            except KeyError:
                pass
        print nd_count


if __name__ == "__main__":
    main()
