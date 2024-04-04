#!/usr/bin/env python3

from collections import OrderedDict
from adjustText import adjust_text
import matplotlib.pyplot as plt
import numpy as np
import sys

def argsort(seq):
    return [x for x,y in sorted(enumerate(seq), key = lambda x: x[1])]

def write_roman(num):
    roman = OrderedDict()
    roman[1000] = "M"
    roman[900] = "CM"
    roman[500] = "D"
    roman[400] = "CD"
    roman[100] = "C"
    roman[90] = "XC"
    roman[50] = "L"
    roman[40] = "XL"
    roman[10] = "X"
    roman[9] = "IX"
    roman[5] = "V"
    roman[4] = "IV"
    roman[1] = "I"

    def roman_num(num):
        for r in roman.keys():
            x, y = divmod(num, r)
            yield roman[r] * x
            num -= (r * x)
            if num <= 0:
                break

    return "".join([a for a in roman_num(num)])

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

colors = ["#FF0000", "#00FF00", "#0000FF", "#FF00FF"]
lightcolors = ["#FF7F7F", "#7FFF7F", "#7F7FFF", "#FF7FFF"]

plt.figure(figsize=(12, 8))
for model_idx in range(0, num_models):
    x, y = data[:NUM_4K_FORMATS, model_idx, 0], data[:NUM_4K_FORMATS, model_idx, 1]
    pareto = [all([(tx <= ox or ty >= oy) for ox, oy in zip(x, y)]) for tx, ty in zip(x, y)]
    points = list(zip(data[:NUM_4K_FORMATS, model_idx, 0], data[:NUM_4K_FORMATS, model_idx, 1]))
    indices = list(filter(lambda idx: pareto[idx], argsort(points)))
    p1 = [points[idx][0] for idx in indices]
    p2 = [points[idx][1] for idx in indices]
    plt.plot(p1, p2, c="gray", linestyle='dashed')

for model_idx in range(0, num_models):
    x, y = data[:NUM_4K_FORMATS, model_idx, 0], data[:NUM_4K_FORMATS, model_idx, 1]
    pareto = [all([(tx <= ox or ty >= oy) for ox, oy in zip(x, y)]) for tx, ty in zip(x, y)]
    plt.scatter(x, y, c=[colors[model_idx] if p else lightcolors[model_idx] for p in pareto], label=models[model_idx], s=[float(p) * 25.0 + 15.0 for p in pareto])

texts = []
for model_idx in range(0, num_models):
    for format_idx in range(0, NUM_4K_FORMATS):
        x, y = data[format_idx, model_idx, 0], data[format_idx, model_idx, 1]
        if y == 0.0 and formats[format_idx].startswith("SV") and models[model_idx] == "sponza":
            y = format_idx / 1000.0
        texts.append(plt.text(x, y, write_roman(format_idx + 1)))

for format_idx in range(0, NUM_4K_FORMATS):
    print(formats[format_idx], write_roman(format_idx + 1))

plot_cutoff = 0.0 in data[:NUM_4K_FORMATS, :, 1]
if plot_cutoff:
    plt.axvline(2 ** 32)
    plt.text((2 ** 32) * 1.01, 2, 'GPU Size Cutoff (4 GiB)', rotation=270)

plt.xscale('log')
plt.xlabel("Model Size (bytes)")
plt.ylabel("Ray Intersection Performance (FPS)")
plt.title("Rendering Performance vs. Compression for 4096^3 formats")
adjust_text(texts)
plt.legend()
plt.show()

print("")

plt.figure(figsize=(12, 8))
for model_idx in range(0, num_models):
    x, y = data[NUM_4K_FORMATS:, model_idx, 0], data[NUM_4K_FORMATS:, model_idx, 1]
    pareto = [all([(tx <= ox or ty >= oy) for ox, oy in zip(x, y)]) for tx, ty in zip(x, y)]
    points = list(zip(data[NUM_4K_FORMATS:, model_idx, 0], data[NUM_4K_FORMATS:, model_idx, 1]))
    indices = list(filter(lambda idx: pareto[idx], argsort(points)))
    p1 = [points[idx][0] for idx in indices]
    p2 = [points[idx][1] for idx in indices]
    plt.plot(p1, p2, c="gray", linestyle='dashed')

for model_idx in range(0, num_models):
    x, y = data[NUM_4K_FORMATS:, model_idx, 0], data[NUM_4K_FORMATS:, model_idx, 1]
    pareto = [all([(tx <= ox or ty >= oy) for ox, oy in zip(x, y)]) for tx, ty in zip(x, y)]
    plt.scatter(x, y, c=[colors[model_idx] if p else lightcolors[model_idx] for p in pareto], label=models[model_idx], s=[float(p) * 25.0 + 15.0 for p in pareto])

texts = []
for model_idx in range(0, num_models):
    for format_idx in range(NUM_4K_FORMATS, num_formats):
        x, y = data[format_idx, model_idx, 0], data[format_idx, model_idx, 1]
        texts.append(plt.text(x, y, write_roman(format_idx + 1 - NUM_4K_FORMATS)))

for format_idx in range(NUM_4K_FORMATS, num_formats):
    print(formats[format_idx], write_roman(format_idx + 1 - NUM_4K_FORMATS))

plt.xscale('log')
plt.xlabel("Model Size (bytes)")
plt.ylabel("Ray Intersection Performance (FPS)")
plt.title("Rendering Performance vs. Compression for 512^3 formats")
adjust_text(texts)
plt.legend()
plt.show()
