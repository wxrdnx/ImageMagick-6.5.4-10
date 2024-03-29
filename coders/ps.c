/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                               PPPP   SSSSS                                  %
%                               P   P  SS                                     %
%                               PPPP    SSS                                   %
%                               P         SS                                  %
%                               P      SSSSS                                  %
%                                                                             %
%                                                                             %
%                         Read/Write Postscript Format                        %
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
#include "magick/delegate.h"
#include "magick/delegate-private.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/monitor-private.h"
#include "magick/option.h"
#include "magick/profile.h"
#include "magick/resource_.h"
#include "magick/pixel-private.h"
#include "magick/property.h"
#include "magick/quantum-private.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/module.h"
#include "magick/token.h"
#include "magick/transform.h"
#include "magick/utility.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WritePSImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n v o k e P o s t s r i p t D e l e g a t e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InvokePostscriptDelegate() executes the postscript interpreter with the
%  specified command.
%
%  The format of the InvokePostscriptDelegate method is:
%
%      MagickBooleanType InvokePostscriptDelegate(
%        const MagickBooleanType verbose,const char *command)
%
%  A description of each parameter follows:
%
%    o verbose: A value other than zero displays the command prior to
%      executing it.
%
%    o command: the address of a character string containing the command to
%      execute.
%
*/
static MagickBooleanType InvokePostscriptDelegate(
  const MagickBooleanType verbose,const char *command)
{
#if defined(MAGICKCORE_GS_DELEGATE) || defined(__WINDOWS__)
  char
    **argv;

  const GhostscriptVectors
    *gs_func;

  gs_main_instance
    *interpreter;

  int
    argc,
    code,
    status;

  register long
    i;

#if defined(__WINDOWS__)
  gs_func=NTGhostscriptDLLVectors();
#else
  GhostscriptVectors
    gs_func_struct;

  gs_func=(&gs_func_struct);
  (void) ResetMagickMemory(&gs_func,0,sizeof(gs_func));
  gs_func_struct.new_instance=(int (*)(gs_main_instance **,void *))
    gsapi_new_instance;
  gs_func_struct.init_with_args=(int (*)(gs_main_instance *,int,char **))
    gsapi_init_with_args;
  gs_func_struct.run_string=(int (*)(gs_main_instance *,const char *,int,int *))
    gsapi_run_string;
  gs_func_struct.delete_instance=(void (*)(gs_main_instance *))
    gsapi_delete_instance;
  gs_func_struct.exit=(int (*)(gs_main_instance *)) gsapi_exit;
#endif
  if (gs_func == (GhostscriptVectors *) NULL)
    return(SystemCommand(verbose,command) == 0 ? MagickFalse : MagickTrue);
  if (verbose != MagickFalse)
    {
      (void) fputs("[ghostscript library]",stdout);
      (void) fputs(strchr(command,' '),stdout);
    }
  status=(gs_func->new_instance)(&interpreter,(void *) NULL);
  if (status < 0)
    return(SystemCommand(verbose,command) == 0 ? MagickFalse : MagickTrue);
  argv=StringToArgv(command,&argc);
  status=(gs_func->init_with_args)(interpreter,argc-1,argv+1);
  if (status == 0)
    status=(gs_func->run_string)(interpreter,"systemdict /start get exec\n",0,
      &code);
  (gs_func->exit)(interpreter);
  (gs_func->delete_instance)(interpreter);
#if defined(__WINDOWS__)
  NTGhostscriptUnLoadDLL();
#endif
  for (i=0; i < (long) argc; i++)
    argv[i]=DestroyString(argv[i]);
  argv=(char **) RelinquishMagickMemory(argv);
  if ((status == 0) || (status == -101))
    return(MagickFalse);
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "Ghostscript returns status %d, exit code %d",status,code);
  return(MagickTrue);
#else
  return(SystemCommand(verbose,command) != 0 ? MagickTrue : MagickFalse);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s P S                                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPS() returns MagickTrue if the image format type, identified by the
%  magick string, is PS.
%
%  The format of the IsPS method is:
%
%      MagickBooleanType IsPS(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: compare image format pattern against these bytes.
%
%    o length: Specifies the length of the magick string.
%
*/
static MagickBooleanType IsPS(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (memcmp(magick,"%!",2) == 0)
    return(MagickTrue);
  if (memcmp(magick,"\004%!",3) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P S I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPSImage() reads a Postscript image file and returns it.  It allocates
%  the memory necessary for the new Image structure and returns a pointer
%  to the new image.
%
%  The format of the ReadPSImage method is:
%
%      Image *ReadPSImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickBooleanType IsPostscriptRendered(const char *path)
{
  MagickBooleanType
    status;

  struct stat
    attributes;

  if ((path == (const char *) NULL) || (*path == '\0'))
    return(MagickFalse);
  status=GetPathAttributes(path,&attributes);
  if ((status != MagickFalse) && S_ISREG(attributes.st_mode) &&
      (attributes.st_size > 0))
    return(MagickTrue);
  return(MagickFalse);
}

static inline int ProfileInteger(Image *image,short int *hex_digits)
{
  int
    c,
    l,
    value;

  register long
    i;

  l=0;
  value=0;
  for (i=0; i < 2; )
  {
    c=ReadBlobByte(image);
    if ((c == EOF) || ((c == '%') && (l == '%')))
      {
        value=(-1);
        break;
      }
    l=c;
    c&=0xff;
    if (isxdigit(c) == MagickFalse)
      continue;
    value=(int) ((unsigned long) value << 4)+hex_digits[c];
    i++;
  }
  return(value);
}

static Image *ReadPSImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define BoundingBox  "BoundingBox:"
#define BeginDocument  "BeginDocument:"
#define BeginXMPPacket  "<?xpacket begin="
#define EndXMPPacket  "<?xpacket end="
#define ICCProfile "BeginICCProfile:"
#define CMYKCustomColor  "CMYKCustomColor:"
#define DocumentMedia  "DocumentMedia:"
#define DocumentCustomColors  "DocumentCustomColors:"
#define DocumentProcessColors  "DocumentProcessColors:"
#define EndDocument  "EndDocument:"
#define HiResBoundingBox  "HiResBoundingBox:"
#define ImageData  "ImageData:"
#define PageBoundingBox  "PageBoundingBox:"
#define LanguageLevel  "LanguageLevel:"
#define PageMedia  "PageMedia:"
#define Pages  "Pages:"
#define PhotoshopProfile  "BeginPhotoshop:"
#define PostscriptLevel  "!PS-"
#define RenderPostscriptText  "  Rendering Postscript...  "
#define SpotColor  "+ "

  char
    command[MaxTextExtent],
    density[MaxTextExtent],
    filename[MaxTextExtent],
    geometry[MaxTextExtent],
    input_filename[MaxTextExtent],
    options[MaxTextExtent],
    postscript_filename[MaxTextExtent],
    translate_geometry[MaxTextExtent];

  const char
    *option;

  const DelegateInfo
    *delegate_info;

  GeometryInfo
    geometry_info;

  Image
    *image,
    *next,
    *postscript_image;

  ImageInfo
    *read_info;

  int
    c,
    file;

  MagickBooleanType
    cmyk,
    skip,
    status;

  MagickStatusType
    flags;

  PointInfo
    delta;

  RectangleInfo
    page;

  register char
    *p;

  register long
    i;

  SegmentInfo
    bounds,
    hires_bounds;

  short int
    hex_digits[256];

  size_t
    length;

  ssize_t
    count;

  StringInfo
    *profile;

  unsigned long
    columns,
    extent,
    language_level,
    pages,
    rows,
    scene,
    spotcolor;

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
  status=AcquireUniqueSymbolicLink(image_info->filename,input_filename);
  if (status == MagickFalse)
    {
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        image_info->filename);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Initialize hex values.
  */
  (void) ResetMagickMemory(hex_digits,0,sizeof(hex_digits));
  hex_digits[(int) '0']=0;
  hex_digits[(int) '1']=1;
  hex_digits[(int) '2']=2;
  hex_digits[(int) '3']=3;
  hex_digits[(int) '4']=4;
  hex_digits[(int) '5']=5;
  hex_digits[(int) '6']=6;
  hex_digits[(int) '7']=7;
  hex_digits[(int) '8']=8;
  hex_digits[(int) '9']=9;
  hex_digits[(int) 'a']=10;
  hex_digits[(int) 'b']=11;
  hex_digits[(int) 'c']=12;
  hex_digits[(int) 'd']=13;
  hex_digits[(int) 'e']=14;
  hex_digits[(int) 'f']=15;
  hex_digits[(int) 'A']=10;
  hex_digits[(int) 'B']=11;
  hex_digits[(int) 'C']=12;
  hex_digits[(int) 'D']=13;
  hex_digits[(int) 'E']=14;
  hex_digits[(int) 'F']=15;
  /*
    Set the page density.
  */
  delta.x=DefaultResolution;
  delta.y=DefaultResolution;
  if ((image->x_resolution == 0.0) || (image->y_resolution == 0.0))
    {
      flags=ParseGeometry(PSDensityGeometry,&geometry_info);
      image->x_resolution=geometry_info.rho;
      image->y_resolution=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        image->y_resolution=image->x_resolution;
    }
  /*
    Determine page geometry from the Postscript bounding box.
  */
  (void) ResetMagickMemory(&bounds,0,sizeof(bounds));
  (void) ResetMagickMemory(&hires_bounds,0,sizeof(hires_bounds));
  (void) ResetMagickMemory(&page,0,sizeof(page));
  (void) ResetMagickMemory(command,0,sizeof(command));
  hires_bounds.x2=0.0;
  hires_bounds.y2=0.0;
  columns=0;
  rows=0;
  extent=0;
  spotcolor=0;
  language_level=1;
  skip=MagickFalse;
  cmyk=image_info->colorspace == CMYKColorspace ? MagickTrue : MagickFalse;
  pages=(~0UL);
  p=command;
  for (c=ReadBlobByte(image); c != EOF; c=ReadBlobByte(image))
  {
    /*
      Note document structuring comments.
    */
    *p++=(char) c;
    if ((strchr("\n\r%",c) == (char *) NULL) &&
        ((size_t) (p-command) < (MaxTextExtent-1)))
      continue;
    *p='\0';
    p=command;
    /*
      Skip %%BeginDocument thru %%EndDocument.
    */
    if (LocaleNCompare(BeginDocument,command,strlen(BeginDocument)) == 0)
      skip=MagickTrue;
    if (LocaleNCompare(EndDocument,command,strlen(EndDocument)) == 0)
      skip=MagickFalse;
    if (skip != MagickFalse)
      continue;
    if (LocaleNCompare(PostscriptLevel,command,strlen(PostscriptLevel)) == 0)
      {
        (void) SetImageProperty(image,"ps:Level",command+4);
        if (GlobExpression(command,"*EPSF-*",MagickTrue) != MagickFalse)
          pages=1;
      }
    if (LocaleNCompare(LanguageLevel,command,strlen(LanguageLevel)) == 0)
      (void) sscanf(command,LanguageLevel " %lu",&language_level);
    if (LocaleNCompare(Pages,command,strlen(Pages)) == 0)
      (void) sscanf(command,Pages " %lu",&pages);
    if (LocaleNCompare(ImageData,command,strlen(Pages)) == 0)
      (void) sscanf(command,ImageData " %lu %lu",&columns,&rows);
    if (LocaleNCompare(ICCProfile,command,strlen(PhotoshopProfile)) == 0)
      {
        unsigned char
          *datum;

        /*
          Read ICC profile.
        */
        profile=AcquireStringInfo(65536);
        for (i=0; (c=ProfileInteger(image,hex_digits)) != EOF; i++)
        {
          SetStringInfoLength(profile,(size_t) i+1);
          datum=GetStringInfoDatum(profile);
          datum[i]=(unsigned char) c;
        }
        (void) SetImageProfile(image,"icc",profile);
        profile=DestroyStringInfo(profile);
        continue;
      }
    if (LocaleNCompare(PhotoshopProfile,command,strlen(PhotoshopProfile)) == 0)
      {
        unsigned char
          *p;

        /*
          Read Photoshop profile.
        */
        count=(ssize_t) sscanf(command,PhotoshopProfile " %lu",&extent);
        if (count != 1)
          continue;
        length=extent;
        profile=AcquireStringInfo(length);
        p=GetStringInfoDatum(profile);
        for (i=0; i < (long) length; i++)
          *p++=(unsigned char) ProfileInteger(image,hex_digits);
        (void) SetImageProfile(image,"8bim",profile);
        profile=DestroyStringInfo(profile);
        continue;
      }
    if (LocaleNCompare(BeginXMPPacket,command,strlen(BeginXMPPacket)) == 0)
      {
        register size_t
          i;

        /*
          Read XMP profile.
        */
        p=command;
        profile=StringToStringInfo(command);
        for (i=GetStringInfoLength(profile)-1; c != EOF; i++)
        {
          SetStringInfoLength(profile,i+1);
          c=ReadBlobByte(image);
          GetStringInfoDatum(profile)[i]=(unsigned char) c;
          *p++=(char) c;
          if ((strchr("\n\r%",c) == (char *) NULL) &&
              ((size_t) (p-command) < (MaxTextExtent-1)))
            continue;
          *p='\0';
          p=command;
          if (LocaleNCompare(EndXMPPacket,command,strlen(EndXMPPacket)) == 0)
            break;
        }
        SetStringInfoLength(profile,i);
        (void) SetImageProfile(image,"xmp",profile);
        profile=DestroyStringInfo(profile);
        continue;
      }
    /*
      Is this a CMYK document?
    */
    length=strlen(DocumentProcessColors);
    if (LocaleNCompare(DocumentProcessColors,command,length) == 0)
      {
        if ((GlobExpression(command,"*Cyan*",MagickTrue) != MagickFalse) ||
            (GlobExpression(command,"*Magenta*",MagickTrue) != MagickFalse) ||
            (GlobExpression(command,"*Yellow*",MagickTrue) != MagickFalse))
          cmyk=MagickTrue;
      }
    if (LocaleNCompare(CMYKCustomColor,command,strlen(CMYKCustomColor)) == 0)
      cmyk=MagickTrue;
    length=strlen(DocumentCustomColors);
    if ((LocaleNCompare(DocumentCustomColors,command,length) == 0) ||
        (LocaleNCompare(CMYKCustomColor,command,strlen(CMYKCustomColor)) == 0) ||
        (LocaleNCompare(SpotColor,command,strlen(SpotColor)) == 0))
      {
        char
          property[MaxTextExtent],
          *value;

        register char
          *p;

        /*
          Note spot names.
        */
        (void) FormatMagickString(property,MaxTextExtent,"ps:SpotColor-%lu",
          spotcolor++);
        for (p=command; *p != '\0'; p++)
          if (isspace((int) (unsigned char) *p) != 0)
            break;
        value=AcquireString(p);
        (void) SubstituteString(&value,"(","");
        (void) SubstituteString(&value,")","");
        (void) StripString(value);
        (void) SetImageProperty(image,property,value);
        value=DestroyString(value);
        continue;
      }
    /*
      Note region defined by bounding box.
    */
    count=0;
    if (LocaleNCompare(BoundingBox,command,strlen(BoundingBox)) == 0)
      count=(ssize_t) sscanf(command,BoundingBox " %lf %lf %lf %lf",&bounds.x1,
        &bounds.y1,&bounds.x2,&bounds.y2);
    if (LocaleNCompare(DocumentMedia,command,strlen(DocumentMedia)) == 0)
      count=(ssize_t) sscanf(command,DocumentMedia " %*s %lf %lf",&bounds.x2,
        &bounds.y2)+2;
    if (LocaleNCompare(HiResBoundingBox,command,strlen(HiResBoundingBox)) == 0)
      count=(ssize_t) sscanf(command,HiResBoundingBox " %lf %lf %lf %lf",
        &bounds.x1,&bounds.y1,&bounds.x2,&bounds.y2);
    if (LocaleNCompare(PageBoundingBox,command,strlen(PageBoundingBox)) == 0)
      count=(ssize_t) sscanf(command,PageBoundingBox " %lf %lf %lf %lf",
        &bounds.x1,&bounds.y1,&bounds.x2,&bounds.y2);
    if (LocaleNCompare(PageMedia,command,strlen(PageMedia)) == 0)
      count=(ssize_t) sscanf(command,PageMedia " %*s %lf %lf",&bounds.x2,
        &bounds.y2)+2;
    if (count != 4)
      continue;
    if (((bounds.x2 > hires_bounds.x2) && (bounds.y2 > hires_bounds.y2)) ||
        ((hires_bounds.x2 == 0.0) && (hires_bounds.y2 == 0.0)))
      {
        /*
          Set Postscript render geometry.
        */
        (void) FormatMagickString(geometry,MaxTextExtent,"%gx%g%+g%+g",
          bounds.x2-bounds.x1,bounds.y2-bounds.y1,bounds.x1,bounds.y1);
        (void) SetImageProperty(image,"ps:HiResBoundingBox",geometry);
        page.width=(unsigned long) (bounds.x2-bounds.x1+0.5);
        page.height=(unsigned long) (bounds.y2-bounds.y1+0.5);
        hires_bounds=bounds;
      }
  }
  (void) CloseBlob(image);
  if (image_info->colorspace == RGBColorspace)
    cmyk=MagickFalse;
  /*
    Create Ghostscript control file.
  */
  file=AcquireUniqueFileResource(postscript_filename);
  if (file == -1)
    {
      ThrowFileException(&image->exception,FileOpenError,"UnableToOpenFile",
        image_info->filename);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  (void) CopyMagickString(command,"/setpagedevice {pop} bind 1 index where {"
    "dup wcheck {3 1 roll put} {pop def} ifelse} {def} ifelse\n"
    "<</UseCIEColor true>>setpagedevice\n",MaxTextExtent);
  count=write(file,command,(unsigned int) strlen(command));
  (void) FormatMagickString(translate_geometry,MaxTextExtent,
    "%g %g translate\n",-bounds.x1,-bounds.y1);
  count=write(file,translate_geometry,strlen(translate_geometry));
  file=close(file)-1;
  /*
    Render Postscript with the Ghostscript delegate.
  */
  if (image_info->monochrome != MagickFalse)
    delegate_info=GetDelegateInfo("ps:mono",(char *) NULL,exception);
  else
    if (cmyk != MagickFalse)
      delegate_info=GetDelegateInfo("ps:cmyk",(char *) NULL,exception);
    else
      if (pages == 1)
        delegate_info=GetDelegateInfo("ps:alpha",(char *) NULL,exception);
      else
        delegate_info=GetDelegateInfo("ps:color",(char *) NULL,exception);
  if (delegate_info == (const DelegateInfo *) NULL)
    {
      (void) RelinquishUniqueFileResource(postscript_filename);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  *options='\0';
  if ((page.width == 0) || (page.height == 0))
    (void) ParseAbsoluteGeometry(PSPageGeometry,&page);
  if (image_info->page != (char *) NULL)
    (void) ParseAbsoluteGeometry(image_info->page,&page);
  (void) FormatMagickString(density,MaxTextExtent,"%gx%g",image->x_resolution,
    image->y_resolution);
  page.width=(unsigned long) (page.width*image->x_resolution/delta.x+0.5);
  page.height=(unsigned long) (page.height*image->y_resolution/delta.y+0.5);
  (void) FormatMagickString(options,MaxTextExtent,"-g%lux%lu ",
    page.width,page.height);
  read_info=CloneImageInfo(image_info);
  *read_info->magick='\0';
  if (read_info->number_scenes != 0)
    {
      char
        pages[MaxTextExtent];

      (void) FormatMagickString(pages,MaxTextExtent,"-dFirstPage=%lu "
        "-dLastPage=%lu",read_info->scene+1,read_info->scene+
        read_info->number_scenes);
      (void) ConcatenateMagickString(options,pages,MaxTextExtent);
      read_info->number_scenes=0;
      if (read_info->scenes != (char *) NULL)
        *read_info->scenes='\0';
    }
  option=GetImageOption(image_info,"ps:use-cropbox");
  if ((option != (const char *) NULL) && (IsMagickTrue(option) != MagickFalse))
    (void) ConcatenateMagickString(options,"-dEPSCrop ",MaxTextExtent);
  (void) CopyMagickString(filename,read_info->filename,MaxTextExtent);
  (void) AcquireUniqueFilename(read_info->filename);
  (void) FormatMagickString(command,MaxTextExtent,
    GetDelegateCommands(delegate_info),
    read_info->antialias != MagickFalse ? 4 : 1,
    read_info->antialias != MagickFalse ? 4 : 1,density,options,
    read_info->filename,postscript_filename,input_filename);
  status=InvokePostscriptDelegate(read_info->verbose,command);
  if ((status != MagickFalse) ||
      (IsPostscriptRendered(read_info->filename) == MagickFalse))
    {
      (void) ConcatenateMagickString(command," -c showpage",MaxTextExtent);
      status=InvokePostscriptDelegate(read_info->verbose,command);
    }
  postscript_image=(Image *) NULL;
  if (status == MagickFalse)
    postscript_image=ReadImage(read_info,exception);
  (void) RelinquishUniqueFileResource(postscript_filename);
  (void) RelinquishUniqueFileResource(read_info->filename);
  (void) RelinquishUniqueFileResource(input_filename);
  read_info=DestroyImageInfo(read_info);
  if (postscript_image == (Image *) NULL)
    {
      image=DestroyImageList(image);
      ThrowFileException(exception,DelegateError,"PostscriptDelegateFailed",
        image_info->filename);
      return((Image *) NULL);
    }
  if (LocaleCompare(postscript_image->magick,"BMP") == 0)
    {
      Image
        *cmyk_image;

      cmyk_image=ConsolidateCMYKImages(postscript_image,exception);
      if (cmyk_image != (Image *) NULL)
        {
          postscript_image=DestroyImageList(postscript_image);
          postscript_image=cmyk_image;
        }
    }
  if (image_info->number_scenes != 0)
    {
      Image
        *clone_image;

      register long
        i;

      /*
        Add place holder images to meet the subimage specification requirement.
      */
      for (i=0; i < (long) image_info->scene; i++)
      {
        clone_image=CloneImage(postscript_image,1,1,MagickTrue,exception);
        if (clone_image != (Image *) NULL)
          PrependImageToList(&postscript_image,clone_image);
      }
    }
  do
  {
    (void) CopyMagickString(postscript_image->filename,filename,MaxTextExtent);
    if (columns != 0)
      postscript_image->magick_columns=columns;
    if (rows != 0)
      postscript_image->magick_rows=rows;
    postscript_image->page=page;
    (void) CloneImageProfiles(postscript_image,image);
    (void) CloneImageProperties(postscript_image,image);
    next=SyncNextImageInList(postscript_image);
    if (next != (Image *) NULL)
      postscript_image=next;
  } while (next != (Image *) NULL);
  image=DestroyImageList(image);
  scene=0;
  for (next=GetFirstImageInList(postscript_image); next != (Image *) NULL; )
  {
    next->scene=scene++;
    next=GetNextImageInList(next);
  }
  return(GetFirstImageInList(postscript_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P S I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPSImage() adds properties for the PS image format to
%  the list of supported formats.  The properties include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPSImage method is:
%
%      unsigned long RegisterPSImage(void)
%
*/
ModuleExport unsigned long RegisterPSImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("EPI");
  entry->decoder=(DecodeImageHandler *) ReadPSImage;
  entry->encoder=(EncodeImageHandler *) WritePSImage;
  entry->magick=(IsImageFormatHandler *) IsPS;
  entry->adjoin=MagickFalse;
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=EncoderThreadSupport;
  entry->description=ConstantString(
   "Encapsulated PostScript Interchange format");
  entry->module=ConstantString("PS");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("EPS");
  entry->decoder=(DecodeImageHandler *) ReadPSImage;
  entry->encoder=(EncodeImageHandler *) WritePSImage;
  entry->magick=(IsImageFormatHandler *) IsPS;
  entry->adjoin=MagickFalse;
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=EncoderThreadSupport;
  entry->description=ConstantString("Encapsulated PostScript");
  entry->module=ConstantString("PS");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("EPSF");
  entry->decoder=(DecodeImageHandler *) ReadPSImage;
  entry->encoder=(EncodeImageHandler *) WritePSImage;
  entry->magick=(IsImageFormatHandler *) IsPS;
  entry->adjoin=MagickFalse;
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->description=ConstantString("Encapsulated PostScript");
  entry->module=ConstantString("PS");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("EPSI");
  entry->decoder=(DecodeImageHandler *) ReadPSImage;
  entry->encoder=(EncodeImageHandler *) WritePSImage;
  entry->magick=(IsImageFormatHandler *) IsPS;
  entry->adjoin=MagickFalse;
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=EncoderThreadSupport;
  entry->description=ConstantString(
    "Encapsulated PostScript Interchange format");
  entry->module=ConstantString("PS");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PS");
  entry->decoder=(DecodeImageHandler *) ReadPSImage;
  entry->encoder=(EncodeImageHandler *) WritePSImage;
  entry->magick=(IsImageFormatHandler *) IsPS;
  entry->module=ConstantString("PS");
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=EncoderThreadSupport;
  entry->description=ConstantString("PostScript");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P S I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPSImage() removes format registrations made by the
%  PS module from the list of supported formats.
%
%  The format of the UnregisterPSImage method is:
%
%      UnregisterPSImage(void)
%
*/
ModuleExport void UnregisterPSImage(void)
{
  (void) UnregisterMagickInfo("EPI");
  (void) UnregisterMagickInfo("EPS");
  (void) UnregisterMagickInfo("EPSF");
  (void) UnregisterMagickInfo("EPSI");
  (void) UnregisterMagickInfo("PS");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P S I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WritePSImage translates an image to encapsulated Postscript
%  Level I for printing.  If the supplied geometry is null, the image is
%  centered on the Postscript page.  Otherwise, the image is positioned as
%  specified by the geometry.
%
%  The format of the WritePSImage method is:
%
%      MagickBooleanType WritePSImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image: the image.
%
*/

static inline size_t MagickMin(const size_t x,const size_t y)
{
  if (x < y)
    return(x);
  return(y);
}

static inline unsigned char *PopHexPixel(const char **hex_digits,
  const unsigned long pixel,unsigned char *pixels)
{
  register const char
    *hex;

  hex=hex_digits[pixel];
  *pixels++=(unsigned char) (*hex++);
  *pixels++=(unsigned char) (*hex);
  return(pixels);
}

static MagickBooleanType WritePSImage(const ImageInfo *image_info,Image *image)
{
#define WriteRunlengthPacket(image,pixel,length,p) \
{ \
  if ((image->matte != MagickFalse) && \
      (p->opacity == (Quantum) TransparentOpacity)) \
    { \
      q=PopHexPixel(hex_digits,0xff,q); \
      q=PopHexPixel(hex_digits,0xff,q); \
      q=PopHexPixel(hex_digits,0xff,q); \
    } \
  else \
    { \
      q=PopHexPixel(hex_digits,ScaleQuantumToChar(pixel.red),q); \
      q=PopHexPixel(hex_digits,ScaleQuantumToChar(pixel.green),q); \
      q=PopHexPixel(hex_digits,ScaleQuantumToChar(pixel.blue),q); \
    } \
  q=PopHexPixel(hex_digits,(const unsigned long) MagickMin(length,0xff),q); \
}

  static const char
    *hex_digits[] =
    {
      "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B",
      "0C", "0D", "0E", "0F", "10", "11", "12", "13", "14", "15", "16", "17",
      "18", "19", "1A", "1B", "1C", "1D", "1E", "1F", "20", "21", "22", "23",
      "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
      "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B",
      "3C", "3D", "3E", "3F", "40", "41", "42", "43", "44", "45", "46", "47",
      "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", "50", "51", "52", "53",
      "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
      "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B",
      "6C", "6D", "6E", "6F", "70", "71", "72", "73", "74", "75", "76", "77",
      "78", "79", "7A", "7B", "7C", "7D", "7E", "7F", "80", "81", "82", "83",
      "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
      "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B",
      "9C", "9D", "9E", "9F", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7",
      "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF", "B0", "B1", "B2", "B3",
      "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
      "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB",
      "CC", "CD", "CE", "CF", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
      "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF", "E0", "E1", "E2", "E3",
      "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
      "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB",
      "FC", "FD", "FE", "FF",  (char *) NULL
    },
    *PostscriptProlog[]=
    {
      "%%BeginProlog",
      "%",
      "% Display a color image.  The image is displayed in color on",
      "% Postscript viewers or printers that support color, otherwise",
      "% it is displayed as grayscale.",
      "%",
      "/DirectClassPacket",
      "{",
      "  %",
      "  % Get a DirectClass packet.",
      "  %",
      "  % Parameters:",
      "  %   red.",
      "  %   green.",
      "  %   blue.",
      "  %   length: number of pixels minus one of this color (optional).",
      "  %",
      "  currentfile color_packet readhexstring pop pop",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 3 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add 3 mul def",
      "  } ifelse",
      "  0 3 number_pixels 1 sub",
      "  {",
      "    pixels exch color_packet putinterval",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/DirectClassImage",
      "{",
      "  %",
      "  % Display a DirectClass image.",
      "  %",
      "  systemdict /colorimage known",
      "  {",
      "    columns rows 8",
      "    [",
      "      columns 0 0",
      "      rows neg 0 rows",
      "    ]",
      "    { DirectClassPacket } false 3 colorimage",
      "  }",
      "  {",
      "    %",
      "    % No colorimage operator;  convert to grayscale.",
      "    %",
      "    columns rows 8",
      "    [",
      "      columns 0 0",
      "      rows neg 0 rows",
      "    ]",
      "    { GrayDirectClassPacket } image",
      "  } ifelse",
      "} bind def",
      "",
      "/GrayDirectClassPacket",
      "{",
      "  %",
      "  % Get a DirectClass packet;  convert to grayscale.",
      "  %",
      "  % Parameters:",
      "  %   red",
      "  %   green",
      "  %   blue",
      "  %   length: number of pixels minus one of this color (optional).",
      "  %",
      "  currentfile color_packet readhexstring pop pop",
      "  color_packet 0 get 0.299 mul",
      "  color_packet 1 get 0.587 mul add",
      "  color_packet 2 get 0.114 mul add",
      "  cvi",
      "  /gray_packet exch def",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 1 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add def",
      "  } ifelse",
      "  0 1 number_pixels 1 sub",
      "  {",
      "    pixels exch gray_packet put",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/GrayPseudoClassPacket",
      "{",
      "  %",
      "  % Get a PseudoClass packet;  convert to grayscale.",
      "  %",
      "  % Parameters:",
      "  %   index: index into the colormap.",
      "  %   length: number of pixels minus one of this color (optional).",
      "  %",
      "  currentfile byte readhexstring pop 0 get",
      "  /offset exch 3 mul def",
      "  /color_packet colormap offset 3 getinterval def",
      "  color_packet 0 get 0.299 mul",
      "  color_packet 1 get 0.587 mul add",
      "  color_packet 2 get 0.114 mul add",
      "  cvi",
      "  /gray_packet exch def",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 1 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add def",
      "  } ifelse",
      "  0 1 number_pixels 1 sub",
      "  {",
      "    pixels exch gray_packet put",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/PseudoClassPacket",
      "{",
      "  %",
      "  % Get a PseudoClass packet.",
      "  %",
      "  % Parameters:",
      "  %   index: index into the colormap.",
      "  %   length: number of pixels minus one of this color (optional).",
      "  %",
      "  currentfile byte readhexstring pop 0 get",
      "  /offset exch 3 mul def",
      "  /color_packet colormap offset 3 getinterval def",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 3 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add 3 mul def",
      "  } ifelse",
      "  0 3 number_pixels 1 sub",
      "  {",
      "    pixels exch color_packet putinterval",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/PseudoClassImage",
      "{",
      "  %",
      "  % Display a PseudoClass image.",
      "  %",
      "  % Parameters:",
      "  %   class: 0-PseudoClass or 1-Grayscale.",
      "  %",
      "  currentfile buffer readline pop",
      "  token pop /class exch def pop",
      "  class 0 gt",
      "  {",
      "    currentfile buffer readline pop",
      "    token pop /depth exch def pop",
      "    /grays columns 8 add depth sub depth mul 8 idiv string def",
      "    columns rows depth",
      "    [",
      "      columns 0 0",
      "      rows neg 0 rows",
      "    ]",
      "    { currentfile grays readhexstring pop } image",
      "  }",
      "  {",
      "    %",
      "    % Parameters:",
      "    %   colors: number of colors in the colormap.",
      "    %   colormap: red, green, blue color packets.",
      "    %",
      "    currentfile buffer readline pop",
      "    token pop /colors exch def pop",
      "    /colors colors 3 mul def",
      "    /colormap colors string def",
      "    currentfile colormap readhexstring pop pop",
      "    systemdict /colorimage known",
      "    {",
      "      columns rows 8",
      "      [",
      "        columns 0 0",
      "        rows neg 0 rows",
      "      ]",
      "      { PseudoClassPacket } false 3 colorimage",
      "    }",
      "    {",
      "      %",
      "      % No colorimage operator;  convert to grayscale.",
      "      %",
      "      columns rows 8",
      "      [",
      "        columns 0 0",
      "        rows neg 0 rows",
      "      ]",
      "      { GrayPseudoClassPacket } image",
      "    } ifelse",
      "  } ifelse",
      "} bind def",
      "",
      "/DisplayImage",
      "{",
      "  %",
      "  % Display a DirectClass or PseudoClass image.",
      "  %",
      "  % Parameters:",
      "  %   x & y translation.",
      "  %   x & y scale.",
      "  %   label pointsize.",
      "  %   image label.",
      "  %   image columns & rows.",
      "  %   class: 0-DirectClass or 1-PseudoClass.",
      "  %   compression: 0-none or 1-RunlengthEncoded.",
      "  %   hex color packets.",
      "  %",
      "  gsave",
      "  /buffer 512 string def",
      "  /byte 1 string def",
      "  /color_packet 3 string def",
      "  /pixels 768 string def",
      "",
      "  currentfile buffer readline pop",
      "  token pop /x exch def",
      "  token pop /y exch def pop",
      "  x y translate",
      "  currentfile buffer readline pop",
      "  token pop /x exch def",
      "  token pop /y exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /pointsize exch def pop",
      "  /Times-Roman findfont pointsize scalefont setfont",
      (char *) NULL
    },
    *PostscriptEpilog[]=
    {
      "  x y scale",
      "  currentfile buffer readline pop",
      "  token pop /columns exch def",
      "  token pop /rows exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /class exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /compression exch def pop",
      "  class 0 gt { PseudoClassImage } { DirectClassImage } ifelse",
      (char *) NULL
    };

  char
    buffer[MaxTextExtent],
    date[MaxTextExtent],
    **labels,
    page_geometry[MaxTextExtent];

  const char
    **s,
    *value;

  const StringInfo
    *profile;

  double
    pointsize;

  GeometryInfo
    geometry_info;

  IndexPacket
    index;

  long
    j,
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  MagickStatusType
    flags;

  PixelPacket
    pixel;

  PointInfo
    delta,
    resolution,
    scale;

  RectangleInfo
    geometry,
    media_info,
    page_info;

  register const IndexPacket
    *indexes;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  register unsigned char
    *q;

  SegmentInfo
    bounds;

  size_t
    length;

  time_t
    timer;

  unsigned char
    pixels[2048];

  unsigned long
    bit,
    byte,
    page,
    text_size;

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
  (void) ResetMagickMemory(&bounds,0,sizeof(bounds));
  page=1;
  scene=0;
  do
  {
    /*
      Scale relative to dots-per-inch.
    */
    if ((image->colorspace != RGBColorspace) &&
        (image->colorspace != CMYKColorspace))
      (void) TransformImageColorspace(image,RGBColorspace);
    delta.x=DefaultResolution;
    delta.y=DefaultResolution;
    resolution.x=image->x_resolution;
    resolution.y=image->y_resolution;
    if ((resolution.x == 0.0) || (resolution.y == 0.0))
      {
        flags=ParseGeometry(PSDensityGeometry,&geometry_info);
        resolution.x=geometry_info.rho;
        resolution.y=geometry_info.sigma;
        if ((flags & SigmaValue) == 0)
          resolution.y=resolution.x;
      }
    if (image_info->density != (char *) NULL)
      {
        flags=ParseGeometry(image_info->density,&geometry_info);
        resolution.x=geometry_info.rho;
        resolution.y=geometry_info.sigma;
        if ((flags & SigmaValue) == 0)
          resolution.y=resolution.x;
      }
    if (image->units == PixelsPerCentimeterResolution)
      {
        resolution.x*=2.54;
        resolution.y*=2.54;
      }
    SetGeometry(image,&geometry);
    (void) FormatMagickString(page_geometry,MaxTextExtent,"%lux%lu",
      image->columns,image->rows);
    if (image_info->page != (char *) NULL)
      (void) CopyMagickString(page_geometry,image_info->page,MaxTextExtent);
    else
      if ((image->page.width != 0) && (image->page.height != 0))
        (void) FormatMagickString(page_geometry,MaxTextExtent,"%lux%lu%+ld%+ld",
          image->page.width,image->page.height,image->page.x,image->page.y);
      else
        if ((image->gravity != UndefinedGravity) &&
            (LocaleCompare(image_info->magick,"PS") == 0))
          (void) CopyMagickString(page_geometry,PSPageGeometry,MaxTextExtent);
    (void) ConcatenateMagickString(page_geometry,">",MaxTextExtent);
    (void) ParseMetaGeometry(page_geometry,&geometry.x,&geometry.y,
      &geometry.width,&geometry.height);
    scale.x=(double) (geometry.width*delta.x)/resolution.x;
    geometry.width=(unsigned long) (scale.x+0.5);
    scale.y=(double) (geometry.height*delta.y)/resolution.y;
    geometry.height=(unsigned long) (scale.y+0.5);
    (void) ParseAbsoluteGeometry(page_geometry,&media_info);
    (void) ParseGravityGeometry(image,page_geometry,&page_info,
      &image->exception);
    if (image->gravity != UndefinedGravity)
      {
        geometry.x=(-page_info.x);
        geometry.y=(long) (media_info.height+page_info.y-image->rows);
      }
    pointsize=12.0;
    if (image_info->pointsize != 0.0)
      pointsize=image_info->pointsize;
    text_size=0;
    value=GetImageProperty(image,"label");
    if (value != (const char *) NULL)
      text_size=(unsigned long) (MultilineCensus(value)*pointsize+12);
    if (page == 1)
      {
        /*
          Output Postscript header.
        */
        if (LocaleCompare(image_info->magick,"PS") == 0)
          (void) CopyMagickString(buffer,"%!PS-Adobe-3.0\n",MaxTextExtent);
        else
          (void) CopyMagickString(buffer,"%!PS-Adobe-3.0 EPSF-3.0\n",
            MaxTextExtent);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,"%%Creator: (ImageMagick)\n");
        (void) FormatMagickString(buffer,MaxTextExtent,"%%%%Title: (%s)\n",
          image->filename);
        (void) WriteBlobString(image,buffer);
        timer=time((time_t *) NULL);
        (void) FormatMagickTime(timer,MaxTextExtent,date);
        (void) FormatMagickString(buffer,MaxTextExtent,
          "%%%%CreationDate: (%s)\n",date);
        (void) WriteBlobString(image,buffer);
        bounds.x1=(double) geometry.x;
        bounds.y1=(double) geometry.y;
        bounds.x2=(double) geometry.x+scale.x;
        bounds.y2=(double) geometry.y+(geometry.height+text_size);
        if ((image_info->adjoin != MagickFalse) &&
            (GetNextImageInList(image) != (Image *) NULL))
          (void) CopyMagickString(buffer,"%%%%BoundingBox: (atend)\n",
            MaxTextExtent);
        else
          {
            (void) FormatMagickString(buffer,MaxTextExtent,
              "%%%%BoundingBox: %ld %ld %ld %ld\n",(long) (bounds.x1+0.5),
              (long) (bounds.y1+0.5),(long) (bounds.x2+0.5),
              (long) (bounds.y2+0.5));
            (void) WriteBlobString(image,buffer);
            (void) FormatMagickString(buffer,MaxTextExtent,
              "%%%%HiResBoundingBox: %g %g %g %g\n",bounds.x1,bounds.y1,
              bounds.x2,bounds.y2);
          }
        (void) WriteBlobString(image,buffer);
        profile=GetImageProfile(image,"8bim");
        if (profile != (StringInfo *) NULL)
          {
            /*
              Embed Photoshop profile.
            */
            (void) FormatMagickString(buffer,MaxTextExtent,
              "%%BeginPhotoshop: %lu",(unsigned long) GetStringInfoLength(
              profile));
            (void) WriteBlobString(image,buffer);
            for (i=0; i < (long) GetStringInfoLength(profile); i++)
            {
              if ((i % 32) == 0)
                (void) WriteBlobString(image,"\n% ");
              (void) FormatMagickString(buffer,MaxTextExtent,"%02X",
                (unsigned int) (GetStringInfoDatum(profile)[i] & 0xff));
              (void) WriteBlobString(image,buffer);
            }
            (void) WriteBlobString(image,"\n%EndPhotoshop\n");
          }
        profile=GetImageProfile(image,"xmp");
        if (profile != (StringInfo *) NULL)
          {
            /*
              Embed XML profile.
            */
            (void) WriteBlobString(image,"\n%begin_xml_code\n");
            for (i=0; i < (long) GetStringInfoLength(profile); i++)
              (void) WriteBlobByte(image,GetStringInfoDatum(profile)[i]);
            (void) WriteBlobString(image,"\n%end_xml_code\n");
          }
        value=GetImageProperty(image,"label");
        if (value != (const char *) NULL)
          (void) WriteBlobString(image,
            "%%DocumentNeededResources: font Times-Roman\n");
        (void) WriteBlobString(image,"%%DocumentData: Clean7Bit\n");
        (void) WriteBlobString(image,"%%LanguageLevel: 1\n");
        if (LocaleCompare(image_info->magick,"PS") != 0)
          (void) WriteBlobString(image,"%%Pages: 1\n");
        else
          {
            /*
              Compute the number of pages.
            */
            (void) WriteBlobString(image,"%%Orientation: Portrait\n");
            (void) WriteBlobString(image,"%%PageOrder: Ascend\n");
            (void) FormatMagickString(buffer,MaxTextExtent,"%%%%Pages: %lu\n",
              image_info->adjoin != MagickFalse ? (unsigned long)
              GetImageListLength(image) : 1UL);
            (void) WriteBlobString(image,buffer);
          }
        (void) WriteBlobString(image,"%%EndComments\n");
        (void) WriteBlobString(image,"\n%%BeginDefaults\n");
        (void) WriteBlobString(image,"%%EndDefaults\n\n");
        if ((LocaleCompare(image_info->magick,"EPI") == 0) ||
            (LocaleCompare(image_info->magick,"EPSI") == 0) ||
            (LocaleCompare(image_info->magick,"EPT") == 0))
          {
            Image
              *preview_image;

            long
              y;

            Quantum
              pixel;

            register long
              x;

            /*
              Create preview image.
            */
            preview_image=CloneImage(image,0,0,MagickTrue,&image->exception);
            if (preview_image == (Image *) NULL)
              ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
            /*
              Dump image as bitmap.
            */
            (void) FormatMagickString(buffer,MaxTextExtent,
              "%%%%BeginPreview: %lu %lu %lu %lu\n%%  ",preview_image->columns,
              preview_image->rows,1L,(((preview_image->columns+7) >> 3)*
              preview_image->rows+35)/36);
            (void) WriteBlobString(image,buffer);
            q=pixels;
            for (y=0; y < (long) image->rows; y++)
            {
              p=GetVirtualPixels(preview_image,0,y,preview_image->columns,1,
                &preview_image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              indexes=GetVirtualIndexQueue(preview_image);
              bit=0;
              byte=0;
              for (x=0; x < (long) preview_image->columns; x++)
              {
                byte<<=1;
                pixel=PixelIntensityToQuantum(p);
                if (pixel >= (Quantum) (QuantumRange/2))
                  byte|=0x01;
                bit++;
                if (bit == 8)
                  {
                    q=PopHexPixel(hex_digits,byte,q);
                    if ((q-pixels+8) >= 80)
                      {
                        *q++='\n';
                        (void) WriteBlob(image,q-pixels,pixels);
                        q=pixels;
                        (void) WriteBlobString(image,"%  ");
                      };
                    bit=0;
                    byte=0;
                  }
              }
              if (bit != 0)
                {
                  byte<<=(8-bit);
                  q=PopHexPixel(hex_digits,byte,q);
                  if ((q-pixels+8) >= 80)
                    {
                      *q++='\n';
                      (void) WriteBlob(image,q-pixels,pixels);
                      q=pixels;
                      (void) WriteBlobString(image,"%  ");
                    };
                };
            }
            if (q != pixels)
              {
                *q++='\n';
                (void) WriteBlob(image,q-pixels,pixels);
              }
            (void) WriteBlobString(image,"\n%%EndPreview\n");
            preview_image=DestroyImage(preview_image);
          }
        /*
          Output Postscript commands.
        */
        for (s=PostscriptProlog; *s != (char *) NULL; s++)
        {
          (void) FormatMagickString(buffer,MaxTextExtent,"%s\n",*s);
          (void) WriteBlobString(image,buffer);
        }
        value=GetImageProperty(image,"label");
        if (value != (const char *) NULL)
          for (j=(long) MultilineCensus(value)-1; j >= 0; j--)
          {
            (void) WriteBlobString(image,"  /label 512 string def\n");
            (void) WriteBlobString(image,"  currentfile label readline pop\n");
            (void) FormatMagickString(buffer,MaxTextExtent,
              "  0 y %g add moveto label show pop\n",j*pointsize+12);
            (void) WriteBlobString(image,buffer);
          }
        for (s=PostscriptEpilog; *s != (char *) NULL; s++)
        {
          (void) FormatMagickString(buffer,MaxTextExtent,"%s\n",*s);
          (void) WriteBlobString(image,buffer);
        }
        if (LocaleCompare(image_info->magick,"PS") == 0)
          (void) WriteBlobString(image,"  showpage\n");
        (void) WriteBlobString(image,"} bind def\n");
        (void) WriteBlobString(image,"%%EndProlog\n");
      }
    (void) FormatMagickString(buffer,MaxTextExtent,"%%%%Page:  1 %lu\n",page++);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,
      "%%%%PageBoundingBox: %ld %ld %ld %ld\n",geometry.x,geometry.y,
      geometry.x+(long) geometry.width,geometry.y+(long) (geometry.height+
      text_size));
    (void) WriteBlobString(image,buffer);
    if ((double) geometry.x < bounds.x1)
      bounds.x1=(double) geometry.x;
    if ((double) geometry.y < bounds.y1)
      bounds.y1=(double) geometry.y;
    if ((double) (geometry.x+geometry.width-1) > bounds.x2)
      bounds.x2=(double) geometry.x+geometry.width-1;
    if ((double) (geometry.y+(geometry.height+text_size)-1) > bounds.y2)
      bounds.y2=(double) geometry.y+(geometry.height+text_size)-1;
    value=GetImageProperty(image,"label");
    if (value != (const char *) NULL)
      (void) WriteBlobString(image,"%%%%PageResources: font Times-Roman\n");
    if (LocaleCompare(image_info->magick,"PS") != 0)
      (void) WriteBlobString(image,"userdict begin\n");
    (void) WriteBlobString(image,"DisplayImage\n");
    /*
      Output image data.
    */
    (void) FormatMagickString(buffer,MaxTextExtent,"%ld %ld\n%g %g\n%f\n",
      geometry.x,geometry.y,scale.x,scale.y,pointsize);
    (void) WriteBlobString(image,buffer);
    labels=(char **) NULL;
    value=GetImageProperty(image,"label");
    if (value != (const char *) NULL)
      labels=StringToList(value);
    if (labels != (char **) NULL)
      {
        for (i=0; labels[i] != (char *) NULL; i++)
        {
          (void) FormatMagickString(buffer,MaxTextExtent,"%s \n",
            labels[i]);
          (void) WriteBlobString(image,buffer);
          labels[i]=DestroyString(labels[i]);
        }
        labels=(char **) RelinquishMagickMemory(labels);
      }
    (void) ResetMagickMemory(&pixel,0,sizeof(pixel));
    pixel.opacity=(Quantum) TransparentOpacity;
    index=(IndexPacket) 0;
    x=0;
    if ((image_info->type != TrueColorType) &&
        (IsGrayImage(image,&image->exception) != MagickFalse))
      {
        if (IsMonochromeImage(image,&image->exception) == MagickFalse)
          {
            Quantum
              pixel;

            /*
              Dump image as grayscale.
            */
            (void) FormatMagickString(buffer,MaxTextExtent,
              "%lu %lu\n1\n1\n1\n8\n",image->columns,image->rows);
            (void) WriteBlobString(image,buffer);
            q=pixels;
            for (y=0; y < (long) image->rows; y++)
            {
              p=GetVirtualPixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              for (x=0; x < (long) image->columns; x++)
              {
                pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
                q=PopHexPixel(hex_digits,pixel,q);
                i++;
                if ((q-pixels+8) >= 80)
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
          }
        else
          {
            long
              y;

            Quantum
              pixel;

            /*
              Dump image as bitmap.
            */
            (void) FormatMagickString(buffer,MaxTextExtent,
              "%lu %lu\n1\n1\n1\n1\n",image->columns,image->rows);
            (void) WriteBlobString(image,buffer);
            q=pixels;
            for (y=0; y < (long) image->rows; y++)
            {
              p=GetVirtualPixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              indexes=GetVirtualIndexQueue(image);
              bit=0;
              byte=0;
              for (x=0; x < (long) image->columns; x++)
              {
                byte<<=1;
                pixel=PixelIntensityToQuantum(p);
                if (pixel >= (Quantum) (QuantumRange/2))
                  byte|=0x01;
                bit++;
                if (bit == 8)
                  {
                    q=PopHexPixel(hex_digits,byte,q);
                    if ((q-pixels+2) >= 80)
                      {
                        *q++='\n';
                        (void) WriteBlob(image,q-pixels,pixels);
                        q=pixels;
                      };
                    bit=0;
                    byte=0;
                  }
                p++;
              }
              if (bit != 0)
                {
                  byte<<=(8-bit);
                  q=PopHexPixel(hex_digits,byte,q);
                  if ((q-pixels+2) >= 80)
                    {
                      *q++='\n';
                      (void) WriteBlob(image,q-pixels,pixels);
                      q=pixels;
                    }
                };
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
          }
      }
    else
      if ((image->storage_class == DirectClass) ||
          (image->colors > 256) || (image->matte != MagickFalse))
        {
          /*
            Dump DirectClass image.
          */
          (void) FormatMagickString(buffer,MaxTextExtent,"%lu %lu\n0\n%d\n",
            image->columns,image->rows,
            image_info->compression == RLECompression ? 1 : 0);
          (void) WriteBlobString(image,buffer);
          switch (image_info->compression)
          {
            case RLECompression:
            {
              /*
                Dump runlength-encoded DirectColor packets.
              */
              q=pixels;
              for (y=0; y < (long) image->rows; y++)
              {
                p=GetVirtualPixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                pixel=(*p);
                length=255;
                for (x=0; x < (long) image->columns; x++)
                {
                  if ((p->red == pixel.red) && (p->green == pixel.green) &&
                      (p->blue == pixel.blue) &&
                      (p->opacity == pixel.opacity) && (length < 255) &&
                      (x < (long) (image->columns-1)))
                    length++;
                  else
                    {
                      if (x > 0)
                        {
                          WriteRunlengthPacket(image,pixel,length,p);
                          if ((q-pixels+10) >= 80)
                            {
                              *q++='\n';
                              (void) WriteBlob(image,q-pixels,pixels);
                              q=pixels;
                            }
                        }
                      length=0;
                    }
                  pixel=(*p);
                  p++;
                }
                WriteRunlengthPacket(image,pixel,length,p);
                if ((q-pixels+10) >= 80)
                  {
                    *q++='\n';
                    (void) WriteBlob(image,q-pixels,pixels);
                    q=pixels;
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
            case NoCompression:
            default:
            {
              /*
                Dump uncompressed DirectColor packets.
              */
              q=pixels;
              for (y=0; y < (long) image->rows; y++)
              {
                p=GetVirtualPixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                for (x=0; x < (long) image->columns; x++)
                {
                  if ((image->matte != MagickFalse) &&
                      (p->opacity == (Quantum) TransparentOpacity))
                    {
                      q=PopHexPixel(hex_digits,0xff,q);
                      q=PopHexPixel(hex_digits,0xff,q);
                      q=PopHexPixel(hex_digits,0xff,q);
                    }
                  else
                    {
                      q=PopHexPixel(hex_digits,ScaleQuantumToChar(p->red),q);
                      q=PopHexPixel(hex_digits,ScaleQuantumToChar(p->green),q);
                      q=PopHexPixel(hex_digits,ScaleQuantumToChar(p->blue),q);
                    }
                  if ((q-pixels+6) >= 80)
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
          }
          (void) WriteBlobByte(image,'\n');
        }
      else
        {
          /*
            Dump PseudoClass image.
          */
          (void) FormatMagickString(buffer,MaxTextExtent,"%lu %lu\n%d\n%d\n0\n",
            image->columns,image->rows,
            image->storage_class == PseudoClass ? 1 : 0,
            image_info->compression == RLECompression ? 1 : 0);
          (void) WriteBlobString(image,buffer);
          /*
            Dump number of colors and colormap.
          */
          (void) FormatMagickString(buffer,MaxTextExtent,"%lu\n",image->colors);
          (void) WriteBlobString(image,buffer);
          for (i=0; i < (long) image->colors; i++)
          {
            (void) FormatMagickString(buffer,MaxTextExtent,"%02X%02X%02X\n",
              ScaleQuantumToChar(image->colormap[i].red),
              ScaleQuantumToChar(image->colormap[i].green),
              ScaleQuantumToChar(image->colormap[i].blue));
            (void) WriteBlobString(image,buffer);
          }
          switch (image_info->compression)
          {
            case RLECompression:
            {
              /*
                Dump runlength-encoded PseudoColor packets.
              */
              q=pixels;
              for (y=0; y < (long) image->rows; y++)
              {
                p=GetVirtualPixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                indexes=GetVirtualIndexQueue(image);
                index=(*indexes);
                length=255;
                for (x=0; x < (long) image->columns; x++)
                {
                  if ((index == indexes[x]) && (length < 255) &&
                      (x < ((long) image->columns-1)))
                    length++;
                  else
                    {
                      if (x > 0)
                        {
                          q=PopHexPixel(hex_digits,index,q);
                          q=PopHexPixel(hex_digits,(unsigned long)
                            MagickMin(length,0xff),q);
                          i++;
                          if ((q-pixels+6) >= 80)
                            {
                              *q++='\n';
                              (void) WriteBlob(image,q-pixels,pixels);
                              q=pixels;
                            }
                        }
                      length=0;
                    }
                  index=indexes[x];
                  pixel=(*p);
                  p++;
                }
                q=PopHexPixel(hex_digits,index,q);
                q=PopHexPixel(hex_digits,(unsigned long)
                  MagickMin(length,0xff),q);
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
            case NoCompression:
            default:
            {
              /*
                Dump uncompressed PseudoColor packets.
              */
              q=pixels;
              for (y=0; y < (long) image->rows; y++)
              {
                p=GetVirtualPixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                indexes=GetVirtualIndexQueue(image);
                for (x=0; x < (long) image->columns; x++)
                {
                  q=PopHexPixel(hex_digits,indexes[x],q);
                  if ((q-pixels+4) >= 80)
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
          }
          (void) WriteBlobByte(image,'\n');
        }
    if (LocaleCompare(image_info->magick,"PS") != 0)
      (void) WriteBlobString(image,"end\n");
    (void) WriteBlobString(image,"%%PageTrailer\n");
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=SetImageProgress(image,SaveImagesTag,scene++,
      GetImageListLength(image));
    if (status == MagickFalse)
      break;
  } while (image_info->adjoin != MagickFalse);
  (void) WriteBlobString(image,"%%Trailer\n");
  if (page > 2)
    {
      (void) FormatMagickString(buffer,MaxTextExtent,
        "%%%%BoundingBox: %ld %ld %ld %ld\n",(long) (bounds.x1+0.5),
        (long) (bounds.y1+0.5),(long) (bounds.x2+0.5),(long) (bounds.y2+0.5));
      (void) WriteBlobString(image,buffer);
      (void) FormatMagickString(buffer,MaxTextExtent,
        "%%%%HiResBoundingBox: %g %g %g %g\n",bounds.x1,bounds.y1,bounds.x2,
        bounds.y2);
      (void) WriteBlobString(image,buffer);
    }
  (void) WriteBlobString(image,"%%EOF\n");
  (void) CloseBlob(image);
  return(MagickTrue);
}
