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

  MagickCore quantum inline methods.
*/
#ifndef _MAGICKCORE_QUANTUM_H
#define _MAGICKCORE_QUANTUM_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "magick/semaphore.h"

typedef enum
{
  UndefinedEndian,
  LSBEndian,
  MSBEndian
} EndianType;

typedef enum
{
  UndefinedQuantum,
  AlphaQuantum,
  BlackQuantum,
  BlueQuantum,
  CMYKAQuantum,
  CMYKQuantum,
  CyanQuantum,
  GrayAlphaQuantum,
  GrayQuantum,
  GreenQuantum,
  IndexAlphaQuantum,
  IndexQuantum,
  MagentaQuantum,
  OpacityQuantum,
  RedQuantum,
  RGBAQuantum,
  RGBOQuantum,
  RGBQuantum,
  YellowQuantum
} QuantumType;

typedef enum
{
  UndefinedQuantumFormat,
  FloatingPointQuantumFormat,
  SignedQuantumFormat,
  UnsignedQuantumFormat
} QuantumFormatType;

typedef struct _QuantumInfo
{
  unsigned long
    quantum;

  QuantumFormatType
    format;

  double
    minimum,
    maximum,
    scale;

  size_t
    pad;

  MagickBooleanType
    min_is_white,
    pack;

  SemaphoreInfo
    *semaphore;

  unsigned long
    signature;
} QuantumInfo;

static inline MagickRealType ClipToQuantum(const MagickRealType value)
{
  if (value <= 0.0)
    return(0.0);
  if (value >= QuantumRange)
    return((MagickRealType) QuantumRange);
  return(value);
}

static inline Quantum RoundToQuantum(const MagickRealType value)
{
#if defined(UseHDRI)
  return((Quantum) value);
#else
  if (value <= 0.0)
    return((Quantum) 0);
  if (value >= QuantumRange)
    return((Quantum) QuantumRange);
  return((Quantum) (value+0.5));
#endif
}

static inline Quantum ScaleAnyToQuantum(const QuantumAny quantum,
  const unsigned long depth)
{
  QuantumAny
    scale;

  if ((depth == 0) || (depth > QuantumDepth))
    return(0);
  scale=(QuantumAny) QuantumRange/((QuantumAny) QuantumRange >>
    (QuantumAny) (QuantumDepth-depth));
  return((Quantum) (scale*quantum));
}

static inline QuantumAny ScaleQuantumToAny(const Quantum quantum,
  const unsigned long depth)
{
  QuantumAny
    scale;

  if ((depth == 0) || (depth > QuantumDepth))
    return(0);
  scale=(QuantumAny) QuantumRange/((QuantumAny) QuantumRange >>
    (QuantumAny) (QuantumDepth-depth));
  return((QuantumAny) quantum/scale);
}

#if (QuantumDepth == 8)
static inline Quantum ScaleCharToQuantum(const unsigned char value)
{
  return((Quantum) value);
}

static inline Quantum ScaleLongToQuantum(const unsigned long value)
{
#if !defined(UseHDRI)
  return((Quantum) ((value+8421504UL)/16843009UL));
#else
  return((Quantum) (value/16843008.0));
#endif
}

static inline Quantum ScaleMapToQuantum(const MagickRealType value)
{
#if defined(UseHDRI)
  return((Quantum) value);
#else
  if (value <= 0.0)
    return(0);
  if (value >= MaxMap)
    return(QuantumRange);
  return((Quantum) (value+0.5));
#endif
}

static inline unsigned char ScaleQuantumToChar(const Quantum quantum)
{
#if !defined(UseHDRI)
  return((unsigned char) quantum);
#else
  if (quantum <= 0.0)
    return(0UL);
  if (quantum >= 255.0)
    return(255);
  return((unsigned char) (quantum+0.5));
#endif
}

static inline unsigned long ScaleQuantumToLong(const Quantum quantum)
{
#if !defined(UseHDRI)
  return((unsigned long) (16843009UL*quantum));
#else
  if (quantum <= 0.0)
    return(0UL);
  if ((16843008.0*quantum) >= 4294967295.0)
    return(4294967295UL);
  return((unsigned long) (16843008.0*quantum));
#endif
}

static inline unsigned long ScaleQuantumToMap(const Quantum quantum)
{
  if (quantum <= 0.0)
    return(0UL);
  if (quantum >= MaxMap)
    return((unsigned long) MaxMap);
#if !defined(UseHDRI)
  return((unsigned long) quantum);
#else
  return((unsigned long) (quantum+0.5));
#endif
}

static inline unsigned short ScaleQuantumToShort(const Quantum quantum)
{
#if !defined(UseHDRI)
  return((unsigned short) (257UL*quantum));
#else
  if (quantum <= 0.0)
    return(0);
  if ((257.0*quantum) >= 65535.0)
    return(65535);
  return((unsigned short) (257.0*quantum));
#endif
}

static inline Quantum ScaleShortToQuantum(const unsigned short value)
{
#if !defined(UseHDRI)
  return((Quantum) ((value+128UL)/257UL));
#else
  return((Quantum) (value/257.0));
#endif
}
#elif (QuantumDepth == 16)
static inline Quantum ScaleCharToQuantum(const unsigned char value)
{
#if !defined(UseHDRI)
  return((Quantum) (257UL*value));
#else
  return((Quantum) (257.0*value));
#endif
}

static inline Quantum ScaleLongToQuantum(const unsigned long value)
{
#if !defined(UseHDRI)
  return((Quantum) ((value+32768UL)/65537UL));
#else
  return((Quantum) (value/65537.0));
#endif
}

static inline Quantum ScaleMapToQuantum(const MagickRealType value)
{
#if defined(UseHDRI)
  return((Quantum) value);
#else
  if (value <= 0.0)
    return(0);
  if (value >= MaxMap)
    return((Quantum) QuantumRange);
  return((Quantum) (value+0.5));
#endif
}

static inline unsigned char ScaleQuantumToChar(const Quantum quantum)
{
#if !defined(UseHDRI)
  return((unsigned char) ((quantum+128UL)/257UL));
#else
  if (quantum <= 0.0)
    return(0);
  if ((quantum/257.0) >= 255.0)
    return(255);
  return((unsigned char) (quantum/257.0));
#endif
}

static inline unsigned long ScaleQuantumToLong(const Quantum quantum)
{
#if !defined(UseHDRI)
  return((unsigned long) (65537UL*quantum));
#else
  if (quantum <= 0.0)
    return(0UL);
  if ((65537.0*quantum) >= 4294967295.0)
    return(4294967295UL);
  return((unsigned long) (65537.0*quantum));
#endif
}

static inline unsigned long ScaleQuantumToMap(const Quantum quantum)
{
  if (quantum <= 0)
    return(0);
  if ((1UL*quantum) >= MaxMap)
    return((unsigned long) MaxMap);
#if !defined(UseHDRI)
  return((unsigned long) quantum);
#else
  return((unsigned long) (quantum+0.5));
#endif
}

static inline unsigned short ScaleQuantumToShort(const Quantum quantum)
{
#if !defined(UseHDRI)
  return((unsigned short) quantum);
#else
  if (quantum <= 0.0)
    return(0);
  if (quantum >= 65535.0)
    return(65535);
  return((unsigned short) (quantum+0.5));
#endif
}

static inline Quantum ScaleShortToQuantum(const unsigned short value)
{
  return((Quantum) value);
}
#elif (QuantumDepth == 32)
static inline Quantum ScaleCharToQuantum(const unsigned char value)
{
#if !defined(UseHDRI)
  return((Quantum) (16843009UL*value));
#else
  return((Quantum) (16843009.0*value));
#endif
}

static inline Quantum ScaleLongToQuantum(const unsigned long value)
{
  return((Quantum) value);
}

static inline Quantum ScaleMapToQuantum(const MagickRealType value)
{
#if defined(UseHDRI)
  return((Quantum) (65537.0*value));
#else
  if (value <= 0.0)
    return(0);
  if (value >= MaxMap)
    return(QuantumRange);
  return((Quantum) (65537UL*value));
#endif
}

static inline unsigned char ScaleQuantumToChar(const Quantum quantum)
{
#if !defined(UseHDRI)
  unsigned char
    pixel;

  pixel=(unsigned char) ((quantum+MagickULLConstant(8421504))/
    MagickULLConstant(16843009));
  return(pixel);
#else
  if (quantum <= 0.0)
    return(0);
  if ((quantum/16843009.0) >= 255.0)
    return(255);
  return((unsigned char) (quantum/16843009.0));
#endif
}

static inline unsigned long ScaleQuantumToLong(const Quantum quantum)
{
  return((unsigned long) quantum);
}

static inline unsigned long ScaleQuantumToMap(const Quantum quantum)
{
  if (quantum <= 0.0)
    return(0);
  if ((quantum/65537.0) >= (MagickRealType) MaxMap)
    return((unsigned long) MaxMap);
#if !defined(UseHDRI)
  {
    unsigned long
      pixel;

    pixel=(unsigned long) ((quantum+MagickULLConstant(32768))/
      MagickULLConstant(65537));
    return(pixel);
  }
#else
  return((unsigned long) (quantum/65537.0));
#endif
}

static inline unsigned short ScaleQuantumToShort(const Quantum quantum)
{
#if !defined(UseHDRI)
  unsigned short
    pixel;

  pixel=(unsigned short) ((quantum+MagickULLConstant(32768))/
    MagickULLConstant(65537));
  return(pixel);
#else
  if (quantum <= 0.0)
    return(0);
  if ((quantum/65537.0) >= 65535.0)
    return(65535);
  return((unsigned short) (quantum/65537.0));
#endif
}

static inline Quantum ScaleShortToQuantum(const unsigned short value)
{
#if !defined(UseHDRI)
  return((Quantum) (65537UL*value));
#else
  return((Quantum) (65537.0*value));
#endif
}
#elif (QuantumDepth == 64)
static inline Quantum ScaleCharToQuantum(const unsigned char value)
{
#if !defined(UseHDRI)
  return((Quantum) (MagickULLConstant(71777214294589695)*value));
#else
  return((Quantum) (71777214294589695.0*value));
#endif
}

static inline Quantum ScaleLongToQuantum(const unsigned long value)
{
#if !defined(UseHDRI)
  return((Quantum) (4294967295UL*value));
#else
  return((Quantum) (4294967295.0*value));
#endif
}

static inline Quantum ScaleMapToQuantum(const MagickRealType value)
{
#if defined(UseHDRI)
  return((Quantum) (281479271612415.0*value));
#else
  if (value <= 0.0)
    return(0);
  if (value >= MaxMap)
    return(QuantumRange);
  return((Quantum) (MagickULLConstant(281479271612415)*value));
#endif
}

static inline unsigned char ScaleQuantumToChar(const Quantum quantum)
{
#if !defined(UseHDRI)
  return((unsigned char) ((quantum+2155839615.0)/71777214294589695.0));
#else
  return((unsigned char) (quantum/71777214294589695.0));
#endif
}

static inline unsigned long ScaleQuantumToLong(const Quantum quantum)
{
#if !defined(UseHDRI)
  return((unsigned long) ((quantum+2147483648.0)/4294967297.0));
#else
  return((unsigned long) (quantum/4294967297.0));
#endif
}

static inline unsigned long ScaleQuantumToMap(const Quantum quantum)
{
  if (quantum <= 0.0)
    return(0);
  if ((quantum/281479271612415.0) >= (MagickRealType) MaxMap)
    return((unsigned long) MaxMap);
#if !defined(UseHDRI)
  return((unsigned long) ((quantum+2147450879.0)/281479271612415.0));
#else
  return((unsigned long) (quantum/281479271612415.0));
#endif
}

static inline unsigned short ScaleQuantumToShort(const Quantum quantum)
{
#if !defined(UseHDRI)
  return((unsigned short) ((quantum+2147450879.0)/281479271612415.0));
#else
  return((unsigned short) (quantum/281479271612415.0));
#endif
}

static inline Quantum ScaleShortToQuantum(const unsigned short value)
{
#if !defined(UseHDRI)
  return((Quantum) (MagickULLConstant(281479271612415)*value));
#else
  return((Quantum) (281479271612415.0*value));
#endif
}
#endif

extern MagickExport MagickBooleanType
  ExportQuantumPixels(Image *,const QuantumInfo *,const QuantumType,
    const unsigned char *),
  ImportQuantumPixels(Image *,const QuantumInfo *,const QuantumType,
    unsigned char *);

extern MagickExport QuantumInfo
  *AcquireQuantumInfo(const ImageInfo *),
  *DestroyQuantumInfo(QuantumInfo *);

extern MagickExport void
  GetQuantumInfo(const ImageInfo *,QuantumInfo *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
