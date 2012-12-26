#!/usr/bin/python3
# Converts OpenStreetMap XML data files (*.osm) to JSON. Useful for obtaining so large test dataasets.
# It is good, but not perfect, sometimes creates invalid JSON.

import xml.sax
import xml.sax.handler
import sys
import os.path

infile = sys.stdin
if len(sys.argv) >= 2:
    infile = open(sys.argv[1], 'r')

#json receiver
class JsonReceiver:

    def receive(self, name, json):
        #self.outfile.write(json + '\n')
        print(json)

# handler
class Handler(xml.sax.handler.ContentHandler):

    def __init__(self, receiver):
        self.receiver = receiver
        self.currentName = None
        self.depth = 0

    def startElement(self, name, attrs):
        if name == 'osm':
            return

        self.depth += 1
        if self.depth == 1:
            self.currentName = name
            self.current = '{'
            #self.subobjSep = ''
        else:
            #self.current += self.subobjSep
            #self.subobjSep = ', '
            self.current += ', "' + name + '" : {'
            
        self.addAttrs(attrs)

    def addAttrs(self, attrs):
        sep = ''
        for k in attrs.keys():
            self.current += sep
            sep = ', '
            self.current += '"' + k + '" : '
            v = attrs[k]
            try:
                f = float(v)
                if  f == int(f):
                    self.current += str(int(f))
                else:
                    self.current += str(f)
            except(ValueError):
                self.current += '"' + v.replace('"', '\\"') + '"'
            
        

    def endElement(self, name):
        self.current += '}'
        if self.depth == 1:
            self.receiver.receive(self.currentName, self.current)
        self.depth -= 1

    
    
# parse
receiver = JsonReceiver()
handler = Handler(receiver)
xml.sax.parse(infile, handler)
