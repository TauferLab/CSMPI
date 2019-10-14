import matplotlib.pyplot as plt
from matplotlib import cm
import numpy as np
import argparse
import cPickle as pkl
import pprint

def generate_d_vs_nd_pie_chart(num_d, num_nd, run, rank, test=False):
    sizes = sorted([num_d, num_nd])
    labels = ["deterministic", "nondeterministic"]
    fig, ax = plt.subplots()
    wedges, texts, autotexts = ax.pie(sizes, 
                                       colors=["cyan", "red"], 
                                       #labels=labels, 
                                       autopct="%1.0f%%", 
                                       pctdistance=0.75,
                                       startangle=25) # somewhat emprically determined
    ### Set wedge edge color to white for visibility
    for w in wedges:
        w.set_edgecolor("white")
    ### Make percent texts visible
    for txt in autotexts:
        txt.set_fontsize(24) 
    ax.axis("equal")
    if test:
        plt.show()
    else:
        plt.savefig("run_"+str(run)+"_rank_"+str(rank)+"_d_vs_nd.png",
                    bbox_inches="tight",
                    transparent=True,
                    pad_inches=0.1,
                    format="png"
                    )

def generate_callstack_to_count_pie_chart(callstack_to_count, run, rank, test=False):
    labels = []
    sizes = []
    for cs in callstack_to_count.keys():
        if callstack_to_count[cs] > 0:
            cs_label = ""
            for i in range(1,len(cs)):
                cs_label += str(cs[i])
                if i != (len(cs)-1):
                    cs_label += "\n"
            labels.append(cs_label)
            sizes.append(callstack_to_count[cs])
    ### Ensures small wedges are near each other
    sizes = sorted(sizes)
    fig, ax = plt.subplots()
    colors = cm.Set3(np.arange(len(sizes))/float(len(sizes)))
    #ax.pie(sizes, labels=labels, colors=colors, autopct="%1.1f%%", pctdistance=0.9, radius=2)
    patches, texts, autotexts= ax.pie(sizes, 
                                      colors=colors, 
                                      autopct="%1.1f%%", 
                                      pctdistance=1.1, 
                                      radius=2)
    ### ensure pie is circle
    ax.axis("equal") 
    print "Run "+str(run)+" Rank "+str(rank)
    pprint.pprint({k:v for k,v in zip(labels,colors)})
    if test:
        plt.show()
    else:
        plt.savefig("run_"+str(run)+"_rank_"+str(rank)+"_callstack_to_count.png",
                    bbox_inches="tight",
                    transparent=True,
                    pad_inches=0.1,
                    format="png"
                    )


def main():
    parser = argparse.ArgumentParser(description="")
    parser.add_argument("-d", "--d_vs_nd", nargs=1,
                        help="")
    parser.add_argument("-c", "--callstack_to_count", nargs=1,
                        help="")
    args = parser.parse_args()
    
    with open(args.d_vs_nd[0], "rb") as pklfile:
        d_vs_nd = pkl.load(pklfile)
        d = d_vs_nd["d"]
        nd = d_vs_nd["nd"]
    generate_d_vs_nd_pie_chart(d, nd, 0, 0, test=True)

    with open(args.callstack_to_count[0], "rb") as pklfile:
        callstack_to_count = pkl.load(pklfile)
    generate_callstack_to_count_pie_chart(callstack_to_count, 0, 0, test=True)


if __name__ == "__main__":
    main()
