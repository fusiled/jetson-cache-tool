/*
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 *  Copyright (C) 2000, Jan-Derk Bakker (J.D.Bakker@its.tudelft.nl)
 *  Copyright (C) 2008, BusyBox Team. -solar 4/26/08
 */
//config:config DEVMEM
//config:	bool "devmem"
//config:	default y
//config:	help
//config:	  devmem is a small program that reads and writes from physical
//config:	  memory using /dev/mem.

//applet:IF_DEVMEM(APPLET(devmem, BB_DIR_SBIN, BB_SUID_DROP))

//kbuild:lib-$(CONFIG_DEVMEM) += devmem.o

//usage:#define devmem_trivial_usage
//usage:	"ADDRESS [WIDTH [VALUE]]"
//usage:#define devmem_full_usage "\n\n"
//usage:       "Read/write from physical address\n"
//usage:     "\n	ADDRESS	Address to act upon"
//usage:     "\n	WIDTH	Width (8/16/...)"
//usage:     "\n	VALUE	Data to be written"

#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

  // Die if we can't open a file and return a fd.
int xopen(const char *pathname, int flags)
{
      int ret;
  
      ret = open(pathname, flags, 0666);
      if (ret < 0) {
          printf("can't open '%s'", pathname);
      }
      return ret;
  }



uint64_t devmem_read( const off_t base_target, const off_t offset_target)
{
	off_t target = base_target + offset_target;
	void *map_base, *virt_addr;
	uint64_t read_result;
	unsigned page_size, mapped_size, offset_in_page;
	int fd;
	unsigned width = 32;

	/* devmem ADDRESS [WIDTH [VALUE]] */
	fd = xopen("/dev/mem",  (O_RDONLY | O_SYNC));
	mapped_size = page_size = getpagesize();
	offset_in_page = (unsigned)target & (page_size - 1);
	if (offset_in_page + width > page_size) {
		/* This access spans pages.
		 * Must map two pages to make it possible: */
		mapped_size *= 2;
	}
	map_base = mmap(NULL,
			mapped_size,
			PROT_READ,
			MAP_SHARED,
			fd,
			target & ~(off_t)(page_size - 1));
	if (map_base == MAP_FAILED)
		printf("mmap failed");

//	printf("Memory mapped at address %p.\n", map_base);

	virt_addr = (char*)map_base + offset_in_page;

		switch (width) {
		case 8:
			read_result = *(volatile uint8_t*)virt_addr;
			break;
		case 16:
			read_result = *(volatile uint16_t*)virt_addr;
			break;
		case 32:
			read_result = *(volatile uint32_t*)virt_addr;
			break;
		case 64:
			read_result = *(volatile uint64_t*)virt_addr;
			break;
		default:
			printf("bad width error");
		}
//		printf("Value at address 0x%"OFF_FMT"X (%p): 0x%llX\n",
//			target, virt_addr,
//			(unsigned long long)read_result);
		/* Zero-padded output shows the width of access just done */
		//printf("0x%0*llX\n", (width >> 2), (unsigned long long)read_result);

	if (munmap(map_base, mapped_size) == -1)
			printf("munmap error");
	close(fd);

	return read_result;
}
