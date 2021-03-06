#!/usr/bin/python

from tr_log_reader import *
from argparse import ArgumentParser
import colors
from PIL import Image
sys.path.insert(0, "visual-experiments")
from gatherer import Gatherer

parser = ArgumentParser()
parser.add_argument("sessiondir", type=str)
parser.add_argument("-t", "--torrent", dest="torrentname", default="")
parser.add_argument("-n", "--filenum", dest="filenum", type=int)
parser.add_argument("-width", type=int, default=1000)
parser.add_argument("-height", type=int, default=500)
parser.add_argument("-output", default="peer_contribution.png")
options = parser.parse_args()

logfilename = "%s/session.log" % options.sessiondir
log = TrLogReader(logfilename, options.torrentname, options.filenum).get_log()
print >> sys.stderr, "found %d chunks" % len(log.chunks)

total_size = max([chunk["end"] for chunk in log.chunks])

def time_to_x(t): return int(t / log.lastchunktime() * options.width)
def byte_to_y(byte_pos): return min(int(float(byte_pos) / total_size * options.height), options.height-1)

class Piece:
    def __init__(self, begin, end):
        self.begin = begin
        self.end = end

peer_colors = [(int(r*255), int(g*255), int(b*255))
                for (r,g,b,a) in colors.colors(len(log.peers))]
peers = {}

image = Image.new("RGB", (options.width, options.height), "white")
pixels = image.load()

def render_slice(x):
    for (gatherer, color) in zip(peers.values(), peer_colors):
        for piece in gatherer.pieces():
            y1 = byte_to_y(piece.begin)
            y2 = byte_to_y(piece.end)
            for y in range(y1, y2):
                pixels[x, y] = color

previous_x = None
for chunk in log.chunks:
    peer_id = log.peeraddr_to_id[chunk["peeraddr"]]
    try:
        peer = peers[peer_id]
    except KeyError:
        peer = peers[peer_id] = Gatherer()
    peer.add(Piece(chunk["begin"], chunk["end"]))
    new_x = time_to_x(chunk["t"])
    if previous_x is None:
        render_slice(new_x)
    else:
        for x in range(previous_x, new_x):
            render_slice(x)
    previous_x = new_x

image.save(options.output)
