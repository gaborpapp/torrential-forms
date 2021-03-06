#!/usr/bin/python

import world
import sys
from argparse import ArgumentParser
import numpy
import re
import math

import ip_locator
import random
from gps import GPS
import locations
import os

sys.path.append("visual-experiments")
import visualizer
from OpenGL.GL import *
from vector import Vector3d, Vector

LAND_COLOR = (.5, .5, .5)
BARS_TOP = 5
BARS_BOTTOM = 0

CAMERA_POSITION = Vector(3, [-17.237835534835536, -14.099999999999966, -24.48634534467994])
CAMERA_Y_ORIENTATION = -1
CAMERA_X_ORIENTATION = 38

HERE_LATITUDE = 52.500556
HERE_LONGITUDE = 13.398889

WORLD_WIDTH = 30
WORLD_HEIGHT = 20
LOCATION_PRECISION = 200

class Geography(visualizer.Visualizer):
    def __init__(self, args):
        self._ip_locator = ip_locator.IpLocator()
        self._gps = GPS(WORLD_WIDTH, WORLD_HEIGHT)
        self._here_x = self._gps.x(HERE_LONGITUDE)
        self._here_y = self._gps.y(HERE_LATITUDE)
        self._load_locations()
        visualizer.Visualizer.__init__(self, args)
        self._set_camera_position(CAMERA_POSITION)
        self._set_camera_orientation(CAMERA_Y_ORIENTATION, CAMERA_X_ORIENTATION)
        self._world = world.World(WORLD_WIDTH, WORLD_HEIGHT)
        self.enable_3d()

    def _load_locations(self):
        self._locations = []
        self._grid = numpy.zeros((LOCATION_PRECISION, LOCATION_PRECISION), int)
        self._add_peers_from_log()
        self._location_max_value = numpy.max(self._grid)

    def _add_peers_from_log(self):
        for location in locations.get_locations():
            self._add_location(location)

    def _add_location(self, location):
        if location and location not in self._locations:
            self._locations.append(location)
            x, y = location
            nx = int(LOCATION_PRECISION * x)
            ny = int(LOCATION_PRECISION * y)
            self._grid[ny, nx] += 1
            return True

    def InitGL(self):
        visualizer.Visualizer.InitGL(self)
        glClearColor(0.0, 0.0, 0.0, 0.0)

    def render(self):
        glEnable(GL_LINE_SMOOTH)
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

        self._render_world()
        #self._render_bar_grid_lines()
        #self._render_bar_grid_points()
        self._render_parabolae()
        self._render_land_points()
        #self._render_locations()

    def _render_world(self):
        glColor3f(*LAND_COLOR)

        for path in self._world.paths:
            self._render_land(path)

    def _render_land(self, path):
        glBegin(GL_LINE_STRIP)
        for x, y in path:
            glVertex3f(x, 0, y)
        glEnd()

    def _render_locations(self):
        glColor4f(1, 1, 1, .01)
        glBegin(GL_LINES)
        for lx, ly in self._locations:
            x = lx * WORLD_WIDTH
            y = ly * WORLD_HEIGHT
            glVertex3f(x, BARS_BOTTOM, y)
            glVertex3f(x, BARS_TOP, y)
        glEnd()

    def _render_bar_grid_lines(self):
        glBegin(GL_LINES)
        ny = 0
        for row in self._grid:
            y = (ny+0.5) / LOCATION_PRECISION * WORLD_HEIGHT
            nx = 0
            for value in row:
                if value > 0:
                    strength = pow(float(value) / self._location_max_value, 0.2)
                    x = (nx+0.5) / LOCATION_PRECISION * WORLD_WIDTH

                    glColor4f(1, 1, 1, 0.5 * strength)
                    glVertex3f(x, BARS_TOP, y)
                    glColor4f(1, 1, 1, 0.05)
                    glVertex3f(x, BARS_BOTTOM, y)

                    # glColor4f(1, 1, 1, 0.2)
                    # glVertex3f(x, BARS_TOP, y)
                    # glVertex3f(x, BARS_TOP - strength*BARS_TOP, y)
                nx += 1
            ny += 1
        glEnd()

    def _render_bar_grid_points(self):
        self._render_grid_points(BARS_TOP)

    def _render_grid_points(self, h):
        glEnable(GL_POINT_SMOOTH)
        glPointSize(3.0)
        glBegin(GL_POINTS)
        glColor4f(1, 1, 1, 0.5)
        ny = 0
        for row in self._grid:
            y = (ny+0.5) / LOCATION_PRECISION * WORLD_HEIGHT
            nx = 0
            for value in row:
                if value > 0:
                    x = (nx+0.5) / LOCATION_PRECISION * WORLD_WIDTH
                    glVertex3f(x, h, y)
                nx += 1
            ny += 1
        glEnd()

    def _render_parabolae(self):
        ny = 0
        for row in self._grid:
            y = (ny+0.5) / LOCATION_PRECISION * WORLD_HEIGHT
            nx = 0
            for value in row:
                if value > 0:
                    x = (nx+0.5) / LOCATION_PRECISION * WORLD_WIDTH
                    strength = pow(float(value) / self._location_max_value, 0.4) * 0.8
                    glColor4f(1, 1, 1, strength)
                    self._render_parabola(x, y, self._here_x, self._here_y)
                nx += 1
            ny += 1

    def _render_parabola(self, x1, y1, x2, y2):
        h1 = 0
        h2 = 2
        glBegin(GL_LINE_STRIP)
        precision = 100
        for n in range(precision):
            r = float(n) / (precision - 1)
            x = x1 + (x2 - x1) * r
            y = y1 + (y2 - y1) * r
            h = h1 + (h2 - h1) * (math.cos((r - 0.5) / 5) - 0.995) / 0.005
            glVertex3f(x, h, y)
        glEnd()

    def _render_land_points(self):
        self._render_grid_points(BARS_BOTTOM)

    def _render_surface_points(self):
        self._render_grid_points(BARS_TOP)

parser = ArgumentParser()
Geography.add_parser_arguments(parser)
options = parser.parse_args()
options.standalone = True

Geography(options).run()
