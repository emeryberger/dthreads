/*
  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore log methods.
*/
#ifndef _MAGICKCORE_LOG_H
#define _MAGICKCORE_LOG_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <stdarg.h>
#include "magick/exception.h"

#if !defined(GetUnadornedModuleName)
# if (((defined(__cplusplus) || defined(c_plusplus)) && defined(HAS_CPP__func__)) || \
      (!(defined(__cplusplus) || defined(c_plusplus)) && defined(HAS_C__func__)))
#  define GetUnadornedModuleName() (__func__)
# elif defined(_VISUALC_) && defined(__FUNCTION__)
#  define GetUnadornedModuleName() (__FUNCTION__)
# else
#  define GetUnadornedModuleName() ("unknown")
# endif
#endif
#if !defined(GetMagickModule)
# define GetMagickModule()  \
  __FILE__,GetUnadornedModuleName(),(unsigned long) __LINE__
#endif

#define MagickLogFilename  "log.xml"

typedef enum
{
  UndefinedEvents,
  NoEvents = 0x0000,
  TraceEvent = 0x0001,
  AnnotateEvent = 0x0002,
  BlobEvent = 0x0004,
  CacheEvent = 0x0008,
  CoderEvent = 0x0010,
  ConfigureEvent = 0x0020,
  DeprecateEvent = 0x0040,
  DrawEvent = 0x0080,
  ExceptionEvent = 0x0100,
  LocaleEvent = 0x0200,
  ModuleEvent = 0x0400,
  ResourceEvent = 0x0800,
  TransformEvent = 0x1000,
  UserEvent = 0x2000,
  WandEvent = 0x4000,
  X11Event = 0x8000,
  AllEvents = 0x7fffffff
} LogEventType;

typedef struct _LogInfo
  LogInfo;

extern MagickExport char
  **GetLogList(const char *,unsigned long *,ExceptionInfo *);

extern MagickExport const char
  *GetLogName(void),
  *SetLogName(const char *);
                                                                                
extern MagickExport const LogInfo
  *GetLogInfo(const char *,ExceptionInfo *),
  **GetLogInfoList(const char *,unsigned long *,ExceptionInfo *);

extern MagickExport LogEventType
  SetLogEventMask(const char *);

extern MagickExport MagickBooleanType
  IsEventLogging(void),
  ListLogInfo(FILE *,ExceptionInfo *),
  LogMagickEvent(const LogEventType,const char *,const char *,
    const unsigned long,const char *,...) 
    magick_attribute((format (printf,5,6))),
  LogMagickEventList(const LogEventType,const char *,const char *,
    const unsigned long,const char *,va_list)
    magick_attribute((format (printf,5,0)));

extern MagickExport void
  CloseWizardLog(void),
  DestroyLogList(void),
  SetLogFormat(const char *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
