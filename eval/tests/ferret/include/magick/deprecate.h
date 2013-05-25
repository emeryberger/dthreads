/*
  Copyright 1999-2007 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore deprecated methods.
*/
#ifndef _MAGICKCORE_DEPRECATE_H
#define _MAGICKCORE_DEPRECATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if !defined(ExcludeMagickDeprecated)

#include <stdarg.h>
#include "magick/blob.h"
#include "magick/cache-view.h"
#include "magick/draw.h"
#include "magick/constitute.h"
#include "magick/magick-config.h"
#include "magick/quantum.h"
#include "magick/registry.h"
#include "magick/semaphore.h"

#if !defined(magick_attribute)
#  if !defined(__GNUC__)
#    define magick_attribute(x) /*nothing*/
#  else
#    define magick_attribute __attribute__
#  endif
#endif

#define Downscale(quantum)  ScaleQuantumToChar(quantum)
#define LABColorspace LabColorspace
#define Intensity(color)  PixelIntensityToQuantum(color)
#define LiberateUniqueFileResource(resource) \
  RelinquishUniqueFileResource(resource)
#define LiberateMagickResource(resource)  RelinquishMagickResource(resource)
#define LiberateSemaphore(semaphore)  RelinquishSemaphore(semaphore)
#define RunlengthEncodedCompression  RLECompression
#define Upscale(value)  ScaleCharToQuantum(value)
#define XDownscale(value)  ScaleShortToQuantum(value)
#define XUpscale(quantum)  ScaleQuantumToShort(quantum)

typedef struct _DoublePixelPacket
{
  double
    red,
    green,
    blue,
    opacity,
    index;
} DoublePixelPacket;

#if !defined(__WINDOWS__)
#if (SIZEOF_LONG_LONG == 8)
typedef long long ExtendedSignedIntegralType;
typedef unsigned long long ExtendedUnsignedIntegralType;
#else
typedef long ExtendedSignedIntegralType;
typedef unsigned long ExtendedUnsignedIntegralType;
#endif
#else
typedef __int64 ExtendedSignedIntegralType;
typedef unsigned __int64 ExtendedUnsignedIntegralType;
#endif
#if defined(HAVE_LONG_DOUBLE)
typedef long double ExtendedRationalType;
#else
typedef double ExtendedRationalType;
#endif

typedef MagickBooleanType
  (*MonitorHandler)(const char *,const MagickOffsetType,const MagickSizeType,
    ExceptionInfo *);

typedef struct _ImageAttribute
{
  char
    *key,
    *value;
                                                                                
  MagickBooleanType
    compression;
                                                                                
  struct _ImageAttribute
    *previous,
    *next;  /* deprecated */
} ImageAttribute;

extern MagickExport char
  *AllocateString(const char *),
  *InterpretImageAttributes(const ImageInfo *,Image *,const char *),
  *PostscriptGeometry(const char *),
  *TranslateText(const ImageInfo *,Image *,const char *);

extern MagickExport const ImageAttribute
  *GetImageAttribute(const Image *,const char *),
  *GetImageClippingPathAttribute(Image *),
  *GetNextImageAttribute(const Image *);

extern MagickExport Image
  *GetImageFromMagickRegistry(const char *,long *id,ExceptionInfo *),
  *GetImageList(const Image *,const long,ExceptionInfo *),
  *GetNextImage(const Image *),
  *GetPreviousImage(const Image *),
  *PopImageList(Image **),
  *ShiftImageList(Image **),
  *SpliceImageList(Image *,const long,const unsigned long,const Image *,
    ExceptionInfo *);

extern MagickExport IndexPacket
  ValidateColormapIndex(Image *,const unsigned long);

extern MagickExport int
  GetImageGeometry(Image *,const char *,const unsigned int,RectangleInfo *),
  ParseImageGeometry(const char *,long *,long *,unsigned long *,
    unsigned long *);

extern MagickExport long
  GetImageListIndex(const Image *),
  SetMagickRegistry(const RegistryType,const void *,const size_t,
    ExceptionInfo *);

extern MagickExport MagickBooleanType
  CloneImageAttributes(Image *,const Image *),
  ColorFloodfillImage(Image *,const DrawInfo *,const PixelPacket,const long,
    const long,const PaintMethod),
  DeleteImageAttribute(Image *,const char *),
  DeleteMagickRegistry(const long),
  DescribeImage(Image *,FILE *,const MagickBooleanType),
  FormatImageAttribute(Image *,const char *,const char *,...)
    magick_attribute((format (printf,3,4))),
  FormatImageAttributeList(Image *,const char *,const char *,va_list)
    magick_attribute((format (printf,3,0))),
  FuzzyColorCompare(const Image *,const PixelPacket *,const PixelPacket *),
  FuzzyOpacityCompare(const Image *,const PixelPacket *,const PixelPacket *),
  MagickMonitor(const char *,const MagickOffsetType,const MagickSizeType,
    void *),
  MatteFloodfillImage(Image *,const PixelPacket,const Quantum,const long,
    const long,const PaintMethod),
  OpaqueImage(Image *,const PixelPacket,const PixelPacket),
  PopImagePixels(Image *,const QuantumType,unsigned char *),
  PushImagePixels(Image *,const QuantumType,const unsigned char *),
  SetExceptionInfo(ExceptionInfo *,ExceptionType),
  SetImageAttribute(Image *,const char *,const char *),
  TransparentImage(Image *,const PixelPacket,const Quantum);

extern MagickExport MonitorHandler
  GetMonitorHandler(void),
  SetMonitorHandler(MonitorHandler);

extern MagickExport MagickOffsetType
  SizeBlob(const Image *image);

extern MagickExport MagickPixelPacket
  InterpolatePixelColor(const Image *,ViewInfo *,const InterpolatePixelMethod,
    const double,const double,ExceptionInfo *);

extern MagickExport PixelPacket
  *GetCacheView(ViewInfo *,const long,const long,const unsigned long,
    const unsigned long);

extern MagickExport unsigned int
  ChannelImage(Image *,const ChannelType),
  ChannelThresholdImage(Image *,const char *),
  DispatchImage(const Image *,const long,const long,const unsigned long,
    const unsigned long,const char *,const StorageType,void *,ExceptionInfo *),
  FuzzyColorMatch(const PixelPacket *,const PixelPacket *,const double),
  GetNumberScenes(const Image *),
  GetMagickGeometry(const char *,long *,long *,unsigned long *,unsigned long *),
  IsSubimage(const char *,const unsigned int),
  PushImageList(Image **,const Image *,ExceptionInfo *),
  QuantizationError(Image *),
  RandomChannelThresholdImage(Image *,const char *,const char *,
    ExceptionInfo *),
  SetImageList(Image **,const Image *,const long,ExceptionInfo *),
  TransformColorspace(Image *,const ColorspaceType),
  ThresholdImage(Image *,const double),
  ThresholdImageChannel(Image *,const char *),
  UnshiftImageList(Image **,const Image *,ExceptionInfo *);

extern MagickExport unsigned long
  GetImageListSize(const Image *);

extern MagickExport void
  *AcquireMemory(const size_t),
  *CloneMemory(void *,const void *,const size_t),
  DestroyImageAttributes(Image *),
  DestroyImages(Image *),
  DestroyMagick(void),
  DestroyMagickRegistry(void),
  *GetConfigureBlob(const char *,char *,size_t *,ExceptionInfo *),
  *GetMagickRegistry(const long,RegistryType *,size_t *,ExceptionInfo *),
  IdentityAffine(AffineMatrix *),
  LiberateMemory(void **),
  LiberateSemaphoreInfo(SemaphoreInfo **),
  FormatString(char *,const char *,...) magick_attribute((format (printf,2,3))),
  FormatStringList(char *,const char *,va_list)
    magick_attribute((format (printf,2,0))),
  InitializeMagick(const char *),
  ReacquireMemory(void **,const size_t),
  ResetImageAttributeIterator(const Image *),
  SetCacheThreshold(const unsigned long),
  SetImage(Image *,const Quantum),
  Strip(char *),
  TemporaryFilename(char *);
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
