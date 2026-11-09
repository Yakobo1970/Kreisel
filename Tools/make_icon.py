"""Draws the Kreisel logo (three rings, two orbiting dots) as the app icon.
Usage: python make_icon.py <out.png>   (1024 x 1024, used by CMake ICON_BIG / ICON_SMALL)"""
import math
import sys
from PIL import Image, ImageDraw

S = 1024
SS = 4  # supersampling
img = Image.new("RGBA", (S * SS, S * SS), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
k = SS

# rounded square background (theme "Nacht")
d.rounded_rectangle([0, 0, S * k - 1, S * k - 1], radius=220 * k, fill=(24, 27, 36, 255))
d.rounded_rectangle([18 * k, 18 * k, (S - 18) * k, (S - 18) * k], radius=205 * k, outline=(35, 40, 56, 255), width=10 * k)

accent = (124, 156, 255, 255)
track = [(255, 95, 109, 255), (255, 169, 77, 255), (255, 212, 59, 255), (105, 219, 124, 255)]
c = S * k / 2
# rings as in the editor logo: radii 6/11/16 of a 30 px logo -> scaled up
for i, r in enumerate([120, 220, 320]):
    rr = r * k
    d.ellipse([c - rr, c - rr, c + rr, c + rr], outline=accent, width=34 * k)

# euclidean beats on the outer ring (8 steps, 5 beats) and the two orbiting dots
for step in range(16):
    a = -math.pi / 2 + step * 2 * math.pi / 16
    hit = (step * 5) % 16 < 5
    rr = 320 * k
    x, y = c + rr * math.cos(a), c + rr * math.sin(a)
    if hit:
        s = 46 * k
        d.ellipse([x - s, y - s, x + s, y + s], fill=track[step % 4])

def dot(radius, angle, size, col):
    x, y = c + radius * k * math.cos(angle), c + radius * k * math.sin(angle)
    s = size * k
    d.ellipse([x - s, y - s, x + s, y + s], fill=col)

dot(220, -0.6, 44, (230, 232, 239, 255))
dot(120, 2.2, 36, (230, 232, 239, 255))
d.ellipse([c - 40 * k, c - 40 * k, c + 40 * k, c + 40 * k], fill=accent)

img = img.resize((S, S), Image.LANCZOS)
out = sys.argv[1] if len(sys.argv) > 1 else "icon.png"
img.save(out)
print("wrote", out)
