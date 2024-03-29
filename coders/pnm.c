/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            PPPP   N   N  M   M                              %
%                            P   P  NN  N  MM MM                              %
%                            PPPP   N N N  M M M                              %
%                            P      N  NN  M   M                              %
%                            P      N   N  M   M                              %
%                                                                             %
%                                                                             %
%               Read/Write PBMPlus Portable Anymap Image Format               %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright 1999-2009 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/cache.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/monitor-private.h"
#include "magick/pixel-private.h"
#include "magick/property.h"
#include "magick/quantum-private.h"
#include "magick/static.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/module.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WritePNMImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s P N M                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPNM() returns MagickTrue if the image format type, identified by the
%  magick string, is PNM.
%
%  The format of the IsPNM method is:
%
%      MagickBooleanType IsPNM(const unsigned char *magick,const size_t extent)
%
%  A description of each parameter follows:
%
%    o magick: compare image format pattern against these bytes.
%
%    o extent: Specifies the extent of the magick string.
%
*/
static MagickBooleanType IsPNM(const unsigned char *magick,const size_t extent)
{
  if (extent < 2)
    return(MagickFalse);
  if ((*magick == (unsigned char) 'P') &&
      ((magick[1] == '1') || (magick[1] == '2') || (magick[1] == '3') ||
       (magick[1] == '4') || (magick[1] == '5') || (magick[1] == '6') ||
       (magick[1] == '7') || (magick[1] == 'F') || (magick[1] == 'f')))
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P N M I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPNMImage() reads a Portable Anymap image file and returns it.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  The format of the ReadPNMImage method is:
%
%      Image *ReadPNMImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline long ConstrainPixel(Image *image,const long offset,
  const unsigned long extent)
{
  if ((offset < 0) || (offset > (long) extent))
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        CorruptImageError,"InvalidPixel","`%s'",image->filename);
      return(0);
    }
  return(offset);
}

static unsigned long PNMInteger(Image *image,const unsigned int base)
{
  char
    *comment;

  int
    c;

  register char
    *p;

  size_t
    extent;

  unsigned long
    value;

  /*
    Skip any leading whitespace.
  */
  extent=MaxTextExtent;
  comment=(char *) NULL;
  p=comment;
  do
  {
    c=ReadBlobByte(image);
    if (c == EOF)
      return(0);
    if (c == (int) '#')
      {
        /*
          Read comment.
        */
        if (comment == (char *) NULL)
          comment=AcquireString((char *) NULL);
        p=comment+strlen(comment);
        for ( ; (c != EOF) && (c != (int) '\n'); p++)
        {
          if ((size_t) (p-comment+1) >= extent)
            {
              extent<<=1;
              comment=(char *) ResizeQuantumMemory(comment,extent+MaxTextExtent,
                sizeof(*comment));
              if (comment == (char *) NULL)
                break;
              p=comment+strlen(comment);
            }
          c=ReadBlobByte(image);
          *p=(char) c;
          *(p+1)='\0';
        }
        if (comment == (char *) NULL)
          return(0);
        continue;
      }
  } while (isdigit(c) == MagickFalse);
  if (comment != (char *) NULL)
    {
      (void) SetImageProperty(image,"comment",comment);
      comment=DestroyString(comment);
    }
  if (base == 2)
    return((unsigned long) (c-(int) '0'));
  /*
    Evaluate number.
  */
  value=0;
  do
  {
    value*=10;
    value+=c-(int) '0';
    c=ReadBlobByte(image);
    if (c == EOF)
      return(value);
  } while (isdigit(c) != MagickFalse);
  return(value);
}

static Image *ReadPNMImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    format;

  double
    quantum_scale;

  Image
    *image;

  long
    row,
    y;

  MagickBooleanType
    status;

  Quantum
    *scale;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  register long
    i;

  size_t
    extent,
    packet_size;

  ssize_t
    count;

  unsigned long
    depth,
    max_value;

  CacheView
    *image_view;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AcquireImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Read PNM image.
  */
  count=ReadBlob(image,1,(unsigned char *) &format);
  do
  {
    /*
      Initialize image structure.
    */
    if ((count != 1) || (format != 'P'))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    max_value=1;
    quantum_type=RGBQuantum;
    quantum_scale=1.0;
    format=(char) ReadBlobByte(image);
    if (format != '7')
      {
        /*
          PBM, PGM, PPM, and PNM.
        */
        image->columns=PNMInteger(image,10);
        image->rows=PNMInteger(image,10);
        if ((format == 'f') || (format == 'F'))
          {
            char
              scale[MaxTextExtent];

            (void) ReadBlobString(image,scale);
            quantum_scale=atof(scale);
          }
        else
          {
            if ((format == '1') || (format == '4'))
              max_value=1;  /* bitmap */
            else
              max_value=PNMInteger(image,10);
          }
      }
    else
      {
        char
          keyword[MaxTextExtent],
          value[MaxTextExtent];

        int
          c;

        register char
          *p;

        /*
          PAM.
        */
        for (c=ReadBlobByte(image); c != EOF; c=ReadBlobByte(image))
        {
          while (isspace((int) ((unsigned char) c)) != 0)
            c=ReadBlobByte(image);
          p=keyword;
          do
          {
            if ((size_t) (p-keyword) < (MaxTextExtent-1))
              *p++=c;
            c=ReadBlobByte(image);
          } while (isalnum(c));
          *p='\0';
          if (LocaleCompare(keyword,"endhdr") == 0)
            break;
          while (isspace((int) ((unsigned char) c)) != 0)
            c=ReadBlobByte(image);
          p=value;
          while (isalnum(c) || (c == '_'))
          {
            if ((size_t) (p-value) < (MaxTextExtent-1))
              *p++=c;
            c=ReadBlobByte(image);
          }
          *p='\0';
          /*
            Assign a value to the specified keyword.
          */
          if (LocaleCompare(keyword,"depth") == 0)
            packet_size=(unsigned long) atol(value);
          if (LocaleCompare(keyword,"height") == 0)
            image->rows=(unsigned long) atol(value);
          if (LocaleCompare(keyword,"maxval") == 0)
            max_value=(unsigned long) atol(value);
          if (LocaleCompare(keyword,"TUPLTYPE") == 0)
            {
              if (LocaleCompare(value,"BLACKANDWHITE") == 0)
                quantum_type=GrayQuantum;
              if (LocaleCompare(value,"BLACKANDWHITE_ALPHA") == 0)
                {
                  quantum_type=GrayAlphaQuantum;
                  image->matte=MagickTrue;
                }
              if (LocaleCompare(value,"GRAYSCALE") == 0)
                quantum_type=GrayQuantum;
              if (LocaleCompare(value,"GRAYSCALE_ALPHA") == 0)
                {
                  quantum_type=GrayAlphaQuantum;
                  image->matte=MagickTrue;
                }
              if (LocaleCompare(value,"RGB_ALPHA") == 0)
                {
                  quantum_type=RGBAQuantum;
                  image->matte=MagickTrue;
                }
              if (LocaleCompare(value,"CMYK") == 0)
                {
                  quantum_type=CMYKQuantum;
                  image->colorspace=CMYKColorspace;
                }
              if (LocaleCompare(value,"CMYK_ALPHA") == 0)
                {
                  quantum_type=CMYKAQuantum;
                  image->colorspace=CMYKColorspace;
                  image->matte=MagickTrue;
                }
            }
          if (LocaleCompare(keyword,"width") == 0)
            image->columns=(unsigned long) atol(value);
        }
      }
    if ((image->columns == 0) || (image->rows == 0))
      ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
    if (max_value >= 65536)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    for (depth=1; GetQuantumRange(depth) < max_value; depth++) ;
    image->depth=depth;
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    /*
      Convert PNM pixels to runextent-encoded MIFF packets.
    */
    status=MagickTrue;
    row=0;
    switch (format)
    {
      case '1':
      {
        /*
          Convert PBM image to pixel packets.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          register long
            x;

          register PixelPacket
            *__restrict q;

          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            q->red=(Quantum) (PNMInteger(image,2) == 0 ? QuantumRange : 0);
            q->green=q->red;
            q->blue=q->red;
            q++;
          }
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        image->type=BilevelType;
        break;
      }
      case '2':
      {
        unsigned long
          intensity;

        /*
          Convert PGM image to pixel packets.
        */
        scale=(Quantum *) NULL;
        if (max_value != (1U*QuantumRange))
          {
            /*
              Compute pixel scaling table.
            */
            scale=(Quantum *) AcquireQuantumMemory((size_t) max_value+1UL,
              sizeof(*scale));
            if (scale == (Quantum *) NULL)
              ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
            for (i=0; i <= (long) max_value; i++)
              scale[i]=(Quantum) (((double) QuantumRange*i)/max_value+0.5);
          }
        for (y=0; y < (long) image->rows; y++)
        {
          register long
            x;

          register PixelPacket
            *__restrict q;

          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            intensity=PNMInteger(image,10);
            if (scale != (Quantum *) NULL)
              intensity=(unsigned long) scale[ConstrainPixel(image,(long)
                intensity,max_value)];
            q->red=(Quantum) intensity;
            q->green=q->red;
            q->blue=q->red;
            q++;
          }
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        image->type=GrayscaleType;
        if (scale != (Quantum *) NULL)
          scale=(Quantum *) RelinquishMagickMemory(scale);
        break;
      }
      case '3':
      {
        MagickPixelPacket
          pixel;

        /*
          Convert PNM image to pixel packets.
        */
        scale=(Quantum *) NULL;
        if (max_value != (1U*QuantumRange))
          {
            /*
              Compute pixel scaling table.
            */
            scale=(Quantum *) AcquireQuantumMemory((size_t) max_value+1UL,
              sizeof(*scale));
            if (scale == (Quantum *) NULL)
              ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
            for (i=0; i <= (long) max_value; i++)
              scale[i]=(Quantum) (((double) QuantumRange*i)/max_value+0.5);
          }
        for (y=0; y < (long) image->rows; y++)
        {
          register long
            x;

          register PixelPacket
            *__restrict q;

          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            pixel.red=(MagickRealType) PNMInteger(image,10);
            pixel.green=(MagickRealType) PNMInteger(image,10);
            pixel.blue=(MagickRealType) PNMInteger(image,10);
            if (scale != (Quantum *) NULL)
              {
                pixel.red=(MagickRealType) scale[ConstrainPixel(image,(long)
                  pixel.red,max_value)];
                pixel.green=(MagickRealType) scale[ConstrainPixel(image,(long)
                  pixel.green,max_value)];
                pixel.blue=(MagickRealType) scale[ConstrainPixel(image,(long)
                  pixel.blue,max_value)];
              }
            q->red=(Quantum) pixel.red;
            q->green=(Quantum) pixel.green;
            q->blue=(Quantum) pixel.blue;
            q++;
          }
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        if (scale != (Quantum *) NULL)
          scale=(Quantum *) RelinquishMagickMemory(scale);
        break;
      }
      case '4':
      {
        /*
          Convert PBM raw image to pixel packets.
        */
        quantum_type=GrayQuantum;
        if (image->storage_class == PseudoClass)
          quantum_type=IndexQuantum;
        quantum_info=AcquireQuantumInfo(image_info,image);
        if (quantum_info == (QuantumInfo *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        SetQuantumMinIsWhite(quantum_info,MagickTrue);
        extent=GetQuantumExtent(image,quantum_info,quantum_type);
        image_view=AcquireCacheView(image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,1) shared(row,status,quantum_type)
#endif
        for (y=0; y < (long) image->rows; y++)
        {
          long
            offset;

          MagickBooleanType
            sync;

          register PixelPacket
            *__restrict q;

          ssize_t
            count;

          size_t
            length;

          unsigned char
            *pixels;

          if (status == MagickFalse)
            continue;
          pixels=GetQuantumPixels(quantum_info);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
          #pragma omp critical (MagickCore_ReadPNMImage)
#endif
          {
            count=ReadBlob(image,extent,pixels);
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (image->previous == (Image *) NULL))
              {
                MagickBooleanType
                  proceed;

                proceed=SetImageProgress(image,LoadImageTag,row,image->rows);
                if (proceed == MagickFalse)
                  status=MagickFalse;
              }
            offset=row++;
          }
          if (count != (ssize_t) extent)
            status=MagickFalse;
          q=QueueCacheViewAuthenticPixels(image_view,0,offset,image->columns,1,
            exception);
          if (q == (PixelPacket *) NULL)
            {
              status=MagickFalse;
              continue;
            }
          length=ImportQuantumPixels(image,image_view,quantum_info,quantum_type,
            pixels,exception);
          if (length != extent)
            status=MagickFalse;
          sync=SyncCacheViewAuthenticPixels(image_view,exception);
          if (sync == MagickFalse)
            status=MagickFalse;
        }
        image_view=DestroyCacheView(image_view);
        quantum_info=DestroyQuantumInfo(quantum_info);
        if (status == MagickFalse)
          ThrowReaderException(CorruptImageError,"UnableToReadImageData");
        SetQuantumImageType(image,quantum_type);
        break;
      }
      case '5':
      {
        QuantumAny
          range;

        /*
          Convert PGM raw image to pixel packets.
        */
        range=GetQuantumRange(image->depth);
        quantum_type=GrayQuantum;
        extent=(image->depth <= 8 ? 1 : 2)*image->columns;
        quantum_info=AcquireQuantumInfo(image_info,image);
        if (quantum_info == (QuantumInfo *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        image_view=AcquireCacheView(image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,1) shared(row,status,quantum_type)
#endif
        for (y=0; y < (long) image->rows; y++)
        {
          long
            offset;

          MagickBooleanType
            sync;

          register const unsigned char
            *p;

          register long
            x;

          register PixelPacket
            *__restrict q;

          ssize_t
            count;

          unsigned char
            *pixels;

          if (status == MagickFalse)
            continue;
          pixels=GetQuantumPixels(quantum_info);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
          #pragma omp critical (MagickCore_ReadPNMImage)
#endif
          {
            count=ReadBlob(image,extent,pixels);
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (image->previous == (Image *) NULL))
              {
                MagickBooleanType
                  proceed;

                proceed=SetImageProgress(image,LoadImageTag,row,image->rows);
                if (proceed == MagickFalse)
                  status=MagickFalse;
              }
            offset=row++;
          }
          if (count != (ssize_t) extent)
            status=MagickFalse;
          q=QueueCacheViewAuthenticPixels(image_view,0,offset,image->columns,1,
            exception);
          if (q == (PixelPacket *) NULL)
            {
              status=MagickFalse;
              continue;
            }
          p=pixels;
          if ((image->depth == 8) || (image->depth == 16))
            (void) ImportQuantumPixels(image,image_view,quantum_info,
              quantum_type,pixels,exception);
          else
            if (image->depth <= 8)
              {
                unsigned char
                  pixel;

                for (x=0; x < (long) image->columns; x++)
                {
                  p=PushCharPixel(p,&pixel);
                  q->red=ScaleAnyToQuantum(pixel,range);
                  q->green=q->red;
                  q->blue=q->red;
                  q++;
                }
              }
            else
              {
                unsigned short
                  pixel;

                for (x=0; x < (long) image->columns; x++)
                {
                  p=PushShortPixel(MSBEndian,p,&pixel);
                  q->red=ScaleAnyToQuantum(pixel,range);
                  q->green=q->red;
                  q->blue=q->red;
                  q++;
                }
              }
          sync=SyncCacheViewAuthenticPixels(image_view,exception);
          if (sync == MagickFalse)
            status=MagickFalse;
        }
        image_view=DestroyCacheView(image_view);
        quantum_info=DestroyQuantumInfo(quantum_info);
        if (status == MagickFalse)
          ThrowReaderException(CorruptImageError,"UnableToReadImageData");
        SetQuantumImageType(image,quantum_type);
        break;
      }
      case '6':
      {
        ImageType
          type;

        QuantumAny
          range;

        /*
          Convert PNM raster image to pixel packets.
        */
        type=BilevelType;
        quantum_type=RGBQuantum;
        extent=3*(image->depth <= 8 ? 1 : 2)*image->columns;
        range=GetQuantumRange(image->depth);
        quantum_info=AcquireQuantumInfo(image_info,image);
        if (quantum_info == (QuantumInfo *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        image_view=AcquireCacheView(image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,1) shared(row,status,type)
#endif
        for (y=0; y < (long) image->rows; y++)
        {
          long
            offset;

          MagickBooleanType
            sync;

          register const unsigned char
            *p;

          register long
            x;

          register PixelPacket
            *__restrict q;

          ssize_t
            count;

          size_t
            length;

          unsigned char
            *pixels;

          if (status == MagickFalse)
            continue;
          pixels=GetQuantumPixels(quantum_info);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
          #pragma omp critical (MagickCore_ReadPNMImage)
#endif
          {
            count=ReadBlob(image,extent,pixels);
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (image->previous == (Image *) NULL))
              {
                MagickBooleanType
                  proceed;

                proceed=SetImageProgress(image,LoadImageTag,row,image->rows);
                if (proceed == MagickFalse)
                  status=MagickFalse;
              }
            offset=row++;
          }
          if (count != (ssize_t) extent)
            status=MagickFalse;
          q=QueueCacheViewAuthenticPixels(image_view,0,offset,image->columns,1,
            exception);
          if (q == (PixelPacket *) NULL)
            {
              status=MagickFalse;
              continue;
            }
          p=pixels;
          if ((image->depth == 8) || (image->depth == 16))
            {
              length=ImportQuantumPixels(image,image_view,quantum_info,
                quantum_type,pixels,exception);
              if (length != extent)
                status=MagickFalse;
            }
          else
            if (image->depth <= 8)
              {
                unsigned char
                  pixel;

                register PixelPacket
                  *__restrict r;

                r=q;
                for (x=0; x < (long) image->columns; x++)
                {
                  p=PushCharPixel(p,&pixel);
                  r->red=ScaleAnyToQuantum(pixel,range);
                  p=PushCharPixel(p,&pixel);
                  r->green=ScaleAnyToQuantum(pixel,range);
                  p=PushCharPixel(p,&pixel);
                  r->blue=ScaleAnyToQuantum(pixel,range);
                  r++;
                }
              }
            else
              {
                unsigned short
                  pixel;

                register PixelPacket
                  *__restrict r;

                r=q;
                for (x=0; x < (long) image->columns; x++)
                {
                  p=PushShortPixel(MSBEndian,p,&pixel);
                  r->red=ScaleAnyToQuantum(pixel,range);
                  p=PushShortPixel(MSBEndian,p,&pixel);
                  r->green=ScaleAnyToQuantum(pixel,range);
                  p=PushShortPixel(MSBEndian,p,&pixel);
                  r->blue=ScaleAnyToQuantum(pixel,range);
                  r++;
                }
              }
          if ((type == BilevelType) || (type == GrayscaleType))
            for (x=0; x < (long) image->columns; x++)
            {
              if ((type == BilevelType) &&
                  (IsMonochromePixel(q) == MagickFalse))
                type=IsGrayPixel(q) == MagickFalse ? UndefinedType :
                  GrayscaleType;
              if ((type == GrayscaleType) && (IsGrayPixel(q) == MagickFalse))
                type=UndefinedType;
              if ((type != BilevelType) && (type != GrayscaleType))
                break;
              q++;
            }
          sync=SyncCacheViewAuthenticPixels(image_view,exception);
          if (sync == MagickFalse)
            status=MagickFalse;
        }
        image_view=DestroyCacheView(image_view);
        quantum_info=DestroyQuantumInfo(quantum_info);
        if (status == MagickFalse)
          ThrowReaderException(CorruptImageError,"UnableToReadImageData");
        if (type != UndefinedType)
          image->type=type;
        break;
      }
      case '7':
      {
        register IndexPacket
          *indexes;

        QuantumAny
          range;

        unsigned long
          channels;

        /*
          Convert PAM raster image to pixel packets.
        */
        range=GetQuantumRange(image->depth);
        switch (quantum_type)
        {
          case GrayQuantum:
          case GrayAlphaQuantum:
          {
            channels=1;
            break;
          }
          case CMYKQuantum:
          case CMYKAQuantum:
          {
            channels=4;
            break;
          }
          default:
          {
            channels=3;
            break;
          }
        }
        if (image->matte != MagickFalse)
          channels++;
        extent=channels*(image->depth <= 8 ? 1 : 2)*image->columns;
        quantum_info=AcquireQuantumInfo(image_info,image);
        if (quantum_info == (QuantumInfo *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        image_view=AcquireCacheView(image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,1) shared(row,status,quantum_type)
#endif
        for (y=0; y < (long) image->rows; y++)
        {
          long
            offset;

          MagickBooleanType
            sync;

          register const unsigned char
            *p;

          register long
            x;

          register PixelPacket
            *__restrict q;

          ssize_t
            count;

          unsigned char
            *pixels;

          if (status == MagickFalse)
            continue;
          pixels=GetQuantumPixels(quantum_info);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
          #pragma omp critical (MagickCore_ReadPNMImage)
#endif
          {
            count=ReadBlob(image,extent,pixels);
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (image->previous == (Image *) NULL))
              {
                MagickBooleanType
                  proceed;

                proceed=SetImageProgress(image,LoadImageTag,row,image->rows);
                if (proceed == MagickFalse)
                  status=MagickFalse;
              }
            offset=row++;
          }
          if (count != (ssize_t) extent)
            status=MagickFalse;
          q=QueueCacheViewAuthenticPixels(image_view,0,offset,image->columns,1,
            exception);
          if (q == (PixelPacket *) NULL)
            {
              status=MagickFalse;
              continue;
            }
          indexes=GetCacheViewAuthenticIndexQueue(image_view);
          p=pixels;
          if ((image->depth == 8) || (image->depth == 16))
            (void) ImportQuantumPixels(image,image_view,quantum_info,
              quantum_type,pixels,exception);
          else
            switch (quantum_type)
            {
              case GrayQuantum:
              case GrayAlphaQuantum:
              {
                if (image->depth <= 8)
                  {
                    unsigned char
                      pixel;

                    for (x=0; x < (long) image->columns; x++)
                    {
                      p=PushCharPixel(p,&pixel);
                      q->red=ScaleAnyToQuantum(pixel,range);
                      q->green=q->red;
                      q->blue=q->red;
                      q->opacity=OpaqueOpacity;
                      if (image->matte != MagickFalse)
                        {
                          p=PushCharPixel(p,&pixel);
                          q->opacity=ScaleAnyToQuantum(pixel,range);
                        }
                      q++;
                    }
                  }
                else
                  {
                    unsigned short
                      pixel;

                    for (x=0; x < (long) image->columns; x++)
                    {
                      p=PushShortPixel(MSBEndian,p,&pixel);
                      q->red=ScaleAnyToQuantum(pixel,range);
                      q->green=q->red;
                      q->blue=q->red;
                      q->opacity=OpaqueOpacity;
                      if (image->matte != MagickFalse)
                        {
                          p=PushShortPixel(MSBEndian,p,&pixel);
                          q->opacity=ScaleAnyToQuantum(pixel,range);
                        }
                      q++;
                    }
                  }
                break;
              }
              case CMYKQuantum:
              case CMYKAQuantum:
              {
                if (image->depth <= 8)
                  {
                    unsigned char
                      pixel;

                    for (x=0; x < (long) image->columns; x++)
                    {
                      p=PushCharPixel(p,&pixel);
                      q->red=ScaleAnyToQuantum(pixel,range);
                      p=PushCharPixel(p,&pixel);
                      q->green=ScaleAnyToQuantum(pixel,range);
                      p=PushCharPixel(p,&pixel);
                      q->blue=ScaleAnyToQuantum(pixel,range);
                      p=PushCharPixel(p,&pixel);
                      indexes[x]=ScaleAnyToQuantum(pixel,range);
                      q->opacity=OpaqueOpacity;
                      if (image->matte != MagickFalse)
                        {
                          p=PushCharPixel(p,&pixel);
                          q->opacity=ScaleAnyToQuantum(pixel,range);
                        }
                      q++;
                    }
                  }
                else
                  {
                    unsigned short
                      pixel;

                    for (x=0; x < (long) image->columns; x++)
                    {
                      p=PushShortPixel(MSBEndian,p,&pixel);
                      q->red=ScaleAnyToQuantum(pixel,range);
                      p=PushShortPixel(MSBEndian,p,&pixel);
                      q->green=ScaleAnyToQuantum(pixel,range);
                      p=PushShortPixel(MSBEndian,p,&pixel);
                      q->blue=ScaleAnyToQuantum(pixel,range);
                      p=PushShortPixel(MSBEndian,p,&pixel);
                      indexes[x]=ScaleAnyToQuantum(pixel,range);
                      q->opacity=OpaqueOpacity;
                      if (image->matte != MagickFalse)
                        {
                          p=PushShortPixel(MSBEndian,p,&pixel);
                          q->opacity=ScaleAnyToQuantum(pixel,range);
                        }
                      q++;
                    }
                  }
                break;
              }
              default:
              {
                if (image->depth <= 8)
                  {
                    unsigned char
                      pixel;

                    for (x=0; x < (long) image->columns; x++)
                    {
                      p=PushCharPixel(p,&pixel);
                      q->red=ScaleAnyToQuantum(pixel,range);
                      p=PushCharPixel(p,&pixel);
                      q->green=ScaleAnyToQuantum(pixel,range);
                      p=PushCharPixel(p,&pixel);
                      q->blue=ScaleAnyToQuantum(pixel,range);
                      q->opacity=OpaqueOpacity;
                      if (image->matte != MagickFalse)
                        {
                          p=PushCharPixel(p,&pixel);
                          q->opacity=ScaleAnyToQuantum(pixel,range);
                        }
                      q++;
                    }
                  }
                else
                  {
                    unsigned short
                      pixel;

                    for (x=0; x < (long) image->columns; x++)
                    {
                      p=PushShortPixel(MSBEndian,p,&pixel);
                      q->red=ScaleAnyToQuantum(pixel,range);
                      p=PushShortPixel(MSBEndian,p,&pixel);
                      q->green=ScaleAnyToQuantum(pixel,range);
                      p=PushShortPixel(MSBEndian,p,&pixel);
                      q->blue=ScaleAnyToQuantum(pixel,range);
                      q->opacity=OpaqueOpacity;
                      if (image->matte != MagickFalse)
                        {
                          p=PushShortPixel(MSBEndian,p,&pixel);
                          q->opacity=ScaleAnyToQuantum(pixel,range);
                        }
                      q++;
                    }
                  }
                break;
              }
            }
          sync=SyncCacheViewAuthenticPixels(image_view,exception);
          if (sync == MagickFalse)
            status=MagickFalse;
        }
        image_view=DestroyCacheView(image_view);
        quantum_info=DestroyQuantumInfo(quantum_info);
        if (status == MagickFalse)
          ThrowReaderException(CorruptImageError,"UnableToReadImageData");
        SetQuantumImageType(image,quantum_type);
        break;
      }
      case 'F':
      case 'f':
      {
        /*
          Convert PFM raster image to pixel packets.
        */
        quantum_type=format == 'f' ? GrayQuantum : RGBQuantum;
        image->endian=quantum_scale < 0.0 ? LSBEndian : MSBEndian;
        image->depth=32;
        quantum_info=AcquireQuantumInfo(image_info,image);
        if (quantum_info == (QuantumInfo *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        status=SetQuantumDepth(image,quantum_info,32);
        if (status == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        status=SetQuantumFormat(image,quantum_info,FloatingPointQuantumFormat);
        if (status == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        SetQuantumScale(quantum_info,(MagickRealType) QuantumRange*
          fabs(quantum_scale));
        extent=GetQuantumExtent(image,quantum_info,quantum_type);
        image_view=AcquireCacheView(image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,1) shared(row,status,quantum_type)
#endif
        for (y=0; y < (long) image->rows; y++)
        {
          long
            offset;

          MagickBooleanType
            sync;

          register PixelPacket
            *__restrict q;

          ssize_t
            count;

          size_t
            length;

          unsigned char
            *pixels;

          if (status == MagickFalse)
            continue;
          pixels=GetQuantumPixels(quantum_info);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
          #pragma omp critical (MagickCore_ReadPNMImage)
#endif
          {
            count=ReadBlob(image,extent,pixels);
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (image->previous == (Image *) NULL))
              {
                MagickBooleanType
                  proceed;

                proceed=SetImageProgress(image,LoadImageTag,row,image->rows);
                if (proceed == MagickFalse)
                  status=MagickFalse;
              }
            offset=row++;
          }
          if ((size_t) count != extent)
            status=MagickFalse;
          q=QueueCacheViewAuthenticPixels(image_view,0,(long) (image->rows-
            offset-1),image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            {
              status=MagickFalse;
              continue;
            }
          length=ImportQuantumPixels(image,image_view,quantum_info,quantum_type,
            pixels,exception);
          if (length != extent)
            status=MagickFalse;
          sync=SyncCacheViewAuthenticPixels(image_view,exception);
          if (sync == MagickFalse)
            status=MagickFalse;
        }
        image_view=DestroyCacheView(image_view);
        quantum_info=DestroyQuantumInfo(quantum_info);
        if (status == MagickFalse)
          ThrowReaderException(CorruptImageError,"UnableToReadImageData");
        SetQuantumImageType(image,quantum_type);
        break;
      }
      default:
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    }
    if (EOFBlob(image) != MagickFalse)
      (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
        "UnexpectedEndOfFile","`%s'",image->filename);
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if ((format == '1') || (format == '2') || (format == '3'))
      do
      {
        /*
          Skip to end of line.
        */
        count=ReadBlob(image,1,(unsigned char *) &format);
        if (count == 0)
          break;
        if ((count != 0) && (format == 'P'))
          break;
      } while (format != '\n');
    count=ReadBlob(image,1,(unsigned char *) &format);
    if ((count == 1) && (format == 'P'))
      {
        /*
          Allocate next image structure.
        */
        AcquireNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
        status=SetImageProgress(image,LoadImagesTag,TellBlob(image),
          GetBlobSize(image));
        if (status == MagickFalse)
          break;
      }
  } while ((count == 1) && (format == 'P'));
  (void) CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P N M I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPNMImage() adds properties for the PNM image format to
%  the list of supported formats.  The properties include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPNMImage method is:
%
%      unsigned long RegisterPNMImage(void)
%
*/
ModuleExport unsigned long RegisterPNMImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("PAM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->description=ConstantString("Common 2-dimensional bitmap format");
  entry->module=ConstantString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PBM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->description=ConstantString("Portable bitmap format (black and white)");
  entry->module=ConstantString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PFM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->description=ConstantString("Portable float format");
  entry->module=ConstantString("PFM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PGM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->description=ConstantString("Portable graymap format (gray scale)");
  entry->module=ConstantString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PNM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->magick=(IsImageFormatHandler *) IsPNM;
  entry->description=ConstantString("Portable anymap");
  entry->module=ConstantString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PPM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->description=ConstantString("Portable pixmap format (color)");
  entry->module=ConstantString("PNM");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P N M I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPNMImage() removes format registrations made by the
%  PNM module from the list of supported formats.
%
%  The format of the UnregisterPNMImage method is:
%
%      UnregisterPNMImage(void)
%
*/
ModuleExport void UnregisterPNMImage(void)
{
  (void) UnregisterMagickInfo("PAM");
  (void) UnregisterMagickInfo("PBM");
  (void) UnregisterMagickInfo("PGM");
  (void) UnregisterMagickInfo("PNM");
  (void) UnregisterMagickInfo("PPM");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P N M I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Procedure WritePNMImage() writes an image to a file in the PNM rasterfile
%  format.
%
%  The format of the WritePNMImage method is:
%
%      MagickBooleanType WritePNMImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: the image info.
%
%    o image:  The image.
%
*/
static MagickBooleanType WritePNMImage(const ImageInfo *image_info,Image *image)
{
  char
    buffer[MaxTextExtent],
    format,
    magick[MaxTextExtent];

  const char
    *value;

  IndexPacket
    index;

  long
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  QuantumAny
    pixel;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  register long
    i;

  register unsigned char
    *pixels,
    *q;

  ssize_t
    count;

  size_t
    extent,
    packet_size;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  scene=0;
  do
  {
    /*
      Write PNM file header.
    */
    packet_size=3;
    quantum_type=RGBQuantum;
    (void) CopyMagickString(magick,image_info->magick,MaxTextExtent);
    switch (magick[1])
    {
      case 'A':
      case 'a':
      {
        format='7';
        break;
      }
      case 'B':
      case 'b':
      {
        format='4';
        if (image_info->compression == NoCompression)
          format='1';
        break;
      }
      case 'F':
      case 'f':
      {
        format='F';
        if (IsGrayImage(image,&image->exception) != MagickFalse)
          format='f';
        break;
      }
      case 'G':
      case 'g':
      {
        format='5';
        if (image_info->compression == NoCompression)
          format='2';
        break;
      }
      case 'N':
      case 'n':
      {
        if ((image_info->type != TrueColorType) &&
            (IsGrayImage(image,&image->exception) != MagickFalse))
          {
            format='5';
            if (image_info->compression == NoCompression)
              format='2';
            if (IsMonochromeImage(image,&image->exception) != MagickFalse)
              {
                format='4';
                if (image_info->compression == NoCompression)
                  format='1';
              }
            break;
          }
      }
      default:
      {
        format='6';
        if (image_info->compression == NoCompression)
          format='3';
        break;
      }
    }
    (void) FormatMagickString(buffer,MaxTextExtent,"P%c\n",format);
    (void) WriteBlobString(image,buffer);
    value=GetImageProperty(image,"comment");
    if (value != (const char *) NULL)
      {
        register const char
          *p;

        /*
          Write comments to file.
        */
        (void) WriteBlobByte(image,'#');
        for (p=value; *p != '\0'; p++)
        {
          (void) WriteBlobByte(image,(unsigned char) *p);
          if ((*p == '\r') && (*(p+1) != '\0'))
            (void) WriteBlobByte(image,'#');
          if ((*p == '\n') && (*(p+1) != '\0'))
            (void) WriteBlobByte(image,'#');
        }
        (void) WriteBlobByte(image,'\n');
      }
    if (format != '7')
      {
        if (image->colorspace != RGBColorspace)
          (void) TransformImageColorspace(image,RGBColorspace);
        (void) FormatMagickString(buffer,MaxTextExtent,"%lu %lu\n",
          image->columns,image->rows);
        (void) WriteBlobString(image,buffer);
      }
    else
      {
        char
          type[MaxTextExtent];

        /*
          PAM header.
        */
        (void) FormatMagickString(buffer,MaxTextExtent,
          "WIDTH %lu\nHEIGHT %lu\n",image->columns,image->rows);
        (void) WriteBlobString(image,buffer);
        quantum_type=GetQuantumType(image,&image->exception);
        switch (quantum_type)
        {
          case CMYKQuantum:
          case CMYKAQuantum:
          {
            packet_size=4;
            (void) CopyMagickString(type,"CMYK",MaxTextExtent);
            break;
          }
          case GrayQuantum:
          case GrayAlphaQuantum:
          {
            packet_size=1;
            (void) CopyMagickString(type,"GRAYSCALE",MaxTextExtent);
            break;
          }
          default:
          {
            quantum_type=RGBQuantum;
            if (image->matte != MagickFalse)
              quantum_type=RGBAQuantum;
            packet_size=3;
            (void) CopyMagickString(type,"RGB",MaxTextExtent);
            break;
          }
        }
        if (image->matte != MagickFalse)
          {
            packet_size++;
            (void) ConcatenateMagickString(type,"_ALPHA",MaxTextExtent);
          }
        if (image->depth > 16)
          image->depth=16;
        (void) FormatMagickString(buffer,MaxTextExtent,
          "DEPTH %lu\nMAXVAL %lu\n",(unsigned long) packet_size,(unsigned long)
          GetQuantumRange(image->depth));
        (void) WriteBlobString(image,buffer);
        (void) FormatMagickString(buffer,MaxTextExtent,"TUPLTYPE %s\nENDHDR\n",
          type);
        (void) WriteBlobString(image,buffer);
      }
    /*
      Convert runextent encoded to PNM raster pixels.
    */
    switch (format)
    {
      case '1':
      {
        unsigned char
          pixels[2048];

        /*
          Convert image to a PBM image.
        */
        q=pixels;
        for (y=0; y < (long) image->rows; y++)
        {
          register const IndexPacket
            *__restrict indexes;

          register const PixelPacket
            *__restrict p;

          register long
            x;

          p=GetVirtualPixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          indexes=GetVirtualIndexQueue(image);
          for (x=0; x < (long) image->columns; x++)
          {
            pixel=PixelIntensityToQuantum(p);
            *q++=(unsigned char) (pixel >= (Quantum) (QuantumRange/2) ?
              '0' : '1');
            *q++=' ';
            if ((q-pixels+2) >= 80)
              {
                *q++='\n';
                (void) WriteBlob(image,q-pixels,pixels);
                q=pixels;
                i=0;
              }
            p++;
          }
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        if (q != pixels)
          {
            *q++='\n';
            (void) WriteBlob(image,q-pixels,pixels);
          }
        break;
      }
      case '2':
      {
        unsigned char
          pixels[2048];

        /*
          Convert image to a PGM image.
        */
        if (image->depth <= 8)
          (void) WriteBlobString(image,"255\n");
        else
          (void) WriteBlobString(image,"65535\n");
        q=pixels;
        for (y=0; y < (long) image->rows; y++)
        {
          register const PixelPacket
            *__restrict p;

          register long
            x;

          p=GetVirtualPixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            index=PixelIntensityToQuantum(p);
            if (image->depth <= 8)
              count=(ssize_t) FormatMagickString(buffer,MaxTextExtent,"%u ",
                ScaleQuantumToChar(index));
            else
              count=(ssize_t) FormatMagickString(buffer,MaxTextExtent,"%u ",
                ScaleQuantumToShort(index));
            extent=(size_t) count;
            (void) strncpy((char *) q,buffer,extent);
            q+=extent;
            if ((q-pixels+extent) >= 80)
              {
                *q++='\n';
                (void) WriteBlob(image,q-pixels,pixels);
                q=pixels;
              }
            p++;
          }
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        if (q != pixels)
          {
            *q++='\n';
            (void) WriteBlob(image,q-pixels,pixels);
          }
        break;
      }
      case '3':
      {
        unsigned char
          pixels[2048];

        /*
          Convert image to a PNM image.
        */
        if (image->depth <= 8)
          (void) WriteBlobString(image,"255\n");
        else
          (void) WriteBlobString(image,"65535\n");
        q=pixels;
        for (y=0; y < (long) image->rows; y++)
        {
          register const PixelPacket
            *__restrict p;

          register long
            x;

          p=GetVirtualPixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            if (image->depth <= 8)
              count=(ssize_t) FormatMagickString(buffer,MaxTextExtent,
                "%u %u %u ",ScaleQuantumToChar(p->red),
                ScaleQuantumToChar(p->green),ScaleQuantumToChar(p->blue));
            else
              count=(ssize_t) FormatMagickString(buffer,MaxTextExtent,
                "%u %u %u ",ScaleQuantumToShort(p->red),
                ScaleQuantumToShort(p->green),ScaleQuantumToShort(p->blue));
            extent=(size_t) count;
            (void) strncpy((char *) q,buffer,extent);
            q+=extent;
            if ((q-pixels+extent) >= 80)
              {
                *q++='\n';
                (void) WriteBlob(image,q-pixels,pixels);
                q=pixels;
              }
            p++;
          }
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        if (q != pixels)
          {
            *q++='\n';
            (void) WriteBlob(image,q-pixels,pixels);
          }
        break;
      }
      case '4':
      {
        /*
          Convert image to a PBM image.
        */
        image->depth=1;
        quantum_info=AcquireQuantumInfo((const ImageInfo *) NULL,image);
        if (quantum_info == (QuantumInfo *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        quantum_info->min_is_white=MagickTrue;
        pixels=GetQuantumPixels(quantum_info);
        for (y=0; y < (long) image->rows; y++)
        {
          register const PixelPacket
            *__restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          extent=ExportQuantumPixels(image,(const CacheView *) NULL,
            quantum_info,GrayQuantum,pixels,&image->exception);
          count=WriteBlob(image,extent,pixels);
          if (count != (ssize_t) extent)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        quantum_info=DestroyQuantumInfo(quantum_info);
        break;
      }
      case '5':
      {
        QuantumAny
          range;

        /*
          Convert image to a PGM image.
        */
        if (image->depth > 8)
          image->depth=16;
        (void) FormatMagickString(buffer,MaxTextExtent,"%lu\n",(unsigned long)
          GetQuantumRange(image->depth));
        (void) WriteBlobString(image,buffer);
        quantum_info=AcquireQuantumInfo((const ImageInfo *) NULL,image);
        if (quantum_info == (QuantumInfo *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        quantum_info->min_is_white=MagickTrue;
        pixels=GetQuantumPixels(quantum_info);
        extent=GetQuantumExtent(image,quantum_info,GrayQuantum);
        range=GetQuantumRange(image->depth);
        for (y=0; y < (long) image->rows; y++)
        {
          register const PixelPacket
            *__restrict p;

          register long
            x;

          p=GetVirtualPixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          q=pixels;
          if ((image->depth == 8) || (image->depth == 16))
            extent=ExportQuantumPixels(image,(const CacheView *) NULL,
              quantum_info,GrayQuantum,pixels,&image->exception);
          else
            {
              if (image->depth <= 8)
                for (x=0; x < (long) image->columns; x++)
                {
                  if (IsGrayPixel(p) == MagickFalse)
                    pixel=ScaleQuantumToAny(PixelIntensityToQuantum(p),range);
                  else
                    {
                      if (image->depth == 8)
                        pixel=ScaleQuantumToChar(p->red);
                      else
                        pixel=ScaleQuantumToAny(p->red,range);
                    }
                  q=PopCharPixel((unsigned char) pixel,q);
                  p++;
                }
              else
                for (x=0; x < (long) image->columns; x++)
                {
                  if (IsGrayPixel(p) == MagickFalse)
                    pixel=ScaleQuantumToAny(PixelIntensityToQuantum(p),range);
                  else
                    {
                      if (image->depth == 16)
                        pixel=ScaleQuantumToShort(p->red);
                      else
                        pixel=ScaleQuantumToAny(p->red,range);
                    }
                  q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                  p++;
                }
              extent=(size_t) (q-pixels);
            }
          count=WriteBlob(image,extent,pixels);
          if (count != (ssize_t) extent)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        quantum_info=DestroyQuantumInfo(quantum_info);
        break;
      }
      case '6':
      {
        QuantumAny
          range;

        /*
          Convert image to a PNM image.
        */
        if (image->depth > 8)
          image->depth=16;
        (void) FormatMagickString(buffer,MaxTextExtent,"%lu\n",(unsigned long)
          GetQuantumRange(image->depth));
        (void) WriteBlobString(image,buffer);
        quantum_info=AcquireQuantumInfo((const ImageInfo *) NULL,image);
        if (quantum_info == (QuantumInfo *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        pixels=GetQuantumPixels(quantum_info);
        extent=GetQuantumExtent(image,quantum_info,quantum_type);
        range=GetQuantumRange(image->depth);
        for (y=0; y < (long) image->rows; y++)
        {
          register const PixelPacket
            *__restrict p;

          register long
            x;

          p=GetVirtualPixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          q=pixels;
          if ((image->depth == 8) || (image->depth == 16))
            extent=ExportQuantumPixels(image,(const CacheView *) NULL,
              quantum_info,quantum_type,pixels,&image->exception);
          else
            {
              if (image->depth <= 8)
                for (x=0; x < (long) image->columns; x++)
                {
                  pixel=ScaleQuantumToAny(p->red,range);
                  q=PopCharPixel((unsigned char) pixel,q);
                  pixel=ScaleQuantumToAny(p->green,range);
                  q=PopCharPixel((unsigned char) pixel,q);
                  pixel=ScaleQuantumToAny(p->blue,range);
                  q=PopCharPixel((unsigned char) pixel,q);
                  if (image->matte != MagickFalse)
                    {
                      pixel=ScaleQuantumToAny((Quantum) (QuantumRange-
                        p->opacity),range);
                      q=PopCharPixel((unsigned char) pixel,q);
                    }
                  p++;
                }
              else
                for (x=0; x < (long) image->columns; x++)
                {
                  pixel=ScaleQuantumToAny(p->red,range);
                  q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                  pixel=ScaleQuantumToAny(p->green,range);
                  q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                  pixel=ScaleQuantumToAny(p->blue,range);
                  q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                  if (image->matte != MagickFalse)
                    {
                      pixel=ScaleQuantumToAny((Quantum) (QuantumRange-
                        p->opacity),range);
                      q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                    }
                  p++;
                }
              extent=(size_t) (q-pixels);
            }
          count=WriteBlob(image,extent,pixels);
          if (count != (ssize_t) extent)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        quantum_info=DestroyQuantumInfo(quantum_info);
        break;
      }
      case '7':
      {
        QuantumAny
          range;

        /*
          Convert image to a PAM.
        */
        if (image->depth > 16)
          image->depth=16;
        quantum_info=AcquireQuantumInfo((const ImageInfo *) NULL,image);
        pixels=GetQuantumPixels(quantum_info);
        range=GetQuantumRange(image->depth);
        for (y=0; y < (long) image->rows; y++)
        {
          register const IndexPacket
            *__restrict indexes;

          register const PixelPacket
            *__restrict p;

          register long
            x;

          p=GetVirtualPixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          indexes=GetVirtualIndexQueue(image);
          q=pixels;
          if ((image->depth == 8) || (image->depth == 16))
            extent=ExportQuantumPixels(image,(const CacheView *) NULL,
              quantum_info,quantum_type,pixels,&image->exception);
          else
            {
              switch (quantum_type)
              {
                case GrayQuantum:
                case GrayAlphaQuantum:
                {
                  if (image->depth <= 8)
                    for (x=0; x < (long) image->columns; x++)
                    {
                      pixel=ScaleQuantumToAny(PixelIntensityToQuantum(p),range);
                      q=PopCharPixel((unsigned char) pixel,q);
                      if (image->matte != MagickFalse)
                        {
                          pixel=(unsigned char) ScaleQuantumToAny(p->opacity,
                            range);
                          q=PopCharPixel((unsigned char) pixel,q);
                        }
                      p++;
                    }
                  else
                    for (x=0; x < (long) image->columns; x++)
                    {
                      pixel=ScaleQuantumToAny(PixelIntensityToQuantum(p),range);
                      q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                      if (image->matte != MagickFalse)
                        {
                          pixel=(unsigned char) ScaleQuantumToAny(p->opacity,
                            range);
                          q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                        }
                      p++;
                    }
                  break;
                }
                case CMYKQuantum:
                case CMYKAQuantum:
                {
                  if (image->depth <= 8)
                    for (x=0; x < (long) image->columns; x++)
                    {
                      pixel=ScaleQuantumToAny(p->red,range);
                      q=PopCharPixel((unsigned char) pixel,q);
                      pixel=ScaleQuantumToAny(p->green,range);
                      q=PopCharPixel((unsigned char) pixel,q);
                      pixel=ScaleQuantumToAny(p->blue,range);
                      q=PopCharPixel((unsigned char) pixel,q);
                      pixel=ScaleQuantumToAny(indexes[x],range);
                      q=PopCharPixel((unsigned char) pixel,q);
                      if (image->matte != MagickFalse)
                        {
                          pixel=ScaleQuantumToAny((Quantum) (QuantumRange-
                            p->opacity),range);
                          q=PopCharPixel((unsigned char) pixel,q);
                        }
                      p++;
                    }
                  else
                    for (x=0; x < (long) image->columns; x++)
                    {
                      pixel=ScaleQuantumToAny(p->red,range);
                      q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                      pixel=ScaleQuantumToAny(p->green,range);
                      q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                      pixel=ScaleQuantumToAny(p->blue,range);
                      q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                      pixel=ScaleQuantumToAny(indexes[x],range);
                      q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                      if (image->matte != MagickFalse)
                        {
                          pixel=ScaleQuantumToAny((Quantum) (QuantumRange-
                            p->opacity),range);
                          q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                        }
                      p++;
                    }
                  break;
                }
                default:
                {
                  if (image->depth <= 8)
                    for (x=0; x < (long) image->columns; x++)
                    {
                      pixel=ScaleQuantumToAny(p->red,range);
                      q=PopCharPixel((unsigned char) pixel,q);
                      pixel=ScaleQuantumToAny(p->green,range);
                      q=PopCharPixel((unsigned char) pixel,q);
                      pixel=ScaleQuantumToAny(p->blue,range);
                      q=PopCharPixel((unsigned char) pixel,q);
                      if (image->matte != MagickFalse)
                        {
                          pixel=ScaleQuantumToAny((Quantum) (QuantumRange-
                            p->opacity),range);
                          q=PopCharPixel((unsigned char) pixel,q);
                        }
                      p++;
                    }
                  else
                    for (x=0; x < (long) image->columns; x++)
                    {
                      pixel=ScaleQuantumToAny(p->red,range);
                      q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                      pixel=ScaleQuantumToAny(p->green,range);
                      q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                      pixel=ScaleQuantumToAny(p->blue,range);
                      q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                      if (image->matte != MagickFalse)
                        {
                          pixel=ScaleQuantumToAny((Quantum) (QuantumRange-
                            p->opacity),range);
                          q=PopShortPixel(MSBEndian,(unsigned short) pixel,q);
                        }
                      p++;
                    }
                  break;
                }
              }
              extent=(size_t) (q-pixels);
            }
          count=WriteBlob(image,extent,pixels);
          if (count != (ssize_t) extent)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        quantum_info=DestroyQuantumInfo(quantum_info);
        break;
      }
      case 'F':
      case 'f':
      {
        (void) WriteBlobString(image,image->endian != LSBEndian ? "1.0\n" :
          "-1.0\n");
        image->depth=32;
        quantum_type=format == 'f' ? GrayQuantum : RGBQuantum;
        quantum_info=AcquireQuantumInfo((const ImageInfo *) NULL,image);
        if (quantum_info == (QuantumInfo *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        status=SetQuantumFormat(image,quantum_info,FloatingPointQuantumFormat);
        if (status == MagickFalse)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        pixels=GetQuantumPixels(quantum_info);
        for (y=(long) image->rows-1; y >= 0; y--)
        {
          register const PixelPacket
            *__restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          extent=ExportQuantumPixels(image,(const CacheView *) NULL,
            quantum_info,quantum_type,pixels,&image->exception);
          (void) WriteBlob(image,extent,pixels);
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        quantum_info=DestroyQuantumInfo(quantum_info);
        break;
      }
    }
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=SetImageProgress(image,SaveImagesTag,scene++,
      GetImageListLength(image));
    if (status == MagickFalse)
      break;
  } while (image_info->adjoin != MagickFalse);
  (void) CloseBlob(image);
  return(MagickTrue);
}
