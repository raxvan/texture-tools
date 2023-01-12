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
			outfile = self.name + "-ch" + str(ind) +".tga"
			output_img.save(os.path.join(outdir, outfile))

			fout.write(f"  - index: {ind}\n")
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
