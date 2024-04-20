#!/usr/bin/env python3

from scipy.stats import gmean

import matplotlib.pyplot as plt
import numpy as np
import sys

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

data = np.zeros((num_formats, num_models))
for datapoint in datapoints:
    format_idx = formats.index(datapoint[0])
    model_idx = models.index(datapoint[1])
    data[format_idx, model_idx] = datapoint[2]

models = list(map(lambda x: x.replace("-low-poly", ""), models))
print(num_models, num_formats)
print(models)
print(formats)
print(data.shape)
print(data)

baseline = data[0:8, :]
whole_level_dedup = data[8:16, :]
df_packing = data[16:24, :]
both = data[24:32, :]
relative_whole_level_dedup = baseline / whole_level_dedup
relative_df_packing = baseline / df_packing
relative_both = baseline / both
relative_whole_level_dedup = np.concatenate((relative_whole_level_dedup, gmean(relative_whole_level_dedup, axis=1).reshape(-1, 1)), axis=1)
relative_df_packing = np.concatenate((relative_df_packing, gmean(relative_df_packing, axis=1).reshape(-1, 1)), axis=1)
relative_both = np.concatenate((relative_both, gmean(relative_both, axis=1).reshape(-1, 1)), axis=1)

plt.figure(figsize=(7, 3.5))
for format_idx in range(8):
    plt.bar(np.arange(num_models + 1) * 0.8 + format_idx * 5.0, relative_whole_level_dedup[format_idx], color=["#FF7F7F", "#7FFF7F", "#7F7FFF", "#FF7FFF", "#7F7F7F"], label=models + ["Geomean"] if format_idx == 0 else None)

plt.axhline(y=1.0, color="#606060", linestyle="dashed")
plt.xticks(np.arange(8) * 5.0 + 1.5, labels=formats[0:8], rotation=30, ha='right', rotation_mode='anchor')
plt.ylabel("Normalized Decrease in Model Size (bytes)", y=0.4)
plt.legend()
plt.savefig("construction-whole-level-dedup.png", bbox_inches='tight', pad_inches=0.02)

print(relative_whole_level_dedup)
