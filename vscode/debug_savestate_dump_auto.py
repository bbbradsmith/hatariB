# Can be used with DEBUG_SAVESTATE_DUMP_AUTO test.
#
# core.c: Set DEBUG_SAVESTATE_DUMP_AUTO to some number of frames
#         for an automatic snapshot after start. (E.g. 1000)
# memorSnapShot.c: Enable LIBRETRO_DEBUG_SNAPSHOT.
#
# Disable host keyboard and host mouse in options to avoid divergence.
#
# Test on both platforms, then use this to compare the two dumps.
#

import struct

dumpa = "hatarib_auto_disk.st.dump.win64"
dumpb = "hatarib_auto_disk.st.dump.ubuntu"
SNAPSHOT_ROUND = 64 * 1024

def split_dump(fn,sc=None,scn=None):
    print("split_dump(\""+fn+"\")")
    d = open(fn,"rb").read()
    o = len(d) - (len(d) % SNAPSHOT_ROUND) # find suffix via the rounded data boundary
    count = (len(d) - o) // 16
    assert(len(d) == (o + (count * 16))) # make sure entry list is as expected
    sd = []
    for i in range(count):
        eo = o + (i * 16)
        (po,ps) = struct.unpack("<LL",d[eo:eo+8])
        name = ""
        for j in range(8):
            c = d[eo+8+j]
            if (c == 0): break
            name += chr(c)
        if len(name) < 1: name = "INVALID%04d" % (i)
        assert ((po + ps) <= len(d)) # make sure entry fits in file
        print("%8X %8X %10d %10d %s" % (po,ps,po,ps,name))
        pd = d[po:po+ps]
        sd.append((po,ps,name,pd)) # note that if a segment differs in size (ps), po will differ for all subsequent segments
        if sc != None and sc[i][1:] != sd[i][1:]: # split out any segments that differ
            fne = "d." + fn + "." + name + ".bin"
            scne = "d." + scn + "." + name + ".bin"
            print("DIFF %s <-> %s" % (fne,scne))
            open(fne,"wb").write(sd[i][3])
            open(scne,"wb").write(sc[i][3])
    print("%d segments" % (count))
    return sd

sa = split_dump(dumpa)
sb = split_dump(dumpb,sa,dumpa)
