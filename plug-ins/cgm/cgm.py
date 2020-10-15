
def read_cgm(file):
	if type(file) == type(''):
		file = open(file, 'rb');
	list = []
	head = file.read(2);
	while head != '':
		if len(head) != 2: break
		el_class = ord(head[0]) >> 4
		el_id = ((ord(head[0]) & 0x0f) << 3) | (ord(head[1]) >> 5)
		el_len = ord(head[1]) & 0x1f
		if el_len == 31:  # long form element
			head = file.read(2)
			if len(head) != 2: break
			el_len = (ord(head[0]) << 8) | ord(head[1])
		print((el_class, el_id, el_len))

		el_body = file.read(el_len)
		if el_len & 1 == 1:  # odd element length
			file.read(1) # discard NULL byte

		list.append((el_class, el_id, el_body))

		if (el_class, el_id) == (0, 2):
			break
		
		head = file.read(2)
	return list
