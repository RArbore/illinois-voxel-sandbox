#!/usr/bin/env python3

from scipy.stats import gmean

import matplotlib.pyplot as plt
import numpy as np
import sys

def argsort(seq):
    return [x for x,y in sorted(enumerate(seq), key = lambda x: len(x[1]))]

with open(sys.argv[1]) as f:
    content = f.readlines()

datapoints = []
while len(content) > 0:
    datapoint = content[:3]
    content = content[3:]
    
    format = datapoint[0].rstrip()
    model = datapoint[1].rstrip()
    size = int(datapoint[2].rstrip())
    datapoints.append((format, model, size))

formats = []
for datapoint in datapoints[::4]:
    formats.append(datapoint[0])
num_formats = len(formats)

models = []
for datapoint in datapoints[:4]:
    models.append(datapoint[1])
num_models = len(models)

data = np.zeros((num_formats, num_models))
for idx, datapoint in enumerate(datapoints):
    format_idx = int(idx / 4);
    model_idx = int(idx % 4)
    data[format_idx, model_idx] = datapoint[2]

models = list(map(lambda x: x.replace("-low-poly", ""), models))
print(num_models, num_formats)
print(models)
print(formats)
print(data.shape)
print(data)

baseline1 = data[0:8, :]
whole_level_dedup = data[8:16, :]
baseline2 = data[16:24, :]
df_packing = data[24:32, :]
relative_whole_level_dedup = baseline1 / whole_level_dedup
relative_df_packing = baseline2 / df_packing
relative_whole_level_dedup = np.concatenate((relative_whole_level_dedup, gmean(relative_whole_level_dedup, axis=1).reshape(-1, 1)), axis=1)
relative_df_packing = np.concatenate((relative_df_packing, gmean(relative_df_packing, axis=1).reshape(-1, 1)), axis=1)

print(formats[0:8])
f_idx = argsort(formats[0:8])
print(f_idx)

plt.figure(figsize=(7, 3.5))
for idx, format_idx in enumerate(f_idx):
    plt.bar(np.arange(num_models + 1) * 0.8 + idx * 5.0, relative_whole_level_dedup[format_idx], color=["#FF7F7F", "#7FFF7F", "#7F7FFF", "#FF7FFF", "#7F7F7F"], label=models + ["Geomean"] if format_idx == 0 else None, edgecolor='#606060')

plt.axhline(y=1.0, color="#606060", linestyle="dashed")
plt.xticks(np.arange(8) * 5.0 + 1.5, labels=[formats[0:8][idx] for idx in f_idx], rotation=30, ha='right', rotation_mode='anchor')
plt.ylabel("Normalized Decrease in Model Size", y=0.4)
plt.legend()
plt.savefig("construction-whole-level-dedup.png", bbox_inches='tight', pad_inches=0.02)

print(relative_whole_level_dedup)

print(formats[16:24])
f_idx = argsort(formats[16:24])
print(f_idx)

plt.figure(figsize=(7, 3.5))
for idx, format_idx in enumerate(f_idx):
    plt.bar(np.arange(num_models + 1) * 0.8 + idx * 5.0, relative_df_packing[format_idx], color=["#FF7F7F", "#7FFF7F", "#7F7FFF", "#FF7FFF", "#7F7F7F"], label=models + ["Geomean"] if format_idx == 0 else None, edgecolor='#606060')

plt.axhline(y=1.0, color="#606060", linestyle="dashed")
plt.xticks(np.arange(8) * 5.0 + 1.5, labels=[formats[16:24][idx] for idx in f_idx], rotation=30, ha='right', rotation_mode='anchor')
plt.ylabel("Normalized Decrease in Model Size", y=0.4)
plt.legend()
plt.savefig("construction-df-packing.png", bbox_inches='tight', pad_inches=0.02)

print(relative_df_packing)
