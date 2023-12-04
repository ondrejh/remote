#!/usr/bin/python3

import os

dirname = "../webi"
outfile = "../sketch/remote/webi.h"

def basename(filename, dirname):
    return(filename[len(dirname):])

def listdir(dirname, files=[]):
    for f in os.listdir(dirname):
        fullname = os.path.join(dirname, f)
        if os.path.isfile(fullname):
            files.append(fullname)
        elif os.path.isdir(fullname):
            listdir(fullname, files)
    return files

def writeList(data, rowCount=20, itemFormat='0x{:02X}'):
    strList = ''
    dl = len(data)
    for i, d in enumerate(data):
        ip = i + 1
        ending = '' if (ip == dl) else (', ' if (rowCount < 1) or (ip % rowCount) else ',\n')
        strList += '{}{}'.format(itemFormat.format(d), ending)
    return strList

files = listdir(dirname)

with open(outfile, 'w') as fd:
    fd.write("#ifndef __WEBI_H__\n#define __WEBI_H__\n\n")

    fd.write("#define NFILES {}\n\n".format(len(files)))
    fd.write('\n// file content\n\n')
    flengths = []
    bnames = []
    pdatas = []

    for f in files:
        bname = basename(f, dirname)
        fdata = open(f, 'rb').read()
        flen = len(fdata)

        cname = bname.strip('/').replace('/', '_').replace('.', '_')
        lname = cname.upper() + '_LEN'

        flengths.append(lname)
        pname = '{}_name'.format(cname)
        pdata = '{}_bin'.format(cname)
        bnames.append(bname)
        pdatas.append(pdata)

        fd.write("#define {} {}\n".format(lname, flen));
        fd.write("//const char *{} PROGMEM = \"{}\";\n".format(pname, bname))
        fd.write("const char {}[{}] PROGMEM = {}\n".format(pdata, lname, '{'))
        fd.write(writeList(fdata))
        fd.write('};\n\n')
        print(bname)

    fd.write('\n// list of files\n\n')

    fd.write("const unsigned long int flengths[NFILES] PROGMEM = {}".format('{'))
    fd.write(writeList(flengths, rowCount=0, itemFormat='{}'))
    fd.write('};\n')

    fd.write("const char *fnames[NFILES] PROGMEM = {}".format('{'))
    fd.write(writeList(bnames, rowCount=0, itemFormat='\"{}\"'))
    fd.write('};\n')

    fd.write("const char *fdata[NFILES] PROGMEM = {}".format('{'))
    fd.write(writeList(pdatas, rowCount=0, itemFormat='{}'))
    fd.write('};\n\n')

    fd.write('#endif')