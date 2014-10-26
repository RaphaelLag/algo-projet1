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


struct decoupage {
        int cout;
        int coupe;
}; 

/**
 * free the memory allocated to tab
 *
 */
static void free_tab(char** tab);

// TODO: replace all parameters by a struct "paragraph_data"
// TODO : replace recursive algorithm by a an iterative one.
// TODO: memoize values of penality(nbspaces, N) in justify_par

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
 * @param n: we compute till the word at the index n of tabwords
 * @param outformat: output stream
 * @param M: line size (in units)
 * @param N: function to optimize is #spaces^N 
 * @param tabwords: array of the words in the paragraph considered
 * @param size_separator: the minimum size of the white space between two consecutive words on a line
 * @param optimal_choice: stocks the choice made to obtain the optimal solution
 * @param return the cost for an optimal alignment for the parapgraph considered 
 */

static int justify_par(int i, int n, int previous_i, struct stream* outformat, unsigned long M,
                unsigned N, char** tabwords, long size_separator,
                int* space_memoization, struct decoupage* phi_memoization);
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
static int Delta(int i, int n, char** tabwords, struct stream* outformat, long
                size_separator);

/** Optimal alignment of a text (read from input file in) with ligns of fixed length (M units) written on output file (out).
 * @param in: input stream, in ascii (eg in == stdin)
 * @param len: maximum number of words in the text (may be larger than the stream)
 * @param outformat: output stream, should be initialed
 * @param M: line size (in units)
 * @param N: function to optimize is #spaces^N 
 * @return: the cost of an optimal alignment
 */

/** draw_wordparagraph displays optimal lines of words
 * @param outformat : the output stream to consider
 * @param tabwords: array of words
 * @param phi_memoization: structure used to get the index for where to make a carriage return
 */
static void draw_wordparagraph(struct stream* outformat, char** tabwords, struct decoupage* phi_memoization, int nbwords);

struct solution{
        int pos;
        struct solution* next;
};


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
                        } else {
                                // Update of paragraph length with the word's size.
                                par_len += wordlength(outformat, buffer);
                                // We add the word to the array. 
                                tabwords[nbwords] = (char*) calloc(n+1, sizeof(char));
                                memcpy(tabwords[nbwords], buffer, strlen(buffer) + 1);

                                nbwords++;
                        }
                }
                if (nbwords > 0) {

                        // memoization of penalties for the current paragraph
                        struct decoupage phi_memoization [nbwords];
                        int space_memoization [nbwords][nbwords];

                        int i,j;
                        // Init of the memoization array with the default value -1
                        for (i = 0; i < nbwords; i++) {
                                phi_memoization[i].cout = -1;
                                phi_memoization[i].coupe = -1;
                                for (j = 0; j < nbwords; j++) {
                                        space_memoization[i][j] = -1;
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
                                sumval_all_paragraphs += justify_par(0, nbwords - 1, 0, outformat, M, N, 
                                                tabwords, size_separator, (int *)space_memoization, (struct decoupage *)phi_memoization);

                                draw_wordparagraph(outformat, tabwords,
                                                (struct decoupage*)phi_memoization, nbwords);
                        }
                        //      on réinitialise les variables nécessaires pour le prochain paragraphe
                        //      + on libère la mémoire
                        par_len = 0;
                        nbwords = 0;
                }
                free_tab(tabwords);

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

        long M = 40 ; // 40
        char* input_file = "texte-test-court.txt"; //"test_text.txt";
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
        fclose(f);
                                                
        return 0 ;
}

/* ************************************************************************** */
/* *************************** Utility Functions **************************** */
/* ************************************************************************** */

static void free_tab(char** tab) {
        int i;
        for (i = 0; tab[i] != NULL; i++) {
                free(tab[i]);
                tab[i] = NULL;
        }
}


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

   @param i: we compute from the word at the index i of tabwords
   @param n: we compute till the word at the index n of tabwords
   @param outformat: output stream
   @param M: line size (in units)
   @param N: function to optimize is #spaces^N 
   @param tabwords: array of the words in the paragraph considered
   @param size_separator: the minimum size of the white space between two consecutive words on a line
   @param penalties_array: stocks penalties of the different solutions
   @param optimal_choice: stocks the choice made to obtain the optimal solution
   @return Min (optimal) penalty of a paragraph
   */
static int justify_par(int i, int n, int previous_i, struct stream* outformat,
                unsigned long M,unsigned N, char** tabwords,
                long size_separator, int* space_memoization,
                struct decoupage* phi_memoization)
{
        int min = INT_MAX;
        int k = i;
        int aux, nbspaces;

        // The paragraphe (= a line in this case) is not long enough to fulfill a line
        // No opitmisation to do.
        if (E(i, n, M, tabwords, outformat, size_separator) >= 0){
                return 0;
        }

        // TODO : ne faut -il pas le mettre dans la boucle ?
        //If a word is larger than M we truncate it 
        if ((nbspaces = E(i, k, M, tabwords, outformat, size_separator)) < 0) {
                //TODO : appel à la fonction de troncature d'un mot
                fprintf(stderr, "Attention mot plus grand que M\n");
        }

        // Computes the min
        do { 
                // Computes the value of phi(k+1) if necessary
                if (phi_memoization[k+1].cout == -1)
                        phi_memoization[k+1].cout = justify_par(k+1, n, i, outformat, M, N,tabwords,
                                size_separator, space_memoization, phi_memoization);
                                        
                aux = phi_memoization[k+1].cout + penality(nbspaces, N);
                
                // Update of the current min penality
                if (aux < min) {
                        min = aux;
                        phi_memoization[i].coupe = k+1;
                }
                k++;
        } while (k < n &&
                        (nbspaces = E(i, k, M, tabwords, outformat, size_separator)) >= 0);

        return min;
}

/*
   ej = E(i;k) = M - Δ(i;k)

   @param i: index of the first word of the line (in tabwords)
   @param n: index of the last word of the line (in tabwords)
   @param M: line size (in units)
   @param tabwords: array containing the whole text.
   @param outformat: output stream
   @param size_separator: the minimum size of the white space between two consecutive words on a line
   @return size of space to add to the line in order to justify the paragraph
   */
static int E(int i, int k, unsigned long M, char** tabwords,
                struct stream* outformat, long size_separator)
{
        return (M - Delta(i, k, tabwords, outformat, size_separator));
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
static int Delta(int i, int k, char** tabwords, struct stream* outformat, long
                size_separator)
{
        //Check erreur bizarre ligne suivante avec indice k
        int words_n_spaces_length = wordlength(outformat, tabwords[i]);
        int j;
        for (j = i + 1; j <= k; j++) {
                words_n_spaces_length += wordlength(outformat, tabwords[j]) +
                        size_separator;
        }
        return words_n_spaces_length;
}

static void draw_wordparagraph(struct stream* outformat, char** tabwords, struct decoupage* phi_memoization, int nbwords)
{
        int i = 0;
        while ((phi_memoization[i].coupe) != -1)
        {
                draw_wordline(outformat, phi_memoization[i].coupe - i, tabwords+i, 0);
                i = phi_memoization[i].coupe;
        }
        draw_wordline(outformat, nbwords - i, tabwords+i, 1);
}
