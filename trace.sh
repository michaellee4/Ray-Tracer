#!/usr/bin/env bash

ROOT=~/CS378h/ray
PICS="${ROOT}/pics"
REF="${ROOT}/ray-solution"
RAY="${ROOT}/build/bin/ray"
BUILD="${ROOT}/build"
RAYARGS="-r 5"
EXT=png
r_flag=false
R_flag=false
c_flag=false
m_flag=false
file=''

if [ $1 = "--help" ] || [ $# = 0 ]
then
	echo "-r Run your ray tracer"
	echo "-R Run the reference ray tracer"
	echo "-c Run both and stitch the result"
	echo "-f Specify the relative file path"
	exit 0
fi

if [ $1 = "--g" ]
then
	${RAY}
	exit 0
fi

if [ $1 = "--raycheck" ]
then
	${ROOT}/raycheck.py --exec ${ROOT}/build/bin/ray --ref ${ROOT}/ray-solution
	exit 0
fi

if [ $1 = "--build" ]
then
	rm -rf ${ROOT}/build
	mkdir ${ROOT}/build
	(cd build; cmake .. -DCMAKE_BUILD_TYPE=Release; make -j8)
	exit 0
fi

# if pics doesn't exist, make
if [ ! -d $PICS ]
then
	mkdir $PICS
fi

if [ ! -f $RAY ]
then
	echo "${RAY} not found."
	exit 1
fi

if [ ! -f $REF ]
then
	echo "${REF} not found."
	exit 1
fi

# if -R flag, run reference sol
while getopts 'rRcmf:' flag; do
  case "${flag}" in
    r) r_flag=true ;;
    R) R_flag=true ;;
	c) c_flag=true ;;
	m) m_flag=true ;;
    f) file="${OPTARG}" ;;
     esac
done

if [ $m_flag = true ]
then
	(cd ${BUILD}; make -j8)
	exit 0
fi

if [ file = '' ] || [ ! -f "${file}" ]
then 
	echo "Enter valid file path"
	exit 1	
fi

# save result as o_ or r_***_render.bmp in pics
filename=${file##*/}
filename=${filename%.*}
ourTrace=${PICS}/o_${filename}.${EXT}
refTrace=${PICS}/r_${filename}.${EXT}
traceDiff=${PICS}/${filename}_diff.${EXT}
if [ $r_flag = true ] || [ $c_flag = true ]
then
	${RAY} ${RAYARGS} ${file} $ourTrace
fi

if [ $R_flag = true ] || [ $c_flag = true ]
then
	${REF} ${RAYARGS} ${file} $refTrace
fi

# diff image
if [ $c_flag = true ]
then
	compare $refTrace $ourTrace -fuzz 5% -compose src $traceDiff
	montage $ourTrace $refTrace $traceDiff -tile 3x1 -geometry +0+0 ${PICS}/${filename}.${EXT}
	rm $ourTrace
	rm $refTrace
	rm $traceDiff
fi
