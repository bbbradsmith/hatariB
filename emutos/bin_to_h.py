import os.path

COLS = 32
FILES = [
    "etos1024k.img",
    "etos192uk.img",
    "etos192us.img",
    ]

def bin_to_h(file):
    print(file)
    data = open(file,"rb").read()
    print("%d bytes" % (len(data)))
    data_name = os.path.splitext(file)[0]
    file_out = data_name + ".h"
    fo = open(file_out,"wt")
    s  = "// %s\n" % (file)
    s += "#define %s_len   %d\n" % (data_name, len(data))
    s += "const uint8_t %s[%s_len] = {\n" % (data_name, data_name)
    fo.write(s)
    pos = 0
    while pos < len(data):
        s = ""
        rc = 0
        while rc < COLS and pos < len(data):
            d = data[pos]
            if (rc == 0):
                s += "\t"
            s += "0x%02X," % (d)
            rc += 1
            pos += 1
        if rc > 0:
            s += "\n"
            fo.write(s)
    s = "};\n"
    fo.write(s)
    fo.close()
    print(file_out)

for f in FILES:
    bin_to_h(f)
    print()
print("Done.")
