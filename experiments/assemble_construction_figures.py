#!/usr/bin/env python3

from collections import OrderedDict
from adjustText import adjust_text
import matplotlib.pyplot as plt
import numpy as np
import sys

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

formats = [
	"DF(16, 16, 16, 6) DF(8, 8, 8, 6) SVDAG(4)",
	"DF(16, 16, 16, 6) Raw(8, 8, 8) SVDAG(4)",
	"Raw(16, 16, 16) Raw(8, 8, 8) SVDAG(4)",
	"Raw(16, 16, 16) SVO(3) SVDAG(4)",
	"Raw(16, 16, 16) Raw(16, 16, 16) Raw(8, 8, 8)",
	"DF(16, 16, 16, 6) SVO(7)",
	"DF(16, 16, 16, 6) SVDAG(7)",
	"DF(64, 64, 64, 6) SVO(5)",
	"DF(64, 64, 64, 6) SVDAG(5)",
	"Raw(16, 16, 16) SVO(7)",
	"Raw(16, 16, 16) SVDAG(7)",
	"Raw(64, 64, 64) SVO(5)",
	"Raw(64, 64, 64) SVDAG(5)",
	"Raw(8, 8, 8) SVDAG(8)",
	"Raw(256, 256, 256) SVDAG(3)",
	"SVO(7) SVDAG(4)",
	"SVO(5) SVDAG(6)",
	"SVO(3) SVDAG(8)",
	"SVO(11)",
	"SVDAG(11)",

	"DF(8, 8, 8, 6) DF(8, 8, 8, 6) SVDAG(3)",
	"DF(8, 8, 8, 6) Raw(8, 8, 8) SVDAG(3)",
	"Raw(8, 8, 8) Raw(8, 8, 8) SVDAG(3)",
	"Raw(8, 8, 8) Raw(8, 8, 8) Raw(8, 8, 8)",
	"Raw(16, 16, 16) Raw(2, 2, 2) Raw(16, 16, 16)",
	"DF(16, 16, 16, 6) SVO(5)",
	"DF(16, 16, 16, 6) SVDAG(5)",
	"Raw(16, 16, 16) SVO(5)",
	"Raw(16, 16, 16) SVDAG(5)",
	"Raw(4, 4, 4) SVDAG(7)",
	"Raw(128, 128, 128) SVDAG(2)",
	"SVO(5) Raw(16, 16, 16)",
	"SVDAG(5) Raw(16, 16, 16)",
	"DF(32, 32, 32, 6) DF(16, 16, 16, 6)",
	"DF(32, 32, 32, 6) Raw(16, 16, 16)",
	"Raw(32, 32, 32) Raw(16, 16, 16)",
	"Raw(512, 512, 512)",
	"DF(512, 512, 512, 6)",
	"SVO(9)",
	"SVDAG(9)"
]
num_formats = len(formats)

models = [
	"san-miguel-low-poly",
	"hairball",
	"buddha",
	"sponza"
]
num_models = len(models)

labels = list(map(write_roman, range(1, 21)))
labels = labels + labels

with open(sys.argv[1]) as f:
    content = f.readlines()

datapoints = []
while len(content) > 0:
    datapoint = content[:4]
    content = content[4:]
    
    format = datapoint[0].rstrip()
    model = datapoint[1].rstrip()
    seconds = datapoint[2].split(":")
    seconds = float(seconds[0])*60.0+float(seconds[1])
    max_mem = int(datapoint[3].rstrip())
    datapoints.append((format, model, seconds, max_mem))

data = np.zeros((num_formats, num_models, 2))
for datapoint in datapoints:
    format_idx = formats.index(datapoint[0])
    model_idx = models.index(datapoint[1])
    data[format_idx, model_idx, 0] = datapoint[2]
    data[format_idx, model_idx, 1] = datapoint[3]

models = list(map(lambda x: x.replace("-low-poly", ""), models))

colors = ["#FF7F7F", "#7FFF7F", "#7F7FFF", "#FF7FFF"]

plt.figure(figsize=(7,3.5))
for model_idx in range(num_models):
    plt.bar(np.arange(num_formats/2) + 1.25 * model_idx * num_formats / 2, data[:20, model_idx, 0].reshape((-1)), label=models[model_idx], color=colors[model_idx], width=1.0, edgecolor='#606060')
plt.xticks(np.sort(np.concatenate((1.25 * np.arange(num_models) * num_formats / 2, 19.0 + 1.25 * np.arange(num_models) * num_formats / 2))), labels=["I", "XX", "I", "XX", "I", "XX", "I", "XX"])
plt.ylabel("Construction Time (seconds)")
plt.legend()
plt.savefig("construction-2048-times.png", bbox_inches='tight', pad_inches=0.02)

plt.figure(figsize=(7,3.5))
for model_idx in range(num_models):
    plt.bar(np.arange(num_formats/2) + 1.25 * model_idx * num_formats / 2, data[20:, model_idx, 0].reshape((-1)), label=models[model_idx], color=colors[model_idx], width=1.0, edgecolor='#606060')
plt.xticks(np.sort(np.concatenate((1.25 * np.arange(num_models) * num_formats / 2, 19.0 + 1.25 * np.arange(num_models) * num_formats / 2))), labels=["I", "XX", "I", "XX", "I", "XX", "I", "XX"])
plt.ylabel("Construction Time (seconds)")
plt.legend()
plt.savefig("construction-512-times.png", bbox_inches='tight', pad_inches=0.02)

print(data[:, :, 1])
print(np.mean(data[:, :, 1] / 1024 / 1024, axis=0))
print(np.min(data[:, :, 1] / 1024 / 1024, axis=0))
print(np.max(data[:, :, 1] / 1024 / 1024, axis=0))
print(np.std(data[:, :, 1] / 1024 / 1024, axis=0))
rdata = data[:, :, 1].reshape((-1)) / 1024 / 1024
rlabels = models + [None, None, None, None] * (data.shape[0] - 1)
rcolors = colors * data.shape[0]
ridx = np.argsort(rdata).astype(int)
rdata = np.array([rdata[idx] for idx in ridx])
rlabels = np.array([rlabels[idx] for idx in ridx])
rcolors = np.array([rcolors[idx] for idx in ridx])
plt.figure(figsize=(7,3.5))
plt.bar(np.arange(160), rdata, color=rcolors, label=rlabels, width=1.0, edgecolor='#606060')
plt.axhline(y=4.0, color="#606060", linestyle="dashed")
plt.xticks([])
plt.ylabel("Maximum Memory Used (GiB)")
handles, labels = plt.gca().get_legend_handles_labels()
order = [3, 2, 1, 0]
plt.legend([handles[idx] for idx in order],[labels[idx] for idx in order])
plt.savefig("construction-max-memory.png", bbox_inches='tight', pad_inches=0.02)
