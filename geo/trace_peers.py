#!/usr/bin/python

from argparse import ArgumentParser
import subprocess
import re
import ip_locator
import cPickle
import threading
import time

import sys, os
sys.path.append(os.path.dirname(__file__)+"/..")
from tr_log_reader import *

parser = ArgumentParser()
parser.add_argument("sessiondir", type=str)
args = parser.parse_args()

logfilename = "%s/session.log" % args.sessiondir
log = TrLogReader(logfilename).get_log()
output_filename = "%s/traces.data" % args.sessiondir
output = open(output_filename, "w")

ip_locator = ip_locator.IpLocator()
ip_matcher = re.compile(' (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}) ')

lock = threading.Lock()

def trace(peer_addr):
    global traces, lock
    result = []
    p = subprocess.Popen("traceroute -n %s -w 1.0 -N 1" % peer_addr,
                         shell=True,
                         stdout=subprocess.PIPE)
    for line in p.stdout:
        m = ip_matcher.search(line)
        if m:
            hop_addr = m.group(1)
            location = ip_locator.locate(hop_addr)
            if location:
                result.insert(0, location)
    print "OK1 %s" % peer_addr
    with lock:
        traces[peer_addr] = result
    print "OK2 %s" % peer_addr

def trace_in_new_thread(addr):
    thread = threading.Thread(target=trace, args=[addr])
    thread.daemon = True
    thread.start()

traces = {}
for addr in log.peers:
    print addr
    #traces[addr] = trace(addr)
    trace_in_new_thread(addr)

finished = False
while not finished:
    time.sleep(0.1)
    with lock:
        if len(traces) == len(log.peers):
            finished = True

cPickle.dump(traces, output)
output.close()
