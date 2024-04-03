#!/usr/bin/env python3

from adjustText import adjust_text
import matplotlib.pyplot as plt
import numpy as np
import sys

NUM_4K_FORMATS = 9

with open(sys.argv[1]) as f:
    content = f.readlines()

datapoints = []
while len(content) > 0:
    datapoint = content[:4]
    content = content[4:]
    
    format = datapoint[0].rstrip()
    model = datapoint[1].rstrip()
    size = int(datapoint[2].rstrip())
    fps = datapoint[3].rstrip()
    if "INFO: Final FPS measurement: " in fps:
        fps = float(fps[len("INFO: Final FPS measurement: "):])
    else:
        fps = 0.0
    datapoints.append((format, model, size, fps))

formats = []
set_formats = set()
for datapoint in datapoints:
    if not datapoint[0] in set_formats:
        set_formats.add(datapoint[0])
        formats.append(datapoint[0])
num_formats = len(formats)

models = []
set_models = set()
for datapoint in datapoints:
    if not datapoint[1] in set_models:
        set_models.add(datapoint[1])
        models.append(datapoint[1])
num_models = len(models)

data = np.zeros((num_formats, num_models, 2))
for datapoint in datapoints:
    format_idx = formats.index(datapoint[0])
    model_idx = models.index(datapoint[1])
    data[format_idx, model_idx, 0] = datapoint[2]
    data[format_idx, model_idx, 1] = datapoint[3]

for model_idx in range(0, num_models):
    plt.figure(figsize=(12, 8))
    plt.scatter(data[:NUM_4K_FORMATS, model_idx, 0], data[:NUM_4K_FORMATS, model_idx, 1])
    texts = []
    for format_idx in range(0, NUM_4K_FORMATS):
        x, y = data[format_idx, model_idx, 0], data[format_idx, model_idx, 1]
        if y == 0.0 and formats[format_idx].startswith("SV") and models[model_idx] == "sponza":
            y = format_idx / 1000.0
        texts.append(plt.text(x, y, formats[format_idx]))

    plot_cutoff = 0.0 in data[:NUM_4K_FORMATS, model_idx, 1]

    if plot_cutoff:
        plt.axvline(2 ** 32)
        plt.text((2 ** 32) * 1.01, 2, 'GPU Size Cutoff', rotation=270)
    plt.xscale('log')
    plt.xlabel("Model Size (bytes)")
    plt.ylabel("Ray Intersection Performance (FPS)")
    plt.title("Rendering Performance vs. Compression for 4096^3 formats (" + models[model_idx] + ", " + sys.argv[1] + ")")
    adjust_text(texts)
    plt.show()

for model_idx in range(0, num_models):
    plt.figure(figsize=(12, 8))
    plt.scatter(data[NUM_4K_FORMATS:, model_idx, 0], data[NUM_4K_FORMATS:, model_idx, 1])
    texts = []
    for format_idx in range(NUM_4K_FORMATS, num_formats):
        x, y = data[format_idx, model_idx, 0], data[format_idx, model_idx, 1]
        texts.append(plt.text(x, y, formats[format_idx]))

    plt.xscale('log')
    plt.xlabel("Model Size (bytes)")
    plt.ylabel("Ray Intersection Performance (FPS)")
    plt.title("Rendering Performance vs. Compression for 512^3 formats (" + models[model_idx] + ", " + sys.argv[1] + ")")
    adjust_text(texts)
    plt.show()
