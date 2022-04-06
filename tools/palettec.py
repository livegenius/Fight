from glob import glob
from sys import argv
from os.path import basename

if len(argv) < 2:
    print("Usage: palettec.py folder_with_palettes [output_file]")
    quit()

palettesPath = argv[1]
outputFilepath = basename(palettesPath) + ".pal4"
if len(argv) >= 3:
    outputFilepath = argv[2]

palFiles = glob(palettesPath+"/*.act")
output = open(outputFilepath, "wb")

nPalettes = len(palFiles)
output.write(nPalettes.to_bytes(4, "little"))

for palFile in palFiles:
    with open(palFile, 'rb') as f:
        for i in range(256):
            b = f.read(3)
            output.write(b)
            if i == 0:
                output.write(b'\x00')
            else:
                output.write(b'\xFF')

output.close()

