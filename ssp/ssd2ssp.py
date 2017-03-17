#!/usr/bin/python
from __future__ import print_function
import sys
import xml.etree.ElementTree as e

def warning(*objs):
    print("WARNING: ", *objs, file=sys.stderr)

def error(*objs):
    print('#error ',*objs)
    print("ERROR: ", *objs, file=sys.stderr)
if len(sys.argv) < 2:
    print('USAGE: '+sys.argv[0] +' modelDescription-filename > header-filename', file=sys.stderr)
    print('Example: '+sys.argv[0]+' modelDescription.xml > header.h', file=sys.stderr)
    exit(1)

tree = e.parse(sys.argv[1])
root = tree.getroot()

ns = {
    'ssd': 'http://www.pmsf.net/xsd/SystemStructureDescriptionDraft',
    'ssv': 'http://www.pmsf.net/xsd/SystemStructureParameterValuesDraft',
    'ssm': 'http://www.pmsf.net/xsd/SystemStructureParameterMappingDraft',
    'umit':'http://umit.math.umu.se/UMITSSD',
}

system =  root.find('ssd:System',ns)
elements  = system.find('ssd:Elements',ns)
resources = []
for comp in elements:
    if(comp.get('source')):
        for source in comp.get('source').split('/'):
            for fmu in source.split('.'):
                if fmu not in resources:
                    resources += [fmu]

if 'resources' in resources:
    resources.remove('resources')

if 'fmu' in resources:
    resources.remove('fmu')

ssdfile = sys.argv[1].split('/')[-1]
sspname = system.get('name')
path = "CMakeLists.txt"

with open(path,'w') as f:
    cmakestuff = ('''#This file was generated by ssd2ssp.py. DO NOT EDIT

CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

set(FMUS
    %s)
set(ZIP_COMMAND  ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/%s.ssp --format=zip .)
set(TARGET %s)
set(ZIPDIR /tmp/%s)

add_custom_target(${TARGET} ALL)
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ZIPDIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ZIPDIR}/resources
    COMMAND COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/SystemStructure.ssd ${ZIPDIR}
    COMMAND COMMAND ${CMAKE_COMMAND} -E copy ${FMUS} ${ZIPDIR}/resources

    COMMAND ${CMAKE_COMMAND} -E chdir ${ZIPDIR} ${ZIP_COMMAND}
    DEPENDS ${FMUS}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${ZIPDIR}
    )
'''% (
       '\n    '.join('${'+str(r).upper()+'_FMU}' for r in resources),
    sspname.lower(),
    sspname.upper(),
    sspname.lower(),
))
    f.write( cmakestuff )
    f.close()
