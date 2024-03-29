#  Copyright 1999-2009 ImageMagick Studio LLC, a non-profit organization
#  dedicated to making software imaging solutions freely available.
#
#  You may not use this file except in compliance with the License.  You may
#  obtain a copy of the License at
#
#    http://www.imagemagick.org/script/license.php
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  Exercise all regression tests:
#  
#    make test
#
#  Exersise one regression test:
#
#    make TEST_VERBOSE=1 TEST_FILES=t/filter.t test
#

use ExtUtils::MakeMaker;
use Config;

# Compute test specification
my $delegate_tests='t/*.t';
my $delegate;
foreach $delegate (qw/bzlib djvu fftw fontconfig freetype jpeg jng jp2 lcms mpeg png tiff x11 xml wmf zlib/) {
  if ( -d "t/$delegate" ) {
    if ($delegate =~ /x11/) {
      if ( defined $ENV{'DISPLAY'} ) {
        $delegate_tests .= " t/$delegate/*.t";
      }
      next;
    }
    $delegate_tests .= " t/$delegate/*.t";
  }
}

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile
  (
   # Module description
   'ABSTRACT'	=> 'ImageMagick PERL Extension',

   # Perl module name is Image::Magick
   'NAME'	=> 'Image::Magick',

   # Module author
   'AUTHOR' => 'ImageMagick Studio LLC',

   # Module version
   'VERSION' => '6.5.4',

   # Preprocessor defines
   'DEFINE'	=> ' -D_LARGE_FILES=1 -DHAVE_CONFIG_H',     # e.g., '-DHAVE_SOMETHING' 

   # Header search specfication and preprocessor flags
   'INC'	=> '-I../ -I.. -I/usr/include/freetype2 -I/usr/include/libxml2',

   # C compiler
   #'CC' => 'gcc -std=gnu99',

   # C pre-processor flags (e.g. -I & -D options)
   # 'CPPFLAGS' => "$Config{'cppflags'} -I/usr/include/freetype2 -I/usr/include/libxml2",

   # C compiler flags (e.g. -O -g)
   'CCFLAGS' => "$Config{'ccflags'} -fopenmp -O3 -g -Wall -W -pthread",

   # Linker
   #'LD' => $Config{'ld'} == $Config{'cc'} ? 'gcc -std=gnu99' : $Config{'ld'},

   # Linker flags for building an executable
   'LDFLAGS' =>  "-L../magick/.libs -lMagickCore -L../wand/.libs -lMagickWand $Config{'ldflags'} -Wl,--as-needed -lfreetype",

   # Linker flags for building a dynamically loadable module
   'LDDLFLAGS' => "-L../magick/.libs -lMagickCore -L../wand/.libs -lMagickWand $Config{'lddlflags'} -Wl,--as-needed -lfreetype",

   # Install PerlMagick binary into ImageMagick bin directory
   'INSTALLBIN'	=> '/usr/bin',

   # Library specification
   'LIBS' => [ '-L../magick/.libs -lMagickCore -L../wand/.libs -lMagickWand -lperl -lm' ],

   # Perl binary name (if a Perl binary is built)
   'MAP_TARGET'	=> 'PerlMagick',

   # Let CFLAGS drive optimization flags by setting OPTIMIZE to empty
   # 'OPTIMIZE'	=> '',

   # Use same compiler as ImageMagick
   'PERLMAINCC'	=> ' -fopenmp',

   # Set Perl installation prefix to ImageMagick installation prefix
#   'PREFIX'	=> '/usr',

   # Include delegate directories in tests
   test	=> { TESTS	=>	$delegate_tests},

   ($Config{'archname'} =~ /-object$/i ? ('CAPI' => 'TRUE') : ()),
);


#
# Substitutions for "makeaperl" section.
#
sub MY::makeaperl {
     package MY; # so that "SUPER" works right
     my $inherited = shift->SUPER::makeaperl(@_);

     # Stinky ExtUtils::MM_Unix likes to append its own library path to $(CC),
     # prior to any user-specified library path so that an installed library is
     # used rather than the library just built.  This substitution function
     # tries to insert our library path first. Also, use the same compiler used
     # to build perlmain.c to link so that a C++ compiler may be used if
     # necessary.
     $inherited =~ s:MAP_LINKCMD\s.*\s*\$\(CC\):MAP_LINKCMD   = \$(PERLMAINCC) -L/usr/lib: ;
     $inherited;
 }
