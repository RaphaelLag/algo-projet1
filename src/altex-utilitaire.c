/** Programme TP Ensimag 2A - AOD. Justification optimale d'un texte.  * altex-utilitaire.c : implementation of altex-utilitaire.h
 * @author Jean-Louis.Roch@imag.fr, David.Glesser@imag.fr
 * @date 15/9/2014
 */

#include "altex-utilitaire.h"
#include <stdio.h>
#include <sys/stat.h>
#include <math.h>
#include <stdlib.h>
#include <locale.h>
#include <limits.h>
#include <ctype.h>
#include <alloca.h>

// The Flag _DO_NOT_USE_PDF_ may be defined to compile without cairo library 
#ifndef _DO_NOT_USE_PDF_ 
#include <cairo.h>
#include <cairo-ft.h>
#include <cairo-pdf.h>
#endif // _DO_NOT_USE_PDF_


/*****************************************************************************/
long penality(long nbspaces, unsigned expo) {
  if (nbspaces ==0) return 0 ;
  return  (long) (pow( (double)nbspaces, (double)expo )) ; 
}


/*****************************************************************************/
struct parser ; 

void init_parser(FILE *in, struct parser* p) {
   char c ; // First non space characters in the file
   p->instream = in ;
   while ( ((c=fgetc(in))!=EOF) && isspace(c) ) { }
   p->prev_char = c ;
}

size_t read_word( struct parser* p_in,  int* is_paragraph_end, char* buffer, size_t n ) {
   char c = p_in-> prev_char ;
   if ((c == EOF)  || (n==0)) { return 0 ; }

   *is_paragraph_end = 0 ;
   size_t nread = 0 ;  
   while ( (c!=EOF) && (!isspace(c)) && (nread < n) ) {
       buffer[ nread++ ] = c ;
       c= fgetc(p_in->instream) ; 
   }   
   int nb_line_return = 0 ; 
   while ((c!=EOF) && isspace(c)) {
     if (! isblank(c) ) nb_line_return++ ;
     // if ((c == L'\n') || (c==L'\r')) nb_line_return++ ;
     c= fgetc(p_in->instream) ; 
   }
   if (nb_line_return > 1) *is_paragraph_end = 1 ; 
   p_in-> prev_char = c ;

   if (nread <n) buffer[nread] = '\0' ; 
   else buffer[n-1] = '\0' ; 
   return nread ; 
}



/*****************************************************************************/
// Ouput


struct stream {
   /// common values
   int is_pdf;//is it a pdf stream?
   long sizeSeparator;//the value is cached
   long M;
   int line_number;
   
   /// PDF
#ifndef _DO_NOT_USE_PDF_
   cairo_t *cr;// cairo drawer
   cairo_surface_t* tgt;// cairo surface
   double margin;
#endif // _DO_NOT_USE_PDF_
   
   /// TEXT
   FILE* out;
   char** sized_space_separators;
};

struct stream* malloc_format()
{
   struct stream* f = malloc(sizeof(struct stream));
#ifndef _DO_NOT_USE_PDF_
   f->cr = 0;
   f->tgt = 0;
#endif // _DO_NOT_USE_PDF_
   f->out = 0;
   f->sized_space_separators= (char**) 0; // Preallocated char* with spaces of size from 0 to M
   return f;
}


void free_format(struct stream* o)
{
#ifndef _DO_NOT_USE_PDF_
   if(o->tgt)
      cairo_surface_finish (o->tgt);
   if(o->tgt)
      cairo_surface_destroy(o->tgt);
   if(o->cr)
      cairo_destroy(o->cr);
#endif // _DO_NOT_USE_PDF_
   if(o->out)
      fclose(o->out);
   if (o->sized_space_separators)
      free (o->sized_space_separators) ;
   free(o);
}


//////////////////////////////////////////////////////////////////////////////
/// PDF Ouput
#ifndef _DO_NOT_USE_PDF_

#define INTERLINE 1.2
#define MARGIN_TOP 100.0
#define TEXT_SIZE 8
// #define DRAW_GUIDES

void init_page(struct stream* f)
{
   cairo_set_line_width (f->cr, 0.0);
   #ifdef DRAW_GUIDES
   cairo_set_source_rgba (f->cr, 0.9, 0.9, 0.9, 1.0);
   cairo_rectangle (f->cr, f->margin, MARGIN_TOP, A4_WIDTH-f->margin*2, A4_HEIGHT-MARGIN_TOP*2);
   cairo_fill (f->cr);
   #endif
   cairo_set_source_rgba (f->cr, 0, 0, 0, 1);
}

void create_new_page(struct stream* f)
{
   cairo_show_page (f->cr);
   init_page(f);
}

struct stream* pdf_init_stream(char* output_file, char* font_path, long M)
{
   FT_Library ft_library;
   FT_Face ft_face;
   
   struct stream* o = malloc_format();
   
   o->is_pdf = 1;
   
   o->line_number = 0;
   

   if (FT_Init_FreeType (&ft_library)) {
      fprintf( stderr, "FT_Init_FreeType failed\n");
      return 0;
   }

   if (FT_New_Face (ft_library, font_path, 0, &ft_face)) {
      fprintf( stderr, "FT_New_Face failed\n");
      return 0;
   }
   
   if(!output_file)
   {
      fprintf( stderr, "stdout is not support when printing to pdf.\n");
      return 0;
   }
   
//    o->tgt = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
   o->tgt = cairo_pdf_surface_create(output_file,  A4_WIDTH, A4_HEIGHT);
   o->cr = cairo_create(o->tgt);
   cairo_font_face_t* cr_face =cairo_ft_font_face_create_for_ft_face (ft_face, 0);
   cairo_set_font_size (o->cr, TEXT_SIZE);
   cairo_set_font_face (o->cr, cr_face);
   
   o->margin = (A4_WIDTH - (double)M)/2.0;
   if( o->margin < 0 || o->margin > A4_WIDTH) {
      fprintf( stderr, "M should be between >0 and < %i when printing to PDF\n", (int)A4_WIDTH);
      return 0;
   }
   
   o->sizeSeparator = wordlength(o, "_");
   
   o->M = M;
   
   init_page(o);
   
   return o;
}


double pdf_wordlength(struct stream* f, char* w) {
   cairo_text_extents_t e;
   cairo_text_extents(f->cr, w, &e);
   // e.x_advance correspond to where to be placed to draw the next char.
   // e.width is the witdh actually drawn.
   return (e.width);
}


void pdf_draw_wordline(struct stream *f, int nbwords, char** tabwords, int end_of_pragraph)
{
   
   
   int i;
   double left_margin = 0;
   double width_tot = f->M;
   
   double size_of_spaces;
   if(end_of_pragraph) //No justify
   {
      size_of_spaces = sizeSeparator(f);
   } else //Justify
   {
      
      double sum_size_written_text = 0;
      for (i=0; i < nbwords; i++)
         sum_size_written_text += pdf_wordlength(f, tabwords[i]);

      if(nbwords <= 1)
      {
         fprintf(stderr, "*** Warning: only one word on line makes alignment impossible, no white space added at end.\n" ) ;fflush(stderr);
         size_of_spaces = 0;
      } else
         size_of_spaces = (width_tot-sum_size_written_text)/(nbwords-1);
   }
   
   for(i=0;i<nbwords;i++)
   {
      cairo_move_to (f->cr, f->margin+left_margin,MARGIN_TOP+((double)TEXT_SIZE)*(1.0+(double)f->line_number)*INTERLINE);
      
      cairo_show_text (f->cr, tabwords[i]);
      
      left_margin += pdf_wordlength( f, tabwords[i] ) + size_of_spaces;
   }
   
   if(end_of_pragraph)
      f->line_number++;
   
   if( MARGIN_TOP+((double)TEXT_SIZE)*(1.0+(double)f->line_number+1.0)*INTERLINE > A4_HEIGHT-MARGIN_TOP)
   {
      create_new_page(f);
      f->line_number = -1;//because we still on the previous line
   }
   
   f->line_number++;
   
      
}

#endif // _DO_NOT_USE_PDF_


//////////////////////////////////////////////////////////////////////////////
/// TEXT Ouput


struct stream* text_init_stream(char* output_file, long M)
{
   struct stream* o = malloc_format();
   o->is_pdf = 0;
   o->sizeSeparator = 1;
   if(output_file)
   {
      o->out = fopen(output_file, "w") ; 
      if( o->out == NULL ) {
         perror ("Error opening input file.");
         return 0;
      }
   } else
      o->out = stdout;
   o->M = M;
   o->line_number=1 ;
   // Preallocation of ia table including char* spaces of size from 0 to M
   // compact Implementation in one table:  first store the pointers than the data.
   o->sized_space_separators= (char**) malloc( (M+1) * sizeof(char*) + ((M+1)*(M+2)) / 2 * sizeof(char)); 
   int i ;
   char *spacecour = (char*)  & (o->sized_space_separators[M+1]) ;
   for (i=0; i<= M ; ++i) {
      // sized_space_separators[i] points to a space of size i 
      o->sized_space_separators[i] = spacecour ;
      int k; 
      for (k=0; k <i; ++k ) *(spacecour++) = ' ' ;
      *(spacecour++) = '\0' ;
   }
   return o;
}

long text_wordlength(char* w) {
   int i = 0, n = 0;
   while (w[i]) {
     //detect special utf-8 characters
     if ((w[i] & 0xc0) != 0x80) n++;
     i++;
   }
   return n;
}


void text_draw_wordline(struct stream *f, size_t nbwords, char** tabwords, int end_of_pragraph)
{
   if(!end_of_pragraph)
   {
      if( nbwords <= 0 )
      {
         fprintf(stderr, "*** Warning: draw_wordline is called on an empty line!");fflush(stderr);
         return;
      } else if(nbwords == 1 ) {
         if (wordlength(f, tabwords[0]) != f->M) {
             fprintf(stderr, "*** Warning: line %d is %s (only one word on line makes alignment impossible).\n", 
                            f->line_number, (wordlength(f, tabwords[0]) < f->M ) ? "underfull" : "overfull") ;
             fflush(stderr);
         }
         fprintf(f->out, "%s\n", tabwords[0]) ; fflush(f->out);
         f->line_number++;
         return;
      } else
      {
         int sum_size_written_text = 0;
         { size_t i; 
           for (i=0; i < nbwords; i++) sum_size_written_text += wordlength(f, tabwords[i]);
         }
         int nbspaces = nbwords - 1 ; // The number of spaces word
         // There are at most two possible size for space separators : min_blank_size and  min_blank_size+1
         int min_blank_size = 1 ;
         // ismaxspace[i] == either 0 or 1 iff the space between words i and i+1 has 
         //                  size min_blank_size + either 0 or 1; 
         int* ismaxspace = (int*) alloca( nbspaces * sizeof(int) );
         { // ismaxspace is INIT to 0 
           int* spacecour=ismaxspace; int i; for (i=nbspaces; i>0; --i) *(spacecour++)=0;  
         } 
         int total_space_size = f->M - sum_size_written_text ;
         if (total_space_size < nbspaces) { 
             fprintf(stderr, "*** Warning: line %d is overfull (not enough spaces lept on the linee.\n",
                            f->line_number) ; fflush(stderr);
         }
         else { 
            min_blank_size = total_space_size / nbspaces ;
            int nb_max_blank_size = total_space_size % nbspaces ; 
            if (nb_max_blank_size != 0) { 
               // The nb_max_blank_size spaces are distributed cyclically in ismaxspace
               // from a randomly chosen starting position 
               // To have a balanced distribution, we distributed the spaces that are the fewest.
               int nb_min_blank_size = nbspaces - nb_max_blank_size ;
               int k = (nb_min_blank_size < nb_max_blank_size) ? nb_min_blank_size : nb_max_blank_size ;
               // Cyclically distribute k "TRUE" among nbspaces in  ismaxspace, that has been init to "FALSE"
               int first = ((unsigned long) tabwords[0]) % nbspaces ;  // To cycle from a pseudorandom 
               int stride= nbspaces / k ;
               int i ;
               for (i=k ; i>0 ; --i) {
                  ismaxspace[ first ] = 1 ; 
                  first += stride; if (first - (int)nbspaces >=0 ) first = first - (int)nbspaces ; 
               }
               if (nb_min_blank_size < nb_max_blank_size) 
                  for (i=0; i<nbspaces; ++i) ismaxspace[ i ]= (ismaxspace[ i ]==0) ? 1 : 0  ;
            }
         }
         // Ecriture des mots sur la sortie 
         { 
           int i = nbspaces; 
           char** motcour = tabwords ;
           char** min_blank = f->sized_space_separators + min_blank_size ;
           int*  spacecour = ismaxspace ;
           while (i--) fprintf(f->out, "%s%s", *(motcour++), *(min_blank + *(spacecour++))) ;
           fprintf(f->out, "%s\n", *motcour ) ; fflush(f->out);
           f->line_number++;
         }
      }
   } else { // Last line is written as if.
      int i = nbwords-1; 
      char** motcour = tabwords ;
      while (i--) fprintf(f->out, "%s ", *(motcour++) ) ;
      fprintf(f->out, "%s\n\n", *motcour ) ; fflush(f->out);
      f->line_number+=2;
   }
}



//////////////////////////////////////////////////////////////////////////////
/// Common Ouput

struct stream* init_stream(char* output_file, char* font_path, long M)
{
#ifndef _DO_NOT_USE_PDF_
   if(font_path != 0)
      return pdf_init_stream(output_file, font_path, M);
   else
#endif // _DO_NOT_USE_PDF_
      return text_init_stream(output_file, M);
}

long sizeSeparator(struct stream* f)
{
   return f->sizeSeparator;
}

long wordlength(struct stream *f, char* w)
{
#ifndef _DO_NOT_USE_PDF_
   if( f->is_pdf )
      return pdf_wordlength(f, w);
   else
#endif // _DO_NOT_USE_PDF_
      return text_wordlength(w);
}

int word_truncate_at_length(struct stream *f, char* w, long length)
{
   int  n = 0 ; // number of char in w
   while ( w[n] != '\0' ) { n += 1 ; }
   long s = wordlength( f,  w ) ;
   // int is_truncated =  ((l_last > length) || (l_inside > length)) ; 
   int is_truncated =  (s > length) ; 
   if ((is_truncated) && (n>1)) { // truncation
     fprintf( stderr, "*** Warning: The word '%s' is truncated to be displayed in %ld units ", w, length) ;
     do {  // truncation
        n = n-1 ;
        w[n] = '\0' ;
        w[n-1] = '?' ;
        s = wordlength( f, w ) ;
     } while  ( (n>1) && (s > length) ) ;
     fprintf( stderr, "as: '%s'\n",  w ) ;
   }
   return is_truncated ; 
}

void draw_wordline(struct stream *f, int nbwords, char** tabwords, int end_of_pragraph)
{
#ifndef _DO_NOT_USE_PDF_
   if( f->is_pdf )
      return pdf_draw_wordline(f, nbwords, tabwords,end_of_pragraph );
   else
#endif // _DO_NOT_USE_PDF_
      return text_draw_wordline(f, nbwords, tabwords,end_of_pragraph );
}

