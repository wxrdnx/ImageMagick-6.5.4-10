/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        TTTTT  IIIII  FFFFF  FFFFF                           %
%                          T      I    F      F                               %
%                          T      I    FFF    FFF                             %
%                          T      I    F      F                               %
%                          T    IIIII  F      F                               %
%                                                                             %
%                                                                             %
%                        Read/Write TIFF Image Format                         %
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
#include "magick/constitute.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/module.h"
#include "magick/monitor.h"
#include "magick/monitor-private.h"
#include "magick/option.h"
#include "magick/property.h"
#include "magick/quantum.h"
#include "magick/quantum-private.h"
#include "magick/profile.h"
#include "magick/resize.h"
#include "magick/semaphore.h"
#include "magick/splay-tree.h"
#include "magick/static.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/thread_.h"
#include "magick/utility.h"
#include "magick/module.h"
#if defined(MAGICKCORE_TIFF_DELEGATE)
# if defined(MAGICKCORE_HAVE_TIFFCONF_H)
#  include "tiffconf.h"
#endif
# include "tiff.h"
# include "tiffio.h"
# if !defined(COMPRESSION_ADOBE_DEFLATE)
#  define COMPRESSION_ADOBE_DEFLATE  8
# endif

/*
  Typedef declarations.
*/
#if defined(MAGICKCORE_HAVE_TIFFREADEXIFDIRECTORY)
typedef struct _ExifInfo
{
  unsigned int
    tag,
    type;

  const char
    *property;
} ExifInfo;

static const ExifInfo
  exif_info[] = {
    { EXIFTAG_EXPOSURETIME, TIFF_RATIONAL, "exif:ExposureTime" },
    { EXIFTAG_FNUMBER, TIFF_RATIONAL, "exif:FNumber" },
    { EXIFTAG_EXPOSUREPROGRAM, TIFF_SHORT, "exif:ExposureProgram" },
    { EXIFTAG_SPECTRALSENSITIVITY, TIFF_ASCII, "exif:SpectralSensitivity" },
    { EXIFTAG_ISOSPEEDRATINGS, TIFF_SHORT, "exif:ISOSpeedRatings" }, 
    { EXIFTAG_OECF, TIFF_UNDEFINED, "exif:OptoelectricConversionFactor" },
    { EXIFTAG_EXIFVERSION, TIFF_UNDEFINED, "exif:ExifVersion" },
    { EXIFTAG_DATETIMEORIGINAL, TIFF_ASCII, "exif:DateTimeOriginal" },
    { EXIFTAG_DATETIMEDIGITIZED, TIFF_ASCII, "exif:DateTimeDigitized" },
    { EXIFTAG_COMPONENTSCONFIGURATION, TIFF_UNDEFINED, "exif:ComponentsConfiguration" },
    { EXIFTAG_COMPRESSEDBITSPERPIXEL, TIFF_RATIONAL, "exif:CompressedBitsPerPixel" },
    { EXIFTAG_SHUTTERSPEEDVALUE, TIFF_SRATIONAL, "exif:ShutterSpeedValue" },
    { EXIFTAG_APERTUREVALUE, TIFF_RATIONAL, "exif:ApertureValue" },
    { EXIFTAG_BRIGHTNESSVALUE, TIFF_SRATIONAL, "exif:BrightnessValue" },
    { EXIFTAG_EXPOSUREBIASVALUE, TIFF_SRATIONAL, "exif:ExposureBiasValue" },
    { EXIFTAG_MAXAPERTUREVALUE, TIFF_RATIONAL, "exif:MaxApertureValue" },
    { EXIFTAG_SUBJECTDISTANCE, TIFF_RATIONAL, "exif:SubjectDistance" },
    { EXIFTAG_METERINGMODE, TIFF_SHORT, "exif:MeteringMode" },
    { EXIFTAG_LIGHTSOURCE, TIFF_SHORT, "exif:LightSource" },
    { EXIFTAG_FLASH, TIFF_SHORT, "exif:Flash" },
    { EXIFTAG_FOCALLENGTH, TIFF_RATIONAL, "exif:FocalLength" },
    /* { EXIFTAG_SUBJECTAREA, TIFF_SHORT, "exif:SubjectArea" }, */
    { EXIFTAG_MAKERNOTE, TIFF_UNDEFINED, "exif:MakerNote" },
    { EXIFTAG_USERCOMMENT, TIFF_UNDEFINED, "exif:UserComment" },
    { EXIFTAG_SUBSECTIME, TIFF_ASCII, "exif:SubSecTime" },
    { EXIFTAG_SUBSECTIMEORIGINAL, TIFF_ASCII, "exif:SubSecTimeOriginal" },
    { EXIFTAG_SUBSECTIMEDIGITIZED, TIFF_ASCII, "exif:SubSecTimeDigitized" },
    { EXIFTAG_FLASHPIXVERSION, TIFF_UNDEFINED, "exif:FlashpixVersion" },
    { EXIFTAG_PIXELXDIMENSION, TIFF_LONG, "exif:PixelXDimension" },
    { EXIFTAG_PIXELXDIMENSION, TIFF_SHORT, "exif:PixelXDimension" },
    { EXIFTAG_PIXELYDIMENSION, TIFF_LONG, "exif:PixelYDimension" },
    { EXIFTAG_PIXELYDIMENSION, TIFF_SHORT, "exif:PixelYDimension" },
    { EXIFTAG_RELATEDSOUNDFILE, TIFF_ASCII, "exif:RelatedSoundFile" },
    { EXIFTAG_FLASHENERGY, TIFF_RATIONAL, "exif:FlashEnergy" },
    { EXIFTAG_SPATIALFREQUENCYRESPONSE, TIFF_UNDEFINED, "exif:SpatialFrequencyResponse" },
    { EXIFTAG_FOCALPLANEXRESOLUTION, TIFF_RATIONAL, "exif:FocalPlaneXResolution" },
    { EXIFTAG_FOCALPLANEYRESOLUTION, TIFF_RATIONAL, "exif:FocalPlaneYResolution" },
    { EXIFTAG_FOCALPLANERESOLUTIONUNIT, TIFF_SHORT, "exif:FocalPlaneResolutionUnit" },
    { EXIFTAG_SUBJECTLOCATION, TIFF_SHORT, "exif:SubjectLocation" },
    { EXIFTAG_EXPOSUREINDEX, TIFF_RATIONAL, "exif:ExposureIndex" },
    { EXIFTAG_SENSINGMETHOD, TIFF_SHORT, "exif:SensingMethod" },
    { EXIFTAG_FILESOURCE, TIFF_UNDEFINED, "exif:FileSource" },
    { EXIFTAG_SCENETYPE, TIFF_UNDEFINED, "exif:SceneType" },
    { EXIFTAG_CFAPATTERN, TIFF_UNDEFINED, "exif:CFAPattern" },
    { EXIFTAG_CUSTOMRENDERED, TIFF_SHORT, "exif:CustomRendered" },
    { EXIFTAG_EXPOSUREMODE, TIFF_SHORT, "exif:ExposureMode" },
    { EXIFTAG_WHITEBALANCE, TIFF_SHORT, "exif:WhiteBalance" },
    { EXIFTAG_DIGITALZOOMRATIO, TIFF_RATIONAL, "exif:DigitalZoomRatio" },
    { EXIFTAG_FOCALLENGTHIN35MMFILM, TIFF_SHORT, "exif:FocalLengthIn35mmFilm" },
    { EXIFTAG_SCENECAPTURETYPE, TIFF_SHORT, "exif:SceneCaptureType" },
    { EXIFTAG_GAINCONTROL, TIFF_RATIONAL, "exif:GainControl" },
    { EXIFTAG_CONTRAST, TIFF_SHORT, "exif:Contrast" },
    { EXIFTAG_SATURATION, TIFF_SHORT, "exif:Saturation" },
    { EXIFTAG_SHARPNESS, TIFF_SHORT, "exif:Sharpness" },
    { EXIFTAG_DEVICESETTINGDESCRIPTION, TIFF_UNDEFINED, "exif:DeviceSettingDescription" },
    { EXIFTAG_SUBJECTDISTANCERANGE, TIFF_SHORT, "exif:SubjectDistanceRange" },
    { EXIFTAG_IMAGEUNIQUEID, TIFF_ASCII, "exif:ImageUniqueID" },
    { 0, 0, (char *) NULL }
};
#endif

/*
  Global declarations.
*/
static MagickThreadKey
  tiff_exception;

static SemaphoreInfo
  *tiff_semaphore = (SemaphoreInfo *) NULL;

static volatile MagickBooleanType
  instantiate_key = MagickFalse;
#endif

/*
  Forward declarations.
*/
#if defined(MAGICKCORE_TIFF_DELEGATE)
static MagickBooleanType
  WritePTIFImage(const ImageInfo *,Image *),
  WriteTIFFImage(const ImageInfo *,Image *);
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s T I F F                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsTIFF() returns MagickTrue if the image format type, identified by the
%  magick string, is TIFF.
%
%  The format of the IsTIFF method is:
%
%      MagickBooleanType IsTIFF(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: compare image format pattern against these bytes.
%
%    o length: Specifies the length of the magick string.
%
*/
static MagickBooleanType IsTIFF(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (memcmp(magick,"\115\115\000\052",4) == 0)
    return(MagickTrue);
  if (memcmp(magick,"\111\111\052\000",4) == 0)
    return(MagickTrue);
#if defined(TIFF_VERSION_BIG)
  if (length < 8)
    return(MagickFalse);
  if (memcmp(magick,"\115\115\000\053\000\010\000\000",8) == 0)
    return(MagickTrue);
  if (memcmp(magick,"\111\111\053\000\010\000\000\000",8) == 0)
    return(MagickTrue);
#endif
  return(MagickFalse);
}

#if defined(MAGICKCORE_TIFF_DELEGATE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d T I F F I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadTIFFImage() reads a Tagged image file and returns it.  It allocates the
%  memory necessary for the new Image structure and returns a pointer to the
%  new image.
%
%  The format of the ReadTIFFImage method is:
%
%      Image *ReadTIFFImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline size_t MagickMax(const size_t x,const size_t y)
{
  if (x > y)
    return(x);
  return(y);
}

static inline long MagickMin(const long x,const long y)
{
  if (x < y)
    return(x);
  return(y);
}

static MagickBooleanType ReadProfile(Image *image,const char *name,
  unsigned char *datum,long length)
{
  MagickBooleanType
    status;

  register long
    i;

  StringInfo
    *profile;

  if (length < 4)
    return(MagickFalse);
  i=0;
  if ((LocaleCompare(name,"icc") != 0) && (LocaleCompare(name,"xmp") != 0))
    {
      for (i=0; i < (length-4); i+=2)
        if (LocaleNCompare((char *) (datum+i),"8BIM",4) == 0)
          break;
      if (i == length)
        length-=i;
      else
        i=0;
      if (length < 4)
        return(MagickFalse);
    }
  profile=AcquireStringInfo((size_t) length);
  SetStringInfoDatum(profile,datum+i);
  status=SetImageProfile(image,name,profile);
  profile=DestroyStringInfo(profile);
  if (status == MagickFalse)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  return(MagickTrue);
}

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int TIFFCloseBlob(thandle_t image)
{
  (void) CloseBlob((Image *) image);
  return(0);
}

static void TIFFErrors(const char *module,const char *format,va_list error)
{
  char
    message[MaxTextExtent];

  ExceptionInfo
    *exception;

#if defined(MAGICKCORE_HAVE_VSNPRINTF)
  (void) vsnprintf(message,MaxTextExtent,format,error);
#else
  (void) vsprintf(message,format,error);
#endif
  (void) ConcatenateMagickString(message,".",MaxTextExtent);
  exception=(ExceptionInfo *) MagickGetThreadValue(tiff_exception);
  if (exception != (ExceptionInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
      message,"`%s'",module);
}

static void TIFFGetProfiles(TIFF *tiff,Image *image)
{
  uint32
    length;

  unsigned char
    *profile;

#if defined(TIFFTAG_ICCPROFILE)
  length=0;
  if (TIFFGetField(tiff,TIFFTAG_ICCPROFILE,&length,&profile) == 1)
    (void) ReadProfile(image,"icc",profile,(long) length);
#endif
#if defined(TIFFTAG_PHOTOSHOP)
  length=0;
  if (TIFFGetField(tiff,TIFFTAG_PHOTOSHOP,&length,&profile) == 1)
    (void) ReadProfile(image,"8bim",profile,(long) length);
#endif
#if defined(TIFFTAG_RICHTIFFIPTC)
  length=0;
  if (TIFFGetField(tiff,TIFFTAG_RICHTIFFIPTC,&length,&profile) == 1)
    {
      if (TIFFIsByteSwapped(tiff) != 0)
        TIFFSwabArrayOfLong((uint32 *) profile,(unsigned long) length);
      (void) ReadProfile(image,"iptc",profile,4L*length);
    }
#endif
#if defined(TIFFTAG_XMLPACKET)
  length=0;
  if (TIFFGetField(tiff,TIFFTAG_XMLPACKET,&length,&profile) == 1)
    (void) ReadProfile(image,"xmp",profile,(long) length);
#endif
  length=0;
  if (TIFFGetField(tiff,37724,&length,&profile) == 1)
    (void) ReadProfile(image,"tiff:37724",profile,(long) length);
}

static void TIFFGetProperties(TIFF *tiff,Image *image)
{
  char
    *text;

  if (TIFFGetField(tiff,TIFFTAG_ARTIST,&text) == 1)
    (void) SetImageProperty(image,"tiff:artist",text);
  if (TIFFGetField(tiff,TIFFTAG_DATETIME,&text) == 1)
    (void) SetImageProperty(image,"tiff:timestamp",text);
  if (TIFFGetField(tiff,TIFFTAG_SOFTWARE,&text) == 1)
    (void) SetImageProperty(image,"tiff:software",text);
  if (TIFFGetField(tiff,TIFFTAG_HOSTCOMPUTER,&text) == 1)
    (void) SetImageProperty(image,"tiff:hostcomputer",text);
  if (TIFFGetField(tiff,TIFFTAG_DOCUMENTNAME,&text) == 1)
    (void) SetImageProperty(image,"tiff:document",text);
  if (TIFFGetField(tiff,TIFFTAG_MAKE,&text) == 1)
    (void) SetImageProperty(image,"tiff:make",text);
  if (TIFFGetField(tiff,TIFFTAG_MODEL,&text) == 1)
    (void) SetImageProperty(image,"tiff:model",text);
  if (TIFFGetField(tiff,33432,&text) == 1)
    (void) SetImageProperty(image,"tiff:copyright",text);
  if (TIFFGetField(tiff,TIFFTAG_PAGENAME,&text) == 1)
    (void) SetImageProperty(image,"label",text);
  if (TIFFGetField(tiff,TIFFTAG_IMAGEDESCRIPTION,&text) == 1)
    (void) SetImageProperty(image,"comment",text);
}

static void TIFFGetEXIFProperties(TIFF *tiff,Image *image)
{
#if defined(MAGICKCORE_HAVE_TIFFREADEXIFDIRECTORY)
  char
    value[MaxTextExtent];

  register long
    i;

  tdir_t
    directory;

  uint32
    offset;

  /*
    Read EXIF properties.
  */
  if (TIFFGetField(tiff,TIFFTAG_EXIFIFD,&offset) == 0)
    return;
  directory=TIFFCurrentDirectory(tiff);
  if (TIFFReadEXIFDirectory(tiff,offset) == 0)
    return;
  for (i=0; exif_info[i].tag != 0; i++)
  {
    *value='\0';
    switch (exif_info[i].type)
    {
      case TIFF_ASCII:
      {
        char
          *ascii;

        if (TIFFGetField(tiff,exif_info[i].tag,&ascii) != 0)
          (void) CopyMagickMemory(value,ascii,MaxTextExtent);
        break;
      }
      case TIFF_SHORT:
      {
        uint16
          shorty;

        if (TIFFGetField(tiff,exif_info[i].tag,&shorty) != 0)
          (void) FormatMagickString(value,MaxTextExtent,"%d",shorty);
        break;
      }
      case TIFF_LONG:
      {
        uint32
          longy;

        if (TIFFGetField(tiff,exif_info[i].tag,&longy) != 0)
          (void) FormatMagickString(value,MaxTextExtent,"%d",longy);
        break;
      }
      case TIFF_RATIONAL:
      case TIFF_SRATIONAL:
      {
        float
          rational;

        if (TIFFGetField(tiff,exif_info[i].tag,&rational) != 0)
          (void) FormatMagickString(value,MaxTextExtent,"%g",rational);
        break;
      }
      default:
        break;
    }
    if (*value != '\0')
      (void) SetImageProperty(image,exif_info[i].property,value);
  }
  TIFFSetDirectory(tiff,directory);
#else
  (void) tiff;
  (void) image;
#endif
}

static int TIFFMapBlob(thandle_t image,tdata_t *base,toff_t *size)
{
  *base=(tdata_t *) GetBlobStreamData((Image *) image);
  if (*base != (tdata_t *) NULL)
    *size=(toff_t) GetBlobSize((Image *) image);
  if (*base != (tdata_t *) NULL)
    return(1);
  return(0);
}

static tsize_t TIFFReadBlob(thandle_t image,tdata_t data,tsize_t size)
{
  tsize_t
    count;

  count=(tsize_t) ReadBlob((Image *) image,(size_t) size,
    (unsigned char *) data);
  return(count);
}

static int32 TIFFReadPixels(TIFF *tiff,unsigned long bits_per_sample,
  tsample_t sample,long row,tdata_t scanline)
{
  int32
    status;

  (void) bits_per_sample;
  status=TIFFReadScanline(tiff,scanline,(uint32) row,sample);
  return(status);
}

static toff_t TIFFSeekBlob(thandle_t image,toff_t offset,int whence)
{
  return((toff_t) SeekBlob((Image *) image,(MagickOffsetType) offset,whence));
}

static toff_t TIFFGetBlobSize(thandle_t image)
{
  return((toff_t) GetBlobSize((Image *) image));
}

static void TIFFUnmapBlob(thandle_t image,tdata_t base,toff_t size)
{
  (void) image;
  (void) base;
  (void) size;
}

static void TIFFWarnings(const char *module,const char *format,va_list warning)
{
  char
    message[MaxTextExtent];

  ExceptionInfo
    *exception;

#if defined(MAGICKCORE_HAVE_VSNPRINTF)
  (void) vsnprintf(message,MaxTextExtent,format,warning);
#else
  (void) vsprintf(message,format,warning);
#endif
  (void) ConcatenateMagickString(message,".",MaxTextExtent);
  exception=(ExceptionInfo *) MagickGetThreadValue(tiff_exception);
  if (exception != (ExceptionInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
      message,"`%s'",module);
}

static tsize_t TIFFWriteBlob(thandle_t image,tdata_t data,tsize_t size)
{
  tsize_t
    count;

  count=(tsize_t) WriteBlob((Image *) image,(size_t) size,
    (unsigned char *) data);
  return(count);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static Image *ReadTIFFImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  typedef enum
  {
    ReadSingleSampleMethod,
    ReadRGBAMethod,
    ReadCMYKAMethod,
    ReadStripMethod,
    ReadTileMethod,
    ReadGenericMethod
  } TIFFMethodType;

  const char
    *option;

  float
    *chromaticity,
    x_position,
    y_position,
    x_resolution,
    y_resolution;

  Image
    *image;

  long
    y;

  MagickBooleanType
    associated_alpha,
    debug,
    status;

  MagickSizeType
    number_pixels;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  register long
    i;

  size_t
    length,
    pad;

  TIFF
    *tiff;

  TIFFErrorHandler
    error_handler,
    warning_handler;

  TIFFMethodType
    method;

  uint16
    compress_tag,
    bits_per_sample,
    endian,
    extra_samples,
    interlace,
    max_sample_value,
    min_sample_value,
    orientation,
    pages,
    photometric,
    *sample_info,
    sample_format,
    samples_per_pixel,
    units,
    value;

  uint32
    height,
    rows_per_strip,
    width;

  unsigned char
    *pixels;

  unsigned long
    lsb_first;

  /*
    Open image.
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
  (void) MagickSetThreadValue(tiff_exception,exception);
  error_handler=TIFFSetErrorHandler(TIFFErrors);
  warning_handler=TIFFSetWarningHandler(TIFFWarnings);
  tiff=TIFFClientOpen(image->filename,"r",(thandle_t) image,TIFFReadBlob,
    TIFFWriteBlob,TIFFSeekBlob,TIFFCloseBlob,TIFFGetBlobSize,TIFFMapBlob,
    TIFFUnmapBlob);
  if (tiff == (TIFF *) NULL)
    {
      (void) TIFFSetWarningHandler(warning_handler);
      (void) TIFFSetErrorHandler(error_handler);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  debug=IsEventLogging();
  if (image_info->number_scenes != 0)
    {
      /*
        Generate blank images for subimage specification (e.g. image.tif[4].
      */
      for (i=0; i < (long) image_info->scene; i++)
      {
        (void) TIFFReadDirectory(tiff);
        AcquireNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
      }
    }
  do
  {
    if (0 && (image_info->verbose != MagickFalse))
      TIFFPrintDirectory(tiff,stdout,MagickFalse);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_COMPRESSION,&compress_tag);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_ORIENTATION,&orientation);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_IMAGEWIDTH,&width);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_IMAGELENGTH,&height);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_FILLORDER,&endian);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_PLANARCONFIG,&interlace);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,&bits_per_sample);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLEFORMAT,&sample_format);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_MINSAMPLEVALUE,&min_sample_value);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_MAXSAMPLEVALUE,&max_sample_value);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_PHOTOMETRIC,&photometric);
    switch (photometric)
    {
      case PHOTOMETRIC_MINISBLACK:
      {
        (void) SetImageProperty(image,"tiff:photometric","min-is-black");
        break;
      }
      case PHOTOMETRIC_MINISWHITE:
      {
        (void) SetImageProperty(image,"tiff:photometric","min-is-white");
        break;
      }
      case PHOTOMETRIC_PALETTE:
      {
        (void) SetImageProperty(image,"tiff:photometric","palette");
        break;
      }
      case PHOTOMETRIC_RGB:
      {
        (void) SetImageProperty(image,"tiff:photometric","RGB");
        break;
      }
      case PHOTOMETRIC_CIELAB:
      {
        (void) SetImageProperty(image,"tiff:photometric","CIELAB");
        break;
      }
      case PHOTOMETRIC_SEPARATED:
      {
        (void) SetImageProperty(image,"tiff:photometric","separated");
        break;
      }
      default:
      {
        (void) SetImageProperty(image,"tiff:photometric","unknown");
        break;
      }
    }
    if (image->debug != MagickFalse)
      {
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Geometry: %ux%u",
          (unsigned int) width,(unsigned int) height);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Interlace: %u",
          interlace);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Bits per sample: %u",bits_per_sample);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Min sample value: %u",min_sample_value);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Max sample value: %u",max_sample_value);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Photometric "
          "interpretation: %s",GetImageProperty(image,"tiff:photometric"));
      }
    image->columns=(unsigned long) width;
    image->rows=(unsigned long) height;
    image->depth=(unsigned long) bits_per_sample;
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Image depth: %lu",
        image->depth);
    lsb_first=1;
    image->endian=MSBEndian;
    if ((int) (*(char *) &lsb_first) != 0)
      image->endian=LSBEndian;
    if (photometric == PHOTOMETRIC_SEPARATED)
      image->colorspace=CMYKColorspace;
    if (photometric == PHOTOMETRIC_CIELAB)
      image->colorspace=LabColorspace;
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLESPERPIXEL,
      &samples_per_pixel);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_RESOLUTIONUNIT,&units);
    x_resolution=(float) image->x_resolution;
    y_resolution=(float) image->y_resolution;
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_XRESOLUTION,&x_resolution);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_YRESOLUTION,&y_resolution);
    image->x_resolution=x_resolution;
    image->y_resolution=y_resolution;
    x_position=(float) image->page.x/x_resolution;
    y_position=(float) image->page.y/y_resolution;
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_XPOSITION,&x_position);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_YPOSITION,&y_position);
    image->page.x=(long) (x_position*x_resolution+0.5);
    image->page.y=(long) (y_position*y_resolution+0.5);
    image->orientation=(OrientationType) orientation;
    chromaticity=(float *) NULL;
    (void) TIFFGetField(tiff,TIFFTAG_WHITEPOINT,&chromaticity);
    if (chromaticity != (float *) NULL)
      {
        image->chromaticity.white_point.x=chromaticity[0];
        image->chromaticity.white_point.y=chromaticity[1];
      }
    chromaticity=(float *) NULL;
    (void) TIFFGetField(tiff,TIFFTAG_PRIMARYCHROMATICITIES,&chromaticity);
    if (chromaticity != (float *) NULL)
      {
        image->chromaticity.red_primary.x=chromaticity[0];
        image->chromaticity.red_primary.y=chromaticity[1];
        image->chromaticity.green_primary.x=chromaticity[2];
        image->chromaticity.green_primary.y=chromaticity[3];
        image->chromaticity.blue_primary.x=chromaticity[4];
        image->chromaticity.blue_primary.y=chromaticity[5];
      }
    TIFFGetProperties(tiff,image);
    option=GetImageOption(image_info,"tiff:ignore-exif");
    if (option == (const char *) NULL)
      TIFFGetEXIFProperties(tiff,image);
    else
      if (IsMagickTrue(option) == MagickFalse)
        TIFFGetEXIFProperties(tiff,image);
    TIFFGetProfiles(tiff,image);
    /*
      Allocate memory for the image and pixel buffer.
    */
#if defined(MAGICKCORE_HAVE_TIFFISCODECCONFIGURED) || (TIFFLIB_VERSION > 20040919)
    if ((compress_tag != COMPRESSION_NONE) &&
        (TIFFIsCODECConfigured(compress_tag) == 0))
      {
        TIFFClose(tiff);
        ThrowReaderException(CoderError,"CompressNotSupported");
      }
#endif
    switch (compress_tag)
    {
      case COMPRESSION_NONE: image->compression=NoCompression; break;
      case COMPRESSION_CCITTFAX3: image->compression=FaxCompression; break;
      case COMPRESSION_CCITTFAX4: image->compression=Group4Compression; break;
      case COMPRESSION_JPEG:
      {
         image->compression=JPEGCompression;
#if defined(JPEG_SUPPORT)
         {
           char
             sampling_factor[MaxTextExtent];

           uint16
             horizontal,
             vertical;

           (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_YCBCRSUBSAMPLING,
             &horizontal,&vertical);
           (void) FormatMagickString(sampling_factor,MaxTextExtent,"%dx%d",
             horizontal,vertical);
           (void) SetImageProperty(image,"jpeg:sampling-factor",
             sampling_factor);
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "Sampling Factors: %s",sampling_factor);
           if ((samples_per_pixel > 1) && (photometric == PHOTOMETRIC_YCBCR))
             {
               (void) TIFFSetField(tiff,TIFFTAG_JPEGCOLORMODE,
                 JPEGCOLORMODE_RGB);
               photometric=PHOTOMETRIC_RGB;
             }
         }
#endif
        break;
      }
      case COMPRESSION_OJPEG: image->compression=JPEGCompression; break;
      case COMPRESSION_LZW: image->compression=LZWCompression; break;
      case COMPRESSION_DEFLATE: image->compression=ZipCompression; break;
      case COMPRESSION_ADOBE_DEFLATE: image->compression=ZipCompression; break;
      default: image->compression=RLECompression; break;
    }
    quantum_info=AcquireQuantumInfo(image_info,image);
    if (quantum_info == (QuantumInfo *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    if (sample_format == SAMPLEFORMAT_UINT)
      status=SetQuantumFormat(image,quantum_info,UnsignedQuantumFormat);
    if (sample_format == SAMPLEFORMAT_INT)
      status=SetQuantumFormat(image,quantum_info,SignedQuantumFormat);
    if (sample_format == SAMPLEFORMAT_IEEEFP)
      status=SetQuantumFormat(image,quantum_info,FloatingPointQuantumFormat);
    if (status == MagickFalse)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    status=MagickTrue;
    switch (photometric)
    {
      case PHOTOMETRIC_MINISBLACK:
      {
        quantum_info->min_is_white=MagickFalse;
        break;
      }
      case PHOTOMETRIC_MINISWHITE:
      {
        quantum_info->min_is_white=MagickTrue;
        break;
      }
      default:
        break;
    }
    associated_alpha=MagickFalse;
    extra_samples=0;
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_EXTRASAMPLES,&extra_samples,
      &sample_info);
    if (extra_samples == 0)
      {
        if ((samples_per_pixel == 4) && (photometric == PHOTOMETRIC_RGB))
          image->matte=MagickTrue;
      }
    else
      {
        if (samples_per_pixel > 3)
          {
            image->matte=MagickTrue;
            associated_alpha=MagickFalse;
          }
        if (sample_info[0] == EXTRASAMPLE_UNASSALPHA)
          {
            image->matte=MagickTrue;
            associated_alpha=MagickFalse;
          }
        if (sample_info[0] == EXTRASAMPLE_ASSOCALPHA)
          {
            image->matte=MagickTrue;
            associated_alpha=MagickTrue;
            SetQuantumAlphaType(quantum_info,DisassociatedQuantumAlpha);
          }
      }
    option=GetImageOption(image_info,"tiff:alpha");
    if (option != (const char *) NULL)
      associated_alpha=LocaleCompare(option,"associated") == 0 ? MagickTrue :
        MagickFalse;
    if (image->matte != MagickFalse)
      (void) SetImageProperty(image,"tiff:alpha",
        associated_alpha != MagickFalse ? "associated" : "unassociated");
    if ((photometric == PHOTOMETRIC_PALETTE) &&
        (pow(2.0,1.0*bits_per_sample) <= MaxColormapSize))
      {
        unsigned long
          colors;

        colors=(unsigned long) GetQuantumRange(bits_per_sample)+1;
        if (AcquireImageColormap(image,colors) == MagickFalse)
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
      }
    if (units == RESUNIT_INCH)
      image->units=PixelsPerInchResolution;
    if (units == RESUNIT_CENTIMETER)
      image->units=PixelsPerCentimeterResolution;
    value=(unsigned short) image->scene;
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_PAGENUMBER,&value,&pages);
    image->scene=value;
    if (image_info->ping != MagickFalse)
      {
        if (image_info->number_scenes != 0)
          if (image->scene >= (image_info->scene+image_info->number_scenes-1))
            break;
        goto next_tiff_frame;
      }
    method=ReadGenericMethod;
    if (TIFFGetField(tiff,TIFFTAG_ROWSPERSTRIP,&rows_per_strip) != 0)
      {
        char
          value[MaxTextExtent];

        method=ReadStripMethod;
        (void) FormatMagickString(value,MaxTextExtent,"%u",
          (unsigned int) rows_per_strip);
        (void) SetImageProperty(image,"tiff:rows-per-strip",value);
      }
    if ((samples_per_pixel >= 2) && (interlace == PLANARCONFIG_CONTIG))
      method=ReadRGBAMethod;
    if ((samples_per_pixel >= 2) && (interlace == PLANARCONFIG_SEPARATE))
      method=ReadCMYKAMethod;
    if ((photometric != PHOTOMETRIC_RGB) &&
        (photometric != PHOTOMETRIC_CIELAB) &&
        (photometric != PHOTOMETRIC_SEPARATED))
      method=ReadGenericMethod;
    if (image->storage_class == PseudoClass)
      method=ReadSingleSampleMethod;
    if ((photometric == PHOTOMETRIC_MINISBLACK) ||
        (photometric == PHOTOMETRIC_MINISWHITE))
      method=ReadSingleSampleMethod;
    if (TIFFIsTiled(tiff) != MagickFalse)
      method=ReadTileMethod;
    quantum_type=RGBQuantum;
    pixels=GetQuantumPixels(quantum_info);
    switch (method)
    {
      case ReadSingleSampleMethod:
      {
        /*
          Convert TIFF image to PseudoClass MIFF image.
        */
        if ((image->storage_class == PseudoClass) &&
            (photometric == PHOTOMETRIC_PALETTE))
          {
            unsigned long
              range;

            uint16
              *blue_colormap,
              *green_colormap,
              *red_colormap;

            /*
              Initialize colormap.
            */
            (void) TIFFGetField(tiff,TIFFTAG_COLORMAP,&red_colormap,
              &green_colormap,&blue_colormap);
            range=255;  /* might be old style 8-bit colormap */
            for (i=0; i < (long) image->colors; i++)
              if ((red_colormap[i] >= 256) || (green_colormap[i] >= 256) ||
                  (blue_colormap[i] >= 256))
                {
                  range=65535;
                  break;
                }
            for (i=0; i < (long) image->colors; i++)
            {
              image->colormap[i].red=(Quantum) (((double) QuantumRange*
                red_colormap[i])/range+0.5);
              image->colormap[i].green=(Quantum) (((double) QuantumRange*
                green_colormap[i])/range+0.5);
              image->colormap[i].blue=(Quantum) (((double) QuantumRange*
                blue_colormap[i])/range+0.5);
            }
          }
        quantum_type=IndexQuantum;
        pad=(size_t) MagickMax((size_t) samples_per_pixel-1,0);
        if (image->matte != MagickFalse)
          {
            if (image->storage_class != PseudoClass)
              {
                quantum_type=samples_per_pixel == 1 ? AlphaQuantum :
                  GrayAlphaQuantum;
                pad=(size_t) MagickMax((size_t) samples_per_pixel-2,0);
              }
            else
              {
                quantum_type=IndexAlphaQuantum;
                pad=(size_t) MagickMax((size_t) samples_per_pixel-2,0);
              }
          }
        else
          if (image->storage_class != PseudoClass)
            {
              quantum_type=GrayQuantum;
              pad=(size_t) MagickMax((size_t) samples_per_pixel-1,0);
            }
        status=SetQuantumPad(image,quantum_info,pad*((bits_per_sample+7) >> 3));
        if (status == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        pixels=GetQuantumPixels(quantum_info);
        for (y=0; y < (long) image->rows; y++)
        {
          int
            status;

          register PixelPacket
            *__restrict q;

          status=TIFFReadPixels(tiff,bits_per_sample,0,y,(char *) pixels);
          if (status == -1)
            break;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          length=ImportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            quantum_type,pixels,exception);
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case ReadRGBAMethod:
      {
        /*
          Convert TIFF image to DirectClass MIFF image.
        */
        pad=(size_t) MagickMax((size_t) samples_per_pixel-3,0);
        quantum_type=RGBQuantum;
        if (image->matte != MagickFalse)
          {
            quantum_type=RGBAQuantum;
            pad=(size_t) MagickMax((size_t) samples_per_pixel-4,0);
          }
        if (image->colorspace == CMYKColorspace)
          {
            pad=(size_t) MagickMax((size_t) samples_per_pixel-4,0);
            quantum_type=CMYKQuantum;
            if (image->matte != MagickFalse)
              {
                quantum_type=CMYKAQuantum;
                pad=(size_t) MagickMax((size_t) samples_per_pixel-5,0);
              }
          }
        status=SetQuantumPad(image,quantum_info,pad*((bits_per_sample+7) >> 3));
        if (status == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        pixels=GetQuantumPixels(quantum_info);
        for (y=0; y < (long) image->rows; y++)
        {
          int
            status;

          register PixelPacket
            *__restrict q;

          status=TIFFReadPixels(tiff,bits_per_sample,0,y,(char *) pixels);
          if (status == -1)
            break;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          length=ImportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            quantum_type,pixels,exception);
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case ReadCMYKAMethod:
      {
        /*
          Convert TIFF image to DirectClass MIFF image.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          register PixelPacket
            *__restrict q;

          (void) TIFFReadPixels(tiff,bits_per_sample,0,y,(char *) pixels);
          for (i=0; i < (long) samples_per_pixel; i++)
          {
            int
              status;

            status=TIFFReadPixels(tiff,bits_per_sample,(tsample_t) i,y,(char *)
              pixels);
            if (status == -1)
              break;
            q=GetAuthenticPixels(image,0,y,image->columns,1,exception);
            if (q == (PixelPacket *) NULL)
              break;
            if (image->colorspace != CMYKColorspace)
              switch (i)
              {
                case 0: quantum_type=RedQuantum; break;
                case 1: quantum_type=GreenQuantum; break;
                case 2: quantum_type=BlueQuantum; break;
                case 3: quantum_type=AlphaQuantum; break;
                default: quantum_type=UndefinedQuantum; break;
              }
           else
              switch (i)
              {
                case 0: quantum_type=CyanQuantum; break;
                case 1: quantum_type=MagentaQuantum; break;
                case 2: quantum_type=YellowQuantum; break;
                case 3: quantum_type=BlackQuantum; break;
                case 4: quantum_type=AlphaQuantum; break;
                default: quantum_type=UndefinedQuantum; break;
              }
            length=ImportQuantumPixels(image,(CacheView *) NULL,quantum_info,
              quantum_type,pixels,exception);
            if (SyncAuthenticPixels(image,exception) == MagickFalse)
              break;
          }
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case ReadStripMethod:
      {
        register unsigned char
          *p;

        /*
          Convert stripped TIFF image to DirectClass MIFF image.
        */
        i=0;
        p=(unsigned char *) NULL;
        for (y=0; y < (long) image->rows; y++)
        {
          register long
            x;

          register PixelPacket
            *__restrict q;

          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          if (i == 0)
            {
              if (TIFFReadRGBAStrip(tiff,(tstrip_t) y,(uint32 *) pixels) == 0)
                break;
              i=(long) MagickMin((long) rows_per_strip,(long) image->rows-y);
            }
          i--;
          p=pixels+image->columns*i;
          for (x=0; x < (long) image->columns; x++)
          {
            q->red=ScaleCharToQuantum((unsigned char) (TIFFGetR(*p)));
            q->green=ScaleCharToQuantum((unsigned char) (TIFFGetG(*p)));
            q->blue=ScaleCharToQuantum((unsigned char) (TIFFGetB(*p)));
            if (image->matte != MagickFalse)
              q->opacity=ScaleCharToQuantum((unsigned char) (TIFFGetA(*p)));
            p++;
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
        break;
      }
      case ReadTileMethod:
      {
        register uint32
          *p;

        uint32
          *tile_pixels,
          columns,
          rows;

        unsigned long
          number_pixels;

        /*
          Convert tiled TIFF image to DirectClass MIFF image.
        */
        if ((TIFFGetField(tiff,TIFFTAG_TILEWIDTH,&columns) == 0) ||
            (TIFFGetField(tiff,TIFFTAG_TILELENGTH,&rows) == 0))
          {
            TIFFClose(tiff);
            ThrowReaderException(CoderError,"ImageIsNotTiled");
          }
        (void) SetImageStorageClass(image,DirectClass);
        number_pixels=columns*rows;
        tile_pixels=(uint32 *) AcquireQuantumMemory((size_t) columns*rows,
          sizeof(*tile_pixels));
        if (tile_pixels == (uint32 *) NULL)
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        for (y=0; y < (long) image->rows; y+=rows)
        {
          PixelPacket
            *tile;

          register long
            x;

          register PixelPacket
            *__restrict q;

          unsigned long
            columns_remaining,
            rows_remaining;

          rows_remaining=image->rows-y;
          if ((long) (y+rows) < (long) image->rows)
            rows_remaining=rows;
          tile=QueueAuthenticPixels(image,0,y,image->columns,rows_remaining,
            exception);
          if (tile == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x+=columns)
          {
            unsigned long
              column,
              row;

            if (TIFFReadRGBATile(tiff,(uint32) x,(uint32) y,tile_pixels) == 0)
              break;
            columns_remaining=image->columns-x;
            if ((long) (x+columns) < (long) image->columns)
              columns_remaining=columns;
            p=tile_pixels+(rows-rows_remaining)*columns;
            q=tile+(image->columns*(rows_remaining-1)+x);
            for (row=rows_remaining; row > 0; row--)
            {
              if (image->matte != MagickFalse)
                for (column=columns_remaining; column > 0; column--)
                {
                  q->red=ScaleCharToQuantum((unsigned char) TIFFGetR(*p));
                  q->green=ScaleCharToQuantum((unsigned char) TIFFGetG(*p));
                  q->blue=ScaleCharToQuantum((unsigned char) TIFFGetB(*p));
                  q->opacity=(Quantum) (QuantumRange-ScaleCharToQuantum(
                    (unsigned char) TIFFGetA(*p)));
                  q++;
                  p++;
                }
              else
                for (column=columns_remaining; column > 0; column--)
                {
                  q->red=ScaleCharToQuantum((unsigned char) TIFFGetR(*p));
                  q->green=ScaleCharToQuantum((unsigned char) TIFFGetG(*p));
                  q->blue=ScaleCharToQuantum((unsigned char) TIFFGetB(*p));
                  q++;
                  p++;
                }
              p+=columns-columns_remaining;
              q-=(image->columns+columns_remaining);
            }
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
        tile_pixels=(uint32 *) RelinquishMagickMemory(tile_pixels);
        break;
      }
      case ReadGenericMethod:
      default:
      {
        register uint32
          *p;

        uint32
          *pixels;

        /*
          Convert TIFF image to DirectClass MIFF image.
        */
        number_pixels=(MagickSizeType) image->columns*image->rows;
        if ((number_pixels*sizeof(uint32)) != (MagickSizeType) ((size_t)
            (number_pixels*sizeof(uint32))))
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        pixels=(uint32 *) AcquireQuantumMemory(image->columns,image->rows*
          sizeof(uint32));
        if (pixels == (uint32 *) NULL)
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        (void) TIFFReadRGBAImage(tiff,(uint32) image->columns,
          (uint32) image->rows,(uint32 *) pixels,0);
        /*
          Convert image to DirectClass pixel packets.
        */
        p=pixels+number_pixels-1;
        for (y=0; y < (long) image->rows; y++)
        {
          register long
            x;

          register PixelPacket
            *__restrict q;

          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          q+=image->columns-1;
          for (x=0; x < (long) image->columns; x++)
          {
            q->red=ScaleCharToQuantum((unsigned char) TIFFGetR(*p));
            q->green=ScaleCharToQuantum((unsigned char) TIFFGetG(*p));
            q->blue=ScaleCharToQuantum((unsigned char) TIFFGetB(*p));
            if (image->matte != MagickFalse)
              q->opacity=ScaleCharToQuantum((unsigned char) TIFFGetA(*p));
            p--;
            q--;
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
        pixels=(uint32 *) RelinquishMagickMemory(pixels);
        break;
      }
    }
    SetQuantumImageType(image,quantum_type);
  next_tiff_frame:
    if ((photometric == PHOTOMETRIC_LOGL) ||
        (photometric == PHOTOMETRIC_MINISBLACK) ||
        (photometric == PHOTOMETRIC_MINISWHITE))
      {
        image->type=GrayscaleType;
        if (bits_per_sample == 1)
          image->type=BilevelType;
      }
    if (image->storage_class == PseudoClass)
      image->depth=GetImageDepth(image,exception);
    image->endian=MSBEndian;
    if (endian == FILLORDER_LSB2MSB)
      image->endian=LSBEndian;
    if ((photometric == PHOTOMETRIC_LOGL) ||
        (photometric == PHOTOMETRIC_MINISBLACK) ||
        (photometric == PHOTOMETRIC_MINISWHITE))
      {
         image->type=GrayscaleType;
         if (bits_per_sample == 1)
           image->type=BilevelType;
      }
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    status=TIFFReadDirectory(tiff) != 0 ? MagickTrue : MagickFalse;
    if (status == MagickTrue)
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
        status=SetImageProgress(image,LoadImagesTag,image->scene-1,
          image->scene);
        if (status == MagickFalse)
          break;
      }
    quantum_info=DestroyQuantumInfo(quantum_info);
  } while (status == MagickTrue);
  (void) TIFFSetWarningHandler(warning_handler);
  (void) TIFFSetErrorHandler(error_handler);
  TIFFClose(tiff);
  return(GetFirstImageInList(image));
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r T I F F I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterTIFFImage() adds properties for the TIFF image format to
%  the list of supported formats.  The properties include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterTIFFImage method is:
%
%      unsigned long RegisterTIFFImage(void)
%
*/
ModuleExport unsigned long RegisterTIFFImage(void)
{
#define TIFFDescription  "Tagged Image File Format"

  char
    version[MaxTextExtent];

  MagickInfo
    *entry;

  AcquireSemaphoreInfo(&tiff_semaphore);
  if (instantiate_key == MagickFalse)
    {
      if (MagickCreateThreadKey(&tiff_exception) == MagickFalse)
        ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
      instantiate_key=MagickTrue;
    }
  RelinquishSemaphoreInfo(tiff_semaphore);
  *version='\0';
#if defined(TIFF_VERSION)
  (void) FormatMagickString(version,MaxTextExtent,"%d",TIFF_VERSION);
#endif
#if defined(MAGICKCORE_TIFF_DELEGATE)
  {
    const char
      *p;

    register long int
      i;

    p=TIFFGetVersion();
    for (i=0; (i < (MaxTextExtent-1)) && (*p != 0) && (*p != '\n'); i++)
      version[i]=(*p++);
    version[i]='\0';
  }
#endif

  entry=SetMagickInfo("PTIF");
#if defined(MAGICKCORE_TIFF_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadTIFFImage;
  entry->encoder=(EncodeImageHandler *) WritePTIFImage;
#endif
  entry->endian_support=MagickTrue;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=NoThreadSupport;
  entry->description=ConstantString("Pyramid encoded TIFF");
  entry->module=ConstantString("TIFF");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TIF");
#if defined(MAGICKCORE_TIFF_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadTIFFImage;
  entry->encoder=(EncodeImageHandler *) WriteTIFFImage;
#endif
  entry->endian_support=MagickTrue;
  entry->seekable_stream=MagickTrue;
  entry->stealth=MagickTrue;
  entry->thread_support=NoThreadSupport;
  entry->description=ConstantString(TIFFDescription);
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->module=ConstantString("TIFF");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TIFF");
#if defined(MAGICKCORE_TIFF_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadTIFFImage;
  entry->encoder=(EncodeImageHandler *) WriteTIFFImage;
#endif
  entry->magick=(IsImageFormatHandler *) IsTIFF;
  entry->endian_support=MagickTrue;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=NoThreadSupport;
  entry->description=ConstantString(TIFFDescription);
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->module=ConstantString("TIFF");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TIFF64");
#if defined(TIFF_VERSION_BIG)
  entry->decoder=(DecodeImageHandler *) ReadTIFFImage;
  entry->encoder=(EncodeImageHandler *) WritePTIFImage;
#endif
  entry->adjoin=MagickFalse;
  entry->endian_support=MagickTrue;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=NoThreadSupport;
  entry->description=ConstantString("Tagged Image File Format (64-bit)");
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->module=ConstantString("TIFF");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r T I F F I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterTIFFImage() removes format registrations made by the TIFF module
%  from the list of supported formats.
%
%  The format of the UnregisterTIFFImage method is:
%
%      UnregisterTIFFImage(void)
%
*/
ModuleExport void UnregisterTIFFImage(void)
{
  (void) UnregisterMagickInfo("PTIF");
  (void) UnregisterMagickInfo("TIF");
  (void) UnregisterMagickInfo("TIFF");
  (void) UnregisterMagickInfo("TIFF64");
  AcquireSemaphoreInfo(&tiff_semaphore);
  if (instantiate_key != MagickFalse)
    if (MagickDeleteThreadKey(tiff_exception) == MagickFalse)
      ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  instantiate_key=MagickFalse;
  RelinquishSemaphoreInfo(tiff_semaphore);
  DestroySemaphoreInfo(&tiff_semaphore);
}

#if defined(MAGICKCORE_TIFF_DELEGATE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P T I F I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WritePTIFImage() writes an image in the pyrimid-encoded Tagged image file
%  format.
%
%  The format of the WritePTIFImage method is:
%
%      MagickBooleanType WritePTIFImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image:  The image.
%
*/
static MagickBooleanType WritePTIFImage(const ImageInfo *image_info,
  Image *image)
{
  Image
    *images,
    *next,
    *pyramid_image;

  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  unsigned long
    columns,
    rows;

  /*
    Create pyramid-encoded TIFF image.
  */
  images=NewImageList();
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    AppendImageToList(&images,CloneImage(next,0,0,MagickFalse,
      &image->exception));
    columns=next->columns;
    rows=next->rows;
    while ((columns > 64) && (rows > 64))
    {
      columns/=2;
      rows/=2;
      pyramid_image=ResizeImage(next,columns,rows,UndefinedFilter,image->blur,
        &image->exception);
      AppendImageToList(&images,pyramid_image);
    }
  }
  /*
    Write pyramid-encoded TIFF image.
  */
  write_info=CloneImageInfo(image_info);
  *write_info->magick='\0';
  write_info->adjoin=MagickTrue;
  status=WriteTIFFImage(write_info,GetFirstImageInList(images));
  images=DestroyImageList(images);
  write_info=DestroyImageInfo(write_info);
  return(status);
}
#endif

#if defined(MAGICKCORE_TIFF_DELEGATE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%   W r i t e T I F F I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteTIFFImage() writes an image in the Tagged image file format.
%
%  The format of the WriteTIFFImage method is:
%
%      MagickBooleanType WriteTIFFImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image:  The image.
%
*/

typedef struct _TIFFInfo
{
  RectangleInfo
    tile_geometry;

  unsigned char
    *scanline,
    *scanlines,
    *pixels;
} TIFFInfo;

static void DestroyTIFFInfo(TIFFInfo *tiff_info)
{
  assert(tiff_info != (TIFFInfo *) NULL);
  if (tiff_info->scanlines != (unsigned char *) NULL)
    tiff_info->scanlines=(unsigned char *) RelinquishMagickMemory(
      tiff_info->scanlines);
  if (tiff_info->pixels != (unsigned char *) NULL)
    tiff_info->pixels=(unsigned char *) RelinquishMagickMemory(
      tiff_info->pixels);
}

static MagickBooleanType GetTIFFInfo(const ImageInfo *image_info,TIFF *tiff,
  TIFFInfo *tiff_info)
{
  const char
    *option;

  MagickStatusType
    flags;

  assert(tiff_info != (TIFFInfo *) NULL);
  (void) ResetMagickMemory(tiff_info,0,sizeof(*tiff_info));
  option=GetImageOption(image_info,"tiff:tile-geometry");
  if (option == (const char *) NULL)
    return(MagickTrue);
  flags=ParseAbsoluteGeometry(option,&tiff_info->tile_geometry);
  if ((flags & HeightValue) == 0)
    tiff_info->tile_geometry.height=tiff_info->tile_geometry.width;
  (void) TIFFSetField(tiff,TIFFTAG_TILEWIDTH,(uint32)
    tiff_info->tile_geometry.width);
  (void) TIFFSetField(tiff,TIFFTAG_TILELENGTH,(uint32)
    tiff_info->tile_geometry.height);
  tiff_info->scanlines=(unsigned char *) AcquireQuantumMemory((size_t)
    tiff_info->tile_geometry.height*TIFFScanlineSize(tiff),
    sizeof(*tiff_info->scanlines));
  tiff_info->pixels=(unsigned char *) AcquireQuantumMemory((size_t)
    TIFFTileSize(tiff),sizeof(*tiff_info->scanlines));
  if ((tiff_info->scanlines == (unsigned char *) NULL) ||
      (tiff_info->pixels == (unsigned char *) NULL))
    {
      DestroyTIFFInfo(tiff_info);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static int32 TIFFWritePixels(TIFF *tiff,TIFFInfo *tiff_info,long row,
  tsample_t sample,Image *image)
{
  int32
    status;

  long
    bytes_per_pixel,
    j,
    k,
    l;

  register long
    i;

  register unsigned char
    *p,
    *q;

  unsigned long
    number_tiles,
    tile_width;

  if (TIFFIsTiled(tiff) == 0)
    return(TIFFWriteScanline(tiff,tiff_info->scanline,(uint32) row,sample));
  /*
    Fill scanlines to tile height.
  */
  i=(long) (row % tiff_info->tile_geometry.height)*TIFFScanlineSize(tiff);
  (void) CopyMagickMemory(tiff_info->scanlines+i,(char *) tiff_info->scanline,
    (size_t) TIFFScanlineSize(tiff));
  if (((unsigned long) (row % tiff_info->tile_geometry.height) !=
      (tiff_info->tile_geometry.height-1)) && (row != (long) (image->rows-1)))
    return(0);
  /*
    Write tile to TIFF image.
  */
  status=0;
  bytes_per_pixel=TIFFTileSize(tiff)/(long) (tiff_info->tile_geometry.height*
    tiff_info->tile_geometry.width);
  number_tiles=(image->columns+tiff_info->tile_geometry.width)/
    tiff_info->tile_geometry.width;
  for (i=0; i < (long) number_tiles; i++)
  {
    tile_width=(i == (long) (number_tiles-1)) ? image->columns-(i*
      tiff_info->tile_geometry.width) : tiff_info->tile_geometry.width;
    for (j=0; j < (long) ((row % tiff_info->tile_geometry.height)+1); j++)
      for (k=0; k < (long) tile_width; k++)
      {
        if (bytes_per_pixel == 0)
          {
            p=tiff_info->scanlines+(j*TIFFScanlineSize(tiff)+(i*
              tiff_info->tile_geometry.width+k)/8);
            q=tiff_info->pixels+(j*TIFFTileRowSize(tiff)+k/8);
            *q++=(*p++);
            continue;
          }
        p=tiff_info->scanlines+(j*TIFFScanlineSize(tiff)+(i*
          tiff_info->tile_geometry.width+k)*bytes_per_pixel);
        q=tiff_info->pixels+(j*TIFFTileRowSize(tiff)+k*bytes_per_pixel);
        for (l=0; l < bytes_per_pixel; l++)
          *q++=(*p++);
      }
    status=TIFFWriteTile(tiff,tiff_info->pixels,(uint32) (i*
      tiff_info->tile_geometry.width),(uint32) ((row/
      tiff_info->tile_geometry.height)*tiff_info->tile_geometry.height),0,
      sample);
    if (status < 0)
      break;
  }
  return(status);
}

static void TIFFSetProfiles(TIFF *tiff,Image *image)
{
  const char
    *name;

  const StringInfo
    *profile;

  if (image->profiles == (void *) NULL)
    return;
  ResetImageProfileIterator(image);
  for (name=GetNextImageProfile(image); name != (const char *) NULL; )
  {
    profile=GetImageProfile(image,name);
#if defined(TIFFTAG_XMLPACKET)
    if (LocaleCompare(name,"xmp") == 0)
      (void) TIFFSetField(tiff,TIFFTAG_XMLPACKET,(uint32) GetStringInfoLength(
        profile),GetStringInfoDatum(profile));
#endif
#if defined(TIFFTAG_ICCPROFILE)
    if (LocaleCompare(name,"icc") == 0)
      (void) TIFFSetField(tiff,TIFFTAG_ICCPROFILE,(uint32) GetStringInfoLength(
        profile),GetStringInfoDatum(profile));
#endif
    if (LocaleCompare(name,"iptc") == 0)
      {
        size_t
          length;

        StringInfo
          *iptc_profile;

        iptc_profile=CloneStringInfo(profile);
        length=GetStringInfoLength(profile)+4-(GetStringInfoLength(profile) &
          0x03);
        SetStringInfoLength(iptc_profile,length);
        if (TIFFIsByteSwapped(tiff))
          TIFFSwabArrayOfLong((uint32 *) GetStringInfoDatum(iptc_profile),
            (unsigned long) (length/4));
        (void) TIFFSetField(tiff,TIFFTAG_RICHTIFFIPTC,(uint32)
          GetStringInfoLength(iptc_profile)/4,GetStringInfoDatum(iptc_profile));
        iptc_profile=DestroyStringInfo(iptc_profile);
      }
#if defined(TIFFTAG_PHOTOSHOP)
    if (LocaleCompare(name,"8bim") == 0)
      (void) TIFFSetField(tiff,TIFFTAG_PHOTOSHOP,(uint32)
        GetStringInfoLength(profile),GetStringInfoDatum(profile));
#endif
    if (LocaleCompare(name,"tiff:37724") == 0)
      (void) TIFFSetField(tiff,37724,(uint32)GetStringInfoLength(profile),
        GetStringInfoDatum(profile));
    name=GetNextImageProfile(image);
  }
}

static void TIFFSetProperties(TIFF *tiff,Image *image)
{
  const char
    *value;

  value=GetImageProperty(image,"tiff:hostcomputer");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_HOSTCOMPUTER,value);
  value=GetImageProperty(image,"tiff:artist");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_ARTIST,value);
  value=GetImageProperty(image,"tiff:timestamp");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_DATETIME,value);
  value=GetImageProperty(image,"tiff:make");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_MAKE,value);
  value=GetImageProperty(image,"tiff:model");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_MODEL,value);
  (void) TIFFSetField(tiff,TIFFTAG_SOFTWARE,
    GetMagickVersion((unsigned long *) NULL));
  (void) TIFFSetField(tiff,TIFFTAG_DOCUMENTNAME,image->filename);
  value=GetImageProperty(image,"tiff:copyright");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,33432,value);
  value=GetImageProperty(image,"kodak-33423");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,33423,value);
  value=GetImageProperty(image,"kodak-36867");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,36867,value);
  value=GetImageProperty(image,"label");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_PAGENAME,value);
  value=GetImageProperty(image,"comment");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_IMAGEDESCRIPTION,value);
}

static void TIFFSetEXIFProperties(TIFF *tiff,Image *image)
{
#if defined(MAGICKCORE_HAVE_TIFFREADEXIFDIRECTORY)
  const char
    *value;

  register long
    i;

  uint32
    offset;

  /*
    Write EXIF properties.
  */
  offset=0;
  (void) TIFFSetField(tiff,TIFFTAG_SUBIFD,1,&offset);
  for (i=0; exif_info[i].tag != 0; i++)
  {
    value=GetImageProperty(image,exif_info[i].property);
    if (value == (const char *) NULL)
      continue;
    switch (exif_info[i].type)
    {
      case TIFF_ASCII:
      {
        (void) TIFFSetField(tiff,exif_info[i].tag,value);
        break;
      }
      case TIFF_SHORT:
      {
        uint16
          shorty;

        shorty=(uint16) atoi(value);
        (void) TIFFSetField(tiff,exif_info[i].tag,shorty);
        break;
      }
      case TIFF_LONG:
      {
        uint16
          longy;

        longy=(uint16) atol(value);
        (void) TIFFSetField(tiff,exif_info[i].tag,longy);
        break;
      }
      case TIFF_RATIONAL:
      case TIFF_SRATIONAL:
      {
        float
          rational;

        rational=atof(value);
        (void) TIFFSetField(tiff,exif_info[i].tag,rational);
        break;
      }
      default:
        break;
    }
  }
  /* (void) TIFFSetField(tiff,TIFFTAG_EXIFIFD,offset); */
#else
  (void) tiff;
  (void) image;
#endif
}

static MagickBooleanType WriteTIFFImage(const ImageInfo *image_info,
  Image *image)
{
#if !defined(TIFFDefaultStripSize)
#define TIFFDefaultStripSize(tiff,request)  (8192UL/TIFFScanlineSize(tiff))
#endif

  const char
    *mode,
    *option;

  const char
    *value;

  CompressionType
    compression;

  long
    y;

  MagickBooleanType
    debug,
    status;

  MagickOffsetType
    scene;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  register long
    i;

  size_t
    length;

  TIFF
    *tiff;

  TIFFErrorHandler
    error_handler,
    warning_handler;

  TIFFInfo
    tiff_info;

  uint16
    bits_per_sample,
    compress_tag,
    endian,
    photometric;

  uint32
    rows_per_strip;

  unsigned char
    *pixels;

  unsigned long
    lsb_first;

  /*
    Open TIFF file.
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
  (void) MagickSetThreadValue(tiff_exception,&image->exception);
  error_handler=TIFFSetErrorHandler((TIFFErrorHandler) TIFFErrors);
  warning_handler=TIFFSetWarningHandler((TIFFErrorHandler) TIFFWarnings);
  switch (image_info->endian)
  {
    case LSBEndian: mode="wl"; break;
    case MSBEndian: mode="wb"; break;
    default: mode="w"; break;
  }
#if defined(TIFF_VERSION_BIG)
  if (LocaleCompare(image_info->magick,"TIFF64") == 0)
    switch (image_info->endian)
    {
      case LSBEndian: mode="wl8"; break;
      case MSBEndian: mode="wb8"; break;
      default: mode="w8"; break;
    }
#endif
  tiff=TIFFClientOpen(image->filename,mode,(thandle_t) image,TIFFReadBlob,
    TIFFWriteBlob,TIFFSeekBlob,TIFFCloseBlob,TIFFGetBlobSize,TIFFMapBlob,
    TIFFUnmapBlob);
  if (tiff == (TIFF *) NULL)
    {
      (void) TIFFSetWarningHandler(warning_handler);
      (void) TIFFSetErrorHandler(error_handler);
      return(MagickFalse);
    }
  scene=0;
  debug=IsEventLogging();
  do
  {
    /*
      Initialize TIFF fields.
    */
    if ((image_info->type != UndefinedType) &&
        (image_info->type != OptimizeType))
      (void) SetImageType(image,image_info->type);
    quantum_info=AcquireQuantumInfo(image_info,image);
    if (quantum_info == (QuantumInfo *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    if ((image->storage_class != PseudoClass) && (image->depth >= 32) &&
        (quantum_info->format == UndefinedQuantumFormat) &&
        (IsHighDynamicRangeImage(image,&image->exception) != MagickFalse))
      {
        status=SetQuantumFormat(image,quantum_info,FloatingPointQuantumFormat);
        if (status == MagickFalse)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
      }
    if ((LocaleCompare(image_info->magick,"PTIF") == 0) &&
        (GetPreviousImageInList(image) != (Image *) NULL))
      (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_REDUCEDIMAGE);
    if ((image->columns != (uint32) image->columns) ||
        (image->rows != (uint32) image->rows))
      ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
    (void) TIFFSetField(tiff,TIFFTAG_IMAGELENGTH,(uint32) image->rows);
    (void) TIFFSetField(tiff,TIFFTAG_IMAGEWIDTH,(uint32) image->columns);
    compression=image->compression;
    if (image_info->compression != UndefinedCompression)
      compression=image_info->compression;
    switch (compression)
    {
      case FaxCompression:
      {
        compress_tag=COMPRESSION_CCITTFAX3;
        SetQuantumMinIsWhite(quantum_info,MagickTrue);
        break;
      }
      case Group4Compression:
      {
        compress_tag=COMPRESSION_CCITTFAX4;
        SetQuantumMinIsWhite(quantum_info,MagickTrue);
        break;
      }
      case JPEGCompression:
      {
        compress_tag=COMPRESSION_JPEG;
        break;
      }
      case LZWCompression:
      {
        compress_tag=COMPRESSION_LZW;
        break;
      }
      case RLECompression:
      {
        compress_tag=COMPRESSION_PACKBITS;
        break;
      }
      case ZipCompression:
      {
        compress_tag=COMPRESSION_ADOBE_DEFLATE;
        break;
      }
      case NoCompression:
      default:
      {
        compress_tag=COMPRESSION_NONE;
        break;
      }
    }
#if defined(MAGICKCORE_HAVE_TIFFGETCONFIGUREDCODECS) || (TIFFLIB_VERSION > 20040919)
    if (compress_tag != COMPRESSION_NONE)
      {
        TIFFCodec
          *codec;

        codec=TIFFGetConfiguredCODECs();
        while ((codec != (TIFFCodec *) NULL) && (codec->name != (char *) NULL))
        {
          if (codec->scheme == compress_tag)
            break;
          codec++;
        }
        if (((codec == (TIFFCodec *) NULL)) || (codec->scheme != compress_tag))
          {
            (void) ThrowMagickException(&image->exception,GetMagickModule(),
              CoderError,"CompressionNotSupported","`%s'",
              MagickOptionToMnemonic(MagickCompressOptions,(long) compression));
            compress_tag=COMPRESSION_NONE;
            compression=NoCompression;
          }
      }
#else
      switch (compress_tag)
      {
#if defined(CCITT_SUPPORT)
        case COMPRESSION_CCITTFAX3:
        case COMPRESSION_CCITTFAX4:
#endif
#if defined(YCBCR_SUPPORT) && defined(JPEG_SUPPORT)
        case COMPRESSION_JPEG:
#endif
#if defined(LZW_SUPPORT)
        case COMPRESSION_LZW:
#endif
#if defined(PACKBITS_SUPPORT)
        case COMPRESSION_PACKBITS:
#endif
#if defined(ZIP_SUPPORT)
        case COMPRESSION_ADOBE_DEFLATE:
#endif
        case COMPRESSION_NONE:
          break;
        default:
        {
          (void) ThrowMagickException(&image->exception,GetMagickModule(),
            CoderError,"CompressionNotSupported","`%s'",MagickOptionToMnemonic(
              MagickCompressOptions,(long) compression));
          compress_tag=COMPRESSION_NONE;
          compression=NoCompression;
          break;
        }
      }
#endif
    switch (compression)
    {
      case FaxCompression:
      case Group4Compression:
      {
        (void) SetImageType(image,BilevelType);
        break;
      }
      case JPEGCompression:
      {
        (void) SetImageStorageClass(image,DirectClass);
        (void) SetImageDepth(image,8);
        break;
      }
      default:
        break;
    }
    (void) TIFFSetField(tiff,TIFFTAG_COMPRESSION,compress_tag);
    if (image->colorspace == CMYKColorspace)
      {
        photometric=PHOTOMETRIC_SEPARATED;
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,4);
        (void) TIFFSetField(tiff,TIFFTAG_INKSET,INKSET_CMYK);
      }
    else
      {
        /*
          Full color TIFF raster.
        */
        if (image->colorspace == LabColorspace)
          photometric=PHOTOMETRIC_CIELAB;
        else
          if (image->colorspace == YCbCrColorspace)
            {
              photometric=PHOTOMETRIC_YCBCR;
              (void) TIFFSetField(tiff,TIFFTAG_YCBCRSUBSAMPLING,1,1);
              (void) SetImageDepth(image,8);
            }
          else
            {
              if (image->colorspace != RGBColorspace)
                (void) TransformImageColorspace(image,RGBColorspace);
              photometric=PHOTOMETRIC_RGB;
            }
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,3);
        if ((image_info->type != TrueColorType) &&
            (image_info->type != TrueColorMatteType))
          {
            unsigned long
              depth;

            if ((image_info->type != PaletteType) &&
                (IsGrayImage(image,&image->exception) != MagickFalse))
              {
                photometric=(uint16) (quantum_info->min_is_white !=
                  MagickFalse ? PHOTOMETRIC_MINISWHITE :
                  PHOTOMETRIC_MINISBLACK);
                (void) TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,1);
                if ((image_info->depth == 0) &&
                    (IsMonochromeImage(image,&image->exception) != MagickFalse))
                  {
                    status=SetQuantumDepth(image,quantum_info,1);
                    if (status == MagickFalse)
                      ThrowWriterException(ResourceLimitError,
                        "MemoryAllocationFailed");
                  }
              }
            else
              if (image->storage_class == PseudoClass)
                {
                  /*
                    Colormapped TIFF raster.
                  */
                  (void) TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,1);
                  photometric=PHOTOMETRIC_PALETTE;
                  depth=1;
                  while ((GetQuantumRange(depth)+1) < image->colors)
                    depth<<=1;
                  status=SetQuantumDepth(image,quantum_info,depth);
                  if (status == MagickFalse)
                    ThrowWriterException(ResourceLimitError,
                      "MemoryAllocationFailed");
                }
          }
      }
    (void) TIFFSetField(tiff,TIFFTAG_BITSPERSAMPLE,quantum_info->depth);
    if (image->matte != MagickFalse)
      {
        uint16
          extra_samples,
          sample_info[1],
          samples_per_pixel;

        /*
          TIFF has a matte channel.
        */
        extra_samples=1;
        sample_info[0]=EXTRASAMPLE_UNASSALPHA;
        option=GetImageProperty(image,"tiff:alpha");
        if ((option != (const char *) NULL) &&
            (LocaleCompare(option,"associated") == 0))
          sample_info[0]=EXTRASAMPLE_ASSOCALPHA;
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLESPERPIXEL,
          &samples_per_pixel);
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,samples_per_pixel+1);
        (void) TIFFSetField(tiff,TIFFTAG_EXTRASAMPLES,extra_samples,
          &sample_info);
        if (sample_info[0] == EXTRASAMPLE_ASSOCALPHA)
          SetQuantumAlphaType(quantum_info,AssociatedQuantumAlpha);
      }
    (void) TIFFSetField(tiff,TIFFTAG_PHOTOMETRIC,photometric);
    switch (quantum_info->format)
    {
      case FloatingPointQuantumFormat:
      {
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLEFORMAT,SAMPLEFORMAT_IEEEFP);
        (void) TIFFSetField(tiff,TIFFTAG_SMINSAMPLEVALUE,quantum_info->minimum);
        (void) TIFFSetField(tiff,TIFFTAG_SMAXSAMPLEVALUE,quantum_info->maximum);
        break;
      }
      case SignedQuantumFormat:
      {
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLEFORMAT,SAMPLEFORMAT_INT);
        break;
      }
      case UnsignedQuantumFormat:
      {
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLEFORMAT,SAMPLEFORMAT_UINT);
        break;
      }
      default:
        break;
    }
    switch (image->endian)
    {
      case LSBEndian:
      {
        endian=FILLORDER_LSB2MSB;
        break;
      }
      case MSBEndian:
      {
        endian=FILLORDER_MSB2LSB;
        break;
      }
      case UndefinedEndian:
      default:
      {
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_FILLORDER,&endian);
        break;
      }
    }
    lsb_first=1;
    image->endian=MSBEndian;
    if ((int) (*(char *) &lsb_first) != 0)
      image->endian=LSBEndian;
    (void) TIFFSetField(tiff,TIFFTAG_FILLORDER,endian);
    (void) TIFFSetField(tiff,TIFFTAG_ORIENTATION,ORIENTATION_TOPLEFT);
    (void) TIFFSetField(tiff,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG);
    if (photometric == PHOTOMETRIC_RGB)
      if ((image_info->interlace == PlaneInterlace) ||
          (image_info->interlace == PartitionInterlace))
        (void) TIFFSetField(tiff,TIFFTAG_PLANARCONFIG,PLANARCONFIG_SEPARATE);
    rows_per_strip=1;
    if (TIFFScanlineSize(tiff) != 0)
      rows_per_strip=(unsigned long) MagickMax((size_t)
        TIFFDefaultStripSize(tiff,-1),1);
    option=GetImageOption(image_info,"tiff:rows-per-strip");
    if (option != (const char *) NULL)
      rows_per_strip=(unsigned long) strtol(option,(char **) NULL,10);
    switch (compress_tag)
    {
      case COMPRESSION_JPEG:
      {
#if defined(JPEG_SUPPORT)
        const char
          *sampling_factor;

        GeometryInfo
          geometry_info;

        MagickStatusType
          flags;

        rows_per_strip+=(16-(rows_per_strip % 16));
        if (image->quality != 0)
          (void) TIFFSetField(tiff,TIFFTAG_JPEGQUALITY,image->quality);
        if (image_info->quality != UndefinedCompressionQuality)
          (void) TIFFSetField(tiff,TIFFTAG_JPEGQUALITY,image_info->quality);
        (void) TIFFSetField(tiff,TIFFTAG_JPEGCOLORMODE,JPEGCOLORMODE_RAW);
        if (image->colorspace == RGBColorspace)
          {
            (void) TIFFSetField(tiff,TIFFTAG_JPEGCOLORMODE,JPEGCOLORMODE_RGB);
            sampling_factor=(const char *) NULL;
            value=GetImageProperty(image,"jpeg:sampling-factor");
            if (value != (char *) NULL)
              {
                sampling_factor=value;
                if (image->debug != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "  Input sampling-factors=%s",sampling_factor);
              }
            if (image_info->sampling_factor != (char *) NULL)
              sampling_factor=image_info->sampling_factor;
            if (sampling_factor != (const char *) NULL)
              {
                flags=ParseGeometry(sampling_factor,&geometry_info);
                if ((flags & SigmaValue) == 0)
                  geometry_info.sigma=geometry_info.rho;
                (void) TIFFSetField(tiff,TIFFTAG_YCBCRSUBSAMPLING,(uint16)
                  geometry_info.rho,(uint16) geometry_info.sigma);
              }
          }
        if (bits_per_sample == 12)
          (void) TIFFSetField(tiff,TIFFTAG_JPEGTABLESMODE,JPEGTABLESMODE_QUANT);
#endif
        break;
      }
      case COMPRESSION_ADOBE_DEFLATE:
      {
        rows_per_strip=image->rows;
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,
          &bits_per_sample);
        if (((photometric == PHOTOMETRIC_RGB) ||
             (photometric == PHOTOMETRIC_MINISBLACK)) &&
            ((bits_per_sample == 8) || (bits_per_sample == 16)))
          (void) TIFFSetField(tiff,TIFFTAG_PREDICTOR,2);
        (void) TIFFSetField(tiff,TIFFTAG_ZIPQUALITY,image_info->quality ==
          UndefinedCompressionQuality ? 7 : MagickMin(1L*image_info->quality/10,
          9));
        break;
      }
      case COMPRESSION_CCITTFAX3:
      {
        /*
          Byte-aligned EOL.
        */
        rows_per_strip=image->rows;
        (void) TIFFSetField(tiff,TIFFTAG_GROUP3OPTIONS,4);
        break;
      }
      case COMPRESSION_CCITTFAX4:
      {
        rows_per_strip=image->rows;
        break;
      }
      case COMPRESSION_LZW:
      {
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,
          &bits_per_sample);
        if (((photometric == PHOTOMETRIC_RGB) ||
             (photometric == PHOTOMETRIC_MINISBLACK)) &&
            ((bits_per_sample == 8) || (bits_per_sample == 16)))
          (void) TIFFSetField(tiff,TIFFTAG_PREDICTOR,2);
        break;
      }
      default:
        break;
    }
    (void) TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,rows_per_strip);
    if ((image->x_resolution != 0.0) && (image->y_resolution != 0.0))
      {
        unsigned short
          units;

        /*
          Set image resolution.
        */
        units=RESUNIT_NONE;
        if (image->units == PixelsPerInchResolution)
          units=RESUNIT_INCH;
        if (image->units == PixelsPerCentimeterResolution)
          units=RESUNIT_CENTIMETER;
        (void) TIFFSetField(tiff,TIFFTAG_RESOLUTIONUNIT,(uint16) units);
        (void) TIFFSetField(tiff,TIFFTAG_XRESOLUTION,image->x_resolution);
        (void) TIFFSetField(tiff,TIFFTAG_YRESOLUTION,image->y_resolution);
        if ((image->page.x != 0) || (image->page.y != 0))
          {
            /*
              Set image position.
            */
            (void) TIFFSetField(tiff,TIFFTAG_XPOSITION,(float) image->page.x/
              image->x_resolution);
            (void) TIFFSetField(tiff,TIFFTAG_YPOSITION,(float) image->page.y/
              image->y_resolution);
          }
      }
    if (image->chromaticity.white_point.x != 0.0)
      {
        float
          chromaticity[6];

        /*
          Set image chromaticity.
        */
        chromaticity[0]=(float) image->chromaticity.red_primary.x;
        chromaticity[1]=(float) image->chromaticity.red_primary.y;
        chromaticity[2]=(float) image->chromaticity.green_primary.x;
        chromaticity[3]=(float) image->chromaticity.green_primary.y;
        chromaticity[4]=(float) image->chromaticity.blue_primary.x;
        chromaticity[5]=(float) image->chromaticity.blue_primary.y;
        (void) TIFFSetField(tiff,TIFFTAG_PRIMARYCHROMATICITIES,chromaticity);
        chromaticity[0]=(float) image->chromaticity.white_point.x;
        chromaticity[1]=(float) image->chromaticity.white_point.y;
        (void) TIFFSetField(tiff,TIFFTAG_WHITEPOINT,chromaticity);
      }
    if ((image_info->adjoin != MagickFalse) && (GetImageListLength(image) > 1))
      {
        (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_PAGE);
        if (image->scene != 0)
          (void) TIFFSetField(tiff,TIFFTAG_PAGENUMBER,(uint16) image->scene,
            GetImageListLength(image));
      }
    if (image->orientation != UndefinedOrientation)
      (void) TIFFSetField(tiff,TIFFTAG_ORIENTATION,(uint16) image->orientation);
    (void) TIFFSetProfiles(tiff,image);
    {
      uint16
        page,
        pages;

      page=(uint16) scene;
      pages=GetImageListLength(image);
      if ((image_info->adjoin != MagickFalse) && (pages > 1))
        (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_PAGE);
      (void) TIFFSetField(tiff,TIFFTAG_PAGENUMBER,page,pages);
    }
    (void) TIFFSetProperties(tiff,image);
    if (0)
      (void) TIFFSetEXIFProperties(tiff,image);
    /*
      Write image scanlines.
    */
    if (GetTIFFInfo(image_info,tiff,&tiff_info) == MagickFalse)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    pixels=GetQuantumPixels(quantum_info);
    tiff_info.scanline=GetQuantumPixels(quantum_info);
    switch (photometric)
    {
      case PHOTOMETRIC_CIELAB:
      case PHOTOMETRIC_YCBCR:
      case PHOTOMETRIC_RGB:
      {
        /*
          RGB TIFF image.
        */
        switch (image_info->interlace)
        {
          case NoInterlace:
          default:
          {
            quantum_type=RGBQuantum;
            if (image->matte != MagickFalse)
              quantum_type=RGBAQuantum;
            for (y=0; y < (long) image->rows; y++)
            {
              register const PixelPacket
                *__restrict p;

              p=GetVirtualPixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              length=ExportQuantumPixels(image,(const CacheView *) NULL,
                quantum_info,quantum_type,pixels,&image->exception);
              if (TIFFWritePixels(tiff,&tiff_info,y,0,image) == -1)
                break;
              if (image->previous == (Image *) NULL)
                {
                  status=SetImageProgress(image,SaveImageTag,y,image->rows);
                  if (status == MagickFalse)
                    break;
                }
            }
            break;
          }
          case PlaneInterlace:
          case PartitionInterlace:
          {
            /*
              Plane interlacing:  RRRRRR...GGGGGG...BBBBBB...
            */
            for (y=0; y < (long) image->rows; y++)
            {
              register const PixelPacket
                *__restrict p;

              p=GetVirtualPixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              length=ExportQuantumPixels(image,(const CacheView *) NULL,
                quantum_info,RedQuantum,pixels,&image->exception);
              if (TIFFWritePixels(tiff,&tiff_info,y,0,image) == -1)
                break;
            }
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,SaveImageTag,100,400);
                if (status == MagickFalse)
                  break;
              }
            for (y=0; y < (long) image->rows; y++)
            {
              register const PixelPacket
                *__restrict p;

              p=GetVirtualPixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              length=ExportQuantumPixels(image,(const CacheView *) NULL,
                quantum_info,GreenQuantum,pixels,&image->exception);
              if (TIFFWritePixels(tiff,&tiff_info,y,1,image) == -1)
                break;
            }
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,SaveImageTag,200,400);
                if (status == MagickFalse)
                  break;
              }
            for (y=0; y < (long) image->rows; y++)
            {
              register const PixelPacket
                *__restrict p;

              p=GetVirtualPixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              length=ExportQuantumPixels(image,(const CacheView *) NULL,
                quantum_info,BlueQuantum,pixels,&image->exception);
              if (TIFFWritePixels(tiff,&tiff_info,y,2,image) == -1)
                break;
            }
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,SaveImageTag,300,400);
                if (status == MagickFalse)
                  break;
              }
            if (image->matte != MagickFalse)
              for (y=0; y < (long) image->rows; y++)
              {
              register const PixelPacket
                *__restrict p;

                p=GetVirtualPixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                length=ExportQuantumPixels(image,(const CacheView *) NULL,
                  quantum_info,AlphaQuantum,pixels,&image->exception);
                if (TIFFWritePixels(tiff,&tiff_info,y,3,image) == -1)
                  break;
              }
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,SaveImageTag,400,400);
                if (status == MagickFalse)
                  break;
              }
            break;
          }
        }
        break;
      }
      case PHOTOMETRIC_SEPARATED:
      {
        /*
          CMYK TIFF image.
        */
        quantum_type=CMYKQuantum;
        if (image->matte != MagickFalse)
          quantum_type=CMYKAQuantum;
        if (image->colorspace != CMYKColorspace)
          (void) TransformImageColorspace(image,CMYKColorspace);
        for (y=0; y < (long) image->rows; y++)
        {
          register const PixelPacket
            *__restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          length=ExportQuantumPixels(image,(const CacheView *) NULL,
            quantum_info,quantum_type,pixels,&image->exception);
          if (TIFFWritePixels(tiff,&tiff_info,y,0,image) == -1)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case PHOTOMETRIC_PALETTE:
      {
        uint16
          *blue,
          *green,
          *red;

        /*
          Colormapped TIFF image.
        */
        red=(uint16 *) AcquireQuantumMemory(65536,sizeof(*red));
        green=(uint16 *) AcquireQuantumMemory(65536,sizeof(*green));
        blue=(uint16 *) AcquireQuantumMemory(65536,sizeof(*blue));
        if ((red == (uint16 *) NULL) || (green == (uint16 *) NULL) ||
            (blue == (uint16 *) NULL))
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        /*
          Initialize TIFF colormap.
        */
        (void) ResetMagickMemory(red,0,65536*sizeof(*red));
        (void) ResetMagickMemory(green,0,65536*sizeof(*green));
        (void) ResetMagickMemory(blue,0,65536*sizeof(*blue));
        for (i=0; i < (long) image->colors; i++)
        {
          red[i]=ScaleQuantumToShort(image->colormap[i].red);
          green[i]=ScaleQuantumToShort(image->colormap[i].green);
          blue[i]=ScaleQuantumToShort(image->colormap[i].blue);
        }
        (void) TIFFSetField(tiff,TIFFTAG_COLORMAP,red,green,blue);
        red=(uint16 *) RelinquishMagickMemory(red);
        green=(uint16 *) RelinquishMagickMemory(green);
        blue=(uint16 *) RelinquishMagickMemory(blue);
      }
      default:
      {
        /*
          Convert PseudoClass packets to contiguous grayscale scanlines.
        */
        quantum_type=IndexQuantum;
        if (image->matte != MagickFalse)
          {
            if (photometric != PHOTOMETRIC_PALETTE)
              quantum_type=GrayAlphaQuantum;
            else
              quantum_type=IndexAlphaQuantum;
           }
         else
           if (photometric != PHOTOMETRIC_PALETTE)
             quantum_type=GrayQuantum;
        for (y=0; y < (long) image->rows; y++)
        {
          register const PixelPacket
            *__restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          length=ExportQuantumPixels(image,(const CacheView *) NULL,
            quantum_info,quantum_type,pixels,&image->exception);
          if (TIFFWritePixels(tiff,&tiff_info,y,0,image) == -1)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,y,image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
    }
    quantum_info=DestroyQuantumInfo(quantum_info);
    DestroyTIFFInfo(&tiff_info);
    if (0 && (image_info->verbose == MagickTrue))
      TIFFPrintDirectory(tiff,stdout,MagickFalse);
    (void) TIFFWriteDirectory(tiff);
    image->endian=MSBEndian;
    if (endian == FILLORDER_LSB2MSB)
      image->endian=LSBEndian;
    image=SyncNextImageInList(image);
    if (image == (Image *) NULL)
      break;
    status=SetImageProgress(image,SaveImagesTag,scene++,
      GetImageListLength(image));
    if (status == MagickFalse)
      break;
  } while (image_info->adjoin != MagickFalse);
  (void) TIFFSetWarningHandler(warning_handler);
  (void) TIFFSetErrorHandler(error_handler);
  TIFFClose(tiff);
  return(MagickTrue);
}
#endif
