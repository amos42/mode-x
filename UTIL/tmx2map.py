import struct
import sys
import xml.etree.ElementTree as ET
import csv
import io
import numpy as np

class TiledFile():
    map_width: int
    map_height: int
    map_data: any
    tile_width: int
    tile_height: int

    def read_map(self, filename) -> None:
        # <map version="1.10" tiledversion="1.11.2" orientation="orthogonal" renderorder="right-down" width="224" height="32" tilewidth="8" tileheight="8" infinite="0" backgroundcolor="#2040c0" nextlayerid="2" nextobjectid="1">
        #  <tileset firstgid="1" source="Sonic_md_bg1.tsx"/>
        #  <layer id="1" name="Layer 1" width="224" height="32">
        #   <data encoding="csv">
        tmx = ET.parse(filename)
        map_node = tmx.getroot()
        self.tile_width = int(map_node.attrib['tilewidth'])
        self.tile_height = int(map_node.attrib['tileheight'])
        layer_node = map_node.find('layer')
        self.map_width = int(layer_node.attrib['width'])
        self.map_height = int(layer_node.attrib['height'])
        tmx_data = layer_node.find('data').text.strip()
        map_array = csv.reader(io.StringIO(tmx_data))
        self.map_data = np.zeros((self.map_height, self.map_width), dtype=np.uint16)
        for i in range(0, self.map_height):
          row = map_array.__next__()
          for j in range(0, self.map_width):
            idx = int(row[j])
            idx &= 0xFFFF
            idx %= 16 * 19
            self.map_data[i][j] = idx - 1

    def write_map(self, filename) -> None:
        with open(mapname, 'wb') as f:
            hdr = struct.pack("iiii", self.map_width, self.map_height, self.tile_width, self.tile_height)
            f.write(hdr)
            f.write(self.map_data)

if __name__ == "__main__":
    import glob
    import os.path

    args_cnt = len(sys.argv)
    if args_cnt < 2:
        print("Usage: tmx2map.py <input_tmx_file>")
        sys.exit(1)

    file_path = sys.argv[1]
    file_pathname = os.path.basename(file_path)
    output_path = os.path.splitext(file_pathname)[0] + ".map"

    tr = TiledFile()
    tr.read_map(file_path)
    tr.write_map(output_path)

    print(f"Wrote map data to {output_path}")
