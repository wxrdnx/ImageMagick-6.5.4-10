/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                AAA   N   N   AAA   L      Y   Y  ZZZZZ  EEEEE               %
%               A   A  NN  N  A   A  L       Y Y      ZZ  E                   %
%               AAAAA  N N N  AAAAA  L        Y     ZZZ   EEE                 %
%               A   A  N  NN  A   A  L        Y    ZZ     E                   %
%               A   A  N   N  A   A  LLLLL    Y    ZZZZZ  EEEEE               %
%                                                                             %
%               Methods to Compute a Information about an Image               %
%                                                                             %
%                                                                             %
%                             Software Design                                 %
%                               Bill Corbis                                   %
%                              December 1998                                  %
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
*/

/*
  Include declarations.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include "magick/MagickCore.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   a n a l y z e I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  analyzeImage() computes the brightness and saturation mean,  standard
%  deviation, kurtosis and skewness and stores these values as attributes 
%  of the image.
%
%  The format of the analyzeImage method is:
%
%      unsigned long analyzeImage(Image *images,const int argc,
%        char **argv,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the address of a structure of type Image.
%
%    o argc: Specifies a pointer to an integer describing the number of
%      elements in the argument vector.
%
%    o argv: Specifies a pointer to a text array containing the command line
%      arguments.
%
%    o exception: return any errors or warnings in this structure.
%
*/
ModuleExport unsigned long analyzeImage(Image **images,const int argc,
  const char **argv,ExceptionInfo *exception)
{
  char
    text[MaxTextExtent];

  double
    area,
    brightness_mean,
    brightness_standard_deviation,
    brightness_kurtosis,
    brightness_skewness,
    brightness_sum_x,
    brightness_sum_x2,
    brightness_sum_x3,
    brightness_sum_x4,
    saturation_mean,
    saturation_standard_deviation,
    saturation_kurtosis,
    saturation_skewness,
    saturation_sum_x,
    saturation_sum_x2,
    saturation_sum_x3,
    saturation_sum_x4;

  double
    brightness,
    hue,
    saturation;

  Image
    *image;

  long
    y;

  register const PixelPacket
    *p;

  register long
    x;

  assert(images != (Image **) NULL);
  assert(*images != (Image *) NULL);
  assert((*images)->signature == MagickSignature);
  (void) argc;
  (void) argv;
  image=(*images);
  for ( ; image != (Image *) NULL; image=GetNextImageInList(image))
  {
    brightness_sum_x=0.0;
    brightness_sum_x2=0.0;
    brightness_sum_x3=0.0;
    brightness_sum_x4=0.0;
    brightness_mean=0.0;
    brightness_standard_deviation=0.0;
    brightness_kurtosis=0.0;
    brightness_skewness=0.0;
    saturation_sum_x=0.0;
    saturation_sum_x2=0.0;
    saturation_sum_x3=0.0;
    saturation_sum_x4=0.0;
    saturation_mean=0.0;
    saturation_standard_deviation=0.0;
    saturation_kurtosis=0.0;
    saturation_skewness=0.0;
    area=0.0;
    for (y=0; y < (long) image->rows; y++)
    {
      p=GetVirtualPixels(image,0,y,image->columns,1,exception);
      if (p == (const PixelPacket *) NULL)
        break;
      for (x=0; x < (long) image->columns; x++)
      {
        ConvertRGBToHSB(p->red,p->green,p->blue,&hue,&saturation,&brightness);
        brightness*=QuantumRange;
        brightness_sum_x+=brightness;
        brightness_sum_x2+=brightness*brightness;
        brightness_sum_x3+=brightness*brightness*brightness;
        brightness_sum_x4+=brightness*brightness*brightness*brightness;
        saturation*=QuantumRange;
        saturation_sum_x+=saturation;
        saturation_sum_x2+=saturation*saturation;
        saturation_sum_x3+=saturation*saturation*saturation;
        saturation_sum_x4+=saturation*saturation*saturation*saturation;
        area++;
        p++;
      }
    }
    if (area <= 0.0)
      break;
    brightness_mean=brightness_sum_x/area;
    (void) FormatMagickString(text,MaxTextExtent,"%g",brightness_mean);
    (void) SetImageProperty(image,"filter:brightness:mean",text);
    brightness_standard_deviation=sqrt(brightness_sum_x2/area-(brightness_sum_x/
      area*brightness_sum_x/area));
    (void) FormatMagickString(text,MaxTextExtent,"%g",
      brightness_standard_deviation);
    (void) SetImageProperty(image,"filter:brightness:standard-deviation",text);
    if (brightness_standard_deviation != 0)
      brightness_kurtosis=(brightness_sum_x4/area-4.0*brightness_mean*
        brightness_sum_x3/area+6.0*brightness_mean*brightness_mean*
        brightness_sum_x2/area-3.0*brightness_mean*brightness_mean*
        brightness_mean*brightness_mean)/(brightness_standard_deviation*
        brightness_standard_deviation*brightness_standard_deviation*
        brightness_standard_deviation)-3.0;
    (void) FormatMagickString(text,MaxTextExtent,"%g",brightness_kurtosis);
    (void) SetImageProperty(image,"filter:brightness:kurtosis",text);
    if (brightness_standard_deviation != 0)
      brightness_skewness=(brightness_sum_x3/area-3.0*brightness_mean*
        brightness_sum_x2/area+2.0*brightness_mean*brightness_mean*
        brightness_mean)/(brightness_standard_deviation*
        brightness_standard_deviation*brightness_standard_deviation);
    (void) FormatMagickString(text,MaxTextExtent,"%g",brightness_skewness);
    (void) SetImageProperty(image,"filter:brightness:skewness",text);
    saturation_mean=saturation_sum_x/area;
    (void) FormatMagickString(text,MaxTextExtent,"%g",saturation_mean);
    (void) SetImageProperty(image,"filter:saturation:mean",text);
    saturation_standard_deviation=sqrt(saturation_sum_x2/area-(saturation_sum_x/
      area*saturation_sum_x/area));
    (void) FormatMagickString(text,MaxTextExtent,"%g",
      saturation_standard_deviation);
    (void) SetImageProperty(image,"filter:saturation:standard-deviation",text);
    if (saturation_standard_deviation != 0)
      saturation_kurtosis=(saturation_sum_x4/area-4.0*saturation_mean*
        saturation_sum_x3/area+6.0*saturation_mean*saturation_mean*
        saturation_sum_x2/area-3.0*saturation_mean*saturation_mean*
        saturation_mean*saturation_mean)/(saturation_standard_deviation*
        saturation_standard_deviation*saturation_standard_deviation*
        saturation_standard_deviation)-3.0;
    (void) FormatMagickString(text,MaxTextExtent,"%g",saturation_kurtosis);
    (void) SetImageProperty(image,"filter:saturation:kurtosis",text);
    if (saturation_standard_deviation != 0)
      saturation_skewness=(saturation_sum_x3/area-3.0*saturation_mean*
        saturation_sum_x2/area+2.0*saturation_mean*saturation_mean*
        saturation_mean)/(saturation_standard_deviation*
        saturation_standard_deviation*saturation_standard_deviation);
    (void) FormatMagickString(text,MaxTextExtent,"%g",saturation_skewness);
    (void) SetImageProperty(image,"filter:saturation:skewness",text);
  }
  return(MagickImageFilterSignature);
}
