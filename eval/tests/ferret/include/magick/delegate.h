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

  MagickCore delegates methods.
*/
#ifndef _MAGICKCORE_DELEGATE_H
#define _MAGICKCORE_DELEGATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(HasGS)
#include "ghostscript/iapi.h"
#include "ghostscript/ierrors.h"
#endif

#ifndef gs_main_instance_DEFINED
# define gs_main_instance_DEFINED
typedef struct gs_main_instance_s gs_main_instance;
#endif

#if !defined(MagickDLLCall)
#  if defined(__WINDOWS__)
#    define MagickDLLCall __stdcall
#  else
#    define MagickDLLCall
#  endif
#endif

typedef struct _GhostscriptVectors
{
  int
    (MagickDLLCall *exit)(gs_main_instance *);

  int
    (MagickDLLCall *init_with_args)(gs_main_instance *,int,char **);

  int
    (MagickDLLCall *new_instance)(gs_main_instance **,void *);

  int
    (MagickDLLCall *run_string)(gs_main_instance *,const char *,int,int *);

  void
    (MagickDLLCall *delete_instance)(gs_main_instance *);
} GhostscriptVectors;

typedef struct _DelegateInfo
{
  char
    *path,
    *decode,
    *encode,
    *commands;
                                                                                
  long
    mode;
                                                                                
  MagickBooleanType
    thread_support,
    spawn,
    stealth;
                                                                                
  struct _DelegateInfo
    *previous,
    *next;  /* deprecated, use GetDelegateInfoList() */

  unsigned long
    signature;
} DelegateInfo;

extern MagickExport char
  *GetDelegateCommand(const ImageInfo *,Image *,const char *,const char *,
    ExceptionInfo *),
  **GetDelegateList(const char *,unsigned long *,ExceptionInfo *);

extern MagickExport const char
  *GetDelegateCommands(const DelegateInfo *);

extern MagickExport const DelegateInfo
  *GetDelegateInfo(const char *,const char *,ExceptionInfo *exception),
  **GetDelegateInfoList(const char *,unsigned long *,ExceptionInfo *);

extern MagickExport long
  GetDelegateMode(const DelegateInfo *);

extern MagickExport MagickBooleanType
  GetDelegateThreadSupport(const DelegateInfo *),
  InvokeDelegate(ImageInfo *,Image *,const char *,const char *,ExceptionInfo *),
  ListDelegateInfo(FILE *,ExceptionInfo *);

extern MagickExport void
  DestroyDelegateList(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
