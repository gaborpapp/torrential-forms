from OpenGL.GL import *
from OpenGL.GLUT import *
from OpenGL.GLU import *
import sys, os
import liblo
import time
import argparse
import collections
from vector import Vector2d
import logging

dirname = os.path.dirname(__file__)
if dirname:
    sys.path.append(dirname + "/..")
else:
    sys.path.append("..")
from orchestra import VISUALIZER_PORT
from synth_controller import SynthController
from orchestra_controller import OrchestraController
from osc_receiver import OscReceiver

logging.basicConfig(filename="visualizer.log", 
                    level=logging.DEBUG, 
                    filemode="w")

ESCAPE = '\033'
MARGIN = 30
BORDER_OPACITY = 0.7
FAKE_CHUNK_DURATION = 0.1
EXPORT_DIR = "export"

class File:
    def __init__(self, visualizer, filenum, offset, length):
        self.visualizer = visualizer
        self.filenum = filenum
        self.offset = offset
        self.length = length

    def add_chunk(self, chunk):
        pass

    def add_segment(self, segment):
        pass

class Chunk:
    def __init__(self, chunk_id, begin, end, byte_size,
                 filenum, peer_id, bearing,
                 arrival_time, visualizer):
        self.id = chunk_id
        self.begin = begin
        self.end = end
        self.byte_size = byte_size
        self.filenum = filenum
        self.peer_id = peer_id
        self.bearing = bearing
        self.arrival_time = arrival_time
        self.visualizer = visualizer
        self.playing = False
        self.duration = FAKE_CHUNK_DURATION
        self.last_updated = visualizer.current_time()

    def append(self, other):
        self.end = other.end
        self.byte_size = self.end - self.begin
        self.last_updated = self.visualizer.current_time()

    def prepend(self, other):
        self.begin = other.begin
        self.byte_size = self.end - self.begin
        self.last_updated = self.visualizer.current_time()

    def joinable_with(self, other):
        return True

    def peer_position(self):
        return Visualizer.bearing_to_border_position(
            self.bearing, self.visualizer.width, self.visualizer.height)

    def age(self):
        return time.time() - self.arrival_time

    def relative_age(self):
        return self.age() / self.duration

    def is_playing(self):
        return self.relative_age() < 1

    def __str__(self):
        return "Chunk(id=%s, begin=%s, end=%s, filenum=%s)" % (
            self.id, self.begin, self.end, self.filenum)

class Segment(Chunk):
    def __init__(self, chunk_id, begin, end, byte_size,
                 filenum, f, peer_id, bearing, duration,
                 arrival_time, visualizer):
        Chunk.__init__(self, chunk_id, begin, end, byte_size,
                 filenum, peer_id, bearing,
                 arrival_time, visualizer)
        self.duration = duration
        self.f = f

    def playback_byte_cursor(self):
        return self.begin + min(self.relative_age(), 1) * self.byte_size

    def __str__(self):
        return "Segment(id=%s, begin=%s, end=%s, filenum=%s, duration=%s)" % (
            self.id, self.begin, self.end, self.filenum, self.duration)

class Peer:
    def __init__(self, visualizer):
        self.visualizer = visualizer

    def add_segment(self, segment):
        pass

class Visualizer:
    def __init__(self, args,
                 file_class=File,
                 chunk_class=Chunk,
                 segment_class=Segment,
                 peer_class=Peer):
        self.file_class = file_class
        self.chunk_class = chunk_class
        self.segment_class = segment_class
        self.peer_class = peer_class
        self.sync = args.sync
        self.width = args.width
        self.height = args.height
        self.show_fps = args.show_fps
        self.export = args.export
        self.logger = logging.getLogger("visualizer")
        self.files = {}
        self.peers = {}
        self.first_frame = True
        self.synth = SynthController()
        self.exiting = False
        self.time_increment = 0
        if self.show_fps:
            self.fps_history = collections.deque(maxlen=10)
            self.previous_shown_fps_time = None
        self.setup_osc(args.osc_log)
        self.orchestra.register()
        if self.export:
            self.export_fps = args.export_fps
            from exporter import Exporter
            import shutil
            shutil.rmtree(EXPORT_DIR)
            os.mkdir(EXPORT_DIR)
            self.exporter = Exporter(EXPORT_DIR, MARGIN, MARGIN, self.width, self.height)

    def run(self):
        window_width = self.width + MARGIN*2
        window_height = self.height + MARGIN*2
        glutInit(sys.argv)
        glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH)
        glutInitWindowSize(window_width, window_height)
        glutInitWindowPosition(0, 0)
        glutCreateWindow("")
        glutDisplayFunc(self.DrawGLScene)
        glutIdleFunc(self.DrawGLScene)
        glutReshapeFunc(self.ReSizeGLScene)
        glutKeyboardFunc(self.keyPressed)
        self.InitGL()
        self.ReSizeGLScene(window_width, window_height)
        glutMainLoop()

    def handle_torrent_message(self, path, args, types, src, data):
        self.num_files = args[0]

    def handle_file_message(self, path, args, types, src, data):
        (filenum, offset, length) = args
        f = self.files[filenum] = self.file_class(self, filenum, offset, length)
        self.added_file(f)

    def handle_chunk_message(self, path, args, types, src, data):
        (chunk_id, torrent_position, byte_size, filenum,
         peer_id, bearing) = args
        if filenum in self.files:
            begin = torrent_position - self.files[filenum].offset
            end = begin + byte_size
            chunk = self.chunk_class(
                chunk_id, begin, end, byte_size, filenum,
                peer_id, bearing, self.current_time(), self)
            self.files[filenum].add_chunk(chunk)
        else:
            print "ignoring chunk from undeclared file %s" % filenum

    def handle_segment_message(self, path, args, types, src, data):
        (segment_id, torrent_position, byte_size, filenum,
         peer_id, bearing, duration) = args
        if filenum in self.files:
            f = self.files[filenum]
            begin = torrent_position - f.offset
            end = begin + byte_size
            segment = self.segment_class(
                segment_id, begin, end, byte_size, filenum, f,
                peer_id, bearing, duration, self.current_time(), self)
            self.add_segment(segment)
        else:
            print "ignoring segment from undeclared file %s" % filenum

    def add_segment(self, segment):
        f = self.files[segment.filenum]
        segment.f = f
        f.add_segment(segment)

        if not segment.peer_id in self.peers:
            self.peers[segment.peer_id] = self.peer_class(self)
        peer = self.peers[segment.peer_id]
        peer.add_segment(segment)
        segment.peer = peer

    def added_file(self, f):
        pass

    def handle_shutdown(self, path, args, types, src, data):
        self.exiting = True

    def setup_osc(self, log_filename):
        self.orchestra = OrchestraController()
        self.server = OscReceiver(VISUALIZER_PORT, log_filename)
        self.server.add_method("/torrent", "i", self.handle_torrent_message)
        self.server.add_method("/file", "iii", self.handle_file_message)
        self.server.add_method("/chunk", "iiiiif", self.handle_chunk_message)
        self.server.add_method("/segment", "iiiiiff", self.handle_segment_message)
        self.server.add_method("/shutdown", "", self.handle_shutdown)

    def InitGL(self):
        glClearColor(1.0, 1.0, 1.0, 0.0)
        glClearDepth(1.0)
        glShadeModel(GL_SMOOTH)

    def ReSizeGLScene(self, _width, _height):
        if _height == 0:
            _height = 1
        glViewport(0, 0, _width, _height)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        glOrtho(0.0, _width, _height, 0.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW)

    def DrawGLScene(self):
        if self.exiting:
            sys.exit()

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glLoadIdentity()

        if self.export:
            self.current_export_time = float(self.exporter.frame_count) / self.export_fps

        self.now = self.current_time()
        if self.first_frame:
            if self.sync:
                self.synth.sync_beep()
            self.first_frame = False
        else:
            self.time_increment = self.now - self.previous_frame_time
            glTranslatef(MARGIN, MARGIN, 0)
            self.draw_border()
            self.handle_incoming_messages()
            self.render()
            if self.show_fps:
                self.update_fps_history()
                self.show_fps_if_timely()

        glutSwapBuffers()
        self.previous_frame_time = self.now
        if self.export:
            self.exporter.export_frame()

    def handle_incoming_messages(self):
        if self.export:
            self.server.serve_from_log_until(self.now)
        else:
            self.server.serve()

    def update_fps_history(self):
        fps = 1.0 / self.time_increment
        self.fps_history.append(fps)

    def show_fps_if_timely(self):
        if self.previous_shown_fps_time:
            if (self.now - self.previous_shown_fps_time) > 1.0:
                self.calculate_and_show_fps()
        else:
            self.calculate_and_show_fps()

    def calculate_and_show_fps(self):
        print sum(self.fps_history) / len(self.fps_history)
        self.previous_shown_fps_time = self.now

    def draw_border(self):
        x1 = y1 = -1
        x2 = self.width
        y2 = self.height
        glLineWidth(1)
        glColor3f(BORDER_OPACITY, BORDER_OPACITY, BORDER_OPACITY)
        glBegin(GL_LINE_LOOP)
        glVertex2i(x1, y2)
        glVertex2i(x2, y2)
        glVertex2i(x2, y1)
        glVertex2i(x1, y1)
        glEnd()

    def keyPressed(self, *args):
        if args[0] == ESCAPE:
            self.exiting = True

    def playing_segment(self, segment, pan):
        self.orchestra.visualizing_segment(segment.id, pan)
        segment.playing = True

    def current_time(self):
        if self.export:
            return self.current_export_time
        else:
            return time.time()

    def set_color(self, color_vector):
        glColor3f(color_vector[0],
                  color_vector[1],
                  color_vector[2])

    @staticmethod
    def bearing_to_border_position(bearing, width, height):
        total_border_size = width*2 + height*2
        peer_border_position = bearing * total_border_size
        if peer_border_position < width:
            return Vector2d(peer_border_position, 0)
        peer_border_position -= width
        if peer_border_position < height:
            return Vector2d(width, peer_border_position)
        peer_border_position -= height
        if peer_border_position < width:
            return Vector2d(width - peer_border_position, height)
        peer_border_position -= width
        return Vector2d(0, height - peer_border_position)


def run(visualizer_class):
    print "Hit ESC key to quit."

    parser = argparse.ArgumentParser()
    parser.add_argument('-sync', action='store_true')
    parser.add_argument('-width', dest='width', type=int, default=640)
    parser.add_argument('-height', dest='height', type=int, default=480)
    parser.add_argument('-show-fps', dest='show_fps', action='store_true')
    parser.add_argument('-osc-log', dest='osc_log')
    parser.add_argument('-export', dest='export', action='store_true')
    parser.add_argument('-export-fps', dest='export_fps', default=30.0, type=float)
    args = parser.parse_args()

    visualizer_class(args).run()
