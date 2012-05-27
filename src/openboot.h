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

#define MAX_PARTITIONS	256
#define READ_SIZE	1024
#define PARTNAME_SIZE	128
#define DM_TARGET	"linear"

static char options[] = "vhlm:t:";

enum { LIST, MAP, DELETE };

struct partition {
	unsigned long long start;
	unsigned long long size;
	int number;
	int container;
	int major;
	int minor;
};

typedef int (ptreader)(int fd, struct partition all, struct partition *sp, int ns);
extern ptreader read_dos_pt;

char *getblock(int fd, unsigned int secnr);
