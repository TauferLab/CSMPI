def only_numeric(string):
    return filter(type(string).isdigit, string)

def list_to_counts(l):
    d = {}
    for e in l:
        if e not in d:
            d[e] = 1
        else:
            d[e] += 1
    return d

def list_to_percentages(l):
    counts = list_to_counts(l)
    return {k:round(float(v)/sum(counts.values()),2) for k,v in counts.items()}
