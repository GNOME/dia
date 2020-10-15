
MOVE_TO = 0
LINE_TO = 1
CURVE_TO = 2
# these are not actually saved
HLINE_TO = 3
VLINE_TO = 4
SMOOTHCURVE_TO = 5
CLOSE = 6

class Component:
	def __init__(self, type, point1=None, point2=None, point3=None):
		self.type = type
		self.point1 = point1
		self.point2 = point2
		self.point3 = point3
	def dump(self):
		if self.type == MOVE_TO:
			print("Move To", self.point1)
		elif self.type == LINE_TO:
			print("Line To", self.point1)
		elif self.type == CURVE_TO:
			print("Curve To", self.point1, self.point2, self.point3)
		elif self.type == CLOSE:
			print("Close")

def chomp(bpath):
	while bpath and bpath[0] in ' \t\n\r,':
		bpath = bpath[1:]
	return bpath

def read_num(str):
	num = ''
	while str and str[0] in '0123456789.+-':
		num = num + str[0]
		str = str[1:]
	return float(num), chomp(str)

def parse_bpath(bpath):
	p = []

	last_open = (0, 0)
	last_point = (0, 0)
	last_control = (0,0)
	last_type = MOVE_TO
	last_relative = 0

	bpath = chomp(bpath)
	while bpath:
		# grok new commands
		print(bpath)
		if bpath[0] == 'M':
			bpath = chomp(bpath[1:])
			last_type = MOVE_TO
			last_relative = 0
		elif bpath[0] == 'm':
			bpath = chomp(bpath[1:])
			last_type = MOVE_TO
			last_relative = 1
		elif bpath[0] == 'L':
			bpath = chomp(bpath[1:])
			last_type = LINE_TO
			last_relative = 0
		elif bpath[0] == 'l':
			bpath = chomp(bpath[1:])
			last_type = LINE_TO
			last_relative = 1
		elif bpath[0] == 'H':
			bpath = chomp(bpath[1:])
			last_type = HLINE_TO
			last_relative = 0
		elif bpath[0] == 'h':
			bpath = chomp(bpath[1:])
			last_type = HLINE_TO
			last_relative = 1
		elif bpath[0] == 'V':
			bpath = chomp(bpath[1:])
			last_type = VLINE_TO
			last_relative = 0
		elif bpath[0] == 'v':
			bpath = chomp(bpath[1:])
			last_type = VLINE_TO
			last_relative = 1
		elif bpath[0] == 'C':
			bpath = chomp(bpath[1:])
			last_type = CURVE_TO
			last_relative = 0
			last_smooth = 0
		elif bpath[0] == 'c':
			bpath = chomp(bpath[1:])
			last_type = CURVE_TO
			last_relative = 1
			last_smooth = 0
		elif bpath[0] == 'S':
			bpath = chomp(bpath[1:])
			last_type = SMOOTHCURVE_TO
			last_relative = 0
		elif bpath[0] == 's':
			bpath = chomp(bpath[1:])
			last_type = SMOOTHCURVE_TO
			last_relative = 1
		elif bpath[0] in 'Zz':
			bpath = chomp(bpath[1:])
			last_type = CLOSE
			last_relative = 0
		elif bpath[0] not in '0123456789.+-':
			raise TypeError("unexpected input")

		if last_type == MOVE_TO:
			x, bpath = read_num(bpath)
			y, bpath = read_num(bpath)
			if last_relative:
				x = x + last_point[0]
				y = y + last_point[1]
			last_point = (x, y)
			last_control = last_point
			last_open = last_point
			p.append(Component(MOVE_TO, last_point))
		elif last_type == LINE_TO:
			x, bpath = read_num(bpath)
			y, bpath = read_num(bpath)
			if last_relative:
				x = x + last_point[0]
				y = y + last_point[1]
			last_point = (x, y)
			last_control = last_point
			p.append(Component(LINE_TO, last_point))
		elif last_type == HLINE_TO:
			x, bpath = read_num(bpath)
			if last_relative:
				x = x + last_point[0]
			last_point = (x, last_point[1])
			last_control = last_point
			p.append(Component(LINE_TO, last_point))
		elif last_type == VLINE_TO:
			y, bpath = read_num(bpath)
			if last_relative:
				y = y + last_point[1]
			last_point = (last_point[0], y)
			last_control = last_point
			p.append(Component(LINE_TO, last_point))
		elif last_type == CURVE_TO:
			x1, bpath = read_num(bpath)
			y1, bpath = read_num(bpath)
			x2, bpath = read_num(bpath)
			y2, bpath = read_num(bpath)
			x3, bpath = read_num(bpath)
			y3, bpath = read_num(bpath)
			if last_relative:
				x1 = x1 + last_point[0]
				y1 = y1 + last_point[1]
				x2 = x2 + last_point[0]
				y2 = y2 + last_point[1]
				x3 = x3 + last_point[0]
				y3 = y3 + last_point[1]
			last_point = (x3, y3)
			last_control = (x2, y2)
			p.append(Component(CURVE_TO, (x1,y1),(x2,y2),(x3,y3)))
		elif last_type == SMOOTHCURVE_TO:
			x2, bpath = read_num(bpath)
			y2, bpath = read_num(bpath)
			x3, bpath = read_num(bpath)
			y3, bpath = read_num(bpath)
			if last_relative:
				x2 = x2 + last_point[0]
				y2 = y2 + last_point[1]
				x3 = x3 + last_point[0]
				y3 = y3 + last_point[1]
			x1 = 2 * last_point[0] - last_control[0]
			y1 = 2 * last_point[1] - last_control[1]
			last_point = (x3, y3)
			last_control = (x2, y2)
			p.append(Component(CURVE_TO, (x1,y1),(x2,y2),(x3,y3)))
		elif last_type == CLOSE:
			if last_point != last_open:
				p.append(Component(LINE_TO, last_open))
			last_point = last_open
			last_control = last_point
			p.append(Component(CLOSE))
		bpath = chomp(bpath)
	return p

def print_bpath(bpath):
	p = parse_bpath(bpath)
	list(map(Component.dump, p))

if __name__ == '__main__':
	import sys
	if len(sys.argv) > 1:
		print_bpath(sys.argv[1])
