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

  MagickCore cache methods.
*/
#ifndef _MAGICKCORE_CACHE_H
#define _MAGICKCORE_CACHE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "magick/blob.h"

extern MagickExport const PixelPacket
  *AcquireCacheNexus(const Image *,const VirtualPixelMethod,const long,
    const long,const unsigned long,const unsigned long,const unsigned long,
    ExceptionInfo *);

extern MagickExport MagickSizeType
  GetPixelCacheArea(const Image *);

extern MagickExport MagickBooleanType
  PersistCache(Image *,const char *,const MagickBooleanType,MagickOffsetType *,
    ExceptionInfo *),
  SyncCacheNexus(Image *,const unsigned long);

extern MagickExport PixelPacket
  *GetCacheNexus(Image *,const long,const long,const unsigned long,
    const unsigned long,const unsigned long),
  *SetCacheNexus(Image *,const long,const long,const unsigned long,
    const unsigned long,const unsigned long);

extern MagickExport VirtualPixelMethod
  GetCacheVirtualPixelMethod(const Image *),
  SetCacheVirtualPixelMethod(const Image *,const VirtualPixelMethod);

extern MagickExport void
  DestroyCacheResources(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
