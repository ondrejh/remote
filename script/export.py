#!/usr/bin/python3

import os

dirname = "../webi"
outfile = "../sketch/remote/webi.h"
content_type = {
    'html': 'text/html',
    'css': 'text/css',
    'ico': 'image/x-icon',
    'jpg': 'image/jpeg',
    'png': 'image/png'}

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

def contentType(fname):
    ext = fname.split('.')[-1].lower()
    if ext in content_type:
        return ext, content_type[ext]
    else:
        return ext, 'text/plain'
    
def addContent(fname, contents):
    ext, cont = contentType(fname)
    if cont not in contents:
        contents.append(cont)
    return contents.index(cont)

files = listdir(dirname)

with open(outfile, 'w') as fd:
    fd.write("#ifndef __WEBI_H__\n#define __WEBI_H__\n\n")

    fd.write("#define NFILES {}\n\n".format(len(files)))
    fd.write('\n// file content\n\n')
    flengths = []
    bnames = []
    pdatas = []
    contents = []
    contIds = []

    for f in files:
        bname = basename(f, dirname)
        fdata = open(f, 'rb').read()
        flen = len(fdata)

        cont = addContent(bname, contents)

        cname = bname.strip('/').replace('/', '_').replace('.', '_')
        lname = cname.upper() + '_LEN'
        contName = cname.upper() + '_CONT'

        flengths.append(lname)
        pname = '{}_name'.format(cname)
        pdata = '{}_bin'.format(cname)
        bnames.append(bname)
        pdatas.append(pdata)
        contIds.append(cont)

        fd.write("#define {} {}\n".format(lname, flen))
        fd.write("#define {} {}\n".format(contName, cont))
        fd.write("//const char *{} PROGMEM = \"{}\";\n".format(pname, bname))
        fd.write("const char {}[{}] PROGMEM = {}\n".format(pdata, lname, '{'))
        fd.write(writeList(fdata))
        fd.write('};\n\n')
        print(bname)

    #print(contents)

    fd.write('\n// contents\n\n')
    fd.write("const char *contents[{}] PROGMEM = {}".format(len(contents), '{'))
    fd.write(writeList(contents, rowCount=0, itemFormat='\"{}\"'))
    fd.write('};\n')

    fd.write('\n// list of files\n\n')

    fd.write("const unsigned long int flengths[NFILES] PROGMEM = {}".format('{'))
    fd.write(writeList(flengths, rowCount=0, itemFormat='{}'))
    fd.write('};\n')

    fd.write("const char *fnames[NFILES] PROGMEM = {}".format('{'))
    fd.write(writeList(bnames, rowCount=0, itemFormat='\"{}\"'))
    fd.write('};\n')

    fd.write("const char *fdata[NFILES] PROGMEM = {}".format('{'))
    fd.write(writeList(pdatas, rowCount=0, itemFormat='{}'))
    fd.write('};\n')

    fd.write("const int contIds[NFILES] PROGMEM = {}".format('{'))
    fd.write(writeList(contIds, rowCount=0, itemFormat='{}'))
    fd.write('};\n\n')

    fd.write('#endif')