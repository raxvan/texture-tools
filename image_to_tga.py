from PIL import Image
import sys

input_image = sys.argv[1]
output_image = sys.argv[2]

if (output_image.endswith(".tga") == False):
	raise Exception("Invalid output image.")


im = Image.open(input_image)
im.save(output_image)
