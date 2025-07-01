#!/bin/bash
set -e

usage()
{
    echo "Usage: $0 [OPTION]..."
	echo ""
    echo "Optional arguments:"
    echo "  -h, --help     Give this help list"
	echo "  -v, --volume   Bind mount a volume into the container (default = .:/workspace)"
	echo "  -c, --command  Run a command in the container"
    exit 1
}

# check argument
if [ $(($# % 2)) -eq 1 ]; then
	usage
fi

declare -a volumns=()
command=""
while :; do
	case $1 in
		-h|--help) shift; usage
		;;
		-v|--volume) shift; volumns+=("$1") # volume="${volume} ${1}"
		;;
		-c|--command) shift; command="$1"
		;;
		"") break
		;;
		*) echo "Unknown argument: $1"; usage
		;;
	esac
	shift
done

# assign default value if volume is not given
if [ ${#volumns[@]} -eq 0 ]; then
    volumns=(".:/workspace")
fi

# use realpath to convert relative path to absolute path
for i in "${!volumns[@]}"; do
    IFS=':' read -ra parts <<< "${volumns[$i]}"
    path="${parts[0]}"
    absolute_path=$(realpath "$path")
    volumns[$i]="$absolute_path:${parts[1]}"
done

# create a temporary directory as the container root
temp_root=$(mktemp -d)

initialize() {
	cd $temp_root

	mount_dir() {
		mkdir -p "$temp_root/$1"
		mount --rbind "$1" "$temp_root/$1" 2>/dev/null
		# remove the directory when the mount fails
		if [ $? -ne 0 ]; then
			rm -rf "$temp_root/$1"
		fi
	}

	# create and mount the system directories to 'temp_root' besides /home
	ls / | while read dir; do
		if [[ "$dir" == "home" || "$dir" == "u" || "$dir" == "net" ]]; then
			continue
		fi
		mount_dir "/$dir"
	done

	IFS=',' read -ra volumes_array <<< "$volumns"
	for (( i=${#volumes_array[@]}-1 ; i>=0 ; i-- )) ; do
		IFS=':' read -ra parts <<< "${volumes_array[$i]}"
		source_volume="${parts[0]}"
		container_dir="${parts[1]}"
		mkdir -p "$temp_root/$container_dir"
		mount --bind "$source_volume" "$temp_root/$container_dir"
	done

	if [ -z "$command" ]; then
		command="/bin/bash"
	fi

	# change the root directory to 'temp_root'
	chroot $temp_root /bin/bash -i -c "
		cd $container_dir
		exec /bin/bash -i -c \"$command\""

	exit 0
}

volumes_str=$(IFS=','; echo "${volumns[*]}")

# make a copy of the mounting table to prevent modifications to the original table
unshare -Umr /bin/bash -i -c "
	export volumns=\"${volumes_str}\"
	export command=\"$command\"
	export temp_root=\"${temp_root}\"
	$(declare -f initialize); initialize
	exec /bin/bash"

# remove the temporary directory
rm -rf $temp_root
