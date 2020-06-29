#!/bin/bash

writtenLinks=()

pageContent () {
	k=$(( ( RANDOM % ($1-2000) )  + 1 ))
	m=$(( ( RANDOM % 1000 ) + 1000 ))
	##############links to write in file##############
	f=$(("$6" / 2))
	let f++
	q=$(("$5" / 2))
	let q++
	declare -a links=("${!2}")
	declare -a lflag=()
	linkcounter=${#links[@]}
	counter=0
	while [[ "$counter" -lt "$linkcounter" ]]
	do
		str="${links[$counter]}"
		lflag[$counter]=${str:5}
		lflag[$counter]="${lflag[$counter]%%/*}"
		let counter++
	done
	linksToBeWritter=()
	flag=0
	inflag=0
	exflag=0
	excount=$q
	incount=$f
	count=0
	while [[ $flag -lt 2 ]]
	do
		ran=$(( ( RANDOM % $linkcounter ) ))
		if [[ "${links[$ran]}" != *"$4" ]] || [[ "$6" -eq $f ]] || [[ "$6" -eq 1 ]] #if f!=p && p!=1->doesn't point to itself
		then
			if [[ "${lflag[$ran]}" -eq "$8" ]]	#internal link
			then
				if [[ $inflag -eq 0 ]] && [[ -n "${links[$ran]}" ]]
				then
					linksToBeWritter[$count]="${links[$ran]}"
					unset 'links[$ran]'
					let count++
					let incount--
					if [[ $incount -eq 0 ]]
					then
						inflag=1
						let flag++
					fi
				fi
			else #external link
				if [[ $exflag -eq 0 ]] && [[ -n "${links[$ran]}" ]]
				then
					linksToBeWritter[$count]="${links[$ran]}"
					unset 'links[$ran]'
					let count++
					let excount--
					if [[ $excount -eq 0 ]]
					then
						exflag=1
						let flag++
					fi
				fi
			fi
		fi
	done
	##################################################
	mypage=$3
	mypage="${mypage}/$4"
	echo "    Creating page $mypage with $m lines starting at line $k ..."
	echo "<!DOCTYPE html>" >> $mypage
	echo "<html>" >> $mypage
	echo "    <body>" >> $mypage
	##################################################
	counter=0
	lim=$(($f+$q))
	while [[ "$counter" -lt "$lim" ]]
	do
		temp=$(("$m" / ($f+$q) ))
		templim=$(($k + $temp))
		sed -n "$k","$templim"p "$7" >> $mypage
		echo "    Adding link to ${linksToBeWritter[$counter]}"
		echo "<a href="../${linksToBeWritter[$counter]}">${linksToBeWritter[$counter]}_text</a>" >> $mypage
		##################################################
		if [[ ! " ${writtenLinks[@]} " =~ " ${linksToBeWritter[$counter]} " ]]
    	then
    		writtenLinks+=("${linksToBeWritter[$counter]}")
		fi
		##################################################
		k=$(($k+$temp))
		let k++
		let counter++
	done
	##################################################
	echo "    </body>" >> $mypage
	echo -n "</html>" >> $mypage
}

##################Initial checks##################
if [ ! -e $2 ]
then
	echo -n "file "
	echo -n "$2"
	echo " doesn't exists"
	exit
fi
if [ ! -d $1 ]
then
	echo -n "directory "
	echo -n "$1"
	echo " doesn't exists"
	exit
fi
##################################################
re='^[0-9]+$'
if ! [[ $3 =~ $re ]]
then
	echo "Number of sites is not an integer"
	exit
fi
if ! [[ $4 =~ $re ]]
then
	echo "Number of pages is not an integer"
	exit
fi
##################################################
numOfLines=$(wc -l < $2)
if [[ numOfLines -lt 10000 ]]
then
	echo "File must have at least 10000 lines"
	exit
fi
#################purge directory##################
if [ -n "$(ls -A "$1")" ]	#directory is not empty--delete everything inside it
then
	echo "Warning: directory is full, purging ..."
	rm -R -- "$1"/*/
fi
##############create sites and pages##############
nameHolder=()	# this array has the names of the pages
linkArray=()	# this array has the links of the pages
count=0
i=0
while [[ "$i" -lt "$3" ]]
do
	j=0
	while [[ "$j" -lt "$4" ]]
	do
		nameHolder[$count]=page"$i"_"$RANDOM".html
		linkArray[$count]=/site"$i"/"${nameHolder[$count]}"
		let j++
		let count++
	done
	let i++
done

countr=0
i=0
while [[ "$i" -lt "$3" ]]
do
	mkdir "$1"/site"$i"
	echo "Creating web site $i ..."
	j=0
	while [[ "$j" -lt "$4" ]]
	do
		path=./"$1"/site"$i"
		find $path -type d -exec touch {}/"${nameHolder[$countr]}" \;
		pageContent "$numOfLines" linkArray[@] "$path" "${nameHolder[$countr]}" "$3" "$4" "$2" "$i"
		let j++
		let countr++
	done
	let i++
done
##################################################
if [[ ${#nameHolder[@]} -eq ${#writtenLinks[@]} ]]
then
	echo "All pages have at least one incoming link"
fi
echo "Done."