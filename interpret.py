import copy
from collections import defaultdict

MAX_PAUSE_WITHIN_SEGMENT = 1.0
MAX_SEGMENT_DURATION = 3.0

class Peer:
    def __init__(self):
        self.segment_cursor = None

class Interpreter:
    def interpret(self, chunks, files):
        self._files = files
        segments = []
        peers = defaultdict(Peer)
        for chunk in chunks:
            segment_cursor = len(segments)
            if chunk["peeraddr"] in peers:
                peer = peers[chunk["peeraddr"]]
                if self._chunk_appendable_to_segment(chunk, segments[peer.segment_cursor]):
                    self._append_chunk_to_segment(chunk, segments[peer.segment_cursor])
                else:
                    peer.segment_cursor = segment_cursor
                    segments.append(self._new_segment(chunk))
            else:
                peers[chunk["peeraddr"]].segment_cursor = segment_cursor
                segments.append(self._new_segment(chunk))
        return segments

    def _chunk_appendable_to_segment(self, chunk, segment):
        return (chunk["begin"] == segment["end"] and
                chunk["peeraddr"] == segment["peeraddr"] and
                chunk["filenum"] == segment["filenum"] and
                ((chunk["t"] - (segment["onset"]+segment["duration"])) < MAX_PAUSE_WITHIN_SEGMENT))

    def _new_segment(self, chunk):
        segment = copy.copy(chunk)
        segment["onset"] = chunk["t"]
        segment["duration"] = self._duration_with_unadjusted_rate(chunk)
        segment["id"] = chunk["segment_id"] = chunk["id"]
        return segment

    def _append_chunk_to_segment(self, chunk, segment):
        segment["end"] = chunk["end"]
        duration = chunk["t"] - segment["onset"]
        if duration == 0:
            duration = self._duration_with_unadjusted_rate(segment)
        segment["duration"] = duration
        chunk["segment_id"] = segment["id"]

    def _duration_with_unadjusted_rate(self, chunk):
        file_duration = self._files[chunk["filenum"]]["duration"]
        chunk_size = chunk["end"] - chunk["begin"]
        file_size = self._files[chunk["filenum"]]["length"]
        return float(chunk_size) / file_size * file_duration
