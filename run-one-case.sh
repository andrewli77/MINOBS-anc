scores=$1
file=$2
outputFile=$3

cmd="{ command time -f "%U,%S" timeout 12h ./search $scores $file 10 0 out.txt; } 2>> $outputFile "
echo "$cmd"
eval "$cmd"