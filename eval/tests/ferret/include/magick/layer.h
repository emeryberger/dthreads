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

  MagickCore image layer methods.
*/
#ifndef _MAGICKCORE_LAYER_H
#define _MAGICKCORE_LAYER_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum
{
  UnrecognizedDispose,
  UndefinedDispose = 0,
  NoneDispose = 1,
  BackgroundDispose = 2,
  PreviousDispose = 3
} DisposeType;

typedef enum
{
  UndefinedLayer,
  CoalesceLayer,
  CompareAnyLayer,
  CompareClearLayer,
  CompareOverlayLayer,
  DisposeLayer,
  OptimizeLayer,
  OptimizeImageLayer,
  OptimizePlusLayer,
  OptimizeTransLayer,
  RemoveDupsLayer,
  RemoveZeroLayer,
  CompositeLayer
} MagickLayerMethod;

extern MagickExport Image
  *CoalesceImages(const Image *,ExceptionInfo *),
  *DisposeImages(const Image *,ExceptionInfo *),
  *CompareImageLayers(const Image *,const MagickLayerMethod,ExceptionInfo *),
  *DeconstructImages(const Image *,ExceptionInfo *),
  *OptimizeImageLayers(const Image *,ExceptionInfo *),
  *OptimizePlusImageLayers(const Image *,ExceptionInfo *);

extern MagickExport void
  OptimizeImageTransparency(const Image *,ExceptionInfo *),
  RemoveDuplicateLayers(Image **,ExceptionInfo *),
  RemoveZeroDelayLayers(Image **,ExceptionInfo *),
  CompositeLayers(Image *,const CompositeOperator,Image *,const long,
    const long,ExceptionInfo *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
