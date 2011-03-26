/*
Copyright (C) 2009-2011 Parallel Realities

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "headers.h"

FILE *pak;
int totalFiles, fileID = 0;
FileData *fileData;

static int countFiles(char *);
static void cleanup(void);
static void recurseDirectory(char *);
static void testPak(char *);

int main(int argc, char *argv[])
{
	int i;
	long length;

	if (argc == 3)
	{
		if (strcmpignorecase(argv[1], "-test") == 0)
		{
			testPak(argv[2]);

			exit(0);
		}
	}

	else if (argc < 3)
	{
		printf("Usage   : pak <directory names> <outputname>\n");
		printf("Example : pak data music gfx sound font edgar.pak\n");

		exit(1);
	}

	pak = fopen(argv[argc - 1], "wb");

	for (i=1;i<argc-1;i++)
	{
		totalFiles += countFiles(argv[i]);
	}

	printf("Will compress %d files\n", totalFiles);

	printf("Compressing 00%%...\r");

	fileData = malloc(totalFiles * sizeof(FileData));

	if (fileData == NULL)
	{
		printf("Failed to create File Data\n");
	}

	atexit(cleanup);

	for (i=1;i<argc-1;i++)
	{
		recurseDirectory(argv[i]);
	}

	length = ftell(pak);

	for (i=0;i<totalFiles;i++)
	{
		if (fileData[i].fileSize == 0)
		{
			break;
		}

		fwrite(&fileData[i], sizeof(FileData), 1, pak);
	}

	fwrite(&length, sizeof(long), 1, pak);
	fwrite(&totalFiles, sizeof(int), 1, pak);

	fclose(pak);

	printf("Compressing 100%%\nCompleted\n");

	return 0;
}

static int countFiles(char *dirName)
{
	DIR *dirp, *dirp2;
	struct dirent *dfile;
	char filename[MAX_FILE_LENGTH];
	int count = 0;

	dirp = opendir(dirName);

	while ((dfile = readdir(dirp)))
	{
		if (dfile->d_name[0] == '.')
		{
			continue;
		}

		sprintf(filename, "%s/%s", dirName, dfile->d_name);

		dirp2 = opendir(filename);

		if (dirp2)
		{
			closedir(dirp2);

			count += countFiles(filename);
		}

		else
		{
			count++;
		}
	}

	return count;
}

static void cleanup()
{

}

static void recurseDirectory(char *dirName)
{
	DIR *dirp, *dirp2;
	struct dirent *dfile;
	FILE *infile;
	char filename[1024];
	int compressionResult;
	unsigned long fileSize, compressedSize, ensuredSize;
	gzFile fp;
	float percentage;
	unsigned char *buffer, *output;

	dirp = opendir(dirName);

	if (dirp == NULL)
	{
		printf("%s: Directory does not exist or is not accessable\n", dirName);

		exit(1);
	}

	while ((dfile = readdir(dirp)))
	{
		if (dfile->d_name[0] == '.')
		{
			continue;
		}

		snprintf(filename, sizeof(filename), "%s/%s", dirName, dfile->d_name);

		dirp2 = opendir(filename);

		if (dirp2)
		{
			closedir(dirp2);

			recurseDirectory(filename);
		}

		else
		{
			infile = fopen(filename, "rb");

			if (!infile)
			{
				printf("Couldn't open %s for reading!\n", filename);

				closedir(dirp);
				gzclose(pak);

				exit(1);
			}

			fseek(infile, 0L, SEEK_END);

			fileSize = ftell(infile);

			if (fileSize == 0)
			{
				printf("%s is an empty file.\n", filename);

				exit(0);
			}

			ensuredSize = fileSize * 1.01 + 12;

			compressedSize = ensuredSize;

			fclose(infile);

			buffer = malloc((fileSize + 1) * sizeof(unsigned char));

			if (buffer == NULL)
			{
				printf("Could not create buffer\n");

				exit(1);
			}

			output = malloc(ensuredSize * sizeof(unsigned char));

			if (output == NULL)
			{
				printf("Could not create output\n");

				exit(1);
			}

			fp = gzopen(filename, "rb");

			if (!fp)
			{
				printf("Couldn't open %s for reading!\n", filename);

				closedir(dirp);

				gzclose(pak);

				exit(1);
			}

			gzread(fp, buffer, fileSize);

			gzclose(fp);

			compressionResult = compress2(output, &compressedSize, buffer, fileSize, 9);

			if (compressionResult != Z_OK)
			{
				printf("Compression of %s failed\n", filename);

				if (compressionResult == Z_BUF_ERROR)
				{
					printf("Buffer too small\n");

					exit(1);
				}

				if (compressionResult == Z_MEM_ERROR)
				{
					printf("Out of RAM\n");

					exit(1);
				}

				if (compressionResult == Z_STREAM_ERROR)
				{
					printf("Stream error\n");

					exit(1);
				}
			}

			percentage = fileID;

			percentage /= totalFiles;

			percentage *= 100;

			printf("Compressing %02d%%...\r", (int)percentage);

			STRNCPY(fileData[fileID].filename, filename, MAX_FILE_LENGTH);

			fileData[fileID].fileSize = fileSize;
			fileData[fileID].compressedSize = compressedSize;
			fileData[fileID].offset = ftell(pak);

			fwrite(output, compressedSize, 1, pak);

			fileID++;

			free(buffer);

			free(output);
		}
	}

	closedir(dirp);
}

static void testPak(char *pakFile)
{
	FileData *fileData;
	int fileCount, i, read;
	long offset;
	unsigned long size;
	FILE *fp;
	unsigned char *source, *dest;

	fp = fopen(pakFile, "rb");

	if (fp == NULL)
	{
		printf("Failed to open PAK file %s\n", pakFile);
	}

	fseek(fp, -(sizeof(long) + sizeof(int)), SEEK_END);

	read = fread(&offset, sizeof(long), 1, fp);
	read = fread(&fileCount, sizeof(int), 1, fp);

	fileData = malloc(fileCount * sizeof(FileData));

	if (fileData == NULL)
	{
		printf("Could not allocate %d bytes for FileData\n", (int)(fileCount * sizeof(FileData)));

		exit(1);
	}

	fseek(fp, offset, SEEK_SET);

	read = fread(fileData, sizeof(FileData), fileCount, fp);

	printf("Loaded up PAK file with %d entries\n", fileCount);

	for (i=0;i<fileCount;i++)
	{
		printf("'%s' at offset %ld : %ld -> %ld\n", fileData[i].filename, fileData[i].offset, fileData[i].compressedSize, fileData[i].fileSize);
	}

	for (i=0;i<fileCount;i++)
	{
		rewind(fp);

		printf("Testing %s...", fileData[i].filename);

		fseek(fp, fileData[i].offset, SEEK_SET);

		source = malloc(fileData[i].compressedSize * sizeof(unsigned char));

		if (source == NULL)
		{
			printf("Failed to allocate %ld bytes to load %s from PAK\n", fileData[i].compressedSize * sizeof(unsigned char), fileData[i].filename);
		}

		dest = malloc((fileData[i].fileSize + 1) * sizeof(unsigned char));

		if (dest == NULL)
		{
			printf("Failed to allocate %ld bytes to load %s from PAK\n", fileData[i].fileSize * sizeof(unsigned char), fileData[i].filename);
		}

		read = fread(source, fileData[i].compressedSize, 1, fp);

		size = fileData[i].fileSize;

		uncompress(dest, &size, source, fileData[i].compressedSize);

		dest[size] = '\0';

		if (size != fileData[i].fileSize)
		{
			printf("Failed to decompress %s. Expected %ld, got %ld\n", fileData[i].filename, fileData[i].fileSize, size);
		}

		free(source);

		free(dest);

		printf("OK\n");
	}

	fclose(fp);

	free(fileData);

	printf("Test completed. No errors found\n");
}
