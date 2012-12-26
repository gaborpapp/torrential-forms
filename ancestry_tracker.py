import copy

class Piece:
    def __init__(self, id, t, begin, end, parents={}, growth=[]):
        self.id = id
        self.t = t
        self.begin = begin
        self.end = end
        self.parents = parents
        self.growth = growth

    def inherit_from(self, pieces):
        pass

    def __repr__(self):
        return "Piece(id=%s, t=%s, begin=%s, end=%s, parent_ids=%s)" % (
            self.id, self.t, self.begin, self.end, self.parents.keys())

class AncestryTracker:
    def __init__(self, piece_class=None):
        if piece_class is None:
            piece_class = Piece
        self._piece_class = piece_class
        self._pieces = dict()
        self._counter = 1

    def add(self, new_piece):
        overlapping_pieces = self._overlapping_pieces(new_piece)
        if len(overlapping_pieces) > 0:
            if len(overlapping_pieces) > 1:
                replacement_id = self._new_piece_id()
                parents = {}
                for parent_id in overlapping_pieces:
                    parents[parent_id] = self._pieces[parent_id]
                growth = []
            else:
                parent = self._pieces[overlapping_pieces[0]]
                replacement_id = new_piece.id
                parents = copy.copy(parent.parents)
                growth = copy.copy(parent.growth)
                growth.append(parent)

            new_extension = [new_piece]
            new_extension.extend([self._pieces[key] for key in overlapping_pieces])
            for key in overlapping_pieces:
                del self._pieces[key]
            replacement_piece = self._piece_class(
                id = replacement_id,
                t = max([piece.t for piece in new_extension]),
                begin = min([piece.begin for piece in new_extension]),
                end = max([piece.end for piece in new_extension]),
                parents = parents,
                growth = growth)
            replacement_piece.inherit_from(new_extension)
            self._add_piece(replacement_piece)
        else:
            self._add_piece(new_piece)

    def _add_piece(self, piece):
        if piece.id in self._pieces:
            raise Exception("piece with ID %s already added" % piece.id)
        self._pieces[piece.id] = piece

    def _new_piece_id(self):
        new_id = "n%d" % self._counter
        self._counter += 1
        return new_id

    def last_pieces(self):
        return self._pieces.values()

    def _overlapping_pieces(self, piece):
        return filter(lambda key: self._pieces_overlap(piece, self._pieces[key]),
                      self._pieces.keys())

    def _pieces_overlap(self, piece1, piece2):
        return ((piece2.begin <= piece1.begin <= piece2.end) or
                (piece2.begin <= piece1.end <= piece2.end) or
                (piece1.begin <= piece2.begin <= piece1.end) or
                (piece1.begin <= piece2.end <= piece1.end))

    def pieces(self):
        return self._pieces
