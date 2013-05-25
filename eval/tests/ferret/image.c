/* AUTORIGHTS
Copyright (C) 2007 Princeton University
      
This file is part of Ferret Toolkit.

Ferret Toolkit is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <magick/ImageMagick.h>
#include "image.h"

#define DEFAULT_SIZE	128

int image_init (const char *path)
{
	InitializeMagick(path);
	return 0;
}

int image_cleanup (void)
{
	DestroyMagick();
	return 0;
}


int image_ping (const char *filename, int *width, int *height)
{
	int ret = -1;
	ExceptionInfo *exception;
	ImageInfo *image_info;
	Image *image;
	exception = AcquireExceptionInfo();
	assert(exception != NULL);
	image_info = AcquireImageInfo();
	assert(image_info != NULL);
	strcpy(image_info->filename, filename);

	image = PingImage(image_info, exception);
	if (image == NULL) CatchException(exception);
	else
	{
		ret = 0;
		*width = image->columns;
		*height = image->rows;
	}
	DestroyImage(image);
	DestroyImageInfo(image_info);
	DestroyExceptionInfo(exception);
	return ret;
}

int image_read_rgb (const char *filename, int *width, int *height, unsigned char **data)
{
	ExceptionInfo *exception;
	ImageInfo *image_info;
	Image	*image; 
	unsigned char *rgb;
	int columns, rows;
	MagickBooleanType ret;
	
	exception = AcquireExceptionInfo();
	assert(exception != NULL);
	image_info = AcquireImageInfo();
	assert(exception != NULL);
	strcpy(image_info->filename, filename);

	image = ReadImage(image_info, exception);
	if (image == NULL)
	{
		CatchException(exception);
	}
	assert(image != NULL);

	columns = image->columns;
	rows = image->rows;
	rgb = (unsigned char *)malloc(columns * rows * CHAN);
	assert(rgb != NULL);
	ret = ExportImagePixels(image, 0, 0, columns, rows, "RGB", CharPixel, rgb, exception);
	if (ret == MagickFalse)
	{
		CatchException(exception);
	}
	assert(ret == MagickTrue);

	DestroyImage(image);
	DestroyImageInfo(image_info);
	DestroyExceptionInfo(exception);

	*width = columns;
	*height = rows;
	*data = rgb;
	return 0;
}

int image_read_hsv (const char *filename, int *width, int *height, unsigned char **data)
{
	ExceptionInfo *exception;
	ImageInfo *image_info;
	Image	*image; 
	unsigned char *hsv;
	int columns, rows;
	MagickBooleanType ret;
	
	exception = AcquireExceptionInfo();
	assert(exception != NULL);
	image_info = AcquireImageInfo();
	assert(exception != NULL);
	strcpy(image_info->filename, filename);

	image = ReadImage(image_info, exception);
	if (image == NULL)
	{
		CatchException(exception);
	}
	assert(image != NULL);
	ret = SetImageColorspace(image, HSBColorspace);
	assert(ret == MagickTrue);
	columns = image->columns;
	rows = image->rows;
	hsv = (unsigned char *)malloc(columns * rows * CHAN);
	assert(hsv != NULL);
	ret = ExportImagePixels(image, 0, 0, columns, rows, "RGB", CharPixel, hsv, exception);
	if (ret == MagickFalse)
	{
		CatchException(exception);
	}
	assert(ret == MagickTrue);

	DestroyImage(image);
	DestroyImageInfo(image_info);
	DestroyExceptionInfo(exception);

	*width = columns;
	*height = rows;
	*data = hsv;
	return 0;
}

int image_write_rgb (const char *filename, int width, int height, unsigned char *data)
{
	ExceptionInfo *exception;
	ImageInfo *image_info;
	Image	*image; 
	MagickBooleanType ret;
	
	exception = AcquireExceptionInfo();
	assert(exception != NULL);
	image_info = AcquireImageInfo();
	assert(exception != NULL);

	image = ConstituteImage(width, height, "RGB", CharPixel, data, exception);
	if (image == NULL) CatchException(exception);
	assert(image != NULL);
	strcpy(image->filename, filename);
	ret = WriteImage(image_info, image);
	assert(ret == MagickTrue);

	DestroyImage(image);
	DestroyImageInfo(image_info);
	DestroyExceptionInfo(exception);
	return 0;
}

int image_read_gray (const char *filename, int *width, int *height, float **data)
{
	ExceptionInfo *exception;
	ImageInfo *image_info;
	Image	*image; 
	float *gray;
	int columns, rows;
	MagickBooleanType ret;
	
	exception = AcquireExceptionInfo();
	assert(exception != NULL);
	image_info = AcquireImageInfo();
	assert(exception != NULL);
	strcpy(image_info->filename, filename);

	image = ReadImage(image_info, exception);
	if (image == NULL)
	{
		CatchException(exception);
	}
	assert(image != NULL);
	columns = image->columns;
	rows = image->rows;
	gray = (float *)malloc(columns * rows * sizeof(float));
	assert(gray != NULL);
	ret = ExportImagePixels(image, 0, 0, columns, rows, "I", FloatPixel, gray, exception);
	if (ret == MagickFalse)
	{
		CatchException(exception);
	}
	assert(ret == MagickTrue);

	DestroyImage(image);
	DestroyImageInfo(image_info);
	DestroyExceptionInfo(exception);

	*width = columns;
	*height = rows;
	*data = gray;
	return 0;
}

int image_read_rgb_hsv (const char *filename, int *width, int *height, unsigned char **data_rgb, unsigned char **data_hsv)
{
	ExceptionInfo *exception;
	ImageInfo *image_info;
	Image	*image, *image2; 
	unsigned char *rgb, *hsv;
	int columns, rows;
	MagickBooleanType ret;
	
	exception = AcquireExceptionInfo();
	assert(exception != NULL);
	image_info = AcquireImageInfo();
	assert(exception != NULL);
	strcpy(image_info->filename, filename);

	image = ReadImage(image_info, exception);
	if (image == NULL)
	{
		CatchException(exception);
	}
	assert(image != NULL);

	image2 = image;
	image = ThumbnailImage(image2, DEFAULT_SIZE, DEFAULT_SIZE, exception);
	assert(image != NULL);
	DestroyImage(image2);

	columns = image->columns;
	rows = image->rows;
	rgb = (unsigned char *)malloc(columns * rows * CHAN);
	assert(rgb != NULL);
	ret = ExportImagePixels(image, 0, 0, columns, rows, "RGB", CharPixel, rgb, exception);
	if (ret == MagickFalse)
	{
		CatchException(exception);
	}
	assert(ret == MagickTrue);

	*data_rgb = rgb;

	ret = SetImageColorspace(image, HSBColorspace);
	assert(ret == MagickTrue);
	columns = image->columns;
	rows = image->rows;
	hsv = (unsigned char *)malloc(columns * rows * CHAN);
	assert(hsv != NULL);
	ret = ExportImagePixels(image, 0, 0, columns, rows, "RGB", CharPixel, hsv, exception);
	if (ret == MagickFalse)
	{
		CatchException(exception);
	}
	assert(ret == MagickTrue);

	*data_hsv = hsv;

	DestroyImage(image);
	DestroyImageInfo(image_info);
	DestroyExceptionInfo(exception);

	*width = columns;
	*height = rows;
	return 0;
}
