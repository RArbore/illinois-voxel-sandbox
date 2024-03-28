# illinois-voxel-sandbox
A high performance voxel engine.

## Run experiments
Run the following commands, in order if starting from scratch. The beginning of each line is the directory to run each command from.

1. `build/: make -j`
2. `experiments/: ./experiment.sh download`
3. `experiments/: ./experiment.sh compile`
4. `build/: make -j`
5. `experiments/: ./experiment.sh convert`
6. `experiments/: ./experiment.sh run`

## Instructions for using hybrid formats

1. Build the entire project (`make -j`)
2. Run `drivers/compiler "[format]"`. `[format]`, in between the quotes, should be a space-separated list of formats, listed below:
  - `Raw(W, H, D)`: `W`, `H`, and `D` are the width, height, and depth of the raw grid
  - `DF(W, H, D, M)`: `W`, `H`, and `D` are the width, height, and depth of the distance field, `M` is the maximum distance stored (construction time scales cubically with `M`)
  - `SVO(L)`: `L` is the maximum number of levels in the SVO - the dimensions of the subvolume are 2^`L` x 2^`L` x 2^`L`
  - `SVDAG(L)`: `L` is the maximum number of levels in the SVDAG - the dimensions of the subvolume are 2^`L` x 2^`L` x 2^`L`
3. Step 2 will produce two files: `[format]_construct.cpp` and `[format]_intersect.glsl`, where `[format]` is the lower-case and underscored version of the format described in step 2 - add these files to `voxels/CMakeLists.txt` and `shaders/CMakeLists.txt`, respectively - copy these files into the `voxels/` and `shaders/` folders, respectively
4. Build the entire project again (`make -j`)
5. Edit `drivers/convert_model.cpp` - add a prototype for the constructing function at the top of the file, with the lower-case and underscored format name from step 3 - add that function as a value in the `format_to_conversion_function` unordered_map, with the format name from step 2 as the key
6. Run `drivers/convert_model [path to obj] 0.0 "[format]"`, where `[path to obj]` is a path to the obj model to voxelize, and `[format]` is the same format used in step 2
7. Run `drivers/model_viewer [path to voxelized model] "[format]"`, where `[path to voxelized model]` is the path to the voxelized model produced in step 6, and `[format]` is the same format used in step 2
