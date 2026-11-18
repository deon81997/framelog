#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later - part of FRAMELOG, https://github.com/deon81997/framelog
"""Draws the 90s home-page graphics used by README.md (all original, made with Pillow; the style and the helper
functions come from NUKED SB-EMU's docs/90s/make90s.py, same author).

    python3 docs/90s/make90s.py          -> writes the GIFs next to this script
    python3 docs/90s/make90s.py title    -> just one of them
"""
import math, os, random, sys
from PIL import Image, ImageDraw, ImageFont

OUT = os.environ.get('OUT90S') or os.path.dirname(os.path.abspath(__file__))  # OUT90S=dir: write elsewhere (serial-bench)
FD = '/usr/share/fonts/truetype/dejavu/'
F = lambda n, s: ImageFont.truetype(FD + n, s)
random.seed(1998)
SCALE = 2  # drawn at 1x, stored at 2x with hard pixel edges


def save(frames, name, ms=120, scale=SCALE):
    frames = [f.convert('RGB').resize((f.width * scale, f.height * scale), Image.NEAREST) for f in frames]
    frames = [f.convert('P', palette=Image.ADAPTIVE, colors=64) for f in frames]
    frames[0].save(os.path.join(OUT, name), save_all=len(frames) > 1, append_images=frames[1:],
                   duration=ms, loop=0, optimize=True, disposal=2)


def crisp(d):
    d.fontmode = '1'
    return d


def rainbow(t):
    i = int(t * 6) % 6
    f = t * 6 - int(t * 6)
    q, p = int(255 * (1 - f)), int(255 * f)
    return [(255, p, 0), (q, 255, 0), (0, 255, p), (0, q, 255), (p, 0, 255), (255, 0, q)][i]


def starfield(w, h, n=60):
    return [(random.randrange(w), random.randrange(h), random.choice([1, 1, 1, 2])) for _ in range(n)]


# ---------------------------------------------------------------- WordArt title
def title(text='FRAMELOG', name='title.gif'):
    w, h = 720, 120
    stars = starfield(w, h, 90)
    font = F('DejaVuSerif-Bold.ttf', 70)
    frames = []
    for k in range(12):
        im = Image.new('RGB', (w, h), (0, 0, 32))
        d = crisp(ImageDraw.Draw(im))
        for i, (x, y, s) in enumerate(stars):
            if (i + k) % 5:
                c = 255 if (i + k) % 3 else 140
                d.rectangle([x, y, x + s - 1, y + s - 1], fill=(c, c, c))
        x = (w - d.textlength(text, font=font)) / 2
        for j, ch in enumerate(text):
            y = 16 + 8 * math.sin((j + k) * 0.6)
            d.text((x + 4, y + 4), ch, font=font, fill=(0, 60, 90))
            d.text((x, y), ch, font=font, fill=rainbow(((j + k) / 12) % 1))
            x += d.textlength(ch, font=font)
        frames.append(im)
    save(frames, name, 110)


def construction():
    w, h = 300, 60
    font = F('DejaVuSans-Bold.ttf', 15)
    frames = []
    for k in range(8):
        im = Image.new('RGB', (w, h), (255, 220, 0))
        d = crisp(ImageDraw.Draw(im))
        for s in range(-h, w + h, 20):
            for y0 in (0, h - 12):
                xs = s + k * 5
                d.polygon([(xs, y0), (xs + 10, y0), (xs + 22, y0 + 12), (xs + 12, y0 + 12)], fill=(0, 0, 0))
        d.rectangle([0, 14, w, h - 15], fill=(255, 220, 0))
        msg = 'UNDER CONSTRUCTION'
        d.text(((w - d.textlength(msg, font=font)) / 2, 20), msg, font=font, fill=(0, 0, 0) if k % 2 else (200, 0, 0))
        d.ellipse([12, 17, 30, 35], fill=(255, 120, 0), outline=(0, 0, 0))
        d.line([w - 30, 18, w - 16 + (k % 2) * 3, 40], fill=(90, 50, 0), width=3)
        d.rectangle([w - 20 + (k % 2) * 3, 38, w - 12 + (k % 2) * 3, 44], fill=(120, 120, 120))
        frames.append(im)
    save(frames, 'construction.gif', 150)


def new():
    frames = []
    font = F('DejaVuSans-Bold.ttf', 13)
    for k in range(2):
        im = Image.new('RGB', (48, 20), (255, 255, 255))
        d = crisp(ImageDraw.Draw(im))
        if k == 0:
            d.polygon([(0, 10), (6, 0), (42, 0), (48, 10), (42, 19), (6, 19)], fill=(255, 0, 0))
            d.text((7, 2), 'NEW!', font=font, fill=(255, 255, 0))
        frames.append(im)
    save(frames, 'new.gif', 450)


def star():
    frames = []
    for k in range(6):
        im = Image.new('RGB', (14, 14), (255, 255, 255))
        d = ImageDraw.Draw(im)
        r = 6 if k % 3 else 4
        c = rainbow(k / 6)
        pts = []
        for i in range(10):
            a = -math.pi / 2 + i * math.pi / 5
            rr = r if i % 2 == 0 else r / 2.3
            pts.append((7 + rr * math.cos(a), 7 + rr * math.sin(a)))
        d.polygon(pts, fill=c, outline=(0, 0, 0))
        frames.append(im)
    save(frames, 'star.gif', 140)


# ---------------------------------------------------------------- 88x31 buttons
def button(name, top, bottom, bg, fg, left=None, anim=None):
    frames = []
    nf = 1 if anim is None else 6
    small, big = F('DejaVuSansCondensed-Bold.ttf', 9), F('DejaVuSansCondensed-Bold.ttf', 11)
    for k in range(nf):
        im = Image.new('RGB', (88, 31), bg)
        d = crisp(ImageDraw.Draw(im))
        d.rectangle([0, 0, 87, 30], outline=(255, 255, 255))
        d.line([(87, 0), (87, 30), (0, 30)], fill=(64, 64, 64))
        x = 3
        if left:
            d.rectangle([3, 3, 26, 27], fill=left[0])
            d.text((15 - d.textlength(left[1], font=big) / 2, 8), left[1], font=big, fill=left[2])
            x = 30
        cw = 88 - x - 3
        col = fg if anim is None else rainbow(k / nf)
        d.text((x + (cw - d.textlength(top, font=small)) / 2, 4), top, font=small, fill=col)
        d.text((x + (cw - d.textlength(bottom, font=big)) / 2, 15), bottom, font=big, fill=fg)
        frames.append(im)
    save(frames, name, 200)


def buttons():
    button('btn-ie.gif', 'viewed with', 'IE 4.0', (0, 0, 128), (255, 255, 255), ((0, 102, 204), 'e', (255, 255, 0)))
    button('btn-800.gif', 'best viewed at', '800 x 600', (0, 0, 0), (0, 255, 0))
    button('btn-notepad.gif', 'made with', 'NOTEPAD', (255, 255, 255), (0, 0, 0), ((0, 0, 128), 'N', (255, 255, 255)))
    button('btn-dos.gif', 'powered by', 'DOS 7.1', (0, 0, 0), (192, 192, 192), ((128, 128, 128), 'C:\\', (0, 0, 0)))
    button('btn-win98.gif', 'runs on', 'WIN 98 SE', (0, 128, 128), (255, 255, 255), ((0, 0, 128), '98', (255, 255, 0)))
    button('btn-3dfx.gif', 'tested on a', 'VOODOO3', (0, 0, 0), (255, 255, 255), ((200, 0, 0), 'fx', (255, 255, 255)), anim=True)
    button('btn-serial.gif', 'null-modem', '115200 bd', (0, 0, 64), (0, 255, 255), ((0, 255, 255), 'RS', (0, 0, 64)))
    button('btn-nofraps.gif', 'no overlay!!', 'NO FRAPS', (128, 0, 0), (255, 255, 255), ((255, 255, 255), 'X', (200, 0, 0)))
    button('btn-gpl.gif', 'free software', 'GPL 2.0+', (128, 0, 0), (255, 255, 255))
    button('btn-download.gif', 'click here to', 'DOWNLOAD!', (0, 128, 0), (255, 255, 0), anim=True)
    im = Image.new('RGB', (88, 31), (0, 0, 0))
    d = crisp(ImageDraw.Draw(im))
    d.rectangle([0, 0, 87, 30], outline=(0, 255, 0))
    d.text((44, 9), 'Y2K', font=F('DejaVuSans-Bold.ttf', 13), fill=(0, 255, 0), anchor='mm')
    d.text((44, 23), 'COMPLIANT!!', font=F('DejaVuSans-Bold.ttf', 9), fill=(255, 255, 255), anchor='mm')
    save([im], 'btn-y2k.gif')


NAV = [
    ('nav-news.gif', "what's", 'NEW!!', (128, 0, 0), (255, 255, 0), ((255, 255, 0), '!', (200, 0, 0)), True),
    ('nav-what.gif', 'what is', 'THIS??', (0, 0, 128), (255, 255, 255), ((255, 255, 255), '?', (0, 0, 128)), False),
    ('nav-features.gif', 'cool', 'FEATURES', (0, 96, 0), (255, 255, 255), ((255, 215, 0), '*', (0, 96, 0)), False),
    ('nav-games.gif', 'my game', 'SCORES', (64, 0, 96), (255, 200, 255), ((0, 0, 0), 'fps', (0, 255, 0)), False),
    ('nav-lab.gif', 'my test', 'LAB', (0, 64, 96), (160, 255, 255), ((192, 192, 192), 'PC', (0, 0, 0)), False),
    ('nav-start.gif', 'getting', 'STARTED', (0, 0, 0), (192, 192, 192), ((128, 128, 128), 'C:\\', (0, 0, 0)), False),
    ('nav-cable.gif', 'the', 'CABLE', (0, 0, 64), (0, 255, 255), ((0, 255, 255), 'RS', (0, 0, 64)), False),
    ('nav-claude.gif', 'built with', 'CLAUDE', (96, 40, 20), (255, 220, 190), ((217, 119, 87), '*', (255, 255, 255)), False),
    ('nav-download.gif', 'download', 'ZONE', (0, 128, 0), (255, 255, 0), ((255, 255, 0), 'v', (0, 128, 0)), True),
    ('nav-source.gif', 'for nerds:', 'SOURCE', (0, 0, 0), (0, 255, 0), ((0, 64, 0), '</>', (0, 255, 0)), False),
    ('nav-todo.gif', 'coming', 'SOON!!', (120, 90, 0), (255, 255, 200), ((255, 215, 0), '...', (120, 60, 0)), False),
    ('nav-thanks.gif', 'thanks to my', 'FRIENDS', (90, 0, 90), (255, 255, 255), ((255, 80, 160), '<3', (255, 255, 255)), False),
]


def nav():
    for n, a, b, bg, fg, left, anim in NAV:
        button(n, a, b, bg, fg, left, anim or None)


def counter(n=6):
    digits = '%06d' % n
    font = F('DejaVuSansMono-Bold.ttf', 18)
    w, h = 6 * 16 + 4, 26
    im = Image.new('RGB', (w, h), (0, 0, 0))
    d = crisp(ImageDraw.Draw(im))
    for i, c in enumerate(digits):
        x = 2 + i * 16
        d.rectangle([x, 2, x + 14, h - 3], fill=(30, 30, 30), outline=(90, 90, 90))
        d.line([x, h // 2, x + 14, h // 2], fill=(0, 0, 0))
        d.text((x + 2, 1), c, font=font, fill=(0, 255, 0))
    save([im], 'counter.gif')


def bar():
    w, h = 600, 6
    frames = []
    for k in range(10):
        im = Image.new('RGB', (w, h))
        d = ImageDraw.Draw(im)
        for x in range(w):
            d.line([x, 0, x, h], fill=rainbow(((x / w) + k / 10) % 1))
        frames.append(im)
    save(frames, 'bar.gif', 100)


def award():
    w, h = 120, 120
    frames = []
    for k in range(8):
        im = Image.new('RGB', (w, h), (0, 0, 0))
        d = crisp(ImageDraw.Draw(im))
        for i in range(16):
            a = i * math.pi / 8 + k * 0.1
            d.polygon([(60, 54), (60 + 52 * math.cos(a), 54 + 52 * math.sin(a)), (60 + 52 * math.cos(a + 0.3), 54 + 52 * math.sin(a + 0.3))],
                      fill=(255, 215, 0) if i % 2 else (200, 140, 0))
        d.ellipse([22, 16, 98, 92], fill=(0, 0, 128), outline=(255, 255, 255))
        d.text((60, 36), 'COOL', font=F('DejaVuSerif-Bold.ttf', 15), fill=(255, 255, 0), anchor='mm')
        d.text((60, 54), 'GPU SITE', font=F('DejaVuSans-Bold.ttf', 11), fill=(255, 255, 255), anchor='mm')
        d.text((60, 70), 'of the day', font=F('DejaVuSans.ttf', 10), fill=rainbow(k / 8), anchor='mm')
        d.polygon([(40, 92), (34, 118), (48, 110), (54, 96)], fill=(160, 0, 0)); d.polygon([(80, 92), (86, 118), (72, 110), (66, 96)], fill=(160, 0, 0))
        frames.append(im)
    save(frames, 'award.gif', 140)


# ---------------------------------------------------------------- mascot: Vee, a little CRT monitor with a stopwatch
def vee_frame(k, size=64, wave=True, bg=(255, 255, 255)):
    im = Image.new('RGB', (size, size), bg)
    d = crisp(ImageDraw.Draw(im))
    bob = 1 if k % 4 in (1, 2) else 0
    y0 = 6 + bob
    d.rectangle([22, y0 + 38, 41, y0 + 42], fill=(150, 140, 120))                    # stand
    d.rectangle([12, y0 + 42, 21, y0 + 46], fill=(40, 40, 40)); d.rectangle([42, y0 + 42, 51, y0 + 46], fill=(40, 40, 40))  # shoes
    d.rounded_rectangle([6, y0, 57, y0 + 38], 5, fill=(214, 204, 178), outline=(90, 80, 60))  # beige case
    d.rounded_rectangle([11, y0 + 4, 52, y0 + 30], 4, fill=(10, 30, 10))            # screen
    for yy in range(y0 + 6, y0 + 29, 2):
        d.line([13, yy, 50, yy], fill=(16, 44, 16))                                  # scanlines
    blink = k % 12 == 11
    for ex in (21, 37):                                                              # eyes
        if blink:
            d.line([ex - 3, y0 + 13, ex + 3, y0 + 13], fill=(0, 255, 0))
        else:
            d.rectangle([ex - 2, y0 + 10, ex + 2, y0 + 15], fill=(0, 255, 0))
    d.arc([22, y0 + 15, 41, y0 + 26], 20, 160, fill=(0, 255, 0))                     # smile
    d.ellipse([46, y0 + 32, 50, y0 + 36], fill=(0, 255, 0) if k % 2 else (0, 120, 0))  # power LED
    # stopwatch in the right hand
    hx = 58
    d.line([52, y0 + 22, hx - 2, y0 + 26 - (2 if wave and k % 4 < 2 else 0)], fill=(90, 80, 60), width=2)
    d.ellipse([hx - 8, y0 + 20, hx + 4, y0 + 32], fill=(230, 230, 230), outline=(0, 0, 0))
    d.rectangle([hx - 3, y0 + 17, hx - 1, y0 + 19], fill=(0, 0, 0))
    a = k * math.pi / 6
    d.line([hx - 2, y0 + 26, hx - 2 + 5 * math.sin(a), y0 + 26 - 5 * math.cos(a)], fill=(200, 0, 0))
    # left arm waving
    wy = y0 + 10 if (wave and k % 4 < 2) else y0 + 18
    d.line([6, y0 + 22, 1, wy], fill=(90, 80, 60), width=2)
    return im


def vee():
    save([vee_frame(k) for k in range(12)], 'vee.gif', 140)
    save([vee_frame(k, wave=False) for k in range(12)], 'vee-small.gif', 140, scale=1)


def vee_hello():
    w, h = 220, 70
    font = F('DejaVuSans-Bold.ttf', 11)
    lines = ['Hi!! I am Vee.', 'I time every frame', 'your game draws!!']
    frames = []
    for k in range(12):
        im = Image.new('RGB', (w, h), (255, 255, 255))
        im.paste(vee_frame(k), (0, 3))
        d = crisp(ImageDraw.Draw(im))
        d.rounded_rectangle([74, 4, w - 4, h - 8], 8, fill=(255, 255, 200), outline=(0, 0, 0))
        d.polygon([(74, 34), (66, 40), (74, 42)], fill=(255, 255, 200), outline=(0, 0, 0))
        d.line([74, 35, 74, 41], fill=(255, 255, 200))
        for i, t in enumerate(lines):
            d.text((82, 10 + i * 16), t, font=font, fill=(0, 0, 160) if i != 2 else rainbow(k / 12))
        frames.append(im)
    save(frames, 'vee-hello.gif', 140)


def vee_walk():
    w, h = 600, 64
    frames = []
    for k in range(40):
        im = Image.new('RGB', (w, h), (255, 255, 255))
        x = int(k * (w + 64) / 40) - 64
        im.paste(vee_frame(k, wave=False), (x, 0))
        d = ImageDraw.Draw(im)
        for i in range(0, max(0, x), 14):                                            # frame ticks left behind
            d.line([i, 56, i, 56 - (4 + (i * 7) % 9)], fill=(0, 160, 0))
        frames.append(im)
    save(frames, 'vee-walk.gif', 90, scale=1)


# ---------------------------------------------------------------- frame-time oscilloscope
def scope():
    w, h = 300, 120
    small = F('DejaVuSansMono-Bold.ttf', 9)
    random.seed(7)
    data = []
    for i in range(400):
        v = 14.3 + random.gauss(0, 0.8)
        if i % 57 == 30: v = 33 + random.random() * 8                                # a hitch now and then
        data.append(v)
    frames = []
    for k in range(30):
        im = Image.new('RGB', (w, h), (192, 192, 192))
        d = crisp(ImageDraw.Draw(im))
        d.rectangle([0, 0, w - 1, h - 1], outline=(255, 255, 255)); d.line([w - 1, 0, w - 1, h - 1, 0, h - 1], fill=(64, 64, 64))
        d.rectangle([3, 3, w - 4, 14], fill=(0, 0, 128))
        d.text((6, 4), 'FLLOG.PY - frame times (ms)', font=small, fill=(255, 255, 255))
        d.rectangle([6, 18, w - 7, h - 20], fill=(0, 0, 0))
        top, bot = 22, h - 24
        sy = lambda v: bot - (min(v, 45) / 45) * (bot - top)
        for g in (16.7, 33.3):
            yy = sy(g)
            for x in range(8, w - 8, 4):
                d.point((x, yy), fill=(0, 90, 0))
            d.text((w - 34, yy - 10), '%.1f' % g, font=small, fill=(0, 140, 0))
        pts = [(8 + j * 2, sy(data[(k * 4 + j) % len(data)])) for j in range((w - 16) // 2)]
        d.line(pts, fill=(0, 255, 0))
        for x, y in pts:
            if y < sy(30): d.rectangle([x - 1, y - 1, x + 1, y + 1], fill=(255, 64, 64))
        seg = [data[(k * 4 + j) % len(data)] for j in range(140)]
        fps = 1000 / (sum(seg) / len(seg))
        d.text((8, h - 17), 'avg %.1f fps   1%% low %.1f   frames %d' % (fps, 1000 / max(seg), 6000 + k * 37),
               font=small, fill=(0, 0, 0))
        frames.append(im)
    save(frames, 'scope.gif', 120)


# ---------------------------------------------------------------- two PCs and a null-modem cable with bits flying
def pc(d, x, y, label, screen):
    d.rectangle([x, y + 30, x + 50, y + 46], fill=(214, 204, 178), outline=(90, 80, 60))       # desktop case
    d.rectangle([x + 34, y + 35, x + 46, y + 37], fill=(40, 40, 40))
    d.rounded_rectangle([x + 4, y, x + 46, y + 28], 3, fill=(214, 204, 178), outline=(90, 80, 60))
    d.rectangle([x + 8, y + 4, x + 42, y + 24], fill=screen)
    d.text((x + 25, y + 56), label, font=F('DejaVuSans-Bold.ttf', 9), fill=(0, 0, 0), anchor='mm')


def cable():
    w, h = 300, 70
    small = F('DejaVuSansMono-Bold.ttf', 8)
    frames = []
    for k in range(16):
        im = Image.new('RGB', (w, h), (255, 255, 255))
        d = crisp(ImageDraw.Draw(im))
        pc(d, 4, 2, 'GAME PC', (0, 0, 120) if k % 2 else (0, 0, 150))
        d.rectangle([12, 8, 16 + (k * 3) % 24, 12], fill=(255, 255, 0))                 # "game" on screen
        pc(d, w - 56, 2, 'HOST PC', (0, 0, 0))
        for i in range(4):
            d.line([w - 48, 9 + i * 4, w - 48 + ((k + i * 3) * 5) % 30, 9 + i * 4], fill=(0, 255, 0))
        d.line([54, 40, 80, 40, 80, 30, w - 80, 30, w - 80, 40, w - 56, 40], fill=(60, 60, 60), width=3)
        for b in range(6):
            x = 84 + ((k * 9 + b * 26) % (w - 172))
            d.text((x, 18), '01'[(k + b) % 2], font=small, fill=rainbow(b / 6))
        d.text((w / 2, 46), 'COM1 -> COM1  115200 8N1', font=small, fill=(0, 0, 128), anchor='mm')
        d.text((w / 2, 58), '9 bytes per frame', font=small, fill=(128, 0, 0), anchor='mm')
        frames.append(im)
    save(frames, 'cable.gif', 110)


# ---------------------------------------------------------------- "what can it time?" part cards
def part(name, label, sub, draw, bg):
    frames = []
    font, small = F('DejaVuSans-Bold.ttf', 10), F('DejaVuSans.ttf', 8)
    for k in range(6):
        im = Image.new('RGB', (66, 66), bg)
        d = crisp(ImageDraw.Draw(im))
        d.rectangle([0, 0, 65, 65], outline=(255, 255, 255)); d.line([65, 0, 65, 65, 0, 65], fill=(64, 64, 64))
        draw(d, k)
        d.text((33, 49), label, font=font, fill=(255, 255, 255), anchor='mm')
        d.text((33, 59), sub, font=small, fill=rainbow(k / 6), anchor='mm')
        frames.append(im)
    save(frames, name, 160)


def parts():
    def voodoo(d, k):
        d.rectangle([8, 12, 58, 34], fill=(0, 100, 40), outline=(0, 60, 20))
        d.rectangle([22, 16, 36, 30], fill=(20, 20, 20)); d.text((29, 23), '3dfx', font=F('DejaVuSans-Bold.ttf', 6), fill=(255, 255, 255), anchor='mm')
        for i in range(10): d.rectangle([10 + i * 5, 34, 12 + i * 5, 38], fill=(220, 180, 0))
        d.rectangle([44, 16, 54, 22], fill=(255, 0, 0) if k % 2 else (90, 0, 0))
    def dos(d, k):
        d.rectangle([8, 8, 58, 36], fill=(0, 0, 0))
        d.text((10, 10), 'C:\\>FRAMELOG', font=F('DejaVuSansMono.ttf', 7), fill=(192, 192, 192))
        if k % 2: d.rectangle([10, 22, 14, 24], fill=(192, 192, 192))
    def win(d, k):
        d.rectangle([8, 8, 58, 36], fill=(0, 128, 128))
        d.rectangle([12, 12, 40, 30], fill=(192, 192, 192)); d.rectangle([12, 12, 40, 15], fill=(0, 0, 128))
        d.rectangle([8, 33, 58, 36], fill=(192, 192, 192)); d.rectangle([9, 34, 18, 35], fill=(0, 128, 0))
    def gl(d, k):
        for i in range(3):
            a = k * 0.5 + i * 2.09
            d.polygon([(33, 22), (33 + 14 * math.cos(a), 22 + 14 * math.sin(a)), (33 + 14 * math.cos(a + 1), 22 + 14 * math.sin(a + 1))],
                      fill=[(255, 0, 0), (0, 200, 0), (0, 80, 255)][i])
    def vesa(d, k):
        for i in range(4):
            d.rectangle([8 + i * 3, 8 + i * 3, 46 + i * 3, 30 + i * 3], fill=rainbow((i + k) / 4), outline=(0, 0, 0))
    def capture(d, k):
        d.rectangle([12, 10, 54, 34], fill=(40, 40, 40))
        d.ellipse([26, 14, 40, 30], fill=(80, 80, 80), outline=(160, 160, 160))
        d.ellipse([30, 18, 36, 26], fill=(0, 0, 0))
        if k % 2: d.ellipse([46, 12, 50, 16], fill=(255, 0, 0))
    part('part-glide.gif', 'GLIDE 2/3', '3dfx Voodoo', voodoo, (0, 64, 32))
    part('part-dos.gif', 'DOS', 'page flips', dos, (60, 60, 60))
    part('part-ddraw.gif', 'DIRECTX', 'D3D 5 - 9', win, (0, 0, 128))
    part('part-opengl.gif', 'OPENGL', 'ICD + MiniGL', gl, (90, 0, 0))
    part('part-vesa.gif', 'VESA', '4F07h flips', vesa, (64, 0, 96))
    part('part-capture.gif', 'CAPTURE', 'VGA grabber', capture, (96, 48, 0))


def serialbench():
    """serial-bench's page: its own title + the shared art (OUT90S=serial-bench/docs/90s)"""
    title('SERIAL-BENCH', 'title.gif')
    for fn in ('construction', 'new', 'star', 'buttons', 'nav', 'bar', 'award', 'vee', 'vee_hello', 'vee_walk', 'scope', 'cable', 'parts'):
        globals()[fn]()
    report()


def report():
    """a little browser window showing flreport's GPU comparison bars"""
    w, h = 300, 150
    small, font = F('DejaVuSansMono-Bold.ttf', 9), F('DejaVuSans-Bold.ttf', 9)
    gpus = [('Voodoo3 3000', 58, (255, 160, 0)), ('GeForce 6800', 91, (0, 200, 0)), ('Radeon X300', 74, (220, 0, 0)),
            ('S3 Trio64', 21, (0, 120, 255))]
    frames = []
    for k in range(14):
        im = Image.new('RGB', (w, h), (192, 192, 192))
        d = crisp(ImageDraw.Draw(im))
        d.rectangle([0, 0, w - 1, h - 1], outline=(255, 255, 255)); d.line([w - 1, 0, w - 1, h - 1, 0, h - 1], fill=(64, 64, 64))
        d.rectangle([3, 3, w - 4, 14], fill=(0, 0, 128))
        d.text((6, 4), 'compare.html (example) - Netscape', font=small, fill=(255, 255, 255))
        d.rectangle([6, 18, w - 7, h - 7], fill=(255, 255, 255))
        d.text((12, 22), 'EXAMPLE: timedemo avg fps', font=font, fill=(0, 0, 128))
        for i, (g, v, c) in enumerate(gpus):
            y = 40 + i * 24
            d.text((12, y + 3), g, font=small, fill=(0, 0, 0))
            bw = int(min(1, k / 10) * v * 1.6)
            d.rectangle([96, y, 96 + bw, y + 14], fill=c, outline=(0, 0, 0))
            if k >= 10: d.text((100 + bw, y + 2), str(v), font=small, fill=(0, 0, 0))
        frames.append(im)
    save(frames, 'report.gif', 120)


# ---------------------------------------------------------------- news ticker (no release asset needed)
def ticker():
    text = ('  +++ test6: Quake II\'s software renderer is timed too!! +++ new wiki with every switch explained '
            '+++ Quake II: same 39.9 fps with or without FRAMELOG +++ Forsaken demo: 0.5% +++ real hardware next!! +++ ')
    font = F('DejaVuSansMono-Bold.ttf', 13)
    w, h = 600, 22
    probe = ImageDraw.Draw(Image.new('L', (1, 1)))
    tw = int(probe.textlength(text, font=font))
    frames = []
    step = 6
    for k in range(tw // step):
        im = Image.new('RGB', (w, h), (0, 0, 0))
        d = crisp(ImageDraw.Draw(im))
        x = -k * step
        while x < w:
            d.text((x, 3), text, font=font, fill=(255, 176, 0))
            x += tw
        for yy in range(0, h, 2):
            d.line([0, yy, w, yy], fill=(0, 0, 0))
        frames.append(im)
    save(frames, 'ticker.gif', 60, scale=1)


ALL = ['title', 'construction', 'new', 'star', 'buttons', 'nav', 'counter', 'bar', 'award', 'vee', 'vee_hello',
       'vee_walk', 'scope', 'cable', 'parts', 'ticker']

if __name__ == '__main__':
    for fn in sys.argv[1:] or ALL:
        globals()[fn]()
    tot = 0
    for f in sorted(os.listdir(OUT)):
        if f.endswith('.gif'):
            tot += os.path.getsize(os.path.join(OUT, f))
    print('gifs: %d KB' % (tot // 1024))
