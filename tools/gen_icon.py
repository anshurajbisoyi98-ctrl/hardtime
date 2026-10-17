"""Original geometric app icon; uses Pillow from the bundled workspace runtime."""
from pathlib import Path
from PIL import Image, ImageDraw

out = Path(__file__).resolve().parents[1] / 'assets'
im = Image.new('RGBA', (512, 512), (0, 0, 0, 0))
d = ImageDraw.Draw(im)
d.rounded_rectangle((16, 16, 496, 496), 98, fill='#15222e', outline='#394c59', width=5)
d.rectangle((120, 92, 390, 416), fill='#293c4b')
d.rectangle((148, 116, 366, 402), fill='#101923')
d.rectangle((268, 116, 366, 402), fill='#e5cb72')
for x in range(152, 260, 28):
    d.rectangle((x, 116, x + 9, 402), fill='#779ebc')
d.polygon([(268, 116), (326, 152), (326, 418), (268, 402)], fill='#516974')
d.ellipse((303, 276, 315, 288), fill='#e5cb72')
d.ellipse((171, 366, 253, 390), fill='#101923')
d.rectangle((183, 266, 241, 350), fill='#e89a54')
d.rectangle((187, 342, 205, 379), fill='#101923')
d.rectangle((219, 342, 237, 379), fill='#101923')
d.rectangle((183, 210, 241, 268), fill='#101923')
d.rectangle((190, 224, 234, 261), fill='#c79872')
d.rectangle((216, 237, 222, 249), fill='#101923')
d.rectangle((216, 280, 232, 290), fill='#f2ead6')
d.rectangle((177, 274, 189, 330), fill='#e89a54')
d.rectangle((237, 274, 249, 330), fill='#e89a54')
d.rectangle((177, 325, 189, 341), fill='#c79872')
d.rectangle((237, 325, 249, 341), fill='#c79872')
im.save(out / 'icon.png')
im.save(out / 'PrisonLife.icns', format='ICNS')
print('Generated original app icon.')
