/* Copyright (C) 2011 Lucas Alvares Gomes <lucasagomes@gmail.com>.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>. */

/*
 * based on kpartx.c
 *
 * Copyrights of the original file applies
 * Copyright (c) 2004, 2005 Christophe Varoqui
 * Copyright (c) 2005 Kiyoshi Ueda
 * Copyright (c) 2005 Lars Soltau
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <libdevmapper.h>

#include "openboot.h"
#include "lopart.h"
#include "devmapper.h"

void usage();
int get_partitions(const char *, struct partition *);
void list_partitions(const char *);
int map_parition(const char *, const int, const char *);

int main(int argc, const char *argv[]){
	int arg, part_number, opt = -1, verbose = 0;
	char *disk_file = NULL;
	char *file_type = NULL;

	while ((arg = getopt(argc, argv, options)) != -1)
		switch(arg){

			case 'l':
				opt = LIST;
				disk_file = strdup(argv[argc-1]);
				break;
			case 'm':
				opt = MAP;
				disk_file = strdup(argv[argc-1]);
				part_number = atoi(optarg);
				break;
			case 't':
				file_type = strdup(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'h':
			default:
				goto help_n_exit;
		}

	/* no valid arguments */
	if (opt == -1)
		goto help_n_exit;

	switch(opt) {

		case LIST:
			list_partitions(disk_file);
			break;
		case MAP:
			map_parition(disk_file, part_number, file_type);
			break;
		case DELETE:
			break;
	}

	return 0;

	help_n_exit:
		usage();
		exit(1);

}

void usage(){
	printf("Usage:\n");
	printf("\topenboot [option(s)] <disk file>\n");
	printf("\topenboot -l <disk file>\t\tlist partition table\n");
	printf("\topenboot -m 1 <disk file>\tadd partition 1 to devmappings\n");
	printf("\topenboot -t vdi -m 1 <disk file>\tadd partition 1 to devmappings\n");
}

int get_partitions(const char *disk_file, struct partition *parts){

	int n, fd;
	struct partition all;
	memset(&all, 0, sizeof(all));

	fd = open(disk_file, O_RDONLY);
	if (fd == -1){
		perror(disk_file);
		exit(1);
	}
	n = read_dos_pt(fd, all, parts, sizeof(struct partition) * MAX_PARTITIONS);
	close(fd);

	return n;
}

void list_partitions(const char *disk_file){
	int n, i=0;
	struct partition parts[MAX_PARTITIONS];

	n = get_partitions(disk_file, parts);

	for (i; i < n; i++)
		printf("%d %llu\n", parts[i].number, parts[i].start);
}

int map_parition(const char *disk_file, const int part_number, const char *type){
	char *loopdev = NULL;
	int n, op, i, loopro = 0;
	char partname[PARTNAME_SIZE], params[PARTNAME_SIZE + 16];
	struct partition parts[MAX_PARTITIONS];

	n = get_partitions(disk_file, parts);

	for (i = 0; i < n; i++){
		if (parts[i].number == part_number){

			loopdev = find_loop_by_file(disk_file);
			if (!loopdev) {
				loopdev = find_unused_loop_device();

				if (set_loop(loopdev, disk_file, 0, &loopro)){
					fprintf(stderr, "can't set up loop\n");
					exit (1);
				}
			
			}

			sprintf(partname, "%s%d", "uhuhu", part_number);

			sprintf(params, "%s %llu", loopdev, parts[i].start);

			if (dm_prereq(DM_TARGET, 0, 0, 0)){
				fprintf(stderr, "device mapper prerequisites not met\n"); 
				exit(1);
			}

			op = (dm_map_present(partname) ?
				DM_DEVICE_RELOAD : DM_DEVICE_CREATE);

			dm_addmap(op, partname, DM_TARGET, params, parts[i].size);

			return 0;
		}

		fprintf(stderr, "partition not found;\n");
		return 1;
	}

	fprintf(stderr, "disk file has no partitions;\n");
	return 1;
}

char *getblock (int fd, unsigned int sector) {
	char *block;

	if (lseek64(fd,(off64_t) sector << 9, SEEK_SET) == -1)
		return NULL;

	block = (char *) malloc(READ_SIZE);

	if (read(fd, block, READ_SIZE) != READ_SIZE) {
		fprintf(stderr, "read error, sector %d\n", sector);
		block = NULL;
	}

	return block;
}
