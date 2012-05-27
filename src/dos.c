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
 * strongly based on util-linux partx dos.c
 *
 * Copyrights of the original file apply 
 * Copyright (c) 2005 Bastian Blank 
 */

#include "openboot.h"
#include "byteorder.h"
#include <stdio.h>
#include <string.h>
#include "dos.h"

static int
is_extended(int type) {
	return (type == 5 || type == 0xf || type == 0x85);
}

static int
read_extended_partition(int fd, struct entry *ep, int en,
			struct partition *sp, int ns)
{
	struct entry p;
	unsigned long start, here, next, tmp_start;
	unsigned char *bp;
	int loopct = 0;
	int moretodo = 1;
	int i, n=0;

	next = start = le32_to_cpu(ep->start_sect);

	while (moretodo) {
		here = next;
		moretodo = 0;
		if (++loopct > 100)
			return n;

		bp = (unsigned char *)getblock(fd, here);
		if (bp == NULL)
			return n;

		if (bp[510] != 0x55 || bp[511] != 0xaa)
			return n;

		for (i=0; i<2; i++) {
			memcpy(&p, bp + 0x1be + i * sizeof (p), sizeof (p));
			if (is_extended(p.sys_type)) {
				if (p.nr_sects && !moretodo) {
					next = start + le32_to_cpu(p.start_sect);
					moretodo = 1;
				}
				continue;
			}
			if (n < ns) {
				tmp_start = here + le32_to_cpu(p.start_sect);
				if (tmp_start != next){
					sp[n].start = tmp_start;
					sp[n].size = le32_to_cpu(p.nr_sects);
					sp[n].number = n + 5;
					n++;
				}
			} else {
				fprintf(stderr,
				    "dos_extd_partition: too many slices\n");
				return n;
			}
			loopct = 0;
		}
	}
	return n;
}

static int
is_gpt(int type) {
	return (type == 0xEE);
}

int
read_dos_pt(int fd, struct partition all, struct partition *sp, int ns) {
	struct entry p;
	unsigned long offset = all.start;
	unsigned long long size;
	int i, n = 0;
	unsigned char *bp;

	bp = (unsigned char *)getblock(fd, offset);
	if (bp == NULL)
		return -1;

	if (bp[510] != 0x55 || bp[511] != 0xaa)
		return -1;

	for (i=0; i<4; i++) {
		memcpy(&p, bp + 0x1be + i * sizeof (p), sizeof (p));
		if (is_gpt(p.sys_type))
			return 0;
		if (n < ns) {
			size = le32_to_cpu(p.nr_sects);
			if (size > 0 && !is_extended(p.sys_type)){
				sp[n].start =  le32_to_cpu(p.start_sect);
				sp[n].size = size;
				sp[n].number = i + 1;
				n++;
			}
		} else {
			fprintf(stderr,
				"dos_partition: too many slices\n");
			break;
		}
		if (is_extended(p.sys_type)) {
			n += read_extended_partition(fd, &p, i, sp+n, ns-n);
		}
	}
	return n;
}
