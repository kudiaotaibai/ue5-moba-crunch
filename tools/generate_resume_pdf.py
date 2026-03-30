from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

W, H = 2480, 3508
MARGIN_X = 150
MARGIN_Y = 110
LINE_GAP = 10
SECTION_GAP = 18
ITEM_GAP = 6
CONTENT_WIDTH = W - MARGIN_X * 2


def text_width(draw, text, font):
    box = draw.textbbox((0, 0), text, font=font)
    return box[2] - box[0]


def wrap_text(draw, text, font, width):
    lines = []
    current = ''
    for ch in text:
        trial = current + ch
        if current and text_width(draw, trial, font) > width:
            lines.append(current)
            current = ch
        else:
            current = trial
    if current:
        lines.append(current)
    return lines


def draw_paragraph(draw, text, x, y, font, bullet=False):
    prefix = '* ' if bullet else ''
    lines = wrap_text(draw, prefix + text, font, CONTENT_WIDTH)
    prefix_width = text_width(draw, '* ', font) if bullet else 0
    yy = y
    for index, line in enumerate(lines):
        xx = x + prefix_width if bullet and index > 0 else x
        draw.text((xx, yy), line, font=font, fill=(34, 34, 34))
        box = draw.textbbox((xx, yy), line, font=font)
        yy += (box[3] - box[1]) + LINE_GAP
    return yy


def draw_center(draw, text, y, font, color):
    width = text_width(draw, text, font)
    x = (W - width) // 2
    draw.text((x, y), text, font=font, fill=color)
    box = draw.textbbox((x, y), text, font=font)
    return box[3]


def main():
    output_image = Path(r'E:\ue\Crunch\_resume_new_image.png')
    output_pdf = Path(r'E:\ue\Crunch\_resume_new.pdf')

    image = Image.new('RGB', (W, H), 'white')
    draw = ImageDraw.Draw(image)

    font_name = ImageFont.truetype(r'C:\Windows\Fonts\simhei.ttf', 62)
    font_section = ImageFont.truetype(r'C:\Windows\Fonts\simhei.ttf', 38)
    font_sub = ImageFont.truetype(r'C:\Windows\Fonts\simhei.ttf', 32)
    font_regular = ImageFont.truetype(r'C:\Windows\Fonts\simhei.ttf', 34)
    font_small = ImageFont.truetype(r'C:\Windows\Fonts\simhei.ttf', 30)

    name = '\u8d75\u4e91\u98de'
    job = '\u6c42\u804c\u610f\u5411\uff1aUE\u5ba2\u6237\u7aef\u5f00\u53d1\u5b9e\u4e60\u751f / \u6e38\u620f\u5ba2\u6237\u7aef\u5f00\u53d1\u5b9e\u4e60\u751f'
    contact = '\u7535\u8bdd\uff1a13180902224 \uff5c \u90ae\u7bb1\uff1a645230806@qq.com \uff5c \u4f5c\u54c1\u94fe\u63a5 / GitHub\uff1a\u53ef\u8865\u5145'

    skills = [
        '\u719f\u6089 Unreal Engine Gameplay Framework\u3001GAS\u3001UMG\u3001Session\u3001Dedicated Server\uff0c\u7406\u89e3 UE \u5ba2\u6237\u7aef\u591a\u4eba\u5bf9\u5c40\u57fa\u672c\u6846\u67b6\u3002',
        '\u719f\u7ec3\u4f7f\u7528 C++17 \u8fdb\u884c UE \u5f00\u53d1\uff0c\u5177\u5907\u59d4\u6258\u3001Lambda\u3001\u6570\u636e\u9a71\u52a8\u5f00\u53d1\u7ecf\u9a8c\u3002',
        '\u5177\u5907 UE5 RDG\u3001Global Shader\u3001Scene View Extension \u5b9e\u8df5\u7ecf\u9a8c\uff0c\u80fd\u5f00\u53d1\u81ea\u5b9a\u4e49\u540e\u5904\u7406 Pass\u3002',
        '\u4e86\u89e3 Python\u3001Flask\u3001Docker\u3001Linux\uff0c\u719f\u6089 Git / P4V \u534f\u4f5c\u6d41\u7a0b\u3002',
    ]

    internship = [
        '\u91cd\u6784\u5e93\u5b58\u7cfb\u7edf\uff0c\u89e3\u8026\u6570\u636e\u5c42\u4e0e UMG \u8868\u73b0\u5c42\uff0c\u652f\u6491\u5546\u5e97\u4e0e\u88c5\u5907\u6a21\u5757\u8fed\u4ee3\u3002',
        '\u57fa\u4e8e DataAsset / DataTable \u91cd\u6784\u7269\u54c1\u914d\u7f6e\u6d41\u7a0b\uff0c\u63d0\u5347\u65b0\u589e\u548c\u8c03\u6574\u6548\u7387\u3002',
        '\u5f00\u53d1 Editor Utility Widget \u5e93\u5b58\u8c03\u8bd5\u5de5\u5177\uff0c\u53c2\u4e0e\u5546\u5e97\u7cfb\u7edf\u5f00\u53d1\u4e0e\u670d\u52a1\u7aef\u6743\u5a01\u4ea4\u6613\u903b\u8f91\u5b9e\u73b0\u3002',
    ]

    moba = [
        '\u57fa\u4e8e UE5 C++ \u72ec\u7acb\u5b8c\u6210\u591a\u4eba\u5bf9\u6218\u6280\u672f\u539f\u578b\uff0c\u8986\u76d6 Gameplay Framework\u3001GAS\u3001UMG\u3001AI\u3001Session\u3001Dedicated Server\u3002',
        '\u642d\u5efa\u591a\u4eba\u5bf9\u5c40\u6846\u67b6\uff0c\u5b9e\u73b0\u89d2\u8272\u751f\u6210\u3001\u9635\u8425\u5206\u914d\u3001\u80dc\u8d1f\u7ed3\u7b97\u4e0e\u5730\u56fe\u5207\u6362\u3002',
        '\u57fa\u4e8e GAS \u5b9e\u73b0\u6280\u80fd\u3001\u5c5e\u6027\u6210\u957f\u3001Buff / Debuff\u3001\u5347\u7ea7\u70b9\u4e0e\u6b7b\u4ea1\u6d41\u7a0b\u3002',
        '\u7f16\u5199 Python Flask \u534f\u8c03\u5668\u4e0e Docker \u90e8\u7f72\u914d\u7f6e\uff0c\u5b8c\u6210\u672c\u5730\u591a\u5b9e\u4f8b\u670d\u52a1\u542f\u52a8\u4e0e\u5bf9\u5c40\u5165\u53e3\u9a8c\u8bc1\u3002',
    ]

    rdg = [
        '\u57fa\u4e8e UE5.5 C++\u3001RDG\u3001Global Shader \u4e0e Scene View Extension \u5b9e\u73b0\u81ea\u5b9a\u4e49\u540e\u5904\u7406\u6e32\u67d3\u63d2\u4ef6\u3002',
        '\u5b9e\u73b0 Depth\u3001Normal\u3001Roughness\u3001SceneColor \u8c03\u8bd5\u89c6\u56fe\uff0c\u4ee5\u53ca Gaussian Blur\u3001Bilateral Blur \u5c4f\u5e55\u7a7a\u95f4\u6ee4\u6ce2\u6548\u679c\u3002',
        '\u72ec\u7acb\u5b9e\u73b0 SSAO\u3001Normal-aware Bilateral Blur \u4e0e AO Composite\uff0c\u5e76\u5f00\u53d1 Slate \u8fd0\u884c\u65f6\u8c03\u53c2\u7a97\u53e3\u3002',
    ]

    awards = [
        '2025 \u5b8c\u7f8e\u4e16\u754c\u9ad8\u6821 MiniGame \u5f00\u53d1\u5927\u8d5b \u6700\u4f73\u4eba\u6c14\u5956 \u300a\u8c03\u548c\u8005\u300b',
        '2025 Godot \u5f00\u53d1\u5927\u8d5b \u4f18\u79c0\u5956 \u300a[NULL] Protocol\u300b',
    ]

    y = MARGIN_Y
    y = draw_center(draw, name, y, font_name, (17, 17, 17)) + 12
    y = draw_center(draw, job, y, font_sub, (60, 60, 60)) + 8
    y = draw_center(draw, contact, y, font_small, (80, 80, 80)) + 24

    def section(title):
        nonlocal_y[0] = nonlocal_y[0]

    nonlocal_y = [y]

    def draw_section(title):
        draw.text((MARGIN_X, nonlocal_y[0]), title, font=font_section, fill=(0, 0, 0))
        box = draw.textbbox((MARGIN_X, nonlocal_y[0]), title, font=font_section)
        nonlocal_y[0] = box[3] + 8

    draw_section('\u6559\u80b2\u80cc\u666f')
    nonlocal_y[0] = draw_paragraph(draw, '\u54c8\u5c14\u6ee8\u5e08\u8303\u5927\u5b66\uff5c\u8ba1\u7b97\u673a\u79d1\u5b66\u4e0e\u6280\u672f\uff08\u672c\u79d1\uff09\uff5c2022.09 - 2026.06\uff08\u9884\u8ba1\uff09', MARGIN_X, nonlocal_y[0], font_regular)
    nonlocal_y[0] += SECTION_GAP

    draw_section('\u4e13\u4e1a\u6280\u80fd')
    for item in skills:
        nonlocal_y[0] = draw_paragraph(draw, item, MARGIN_X, nonlocal_y[0], font_regular, bullet=True)
        nonlocal_y[0] += ITEM_GAP
    nonlocal_y[0] += SECTION_GAP

    draw_section('\u5b9e\u4e60\u7ecf\u5386')
    intern_title = '\u4e5d\u679c\u4e00\u9ea6\u79d1\u6280\uff08UE\u72ec\u7acb\u6e38\u620f\u5de5\u4f5c\u5ba4\uff09\uff5cUE\u7a0b\u5e8f\u5f00\u53d1\u5b9e\u4e60\u751f\uff5c2025.08 - \u81f3\u4eca'
    draw.text((MARGIN_X, nonlocal_y[0]), intern_title, font=font_sub, fill=(17, 17, 17))
    box = draw.textbbox((MARGIN_X, nonlocal_y[0]), intern_title, font=font_sub)
    nonlocal_y[0] = box[3] + 6
    for item in internship:
        nonlocal_y[0] = draw_paragraph(draw, item, MARGIN_X, nonlocal_y[0], font_regular, bullet=True)
        nonlocal_y[0] += ITEM_GAP
    nonlocal_y[0] += SECTION_GAP

    draw_section('\u9879\u76ee\u7ecf\u5386')
    moba_title = 'UE5 \u5728\u7ebf MOBA \u6280\u672f\u539f\u578b\uff5c\u72ec\u7acb\u5f00\u53d1\uff5c2025.04 - 2025.07'
    draw.text((MARGIN_X, nonlocal_y[0]), moba_title, font=font_sub, fill=(17, 17, 17))
    box = draw.textbbox((MARGIN_X, nonlocal_y[0]), moba_title, font=font_sub)
    nonlocal_y[0] = box[3] + 6
    for item in moba:
        nonlocal_y[0] = draw_paragraph(draw, item, MARGIN_X, nonlocal_y[0], font_regular, bullet=True)
        nonlocal_y[0] += ITEM_GAP

    rdg_title = 'UE5 RDG \u6e32\u67d3\u63d2\u4ef6\u5f00\u53d1\uff5c\u72ec\u7acb\u5f00\u53d1\uff5c2026.03'
    draw.text((MARGIN_X, nonlocal_y[0]), rdg_title, font=font_sub, fill=(17, 17, 17))
    box = draw.textbbox((MARGIN_X, nonlocal_y[0]), rdg_title, font=font_sub)
    nonlocal_y[0] = box[3] + 6
    for item in rdg:
        nonlocal_y[0] = draw_paragraph(draw, item, MARGIN_X, nonlocal_y[0], font_regular, bullet=True)
        nonlocal_y[0] += ITEM_GAP
    nonlocal_y[0] += SECTION_GAP

    draw_section('\u83b7\u5956\u7ecf\u5386')
    for item in awards:
        nonlocal_y[0] = draw_paragraph(draw, item, MARGIN_X, nonlocal_y[0], font_regular, bullet=True)
        nonlocal_y[0] += ITEM_GAP

    image.save(output_image)
    image.save(output_pdf, 'PDF', resolution=300.0)
    print(output_pdf)


if __name__ == '__main__':
    main()
