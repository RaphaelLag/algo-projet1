/** Programme TP Ensimag 2A - AOD. Justification optimale d'un texte.
 * Utilitaires pour lire et crire de mots (avec prise en compte de césure conditionnelle.
 * @author Jean-Louis.Roch@imag.fr, David.Glesser@imag.fr
 * @date 15/9/2014
 */

#ifndef _ALTEX_UTILITAIRE_H_
#define _ALTEX_UTILITAIRE_H_

#include <stdio.h>
#include <stdlib.h>


/** Penality of line alignment 
  * @param nbspaces: legth of spaces on the line (in printable units)
  * @param expo: exponent for the penality function
  * @param return nbspaces^expo 
  */
long penality(long  nbspaces, unsigned expo) ; 


/*****************************************************************************/
/** The parser struct is used to read the file word by word (read_word).
  * It is initialized by the parser_init function.
  * 
  */
struct parser { // State to read words
  FILE* instream ;
  char prev_char ; 
} ; 

/** init_parser initializes the content of a struct parser to enable calling read_words. 
 * @param in :  stream that is read from current position (read access required)
 * @param p : pointer to a preallocated struct parser. 
 */
void init_parser(FILE *in, struct parser* p) ;

/**  The read_word function reads from file in : either a word of at most n wide characters, either delimited by space character 
 *   or the end of paragraph (exactly  2 consecutive formfeed '\f' carriage-return '\r' or newline '\n');  or the end of file in.  
 * The caller must provide a pointer to a pre-allocated buffer (buffer*) to store the word read, and the capacity of that buffer is at least (n+1) wchar_t. 
 * The function returns: either  the number of characters stored in buffer always less than n+1 with the '\0' character; 
 * or 0 if EOF encoutered; or -1 if an end of paragraph is encoutered. 
 * @param p_in : parser on the input file; has to be previously initialized throug init_parser; 
 * @param is_paragraph_end : at return, *is_paragraph_end is the word read is followed by an end of paragraph. 
 * @param buffer :  preallocated buffer where, at return,  the read word from p_in is stored as a wide string (terminated by L'\0' )
 * @param n : maximum number of wchar_t to read; preallocated buffer must be preallocated with at least a size (n+1)*sizeof(wchar_t)
 * @return the number of wchar_t in the read word, that have been stored in buffer; or 0 if (n<=0) or if EOF encoutered before reading a non iswspace character.
 */
size_t read_word( struct parser* p_in,  int* is_paragraph_end, char* buffer, size_t n ) ;


/*****************************************************************************/
//Usefull to manipulate A4 paper format:
#define A4_WIDTH 595.0
#define A4_HEIGHT 842.0

/** The stream struct is used to write the output to a stream (stdout) or a file (as a pdf or txt)
  * It is initialized by the init_format function.
  * It is free'd by the free_format function.
  * 
  */
struct stream;

/** free_format free and flush a struct stream
 * It *HAS* to be used a the end of your program, otherwise some output data can be missed.
 * @param s :  the struct stream to free
 */
void free_format(struct stream* s);

/** init_format create a struct stream
 * @param output_file :  the output file. If == 0, the data will be printed to stdin.
 * @param font_path :  the font to use when printing to a PDF file.
 * @param M :  line size (in units)
 */
struct stream* init_stream(char* output_file, char* font_path, long M);

/** sizeSeparator returns the minimum display size of the separator (white space) between two consecutive word on a line. 
 * @param f : the output stream to consider
 * @return : the minimum size of the white space between two consecutive words on a line
 */
long sizeSeparator(struct stream* f) ; 


/** Computes the length of a word in number of printable units. 
 * @param f : the output stream to consider
 * @param w : input word 
 * @return the length is display units. 
 */
long wordlength(struct stream *f, char* w) ; 


/** word_truncate_at_length truncate a word to make it fit in a given display length
 * @param f : the output stream to consider
 * @param w : input word, that will be truncated in place if needed.
 * @param length : length of the word has to be shorter than 'length' when it is displayed  at end of line. 
 * @return 0 if the input word fits, else 1 (in this case, w is truncated in place into z="w[0]..w[k]'?'" where
 *   k is the largest integer such that the length of z when displayed at end of the line is less than length).
 * Example: Let w="12345" with constant unit size characters: word_truncate_at_length(w, 3) returns 1 and w="12?".
 */
int word_truncate_at_length(struct stream *f, char* w, long length) ;
 

/** draw_wordline displays a line of words
 * @param f : the output stream to consider
 * @param nbwords: number of words in 'tabwords'
 * @param tabwords: array of words
 * @param end_of_pragraph: if true the line is not justified and a the paragraph is ended
 */
void draw_wordline(struct stream *f, int nbwords, char** tabwords, int end_of_pragraph);


#endif // _ALTEX_UTILITAIRE_H_

