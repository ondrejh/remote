filename = "../sketch/remote/webi.h"
outputname = "webi_test.h"

print('Copy {} to {}, remove PROGMEM pragmas.'.format(filename, outputname))

with open(filename, 'r') as fdi:
    with open(outputname, 'w') as fdo:
        for line in fdi.readlines():
            fdo.write(line.replace('PROGMEM ', ''))