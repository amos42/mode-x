import struct
import sys
import PIL.Image as pilimg
import numpy as np
import os.path

args_cnt = len(sys.argv)
if args_cnt < 4:
    print("Usage: png2til.py <input_png_file> <tile_width> <tile_height> [tile_count]")
    sys.exit(1)

file_path = sys.argv[1]
tile_width = int(sys.argv[2])
tile_height = int(sys.argv[3])
if args_cnt >= 5:
    tile_count = int(sys.argv[4])
else:
    tile_count = 0

# here = os.path.dirname(os.path.abspath(__file__))
file_pathname = os.path.basename(file_path)
output_path = os.path.splitext(file_pathname)[0] + ".til"
pal_path = os.path.splitext(file_pathname)[0] + ".pal"

# Read image
img = pilimg.open(file_path)

tile_count0 = img.width // tile_width * img.height // tile_height
if tile_count == 0 or tile_count > tile_count0:
    tile_count = tile_count0

if img.mode != 'P':
    raise Exception("Image is not paletted")
 
# Fetch image pixel data to numpy array
pix = np.array(img)

with open(output_path, 'wb') as f:
    hdr = struct.pack("iiii", tile_width, tile_height, tile_count, 0)
    f.write(hdr)

    for i in range(0, img.height, tile_height):
        for j in range(0, img.width, tile_width):
            tile_data = []
            for k in range(tile_height):
                for l in range(tile_width):
                    idx = pix[i + k][j + l]
                    tile_data.append(idx)
            f.write( bytearray(tile_data) )

print(f"Wrote {tile_count} tiles to {output_path}")

with open(pal_path, 'wb') as f:
    palette = img.palette.tobytes()
    f.write(palette)

print(f"Wrote palette to {pal_path}")
