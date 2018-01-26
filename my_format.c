/********************************************************************************************************/
/* Desc: MMN 13 for OS course at OpenU. It's a program to format FAT12 images.
 * Implements minimal quick format functionality: fills FAT and Root Dir area with zeroes, keeps all else.
 * Created By: OS course, Open University
 * Author: Roman Smirnov
 */
/********************************************************************************************************/


/*** imports ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "fat12.h"

/*** global variables and constants ***/
static const int DIRENT_BYTES_SIZE = 32; /* size of a directory entry in bytes */
static const int ONE_BYTE = 1;
int fid; /* global variable set by the open() function */


/*** Function declarations ***/
/* writes a sector to file */
ssize_t fd_write(uint sector_number, char *buffer, uint sector_size);
/* returns the file size */
off_t get_file_size(const char *filename);
/* performs a quick format on file with current fid */
int quick_format(boot_record_t boot);
/* creates a new new floppy image in file with current fid*/
int create_new_image(boot_record_t boot);
/* prints out the boot record of floppy image */
void print_boot_record(boot_record_t boot);
/* returns a boot struct set with some minimal default values */
boot_record_t get_default_boot();

/*** main program ***/
int main(int argc, char *argv[]) {

    /* check cli args */
    if (argc != 2) {
        printf("Usage: %s <floppy_image>\n", argv[0]);
        exit(1);
    }

    /* open or create file */
    if ((fid = open(argv[1], O_RDWR | O_CREAT, 0644)) < 0) {
        perror("Error: ");
        exit(1);
    }

    boot_record_t boot;
    /* check the file size of open file */
    off_t file_size = get_file_size(argv[1]);
    if (file_size == 0) {
        /* the file is newly created (did not exist before) - create a new floppy image with a default boot sector */
        /* get a boot struct with default values */
        boot = get_default_boot();
        create_new_image(boot);
    } else if (file_size > 0) {
        /* it's an existing file */
        /* read the boot sector of the existing image */
        if (read(fid, &boot, sizeof(boot)) < 1) {
            perror("Error: ");
            exit(1);
        }
        /* fill fat table and root dir with zeroes */
        if (quick_format(boot) != 0) {
            exit(1);
        }
    } else {
        /* invalid file size returned   */
        fprintf(stderr, "fatal error: returned invalid file size\n");
        exit(1);
    }

    /* print the values in the boot record */
    print_boot_record(boot);

    /* clean up */
    close(fid);
    return 0;
}


/*** helper functions ***/
off_t get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        return 0;
    }
    return st.st_size;
}

ssize_t fd_write(uint sector_number, char *buffer, uint sector_size) {
    off_t dest;
    ssize_t len;
    dest = lseek(fid, sector_number * sector_size, SEEK_SET);

    if (dest == -1) {
        /* lseek returned with an error */
        perror("Error: ");
        exit(1);
    } else if (dest != (long)sector_number * (long)sector_size){
        /* incorrect offset returned  */
        fprintf(stderr, "fatal error: lseek returned incorrect offset\n");
        exit(1);
    }
    len = write(fid, buffer, sector_size);
    if (len == -1) {
        /* write returned with an error */
        perror("Error: ");
        exit(1);
    } else if (len != DEFAULT_SECTOR_SIZE) {
        /* not all data written */
        fprintf(stderr, "fatal error: write didn't manage to write all required bytes\n");
        exit(1);
    }
    return len;
}

boot_record_t get_default_boot() {
    boot_record_t boot;
    *boot.bootjmp = DEFAULT_BOOT_JMP;
    strncpy((char *)boot.oem_id, DEFAULT_OEM_ID, OEM_ID_BYTES);
    boot.sector_size = DEFAULT_SECTOR_SIZE;
    boot.sectors_per_cluster = DEFAULT_SECTORS_PER_CLUSTER;
    boot.reserved_sector_count = DEFAULT_RESERVED_SECTORS_COUNT;
    boot.number_of_fats = DEFAULT_NUMBER_OF_FATS;
    boot.number_of_dirents = DEFAULT_NUMBER_OF_DIRENTS;
    boot.sector_count = DEFAULT_SECTOR_COUNT;
    boot.media_type = DEFAULT_MEDIA_TYPE;
    boot.fat_size_sectors = DEFAULT_FAT_SIZE_SECTORS;
    boot.sectors_per_track = DEFAULT_SECTORS_PER_TRACK;
    boot.nheads = DEFAULT_NHEADS;
    boot.sectors_hidden = DEFAULT_SECTORS_HIDDEN;
    boot.sector_count_large = DEFAULT_SECTOR_COUNT_LARGE;
    return boot;
}

void print_boot_record(boot_record_t boot) {
    printf("sector_size: %d\n", boot.sector_size);
    printf("sectors_per_cluster: %d\n", boot.sectors_per_cluster);
    printf("number_of_fats: %d\n", boot.number_of_fats);
    printf("fat_size_sectors: %d\n", boot.fat_size_sectors);
    printf("number_of_dirents: %d\n", boot.number_of_dirents);
    printf("sector_count: %d\n", boot.sector_count);
}

int create_new_image(boot_record_t boot) {
    // extend the file size to 2880 sectors
    if (ftruncate(fid, DEFAULT_SECTOR_SIZE * DEFAULT_SECTOR_COUNT) == -1) {
        /* returned with an error */
        perror("Error: ");
        exit(1);
    }
    // create a whole sector containing the boot record
    char *sector_buf = calloc(boot.sector_size, sizeof(char));
    memcpy(sector_buf, &boot, sizeof(boot));
    // write boot sector to file
    fd_write(BOOT_SECTOR_NUM, sector_buf, DEFAULT_SECTOR_SIZE);
    // free resources and return
    free(sector_buf);
    return 0;
}

/* Quick Format fills the FAT and root dir areas with zeroes, leave everything else untouched.
 * Pros: it's fast.
 * Cons: doesn't check for corrupted FAT indices.
 * Depends: doesn't delete the data - often data can be at least partially recovered.
 *
 */
int quick_format(boot_record_t boot) {

    /* calculate # of sectors in FAT and root dir */
    uint16_t root_dir_sectors = (boot.number_of_dirents * (uint8_t) DIRENT_BYTES_SIZE) / boot.sector_size;
    uint16_t fat_sectors = boot.number_of_fats * boot.fat_size_sectors;

    /* initialize a zero filled mem space of size same as sector */
    char *zero_sector_buf = calloc(boot.sector_size, ONE_BYTE);

    /* fill the FAT and root dir disk areas with zeroes, don't touch boot portion */
    for (uint i = boot.reserved_sector_count; i <= fat_sectors + root_dir_sectors; ++i) {
        fd_write(i, zero_sector_buf, boot.sector_size);
    }
    /* clean up */
    free(zero_sector_buf);
    return 0;
}
