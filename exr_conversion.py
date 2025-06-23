import os
import numpy as np
from OpenImageIO import ImageSpec, ImageBuf, ImageInput, ImageOutput

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))


def create_mulichannel_exr(image_dir, output_path):
    channel_names = []
    channel_arrays = []
    for filename in os.listdir(image_dir):
        path = os.path.join(image_dir, filename)
        if not os.path.isfile(path):
            continue

        inp = ImageInput.open(path)
        img = inp.read_image("uint8")
        basename = os.path.splitext(os.path.basename(path))[0]
        for i, ch in enumerate("RGBA"):
            channel_names.append(f"{basename}.{ch}")
            channel_arrays.append(img[..., i])
        inp.close()

    stacked = np.stack(channel_arrays, axis=-1)
    height, width, nchannels = stacked.shape
    spec = ImageSpec(width, height, nchannels, "uint8")
    spec.channelnames = channel_names
    spec.attribute("Compression", "ZIP")
    buf = ImageBuf(spec)
    buf.set_pixels(buf.roi, stacked)

    out = ImageOutput.create(output_path)
    out.open(output_path, spec)
    buf.write(out)
    out.close()


def create_mulilayer_exr(image_dir, output_path):
    images = []
    specs = []
    for filename in os.listdir(image_dir):
        path = os.path.join(image_dir, filename)
        if not os.path.isfile(path):
            continue

        basename = os.path.splitext(filename)[0]

        inp = ImageInput.open(path)
        img = inp.read_image("uint8")
        inp.close()

        height, width, nchannels = img.shape
        spec = ImageSpec(width, height, nchannels, "uint8")
        spec.channelnames = [f"{ch}" for ch in "RGBA"]
        spec.attribute("Compression", "ZIP")
        spec.attribute("name", basename)  # Layer name

        images.append((img, spec))
        specs.append(spec)

    out = ImageOutput.create(output_path)
    out.open(output_path, specs)
    first = True
    for img, spec in images:
        if first:
            first = False
        else:
            out.open(output_path, spec, "AppendSubimage")

        out.write_image(img)

    out.close()


def main():
    image_dir = os.path.join(CURRENT_DIR, "armor")
    output_path = os.path.join(CURRENT_DIR, "armor.exr")
    create_mulilayer_exr(image_dir, output_path)


if __name__ == "__main__":
    main()
