#https://evanw.github.io/font-texture-generator/

import os
import sys
import json

def read_metadata(abs_file_path):
	f = open(abs_file_path)
	data = json.load(f)
	f.close()
	return data

def read_image(abs_image_path):
	from PIL import Image
	img = Image.open(abs_image_path)
	return img

class FontConverter():
	def __init__(self, folder, font_name):
		self.metadata = read_metadata(os.path.join(folder, font_name + ".json"))
		self.image = read_image(os.path.join(folder, font_name + ".png"))
		self.name = font_name

	def save(self, outdir):
		characters = self.metadata['characters']

		fout = open(os.path.join(outdir, self.name + ".yaml"), "w")
		fout.write("characters:\n")
		for k, v in characters.items():
			ind = ord(k)
			x = v['x']
			y = v['y']
			w = v['width']
			h = v['height']
			ox = v['originX']
			oy = v['originY']
			a = v['advance']

			box = (x, y, x + w, y + h)
			output_img = self.image.crop(box)
			outfile = self.name + "-ch" + str(ind) +".png"
			output_img.save(os.path.join(outdir, outfile))

			fout.write("  - index: " + str(ind) + "\n")
			fout.write(f"    texture: \"{outfile}\"\n")
			fout.write(f"    origin: {{ x : {ox}, y : {oy} }}\n")
			fout.write(f"    size: {{ x : {w}, y : {h} }}\n")
			fout.write(f"    advance: {a}\n")

		fout.close()

idir = sys.argv[1]
ifile = sys.argv[2]
odir = sys.argv[3]

f = FontConverter(idir, ifile)
if not os.path.exists(odir):
	os.makedirs(odir)
f.save(odir)

"""

def save_data(data, image, file):
	f = open(os.path.join('../data/fonts', file), "w")

	characters = data['characters']

	chlist = []
	for k, v in characters.items():
		l = ""
		l = l + str(ord(k)) + ","
		l = l + str(v['x']) + ","
		l = l + str(v['y']) + ","
		l = l + str(v['width']) + ","
		l = l + str(v['height']) + ","
		l = l + str(v['originX']) + ","
		l = l + str(v['originY']) + ","
		l = l + str(v['advance']) + "\n"
		chlist.append(l)

	f.write(str(len(chlist)) + "\n")
	f.write(str(data['size']) + "\n")
	f.write(str(data['width']) + "\n")
	f.write(str(data['height']) + "\n")
	for l in chlist:
		f.write(l)

	y = 0
	w = data['width']
	h = data['height']

	while y < h:
		x = 0
		while x < w:
			vf = format(image.getpixel((x,y))[0], 'x')
			if len(vf) == 1:
				vf = "0" + vf
			elif len(vf) == 0:
				vf = "00"
			f.write(vf)
			x = x + 1

		y = y + 1

	f.close()

def process_font(font_name):
	metadata = read_metadata(font_name + ".json");
	image = process_image(font_name + ".png");

	save_data(metadata, image, font_name + ".bf")

process_font("arial_18")
process_font("arial_16")
"""

