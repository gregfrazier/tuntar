#include <stdio.h>
#define TARWIDTH 512

typedef struct posix_header {
	char name[100];
	char a[24];     // unneeded, combined
	char size[12];
	char mtime[12];
	char chksum[8]; // note: this code doesn't checksum
	char typeflag;
	char post[355]; // unneeded, combined
} POSIX_HEADER;

unsigned int getsize(const char *in) {
	unsigned int size = 0, j, count = 1;
	for (j = 11; j > 0; size += ((in[j - 1] - '0') * count), count *= 8, --j);
	return size;
}

int readFile(char *filename) {
	FILE *TARFILE, *OUTPUT;
	char buffer[TARWIDTH];
	POSIX_HEADER pheader;
	int numfiles = 0;
	fpos_t pos = 0;

	fopen_s(&TARFILE, filename, "rb");
	if (!TARFILE) {
		fprintf(stderr, "Failed to load tar\n");
		return -1;
	}
	while (!!fread(&pheader, sizeof(POSIX_HEADER), 1, TARFILE)) {
		if (pheader.name[0] == '\0') // null header says that this can stop.
			break;
		long fileSize = getsize(pheader.size);
		printf("%s, %i bytes\n", pheader.name, fileSize);
		if (pheader.typeflag == 0x35 && _mkdir(pheader.name) != 0) {
			fprintf(stdout, "Failed to create directory %s, not continuing.\n", pheader.name);
			return -1;
		}
		if (pheader.typeflag == 0x0 || fileSize > 0) {
			long adjusted = fileSize % TARWIDTH == 0 ? fileSize : fileSize + (TARWIDTH - (fileSize % TARWIDTH));
			fopen_s(&OUTPUT, pheader.name, "w+b"); // overwrites without asking
			if (OUTPUT != NULL) {
				unsigned int rdlen, remain;
				for (long g = 0; g < (fileSize / TARWIDTH); g++, rdlen = fread(buffer, sizeof(char), TARWIDTH, TARFILE), fwrite(buffer, sizeof(char), rdlen, OUTPUT));
				if (adjusted > fileSize) {
					remain = fileSize - ((fileSize / TARWIDTH) * TARWIDTH);
					rdlen = fread(buffer, sizeof(char), TARWIDTH, TARFILE);
					fwrite(buffer, sizeof(char), remain <= rdlen ? remain : rdlen, OUTPUT);
				}
				fclose(OUTPUT);
				numfiles++;
			} else {
				fprintf(stdout, "Failed to create file %s, skipping.\n", pheader.name);
				fseek(TARFILE, adjusted, SEEK_CUR);
			}
		}
	}
	fclose(TARFILE);
	printf("\nExtracted %i files.\n", numfiles);
	return 0;
}

int main(int argc, char *argv[]) {
	fprintf(stdout, "tuntar - provide the .tar file as an argument.\nWarning, is susceptible to tarbombs.\n");
	if(argc > 1)
		return readFile(argv[1]);
	return 0;
}