# compares savestate dumps in the saves folder
# made with DEBUG_SAVESTATE_INTEGRITY

import os

# after this many mismatched files stop
MAX_ERRORS = 6

def filename_tf(t, f):
    return "hatarib_state_%02d_%03d.bin" % (t,f)

def tf_filename(fn):
    (base,ext) = os.path.splitext(fn)
    if ext.lower() == ".bin" and base.startswith("hatarib_state_"):
        t = int(base[14:16])
        f = int(base[17:20])
        return (t,f)
    return (-1,-1)

# index the files
timelines = {}
for file in os.listdir("."):
    (t,f) = tf_filename(file)
    if (t >= 0):
        if t not in timelines:
            timelines[t] = set()
        timelines[t].add(f)

# list of timelines and frames in first timeline
tlist = sorted(timelines.keys())
t0 = tlist[0]
flist = sorted(timelines[t0])

# compare all frames from first timeline against others
errors = 0
for f in flist:
    d0 = open(filename_tf(t0,f),"rb").read()
    for i in range(1,len(tlist)):
        ti = tlist[i]
        e = ""
        if f in timelines[ti]:
            d1 = open(filename_tf(ti,f),"rb").read()
            if len(d0) != len(d1):
                e = "file length mismatch: %8X != %8X" % (len(d0),len(d1))
                errors += 1
            else:
                for j in range(len(d0)):
                    if d0[j] != d1[j]:
                        e = "mismatch at: %8X" % (j)
                        errors += 1
                        break
        else:
            e = "no matching dump: %s" % (filename_tf(ti,f))
            # don't add this to errors
        print("%02d.%03d vs %02d.%03d %s" % (t0,f,ti,f,e))
    if errors >= MAX_ERRORS:
        print("MAX_ERRORS reached");
        break

if (errors == 0):
    print("Success! All timelines match.")
else:
    print("Failure!")
