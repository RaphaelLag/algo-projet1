/** Programme TP Ensimag 2A - AOD. Justification optimale d'un texte.
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
#include <unistd.h>
#include <string.h>

/**
 * free the memory allocated to tab
 *
 */
//static void free_tab(char** tab, int tab_len);


/**
 * Computes the optimal justification of a parapgraph according to the following
 * algorithm: 
 * L (i,k) = L(m_k) + sum(L(m_j) + mu) (j=1,j<=k-1)
 * E(i,k) = M - L(i,k);
 * phi(i) {
 *  if (E(i,n) >= 0) return 0;
 *  min = M
 *  k = 0
 *  while (k<n && E(i,k)>=0) {  
 *      if (aux = phi(k+1) + N(E(i,k)) < min) min = aux;
 *      k++;
 *  }
 *      
 *  return min;
 * 
 * @param i: we compute from the word at the index i of tabwords
 * @param outformat: output stream
 * @param M: line size (in units)
 * @param N: function to optimize is #spaces^N 
 * @param nbwords: number of words in the paragraph considered
 * @param tabwords: array of the words in the paragraph considered
 * @param size_separator: the minimum size of the white space between two consecutive words on a line
 * @param return the cost for an optimal alignment for the parapgraph considered 
 */
static int justify_par(int i, struct stream* outformat, unsigned long M,
                unsigned N, int nbwords, char** tabwords, long size_separator, int* penalties_array);
/**
 * Computes the space size to add in order to make a line composed of the words from the
 * ith word till the nth word have a total size of M.
 * @param i: we compute from the word at the index i of tabwords
 * @param n: we compute till the word at the index n of tabwords
 * @param M: line size (in units)
 * @param tabwords: array of the words in the paragraph considered
 * @param outformat: output stream
 * @param size_separator: the minimum size of the white space between two consecutive words on a line
 * @param return the size (in units) to add
 */
static int E(int i, int n, unsigned long M, char** tabword,
                struct stream* outformat, long size_separator);
/**
 * return the size(in units) of the words and the minimum spaces between words for the
 * ith word till the nth word.
 * @param i: we compute from the word at the index i of tabwords
 * @param n: we compute till the word at the index n of tabwords
 * @param tabwords: array of the words in the paragraph considered
 * @param outformat: output stream
 * @param size_separator: the minimum size of the white space between two consecutive words on a line
 * @param return the size (in units) of the words between the indexes i and n 
 */
static int L(int i, int n, char** tabwords, struct stream* outformat, long
                size_separator);

/** Optimal alignment of a text (read from input file in) with ligns of fixed length (M units) written on output file (out).
 * @param in: input stream, in ascii (eg in == stdin)
 * @param len: maximum number of words in the text (may be larger than the stream)
 * @param outformat: output stream, should be initialed
 * @param M: line size (in units)
 * @param N: function to optimize is #spaces^N 
 * @return: the cost of an optimal alignment
 */
long altex(FILE* in, size_t len, struct stream *outformat, unsigned long M, unsigned N) ;

long altex(FILE* in, size_t len, struct stream *outformat, unsigned long M,
                unsigned N)
{
        char* buffer = (char*) calloc(len+1, sizeof(char) ) ;  // Preallocated buffer, large enough to store any word...
        size_t buffer_size = len ;
        struct parser p_in;
        init_parser(in, &p_in) ;
        long maxval_all_paragraphs = 0 ;
        long sumval_all_paragraphs = 0 ;
        int endofile = 0;
        char** tabwords = (char** ) calloc(len, sizeof(char*));
        int nbwords = 0;
        long par_len = 0;
        long size_separator = sizeSeparator(outformat);
        // We read each paragraph from the first one
        while (!endofile) { 
                int is_paragraph_end = 0 ;
                // All words in the paragraph are stored in tabwords. 
                while ( !is_paragraph_end ) { 
                        // Size of the word if any
                        size_t n = read_word( &p_in, &is_paragraph_end, buffer, buffer_size ) ; 
                        if  (n==0) { 
                            endofile = 1 ;
                            break ; 
                        }
                        // Update of paragraph length with the word's size.
                        par_len += wordlength(outformat, buffer);
                        // We add the word to the array. 
                        tabwords[nbwords] = (char*) calloc(n, sizeof(char));
                        memcpy(tabwords[nbwords], buffer, strlen(buffer) + 1);
                        // Print of the word read.
                        printf("%s ", tabwords[nbwords]);
                        fflush(stdout);
                        nbwords++;
                        // printf("%s ", buffer) ;
                        /*else { // word of length n 
                          printf("%s ", buffer) ;
                          }*/
                }

                //Array used for the memoization
                int penalties_array [nbwords][nbwords];
                int i,j;
                //Init of the memoization array with the default value -1
                for (i = 0; i < nbwords; i++) {
                        for (j = 0; j < nbwords; j++) {
                                penalties_array[i][j] = -1;             
                        }
                }
                // Update of paragraph length with spaces' size
                par_len += size_separator * (nbwords - 1);

                // Case when the paragraph's length is less than M (line size)
                if (par_len < M) { 
                        // Nothing to do, we write the paragraph in the output
                        draw_wordline(outformat, nbwords, tabwords, 1);
                } else {
                        // else we recursively compute the optimal jusitfication 
                        //printf("nbwords : %d\n",nbwords);
                        sumval_all_paragraphs += justify_par(0, outformat, M, N, nbwords,
                                        tabwords, size_separator, (int *)penalties_array);
                }
                //      on réinitialise les variables nécessaires pour le prochain paragraphe
                //      + on libère la mémoire
                par_len = 0;
                nbwords = 0;
                //free_tab(tabwords, len);
                printf("\n\n ") ;
        }
        free(tabwords);
        free(buffer) ;
        fprintf(stderr, "\n#@alignment_cost:MAX=%ld;SUM=%ld\n",
                        maxval_all_paragraphs, sumval_all_paragraphs);

        return maxval_all_paragraphs;
}


//////////////////////////////////////////////////////////////////////////////


static void usage(char * prog) {
        fprintf(stderr, "usage: %s -i input_file -o output_file -f format [-m M]\n", prog);
        fprintf(stderr, "\nOptions:\n");
        fprintf(stderr, "\t-i input_file   Input file (default: stdin)\n");
        fprintf(stderr, "\t-o output_file  Output file (default: stdout)\n");
        fprintf(stderr, "\t-f format       Output format, possible formats: \n");
        fprintf(stderr, "\t                    'text' for a text file (default)\n");
        fprintf(stderr, "\t                    'serif' for a pdf file with a serif font\n");
        fprintf(stderr, "\t                    'hand' for a pdf file with a handwritten font\n");
        fprintf(stderr, "\t-m M            Where M is the width (default: 80)\n");
        fprintf(stderr, "\t                    if the format is 'text', it's the number of characters\n");
        fprintf(stderr, "\t                    else, it's in points and should be < %i points\n", (int)A4_WIDTH);
        exit(1);
}


int main(int argc, char *argv[] ) {
        //Command line parsing
        char c;

        long M = 40 ;
        char* input_file = "test.txt";
        char* output_file = 0;
        char* format = 0;

        while ((c = getopt(argc , argv, "i:o:f:m:")) != -1)
        {
                switch (c) {
                        case 'i':
                                input_file = optarg;
                                break;
                        case 'o':
                                output_file = optarg;
                                break;
                        case 'f':
                                format = optarg;
                                break;
                        case 'm':
                                M = atoi(optarg);
                                break;
                        case '?':
                                usage(argv[0]);
                                break;
                }
        }


        FILE* f = stdin;
        if(input_file)
        {
                f = fopen(input_file, "r") ; 
                if( f == NULL ) {
                        fprintf( stderr, "Error opening input file.");
                        return(-1);
                }
        }

        struct stat st;
        fstat(fileno(f), &st);
        size_t fsize = st.st_size;


        if (!setlocale(LC_CTYPE, "")) {
                fprintf( stderr, "Can't set the specified locale! "
                                "Check LANG, LC_CTYPE, LC_ALL.\n");
                return -1;
        }


        struct stream* outstream;

        if(format == 0)
                outstream = init_stream(output_file, 0, M);
        else {
                switch(format[0])
                {
                        case 't': //"text"
                                outstream = init_stream(output_file, 0, M);
                                break;
                        case 's': //"serif"
                                outstream = init_stream(output_file, "DejaVuSerif.ttf", M);
                                break;
                        case 'h': //"hand"
                                outstream = init_stream(output_file, "daniel.ttf", M);
                                break;
                        default:
                                fprintf( stderr, "Unrecognized format.\n");
                                return 1;
                }
        }

        if(!outstream)
                return 1;

        altex(f, fsize, outstream, M,  2) ; 

        //free and flush outstream
        free_format(outstream);
        return 0 ;
}

/* ************************************************************************** */
/* *************************** Utility Functions **************************** */
/* ************************************************************************** */
/*
   static void free_tab(char** tab, int tab_len) {
   int i;
   for (i = 0; i < tab_len; i++) {
   free(tab[i]);
   }
   }
   */

/* ϕ ALGO : 
 * L (i,k) = L(m_k) + sum(L(m_j) + mu) (j=1,j<=k-1)
 * E(i,k) = M - L(i,k);
 * phi(i) {
 *  if (E(i,n) >= 0) return 0;
 *  min = max_entier
 *  while (k<n && E(i,k)>=0) {  
 *      if (aux = phi(k+1) + N(E(i,k)) < min) min = aux;     
 *  }
 *      
 *  return min;
 */

 /*
    ϕ = {
        0 if E(i,n) >= 0
        min[k>=i; E(i,k)>=0] ( ϕ(k+1) + N(E(i,k))) else
    }

    @return Min (optimal) penalty of a paragraph
 */
static int justify_par(int i, struct stream* outformat, unsigned long M,
                unsigned N, int nbwords, char** tabwords, long size_separator, int* penalties_array)
{
        //TODO cas où un mot est plus long que la largeur de ligne demandée
        int min = M;
        int k = i;
        int aux, nbspaces;

        // The paragraphe (= a line in this case) is not long enough to fulfill a line
        // No opitmisation to do.
        if (E(i, nbwords - 1, M, tabwords, outformat, size_separator) >= 0)
                return 0;
        
        //Computes the minimum
        while (k < nbwords - 1 && 
            (nbspaces = E(i, k, M, tabwords, outformat, size_separator)) >= 0){ 
                //printf("justify : %d\n", k);
                // if the penalty value as ever been wordked we return it :
                if (penalties_array[nbwords*i+k] != -1)
                        aux = penalties_array[nbwords*i+k];
                // Else, we compute the penalty for the paragraph beginning at k+1:
                else { 
                        aux = justify_par(k+1, outformat, M, N, nbwords, tabwords,
                                        size_separator, penalties_array) + penality(nbspaces, N);
                        penalties_array[nbwords*i+k] = aux;
                }
                if (aux < min) {
                        min = aux;
                }
                k++;
        }
        return min;
}

/*
    ej = E(i;k) = M - Δ(i;k)

    @return size of space to add to the line in order to justify the paragraph
*/
static int E(int i, int n, unsigned long M, char** tabwords,
                struct stream* outformat, long size_separator)
{
        return (M - L(i, n, tabwords, outformat, size_separator));
}

/*
    Delta(i;k) = L(mk) + Sum[j from i to k-1]( L(mj) + μ )

    @param i: index of the first word of the line (in tabwords)
    @param n: index of the last word of the line (in tabwords)
    @param tabwords: array containing the whole text.
    @param outformat: output stream
    @param size_separator: the minimum size of the white space between two consecutive words on a line
    @return min size of a line (size of the n words + n-1 spaces)
*/
static int L(int i, int n, char** tabwords, struct stream* outformat, long
                size_separator)
{
        int words_n_spaces_length = wordlength(outformat, tabwords[i]);
        int k;
        for (k = i+1; k <= n; k++) {
                words_n_spaces_length += wordlength(outformat, tabwords[k]) +
                        size_separator;
        }
        return words_n_spaces_length;
}
