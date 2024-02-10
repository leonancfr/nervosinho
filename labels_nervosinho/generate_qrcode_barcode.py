import sys
import os
import csv
import qrcode
from PIL import Image, ImageDraw, ImageFont
from tqdm import tqdm
import barcode
from barcode.writer import ImageWriter

def generate_qr_code(data):
    qr = qrcode.QRCode(
        version=1,
        error_correction=qrcode.constants.ERROR_CORRECT_L,
        box_size=50,#67,
        border=0,
    )
    qr.add_data(data)
    qr.make(fit=True)

    img = qr.make_image(fill_color="black", back_color="white")
    return img

def generate_barcode(data, width=20, height=60):
    barcode_class = barcode.get_barcode_class('code128')
    barcode_instance = barcode_class(data, writer=ImageWriter())

    options = {
        'width': width,
        'height': height,
        'module_width': 1.4,
        'module_height': 40,  # Ajuste conforme necessário
        'text': '',
    }

    barcode_img = barcode_instance.render(options)
    barcode_img_without_text = barcode_img.crop((0, 0, barcode_img.width, barcode_img.height - 100))  # Ajuste conforme necessário
    return barcode_img_without_text

def overlay_qr_code(background_img, qr_img, x_qr, y_qr):
    # Converte ambas as imagens para o modo RGBA
    qr_img = qr_img.convert("RGBA")
    background_img = background_img.convert("RGBA")

    # Coloca o código QR sobre a imagem de fundo
    background_img.paste(qr_img, (x_qr, y_qr), qr_img)
    return background_img

def overlay_barcode(background_img, barcode_img, x_barcode, y_barcode):
    barcode_img = barcode_img.convert("RGBA")
    background_img = background_img.convert("RGBA")

    background_img.paste(barcode_img, (x_barcode, y_barcode), barcode_img)
    return background_img

def add_text_to_image(image, text, x_text, y_text, font_size, font_color="black"):
    draw = ImageDraw.Draw(image)
    # Use a fonte Arial por padrão
    font = ImageFont.truetype("Arial Black.ttf", font_size)

    draw.text((x_text, y_text), text, font=font, fill=font_color)

    return image

def save_combined_image(combined_img, output_filename):
    combined_img.save(output_filename)

def main(csv_file, output_dir, revision_number, batch_number):
    template_filename = 'padrao_barcode.png'

    x_qr, y_qr = 2500, 100  # Posição do QR code na imagem existente
    background_img = Image.open(template_filename).convert("RGBA")

    default_img = add_text_to_image(background_img, revision_number, 550, 950, font_size=120, font_color="black")
    default_img = add_text_to_image(default_img, batch_number, 550, 1125, font_size=120, font_color="black")

    with open(csv_file, 'r', newline='', encoding='utf-8') as csv_file:
        reader = csv.reader(csv_file, delimiter=';')
        next(reader)  # Ignora cabeçalho se houver
        total_rows = sum(1 for row in reader)
    
        with tqdm(total=total_rows, desc='Gerando etiquetas') as pbar:
            csv_file.seek(1)
            next(reader)
            for row in reader:
                mac_address = row[0]  # Assume que o MAC address está na primeira coluna

                qr_img = generate_qr_code(mac_address)
                barcode_img = generate_barcode(mac_address)

                combined_img = overlay_qr_code(default_img, qr_img, 2800, 100)
                combined_img = overlay_barcode(combined_img, barcode_img, 100, 1650)
                
                text = f'{mac_address}'
                combined_img = add_text_to_image(combined_img, text, 550, 1300, font_size=120, font_color="black")

                output_filename = f'{output_dir}label_{mac_address.replace(":", "-")}.png'
                save_combined_image(combined_img, output_filename)

                pbar.update(1)

    print(f'{total_rows} etiquetas geradas e salvas na pasta {output_dir}.')

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Uso: python3 generate_qrcode_barcode.py csv_file.csv revision batch_number output_dir(optional)")
        sys.exit(1)

    csv_filename = sys.argv[1]
    revision_number = sys.argv[2]
    batch_number = sys.argv[3]
    if len(sys.argv) == 4:
        output_dir = ""
    else:
        output_dir = sys.argv[4]
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        output_dir = output_dir + "/"

    main(csv_filename, output_dir, revision_number, batch_number)
