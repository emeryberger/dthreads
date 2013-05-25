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

  MagickCore cache view methods.
*/
#ifndef _MAGICKCORE_CACHE_VIEW_H
#define _MAGICKCORE_CACHE_VIEW_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "magick/pixel.h"

typedef enum
{
  UndefinedVirtualPixelMethod,
  BackgroundVirtualPixelMethod,
  ConstantVirtualPixelMethod,  /* deprecated */
  DitherVirtualPixelMethod,
  EdgeVirtualPixelMethod,
  MirrorVirtualPixelMethod,
  RandomVirtualPixelMethod,
  TileVirtualPixelMethod,
  TransparentVirtualPixelMethod,
  MaskVirtualPixelMethod,
  BlackVirtualPixelMethod,
  GrayVirtualPixelMethod,
  WhiteVirtualPixelMethod
} VirtualPixelMethod;

typedef struct _ViewInfo
  ViewInfo;

extern MagickExport const PixelPacket
  *AcquireCacheViewPixels(const ViewInfo *,const long,const long,
    const unsigned long,const unsigned long,ExceptionInfo *);

extern MagickExport ClassType
  GetCacheViewStorageClass(const ViewInfo *);

extern MagickExport ColorspaceType
  GetCacheViewColorspace(const ViewInfo *);

extern MagickExport const IndexPacket
  *AcquireCacheViewIndexes(const ViewInfo *);

extern MagickExport ExceptionInfo
  *GetCacheViewException(const ViewInfo *);

extern MagickExport IndexPacket
  *GetCacheViewIndexes(const ViewInfo *);

extern MagickExport MagickBooleanType
  SetCacheViewStorageClass(ViewInfo *,const ClassType),
  SetCacheViewVirtualPixelMethod(ViewInfo *,const VirtualPixelMethod),
  SyncCacheView(ViewInfo *);

extern MagickExport PixelPacket
  AcquireOneCacheViewPixel(const ViewInfo *,const long,const long,
    ExceptionInfo *),
  *GetCacheViewPixels(ViewInfo *,const long,const long,const unsigned long,
    const unsigned long),
  GetOneCacheViewPixel(const ViewInfo *,const long,const long),
  *SetCacheView(ViewInfo *,const long,const long,const unsigned long,
    const unsigned long);

extern MagickExport ViewInfo
  *CloseCacheView(ViewInfo *),
  *CloneCacheView(const ViewInfo *),
  *OpenCacheView(const Image *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
