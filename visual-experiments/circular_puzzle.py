import circular_visualizer as visualizer
from gatherer import Gatherer
from OpenGL.GL import *
from collections import OrderedDict
from vector import Vector2d, Vector3d, Angle
import random
import math
from bezier import make_bezier
import colorsys
import copy

RADIUS = 80
CIRCLE_THICKNESS = 20
CIRCLE_PRECISION = 50
ARRIVED_OPACITY = 0.5
GREYSCALE = True
CONTROL_POINTS_BEFORE_BRANCH = 15
CURVE_PRECISION = 50
CURVE_OPACITY = 0.8
SEGMENT_DECAY_TIME = 1.0
FORCE_DIRECTED_PLACEMENT = False
DAMPING = 0.95

class Segment(visualizer.Segment):
    def target_position(self):
        return self.f.completion_position(
            self.playback_byte_cursor(),
            (self.f.inner_radius + self.f.radius) / 2)

    def decay_time(self):
        return self.age() - self.duration

    def outdated(self):
        return (self.age() - self.duration) > SEGMENT_DECAY_TIME

    def draw_curve(self):
        glBegin(GL_LINE_STRIP)
        for x,y in self.curve():
            glVertex2f(x, y)
        glEnd()
        if self.is_playing():
            self.draw_cursor_line()

    def curve(self):
        control_points = []
        branching_position = self.peer.smoothed_branching_position.value()
        for i in range(CONTROL_POINTS_BEFORE_BRANCH):
            r = float(i) / (CONTROL_POINTS_BEFORE_BRANCH-1)
            control_points.append(self.peer.departure_position * (1-r) +
                                 branching_position * r)
        if self.is_playing():
            target = self.target_position()
        else:
            target = branching_position + (self.target_position() - branching_position) * \
                (1 - pow(self.decay_time(), 0.3))
        control_points.append(target)
        bezier = make_bezier([(p.x, p.y) for p in control_points])
        return bezier(CURVE_PRECISION)

    def draw_cursor_line(self):
        self.draw_line(self.f.completion_position(self.playback_byte_cursor(),
                                                  self.f.inner_radius),
                       self.f.completion_position(self.playback_byte_cursor(),
                                                  self.f.radius))

    def draw_line(self, p, q):
        glBegin(GL_LINES)
        glVertex2f(p.x, p.y)
        glVertex2f(q.x, q.y)
        glEnd()

    def draw_playing(self):
        trace_age = min(self.duration, 0.2)
        previous_byte_cursor = self.begin + min(self.age()-trace_age, 0) / \
            self.duration * self.byte_size
        if self.relative_age() < 1:
            opacity = 1
        else:
            opacity = 1 - pow((self.age() - self.duration) / SEGMENT_DECAY_TIME, .2)
        self.draw_gradient(previous_byte_cursor, self.playback_byte_cursor(), opacity)

    def draw_gradient(self, source, target, opacity):
        source_angle = Angle(self.f.byte_cursor_to_angle(source))
        target_angle = Angle(self.f.byte_cursor_to_angle(target))
        angular_distance = target_angle - source_angle

        source_color = Vector3d(1, 1, 1)
        target_color = Vector3d(1, 0, 0)
        target_color += (source_color - target_color) * (1-opacity)
        num_vertices = CIRCLE_PRECISION
        glBegin(GL_POLYGON)

        for i in range(num_vertices):
            angle = source_angle + angular_distance * (float(i) / (num_vertices-1))
            p = self.f.completion_position_by_angle(angle.get(), self.f.inner_radius)
            self.visualizer.set_color(source_color + \
                                          (target_color-source_color) * \
                                          (float(i) / (num_vertices-1)))
            glVertex2f(p.x, p.y)
        for i in range(num_vertices):
            angle = source_angle + angular_distance * (float(num_vertices-i-1) / (num_vertices-1))
            p = self.f.completion_position_by_angle(angle.get(), self.f.radius)
            self.visualizer.set_color(source_color + \
                                          (target_color-source_color) * \
                                          (float(num_vertices-i-1) / (num_vertices-1)))
            glVertex2f(p.x, p.y)

        glEnd()
            
class Peer(visualizer.Peer):
    def __init__(self, *args):
        visualizer.Peer.__init__(self, *args)
        self.departure_position = None
        self.smoothed_branching_position = Smoother(5.0)
        self.segments = {}
        self.hue = random.uniform(0, 1)

    def add_segment(self, segment):
        if self.departure_position == None:
            self.departure_position = segment.departure_position
        segment.peer = self
        self.segments[segment.id] = segment

    def update(self):
        outdated = filter(lambda segment_id: self.segments[segment_id].outdated(),
                          self.segments)
        for segment_id in outdated:
            segment = self.segments[segment_id]
            segment.f.gatherer.add(segment)
            del self.segments[segment_id]
        self.update_branching_position()

    def update_branching_position(self):
        if len(self.segments) == 0:
            self.smoothed_branching_position.reset()
        else:
            average_target_position = \
                sum([segment.target_position() for segment in self.segments.values()]) / \
                len(self.segments)
            new_branching_position = self.departure_position*0.4 + average_target_position*0.6
            self.smoothed_branching_position.smooth(
                new_branching_position, self.visualizer.time_increment)

    def draw(self):
        if len(self.segments) > 0:
            glLineWidth(1.0)
            glEnable(GL_LINE_SMOOTH)
            glHint(GL_LINE_SMOOTH_HINT, GL_NICEST)
            glEnable(GL_BLEND)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
            for segment in self.segments.values():
                segment.draw_playing()
            for segment in self.segments.values():
                self.set_color(0)
                segment.draw_curve()
            glDisable(GL_LINE_SMOOTH)
            glDisable(GL_BLEND)

    def set_color(self, relative_age):
        if GREYSCALE:
            glColor3f(1 - CURVE_OPACITY,
                      1 - CURVE_OPACITY,
                      1 - CURVE_OPACITY)
        else:
            color = colorsys.hsv_to_rgb(self.hue, 0.35, 1)
            glColor3f(relative_age + color[0] * (1-relative_age),
                      relative_age + color[1] * (1-relative_age),
                      relative_age + color[2] * (1-relative_age))

class Smoother:
    def __init__(self, response_factor):
        self._response_factor = response_factor
        self._current_value = None

    def smooth(self, new_value, time_increment):
        if self._current_value:
            self._current_value += (new_value - self._current_value) * \
                self._response_factor * time_increment
        else:
            self._current_value = new_value
        return self._current_value

    def value(self):
        return self._current_value

    def reset(self):
        self._current_value = None

class File(visualizer.File):
    def __init__(self, *args):
        visualizer.File.__init__(self, *args)
        self.position = self.visualizer.position_for_new_file()
        self.gatherer = Gatherer()
        self.inner_radius = self.visualizer.scale(RADIUS - CIRCLE_THICKNESS)
        self.radius = self.visualizer.scale(RADIUS)
        self.velocity = Vector2d(0,0)

    def add_segment(self, segment):
        #I suppose this is non-SSR panning?
        #pan = self.completion_position(segment.begin, self.radius).x / self.visualizer.width
        self.visualizer.playing_segment(segment)
        segment.departure_position = segment.peer_position()

    def draw(self):
        glLineWidth(1)
        opacity = 0.5
        glColor3f(1-opacity, 1-opacity, 1-opacity)

        if self.completed():
            self.draw_completed_file()
        else:
            for segment in self.gatherer.pieces():
                self.draw_gathered_piece(segment)

    def completed(self):
        if len(self.gatherer.pieces()) == 1:
            piece = self.gatherer.pieces()[0]
            return piece.byte_size == self.length

    def draw_gathered_piece(self, segment):
        num_vertices = int(CIRCLE_PRECISION * float(segment.end - segment.begin) / segment.byte_size)
        num_vertices = max(num_vertices, 2)
        glBegin(GL_LINE_LOOP)

        for i in range(num_vertices):
            byte_position = segment.begin + segment.byte_size * float(i) / (num_vertices-1)
            p = self.completion_position(byte_position, self.inner_radius)
            glVertex2f(p.x, p.y)
        for i in range(num_vertices):
            byte_position = segment.begin + segment.byte_size * float(num_vertices-i-1) / (num_vertices-1)
            p = self.completion_position(byte_position, self.radius)
            glVertex2f(p.x, p.y)

        glEnd()

    def draw_completed_file(self):
        num_vertices = int(CIRCLE_PRECISION)

        glBegin(GL_LINE_LOOP)
        for i in range(num_vertices):
            byte_position = self.length * float(i) / (num_vertices-1)
            p = self.completion_position(byte_position, self.inner_radius)
            glVertex2f(p.x, p.y)
        glEnd()

        glBegin(GL_LINE_LOOP)
        for i in range(num_vertices):
            byte_position = self.length * float(i) / (num_vertices-1)
            p = self.completion_position(byte_position, self.radius)
            glVertex2f(p.x, p.y)
        glEnd()

    def completion_position(self, byte_cursor, radius):
        angle = self.byte_cursor_to_angle(byte_cursor)
        return self.completion_position_by_angle(angle, radius)

    def completion_position_by_angle(self, angle, radius):
        x = self.visualizer.x_offset + self.position.x + radius * math.cos(angle)
        y = self.visualizer.y_offset + self.position.y + radius * math.sin(angle)
        return Vector2d(x, y)

    def byte_cursor_to_angle(self, byte_cursor):
        return 2 * math.pi * float(byte_cursor) / self.length

class Puzzle(visualizer.Visualizer):
    def __init__(self, args):
        visualizer.Visualizer.__init__(self, args,
                                       file_class=File,
                                       peer_class=Peer,
                                       segment_class=Segment)
        self.x_offset = 0
        self.y_offset = 0
        self.segments = {}
        if FORCE_DIRECTED_PLACEMENT:
            self.force_directed_placer = ForceDirectedPlacer(self)
            self.centralizer = Centralizer(self)
        self.orchestra.enable_smooth_movement()

    def render(self):
        glEnable(GL_LINE_SMOOTH)
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        self.draw_gathered_segments()
        self.draw_branches()
        glDisable(GL_LINE_SMOOTH)
        glDisable(GL_BLEND)

    def draw_gathered_segments(self):
        if FORCE_DIRECTED_PLACEMENT:
            self.force_directed_placer.reposition_files()
            self.centralizer.center()
        for f in self.files.values():
            f.draw()

    def draw_branches(self):
        for peer in self.peers.values():
            peer.update()
            peer.draw()

    def position_for_new_file(self):
        if len(self.files) == 0 or FORCE_DIRECTED_PLACEMENT:
            return self.random_position()
        else:
            return FreePositionLocator(self).free_position()

    def random_position(self):
        return Vector2d(random.uniform(self.scale(RADIUS), self.width - self.scale(RADIUS)),
                        random.uniform(self.scale(RADIUS), self.height - self.scale(RADIUS)))

    def scale(self, unscaled):
        return float(unscaled) / 640 * self.width

    def pan_segment(self, segment):
        self.start_segment_movement_from_peer(segment.id, segment.duration)

class ForceDirectedPlacer:
    def __init__(self, visualizer):
        self.visualizer = visualizer

    def reposition_files(self):
        for f in self.visualizer.files.values():
            f.velocity = (f.velocity + self.repositioning_force(f)) * DAMPING
            f.velocity.limit(3.0)
        for f in self.visualizer.files.values():
            f.position += f.velocity

    def repositioning_force(self, f):
        f.force = Vector2d(0,0)
        self.repel_from_and_attract_to_other_files(f)
        self.attract_to_peers(f)
        return f.force

    def repel_from_and_attract_to_other_files(self, f):
        for other in self.visualizer.files.values():
            if other != f:
                self.apply_coulomb_repulsion(f, other.position)
                self.apply_hooke_attraction(f, other.position)

    def attract_to_peers(self, f):
        for peer in self.visualizer.peers.values():
            if f.filenum in [segment.filenum for segment in peer.segments.values()]:
                self.attract_to_peer(f, peer)

    def attract_to_peer(self, f, peer):
        self.apply_hooke_attraction(f, peer.departure_position)

    def apply_coulomb_repulsion(self, f, other):
        d = f.position - other
        distance = d.mag()
        if distance == 0:
            f.force += Vector2d(random.uniform(0.0, 0.1),
                                random.uniform(0.0, 0.1))
        else:
            d.normalize()
            f.force += d / pow(distance, 2) * 1000

    def apply_hooke_attraction(self, f, other):
        d = other - f.position
        f.force += d * 0.0001

class Centralizer:
    def __init__(self, visualizer):
        self.visualizer = visualizer
        self.x_offset_smoother = Smoother(.5)
        self.y_offset_smoother = Smoother(.5)

    def center(self):
        if len(self.visualizer.files) > 0:
            new_x_offset = self.visualizer.width / 2 - \
                (min([f.position.x for f in self.visualizer.files.values()]) +
                 max([f.position.x for f in self.visualizer.files.values()])) / 2
            new_y_offset = self.visualizer.height / 2 - \
                (min([f.position.y for f in self.visualizer.files.values()]) +
                 max([f.position.y for f in self.visualizer.files.values()])) / 2
            self.x_offset = self.x_offset_smoother.smooth(
                new_x_offset, self.visualizer.time_increment)
            self.y_offset = self.y_offset_smoother.smooth(
                new_y_offset, self.visualizer.time_increment)

class FreePositionLocator:
    def __init__(self, visualizer):
        self.visualizer = visualizer

    def free_position(self):
        return self.pick_freest_random_position()

    def pick_freest_random_position(self):
        positions = [self.visualizer.random_position() for i in range(10)]
        return max(positions, key=self.distance_to_nearest_file)

    def distance_to_nearest_file(self, position):
        return min([(position - other.position).mag()
                    for other in self.visualizer.files.values()])

if __name__ == '__main__':
    visualizer.run(Puzzle)
