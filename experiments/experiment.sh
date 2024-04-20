#!/usr/bin/env bash
set -exo pipefail

MAX_STORAGE_BUFFER_RANGE=4294967295

formats=(
	"DF(16, 16, 16, 6) DF(8, 8, 8, 6) SVDAG(4)"
	"DF(16, 16, 16, 6) Raw(8, 8, 8) SVDAG(4)"
	"Raw(16, 16, 16) Raw(8, 8, 8) SVDAG(4)"
	"Raw(16, 16, 16) SVO(3) SVDAG(4)"
	"Raw(16, 16, 16) Raw(16, 16, 16) Raw(8, 8, 8)"
	"DF(16, 16, 16, 6) SVO(7)"
	"DF(16, 16, 16, 6) SVDAG(7)"
	"DF(64, 64, 64, 6) SVO(5)"
	"DF(64, 64, 64, 6) SVDAG(5)"
	"Raw(16, 16, 16) SVO(7)"
	"Raw(16, 16, 16) SVDAG(7)"
	"Raw(64, 64, 64) SVO(5)"
	"Raw(64, 64, 64) SVDAG(5)"
	"Raw(8, 8, 8) SVDAG(8)"
	"Raw(256, 256, 256) SVDAG(3)"
	"SVO(7) SVDAG(4)"
	"SVO(5) SVDAG(6)"
	"SVO(3) SVDAG(8)"
	"SVO(11)"
	"SVDAG(11)"

	"DF(8, 8, 8, 6) DF(8, 8, 8, 6) SVDAG(3)"
	"DF(8, 8, 8, 6) Raw(8, 8, 8) SVDAG(3)"
	"Raw(8, 8, 8) Raw(8, 8, 8) SVDAG(3)"
	"Raw(8, 8, 8) Raw(8, 8, 8) Raw(8, 8, 8)"
	"Raw(16, 16, 16) Raw(2, 2, 2) Raw(16, 16, 16)"
	"DF(16, 16, 16, 6) SVO(5)"
	"DF(16, 16, 16, 6) SVDAG(5)"
	"Raw(16, 16, 16) SVO(5)"
	"Raw(16, 16, 16) SVDAG(5)"
	"Raw(4, 4, 4) SVDAG(7)"
	"Raw(128, 128, 128) SVDAG(2)"
	"SVO(5) Raw(16, 16, 16)"
	"SVDAG(5) Raw(16, 16, 16)"
	"DF(32, 32, 32, 6) DF(16, 16, 16, 6)"
	"DF(32, 32, 32, 6) Raw(16, 16, 16)"
	"Raw(32, 32, 32) Raw(16, 16, 16)"
	"Raw(512, 512, 512)"
	"DF(512, 512, 512, 6)"
	"SVO(9)"
	"SVDAG(9)"
	)

flags=(
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"

	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	"-whole-level-dedup -df-packing"
	)

models=(
	"san-miguel-low-poly"
	"hairball"
	"buddha"
	"sponza"
	)

camera_positions=(
	"59.0 89.0 23.5 36.5 239.5"
	"41.0 50.0 113.0 0.0 173.5"
	"3.5 37.5 -33.5 12.0 375.0"
	"26.0 69.5 26.0 43.0 103.5"
	)

construction_opt_formats=(
	"SVDAG(11)"
	"Raw(256, 256, 256) SVDAG(3)"
	"DF(64, 64, 64, 6) SVDAG(5)"
	"DF(16, 16, 16, 6) SVDAG(7)"
	"SVDAG(9)"
	"Raw(128, 128, 128) SVDAG(2)"
	"DF(32, 32, 32, 6) Raw(16, 16, 16)"
	"DF(16, 16, 16, 6) SVDAG(5)"
	)

intersection_opt_formats=(
	"SVO(11)"
	"SVDAG(11)"
	"SVO(3) SVDAG(8)"
	"SVO(7) SVDAG(4)"
	"SVO(9)"
	"SVDAG(9)"
	"Raw(16, 16, 16) SVDAG(5)"
	"Raw(8, 8, 8) Raw(8, 8, 8) SVDAG(3)"
	)

construction_opt_flags=(
	""
	"-whole-level-dedup"
	"-df-packing"
	"-whole-level-dedup -df-packing -restart-sv"
	)

intersection_opt_flags=(
	""
	"-restart-sv"
	"-unroll-sv"
	)

if [ "$1" = "download" ]; then
	mkdir -p obj
	wget -nc -O obj/sponza.zip "https://casual-effects.com/g3d/data10/research/model/dabrovic_sponza/sponza.zip" &
	wget -nc -O obj/hairball.zip "https://casual-effects.com/g3d/data10/research/model/hairball/hairball.zip" &
	wget -nc -O obj/san-miguel-low-poly.zip "https://casual-effects.com/g3d/data10/research/model/San_Miguel/San_Miguel.zip" &
	wget -nc -O obj/buddha.zip "https://casual-effects.com/g3d/data10/research/model/buddha/buddha.zip" &
	wait
	
	rm -rf obj/sponza/
	rm -rf obj/hairball/
	rm -rf obj/san-miguel-low-poly/
	rm -rf obj/buddha/
	unzip obj/sponza.zip -d obj/sponza/ &
	unzip obj/hairball.zip -d obj/hairball/ &
	unzip obj/san-miguel-low-poly.zip -d obj/san-miguel-low-poly/ &
	unzip obj/buddha.zip -d obj/buddha/ &
	wait
	
	# For a "fixed" set of models, there sure are a lot of errors in the material files...
	ls obj/sponza/*.mtl | while read m; do tr '\\' '/' <$m >tmpfile && mv tmpfile $m; done
	ls obj/sponza/*.mtl | while read m; do sed 's/SP_LUK.JPG/sp_luk.JPG/' <$m >tmpfile && mv tmpfile $m; done
	ls obj/sponza/*.mtl | while read m; do sed 's/00_SKAP.JPG/00_skap.JPG/' <$m >tmpfile && mv tmpfile $m; done
	ls obj/sponza/*.mtl | while read m; do sed 's/01_S_BA.JPG/01_S_ba.JPG/' <$m >tmpfile && mv tmpfile $m; done
	ls obj/sponza/*.mtl | while read m; do sed 's/01_ST_KP.JPG/01_St_kp.JPG/' <$m >tmpfile && mv tmpfile $m; done
	ls obj/sponza/*.mtl | while read m; do sed 's/X01_ST.JPG/x01_st.JPG/' <$m >tmpfile && mv tmpfile $m; done
	ls obj/sponza/*.mtl | while read m; do sed 's/RELJEF.JPG/reljef.JPG/' <$m >tmpfile && mv tmpfile $m; done
	ls obj/sponza/*.mtl | while read m; do sed 's/PROZOR1.JPG/prozor1.JPG/' <$m >tmpfile && mv tmpfile $m; done
	ls obj/sponza/*.mtl | while read m; do sed 's/VRATA_KO.JPG/vrata_ko.JPG/' <$m >tmpfile && mv tmpfile $m; done
	ls obj/sponza/*.mtl | while read m; do sed 's/VRATA_KR.JPG/vrata_kr.JPG/' <$m >tmpfile && mv tmpfile $m; done
	ls obj/san-miguel-low-poly/*.mtl | while read m; do tr '\\' '/' <$m >tmpfile && mv tmpfile $m; done
elif [ "$1" = "compile" ]; then
	cd ../build
	rm -f cons_cmake ints_cmake decl_conv map_conv
	touch cons_cmake
	touch ints_cmake
	touch decl_conv
	touch map_conv
	for index in "${!formats[@]}"
	do
		format="${formats[$index]}"
		flag="${flags[$index]}"
		iden=`drivers/compile "$format" $flag`
		cp "$iden"_construct.cpp ../voxels/
		cp "$iden"_intersect.glsl ../shaders/
		echo "	$iden"_construct.cpp >> cons_cmake
		echo "	$iden"_intersect.glsl >> ints_cmake
		echo "void $iden"_construct"(Voxelizer &voxelizer, std::ofstream &buffer);" >> decl_conv
		echo "    {\"$format\", $iden"_construct"}," >> map_conv
	done
	cd ../experiments
elif [ "$1" = "convert" ]; then
	cd ../build
	for format in "${formats[@]}"
	do
		iden=`drivers/compile "$format" -just-get-iden`
		for model in "${models[@]}"
		do
			{ /usr/bin/time -v drivers/convert_model ../experiments/obj/$model/$model.obj 1 "$format"; } &> ../experiments/obj/$model/$model.$iden.log &
		done
		wait
	done
	cd ../experiments
elif [ "$1" = "run" ]; then
	cd ../build
	OUT="${2:-measurements}"
	rm -f ../experiments/$OUT
	touch ../experiments/$OUT
	for format in "${formats[@]}"
	do
		iden=`drivers/compile "$format" -just-get-iden`
		for index in "${!models[@]}"
		do
			model="${models[$index]}"
			camera_position="${camera_positions[$index]}"
			FILE="../experiments/obj/$model/$model.$iden"
			FILESIZE=`stat -c%s $FILE`
			echo "$format" >> ../experiments/$OUT
			echo "$model" >> ../experiments/$OUT
			echo "$FILESIZE" >> ../experiments/$OUT
			if (( FILESIZE >= MAX_STORAGE_BUFFER_RANGE )); then
				echo "Not viewing $FILE, since it's too large to fit in a storage buffer." >> ../experiments/$OUT
			else
				drivers/model_viewer $FILE "$format" $camera_position | grep "Final" >> ../experiments/$OUT
			fi
		done
	done
	cd ../experiments
elif [ "$1" = "construction_opt_run" ]; then
	cd ../build
	OUT="${2:-construction-opt-measurements}"
	rm -f ../experiments/$OUT
	touch ../experiments/$OUT
	for flag in "${construction_opt_flags[@]}"
	do
		for format in "${construction_opt_formats[@]}"
		do
			iden=`drivers/compile "$format" $flag`
			cp "$iden"_construct.cpp ../voxels/
			cp "$iden"_intersect.glsl ../shaders/
		done
		make -j$(nproc)
		for model in "${models[@]}"
		do
			for format in "${construction_opt_formats[@]}"
			do
				iden=`drivers/compile "$format" -just-get-iden`
				{ /usr/bin/time -v drivers/convert_model ../experiments/obj/$model/$model.obj 1 "$format"; } &> ../experiments/obj/$model/$model.$iden.log &
			done
			wait
		done
		for format in "${construction_opt_formats[@]}"
		do
			iden=`drivers/compile "$format" -just-get-iden`
			for model in "${models[@]}"
			do
				FILE="../experiments/obj/$model/$model.$iden"
				FILESIZE=`stat -c%s $FILE`
				echo "$format $flag" >> ../experiments/$OUT
				echo "$model" >> ../experiments/$OUT
				echo "$FILESIZE" >> ../experiments/$OUT
			done
		done
	done
	cd ../experiments
elif [ "$1" = "intersection_opt_run" ]; then
	cd ../build
	OUT="${2:-intersection-opt-measurements}"
	rm -f ../experiments/$OUT
	touch ../experiments/$OUT
	for flag in "${intersection_opt_flags[@]}"
	do
		for format in "${intersection_opt_formats[@]}"
		do
			iden=`drivers/compile "$format" $flag`
			cp "$iden"_intersect.glsl ../shaders/
		done
		make -j$(nproc)
		for format in "${intersection_opt_formats[@]}"
		do
			iden=`drivers/compile "$format" -just-get-iden`
			for index in "${!models[@]}"
			do
				model="${models[$index]}"
				camera_position="${camera_positions[$index]}"
				FILE="../experiments/obj/$model/$model.$iden"
				FILESIZE=`stat -c%s $FILE`
				echo "$format $flag" >> ../experiments/$OUT
				echo "$model" >> ../experiments/$OUT
				echo "$FILESIZE" >> ../experiments/$OUT
				if (( FILESIZE >= MAX_STORAGE_BUFFER_RANGE )); then
					echo "Not viewing $FILE, since it's too large to fit in a storage buffer." >> ../experiments/$OUT
				else
					drivers/model_viewer $FILE "$format" $camera_position | grep "Final" >> ../experiments/$OUT
				fi
			done
		done
	done
	cd ../experiments
else
	echo "Unrecognized experiment phase."
	exit 1
fi
