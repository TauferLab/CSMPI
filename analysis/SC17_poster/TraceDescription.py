import argparse 
import glob
import cPickle as pkl

import pprint 
import time

"""
"""
class TraceDescription(object):
    def __init__(self, csmpi_dir):
        self.csmpi_dir = csmpi_dir
        self.run_properties = self.get_run_properties()
        self.rank_to_event_seqs = None
        self.rank_to_event_counts = None

    
    """
    Extracts the properties of the run from the run directory
    name, the CSMPI log files, the stdout and stderr files for 
    the job, etc. 
    """
    def get_run_properties(self):
        props = {"ranks":None} 
        ### Get the set of MPI ranks 
        csmpi_logs = glob.glob(self.csmpi_dir+"/*.log")
        ranks = [ int(x.split("_")[-1].split(".")[0]) for x in csmpi_logs ]
        props["ranks"] = ranks
        return props 

    """ 
    Counts MPI events for all ranks
    Returns: a dict of the form {mpi_rank: {event_type: count}}
    """
    def get_rank_to_event_counts(self):
        rank_to_event_counts = {}
        for rank in self.run_properties["ranks"]:
            rank_to_event_counts[rank] = self.count_events(rank)
        return rank_to_event_counts

        
    """
    Returns: a dict of the form 
             {mpi_rank: {event_type: list of event signatures}}
    """
    def get_rank_to_event_seqs(self):
        rank_to_event_seqs = {}
        for log_file in glob.glob(self.csmpi_dir+"/*.log"):
            rank = int(log_file.split("_")[-1].split(".")[0])
            rank_to_event_seqs[rank] = self.get_event_seq(log_file)
        return rank_to_event_seqs

    
    """
    Counts MPI events for a single rank
    Takes: a dict of the form {event_type: list of event signatures} 
    Returns: a dict of the form {event_type: count}
    """
    def count_events(self, rank):
        return {k:len(v) for k,v in self.rank_to_event_seqs[rank].items()}

   
    """
    Accumulates the sequences of MPI event signatures 
    Takes: a path of a CSMPI log file
    Returns: a dict of the form {event_type: list of event signatures}
    """
    def get_event_seq(self, log_file):
        event_type_to_seq = {}
        with open(log_file, "rb") as log:
            for line in log:
                try:
                    event_type, event_sig = self.recognize_event(line)
                    if event_type not in event_type_to_seq:   
                        event_type_to_seq[event_type] = [event_sig]
                    else:
                        event_type_to_seq[event_type].append(event_sig)
                except:
                    pass
        return event_type_to_seq


    """
    Recognizes MPI events in CSMPI log
    Takes: a single line of a CSMPI log
    Returns: a pair of the form: (event_type, event_signature)
    """
    def recognize_event(self, line):
        if "call=" in line:
            event = [ x.strip() for x in line.split(",") ]
            event_type = event[0].split("=")[-1]
            event_sig = event[1:]
        return (event_type, event_sig)
    
    """
    Builds a description of the run including:
    1. Counts of each send initiation call
    2. Counts of each recv initiation call
    3. Counts of each recv completion call
    """
    def build_description(self, verbose=False):
        print "Building trace description for run: " 
        print "Building event sequences:"
        start_time = time.time()
        self.rank_to_event_seqs = self.get_rank_to_event_seqs()
        print "Elapsed time = " + str(time.time() - start_time)
        self.rank_to_event_counts = self.get_rank_to_event_counts()

        if verbose:
            #print "Rank to event counts:"
            #pprint.pprint(self.rank_to_event_counts)
            print "Rank to send counts:"
            pprint.pprint(self.get_rank_to_send_counts())
            print "Rank to single-recv completion counts:"
            pprint.pprint(self.get_rank_to_single_recv_completion_counts())
            print "Rank to mutli-recv completion counts:"
            pprint.pprint(self.get_rank_to_multi_recv_completion_counts())

            #print "Rank 0 counts of send events grouped by destination"
            #pprint.pprint(self.count_sends_by_dst(0))
            #print "Rank 0 counts of send events grouped by message tag"
            #pprint.pprint(self.count_sends_by_tag(0))
            #print "Rank 0 counts of send events grouped by dest and tag"
            #pprint.pprint(self.count_sends_by_dst_and_tag(0))

            #print "Rank 0 waitany completions grouped by src"
            #pprint.pprint(self.group_single_recv_waits(0, "waitany"))
            #print "Rank 0 waitany completions grouped by tag"
            #pprint.pprint(self.group_single_recv_waits(0, "waitany", groupby_src=False, groupby_tag=True))
            #print "Rank 0 waitany completions grouped by src and tag"
            #pprint.pprint(self.group_single_recv_waits(0, "waitany", groupby_src=True, groupby_tag=True))

    
    """
    """
    def get_rank_to_single_recv_completion_counts(self):
        rank_to_counts = {}
        for rank in self.run_properties["ranks"]:
            counts = self.count_single_recv_completions(rank)
            rank_to_counts[rank] = counts
        return rank_to_counts


    """
    """
    def get_rank_to_multi_recv_completion_counts(self):
        rank_to_counts = {}
        for rank in self.run_properties["ranks"]:
            counts = self.count_multi_recv_completions(rank)
            rank_to_counts[rank] = counts
        return rank_to_counts


    """
    """
    def count_multi_recv_completions(self, rank):
        events = self.rank_to_event_seqs[rank]
        completion_types = ["testsome", "testall", "waitsome", "waitall"]
        completion_type_to_count = {}
        for ct in completion_types:
            try:
                completion_type_to_count[ct] = len(events[ct])
            except KeyError:
                completion_type_to_count[ct] = 0
        return completion_type_to_count


    """
    Counts for a single rank the number of times MPI functions that
    complete a single previously posted receive were called.
    """
    def count_single_recv_completions(self, rank,
                                      groupby_src=False,
                                      groupby_tag=False):
        events = self.rank_to_event_seqs[rank]
        completion_types = ["test", "testany", "wait", "waitany"]
        completion_type_to_count = {}
        for ct in completion_types:
            if groupby_src and groupby_tag:
                exit()
            elif groupby_src and not groupby_tag:
                exit()
            elif groupby_tag and not groupby_src:
                exit()
            else:
                try:
                    completion_type_to_count[ct] = len(events[ct])
                except KeyError:
                    completion_type_to_count[ct] = 0
        return completion_type_to_count


    """
    compl_type should be "wait" or "waitany"
    """
    def group_single_recv_waits(self, rank, compl_type, 
                                groupby_src=True,
                                groupby_tag=False):
        seq = self.rank_to_event_seqs[rank][compl_type]
        ### Group only by source rank
        if groupby_src and not groupby_tag:
            src_to_seq = {}
            for c in seq:
                src = c[0]
                if src not in src_to_seq:
                    src_to_seq[src] = [c]
                else:
                    src_to_seq[src].append(c)
            return src_to_seq
        ### Group only by message tag
        elif not groupby_src and groupby_tag:
            tag_to_seq = {}
            for c in seq:
                tag = c[1]
                if tag not in tag_to_seq:
                    tag_to_seq[tag] = [c]
                else:
                    tag_to_seq[tag].append(c)
            return tag_to_seq
        ### Group by source rank and msg tag
        ### For right now, just support source rank as top lvl grouping
        elif groupby_src and groupby_tag:
            src_to_seqs = {}
            for c in seq:
                src, tag = c
                if src not in src_to_seqs:
                    src_to_seqs[src] = {}
                    src_to_seqs[src][tag] = [c]
                else:
                    if tag not in src_to_seqs[src]:
                        src_to_seqs[src][tag] = [c]
                    else:
                        src_to_seqs[src][tag].append(c)
            return src_to_seqs


    """
    Counts MPI send events for all ranks, grouped by destination rank or 
    message tag if desired. 
    Returns: a dict of the form {mpi_rank: {send_type: count}} where 
             count may be an int or a dict representing the breakdown
             of sends by destination rank or message tag. 
    """
    def get_rank_to_send_counts(self, 
                                groupby_dst=False,
                                groupby_tag=False):
        rank_to_send_counts = {}
        for rank in self.run_properties["ranks"]:
            if groupby_dst and groupby_tag:
                rank_to_send_counts[rank] = self.count_sends_by_dst_and_tag(rank)
            elif groupby_dst and not groupby_tag:
                rank_to_send_counts[rank] = self.count_sends_by_dst(rank)
            elif groupby_tag and not groupby_dst:
                rank_to_send_counts[rank] = self.count_sends_by_tag(rank)
            else:
                rank_to_send_counts[rank] = self.count_sends(rank)
        return rank_to_send_counts

    
    """
    Counts for a single rank the number of times MPI functions that
    initiate a send were called. 
    """
    def count_sends(self, rank):
        events = self.rank_to_event_seqs[rank]
        send_types = filter(lambda x: "send" in x, events)
        send_type_to_counts = {}
        for st in send_types:
            send_type_to_counts[st] = len(events[st])
        return send_type_to_counts

           
    """
    """
    def count_sends_by_dst(self, rank):
        events = self.rank_to_event_seqs[rank]
        send_types = filter(lambda x: "send" in x, events)
        send_type_to_counts = {}
        for st in send_types:
            dst_to_seq = self.group_send_events(rank, st)
            dst_to_count = {k:len(v) for k,v in dst_to_seq.items()}
            send_type_to_counts[st] = dst_to_count
        return send_type_to_counts

    """
    """
    def count_sends_by_tag(self, rank):
        events = self.rank_to_event_seqs[rank]
        send_types = filter(lambda x: "send" in x, events)
        send_type_to_counts = {}
        for st in send_types:
            tag_to_seq = self.group_send_events(rank, st, 
                                                groupby_dst=False,
                                                groupby_tag=True)
            tag_to_count = {k:len(v) for k,v in tag_to_seq.items()}
            send_type_to_counts[st] = tag_to_count
        return send_type_to_counts

    """
    """
    def count_sends_by_dst_and_tag(self, rank):
        events = self.rank_to_event_seqs[rank]
        send_types = filter(lambda x: "send" in x, events)
        send_type_to_counts = {}
        for st in send_types:
            dst_to_seqs = self.group_send_events(rank, st, 
                                                 groupby_dst=True,
                                                 groupby_tag=True)
            counts = {k:{kk:len(vv) for kk,vv in v.items()} for k,v in dst_to_seqs.items()} 
            send_type_to_counts[st] = counts
        return send_type_to_counts


    """
    Group the sequence of send events of one type (e.g., isends) for one rank
    either by destination rank, by tag, or by both
    """
    def group_send_events(self, rank, send_type,
                          groupby_dst=True, groupby_tag=False):
        seq = self.rank_to_event_seqs[rank][send_type]
        ### Group only by destination rank
        if groupby_dst and not groupby_tag:
            dst_to_seq = {}
            for s in seq:
                dst = s[1]
                if dst not in dst_to_seq:
                    dst_to_seq[dst] = [s]
                else:
                    dst_to_seq[dst].append(s)
            return dst_to_seq
        ### Group only by msg tag
        elif not groupby_dst and groupby_tag:
            tag_to_seq = {}
            for s in seq:
                tag = s[2]
                if tag not in tag_to_seq:
                    tag_to_seq[tag] = [s]
                else:
                    tag_to_seq[tag].append(s)
            return tag_to_seq
        ### Group by destination rank and msg tag
        ### For right now, we'll just support the dst as the top lvl grouping
        elif groupby_dst and groupby_tag:
            dst_to_seqs = {}
            for s in seq:
                dst, tag = s[1:]
                if dst not in dst_to_seqs:
                    dst_to_seqs[dst] = {}
                    dst_to_seqs[dst][tag] = [s]
                else:
                    if tag not in dst_to_seqs[dst]:
                        dst_to_seqs[dst][tag] = [s]
                    else:
                        dst_to_seqs[dst][tag].append(s)
            return dst_to_seqs

    
     
        



def main():
    parser = argparse.ArgumentParser(description="Describes a single run of an \
                                     MPI app that has been traced by CSMPI")
    parser.add_argument("-d", "--csmpi_dir", nargs=1, 
                        help="The directory containing the CSMPI traces")
    args = parser.parse_args()
    td = TraceDescription(args.csmpi_dir[0])
    td.build_description()
    


if __name__ == "__main__":
    main()

