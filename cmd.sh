#!/bin/bash

for seed in 0 1 2 3 4 5 6 7 8 9
do
    for file in \
	\
	bbc \
	ad
    do
	echo "#!/bin/bash"								>> cmd-$file-$seed.sh
	echo										>> cmd-$file-$seed.sh
	echo "#SBATCH --account=def-vanbeek"						>> cmd-$file-$seed.sh
	echo "#SBATCH --time=01:05:00 # hh:mm:ss"					>> cmd-$file-$seed.sh
	echo "#"									>> cmd-$file-$seed.sh
	echo "#SBATCH --ntasks=1"							>> cmd-$file-$seed.sh
	echo "#SBATCH --mem-per-cpu=6144 # 6 x 1024 = 6GB"				>> cmd-$file-$seed.sh
	echo "#SBATCH --output=output.txt"						>> cmd-$file-$seed.sh
	echo "#"									>> cmd-$file-$seed.sh
	echo "/home/vanbeek/Bayesian/MINOBS/minobs \\"					>> cmd-$file-$seed.sh
	echo "        /home/vanbeek/Bayesian/Data/Scores/$file.txt 3600 $seed \\"	>> cmd-$file-$seed.sh
	echo "        /home/vanbeek/Bayesian/Experiments_MINOBS/$file-$seed.out \\"	>> cmd-$file-$seed.sh
	echo "        > /home/vanbeek/Bayesian/Experiments_MINOBS/$file-$seed.txt"	>> cmd-$file-$seed.sh
    done
done

