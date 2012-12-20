class GPS:
    def __init__(self, width, height):
        self.width = width
        self.height = height

    def x(self, longitude):
        return (longitude + 180) * (self.width / 360)

    def y(self, latitude):
        return ((latitude * -1) + 90) * (self.height / 180)
