/** Programme TP Ensimag 2A - AOD. Justification optimale d'un texte.
 * @author Jean-Louis.Roch@imag.fr, David.Glesser@imag.fr
 * @date 15/9/2014
 */

#include "altex-utilitaire.h"
#include <stdio.h>
#include <sys/stat.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>


/* ************************************************************************** */
/* *************** Data Structures & Function Signatures ******************** */
/* ************************************************************************** */

struct decoupage {
        int cout;
        int coupe;
}; 

struct solution{
        int pos;
        struct solution* next;
};

struct file_data{
        unsigned long M;          // line size (in units)
        unsigned N;               // coeff such that penalty = #spaces^N 
        long size_separator;      /* min size of the white space between 
                                   * two consecutive words on a line 
                                   */
        struct stream* outformat; // output stream
};

struct paragraph_data{
        char** tab_words;         // array containing the paragraph's words.
        size_t* words_L;          /* Array containing the size of each words
                                   * in the paragraphe.
                                   */
};

/**
 * Reads the file and draw it justified.
 * @param in input file to justify
 * @param len max size of the text
 * @param outformat output stream (txt or pdf)
 * @param M (line size in units)
 * @param N coeff such that penalty = #spaces^N 
 * @return penalty value of the biggest paragraph.
 */
long altex(FILE* in, size_t len, struct stream *outformat, unsigned long M, 
        unsigned N) ;

/**
 * Computes the optimal justification of a parapgraph according to the following
 * - Algorithm in its recursive form : 
 *  ϕ = {
 *        0 if E(i,n) >= 0
 *        min[k>=i; E(i,k)>=0] ( ϕ(k+1) + N(E(i,k))) else
 *      }
 * 
 * @param i is the index of the first word of the paragraph to justify
 * @param n is the index of the last word of the paragraph to justify
 * @param f contains meta-data about the file (see file_data for more details)
 * @param p contains meta-data about the paragraph (see paragraph_data)
 * @param phi_mem  memorises all the values of phi
 * @param space_mem memorises the values of the spaces added
 * @param return the cost for an optimal alignment for the parapgraph considered 
 * @return Cost of the justification 
 */
static int justify_par(int i, int n, struct file_data* f, 
        struct paragraph_data* p, int* space_mem, struct decoupage* phi_mem);

/** ej = E(i;k) = M - Δ(i;k)
 * 
 * Computes the space size to add in order to make a line composed of the words 
 * from the ith word till the nth word have a total size of M.
 * @param i is the index of the first word of the paragraph
 * @param k is the index of the last word of the paragraph
 * @param f contains meta-data about the file (see file_data for more details)
 * @param p contains meta-data about the paragraph (see paragraph_data)
 * @return return the size (in units) to add
 */
static int E(int i, int k, struct file_data* f, struct paragraph_data* p);

/**  Delta(i;k) = L(mk) + Sum[j from i to k-1]( L(mj) + μ )
 * 
 * Returns the size(in units) of the words and the minimum spaces between words 
 * for the ith word till the nth word.
 * @param i is the index of the first word of the paragraph
 * @param k is the index of the last word of the paragraph
 * @param f contains meta-data about the file (see file_data for more details)
 * @param p contains meta-data about the paragraph (see paragraph_data)
 * @return the size (in units) of the words between the indexes i and n 
 */
static int Delta(int i, int k, struct file_data* f, struct paragraph_data* p);

/**
 * This functions prints the paragraph justified.
 * @param outformat is the output stream to consider
 * @param p contains meta-data about the paragraph (see paragraph_data)
 * @param phi_mem memorises all the values of phi
 * @param nb_words number of words of the paragraph
 */
static void draw_wordparagraph(struct stream* outformat, struct paragraph_data* p, 
                struct decoupage* phi_mem, int nb_words);

/**
 * free the memory allocated to tab
 * @param tab: array of string we want to deallocate
 */
static void free_tab_words(char** tab);

/**
 * free the memory allocated to the pragraph_data.
 * Warning, only free the content of the structure.
 * @param p structure for wich we want to free the content
 */
static void free_paragraph_data(struct paragraph_data* p);

/**
 * prints an error message and exits.
 */
static void allocation_error(void);

/**
 * Secured malloc that exits in case of failure.
 * @param size size of the memory to allocate
 * @return allocated memory.
 */
static void *s_malloc(size_t size);

/**
 * allocates memory for an array of nmemb elements of size bytes each and 
 * returns a pointer to the allocated memory.
 * @param nmemb is the number of elements to allocate
 * @param size is the size of an element
 * @return a pointer to the allocated memory.
 */
static void *s_calloc(size_t nmemb, size_t size);



/* ************************************************************************** */
/* ********************************** Main ********************************** */
/* ************************************************************************** */

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

        long M = 80;
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
/* *********************** Algorithmic Functions **************************** */
/* ************************************************************************** */

long altex(FILE* in, size_t len, struct stream *outformat, unsigned long M,
                unsigned N)
{
        // Variables used to parse the file :
        char* buffer = (char*) s_calloc(len+1, sizeof(char) ) ;  
        size_t buffer_size = len ;
        struct parser p_in;
        init_parser(in, &p_in) ;

        // Meta-data about justification efficiency : 
        long maxval_all_paragraphs = -1 ;
        long sumval_all_paragraphs = 0 ;
        // Meta-data about the file :
        struct file_data f_data = {M, N, sizeSeparator(outformat), outformat};
        
        // We read each paragraph from the first one
        int endofile = 0;
        while (!endofile) { 
                // Variables used to memorize a paragraph :
                char** tab_words = (char** ) s_calloc(len, sizeof(char*));
                int nbwords = 0;
                unsigned long par_len = 0;
                // Init of structures stocking meta-data about file and parag
                size_t* words_L = (size_t*) s_malloc(len * sizeof(size_t));
                struct paragraph_data p_data = {tab_words, words_L};
                
                // Reads the paragraph & init meta-data
                int is_paragraph_end = 0 ;
                while ( !is_paragraph_end ) { 
                        // Reading of a word :
                        size_t n = read_word( &p_in, &is_paragraph_end, buffer, 
                                buffer_size ) ; 
                        
                        if  (n==0) { 
                                endofile = 1 ;
                                break ; 
                        } else {
                                // We memoize the word_length value.
                                words_L[nbwords] = wordlength(outformat, buffer); 	       
                                // Update of paragraph length with word's size.
                                par_len += words_L[nbwords];
                                // We add the word to the array. 
                                tab_words[nbwords] = (char*) s_calloc(n+1, sizeof(char));
                                memcpy(tab_words[nbwords], buffer, strlen(buffer) + 1);
                                nbwords++;
                        }
                }
                if (nbwords > 0) {
                        // memoization of penalties for the current paragraph
                        struct decoupage phi_mem [nbwords];
                        int space_mem[nbwords];
                        
                        // Init of the memoization array with the default val -1
                        int i;
                        for (i = 0; i < nbwords; i++) {
                                phi_mem[i].cout = -1;
                                phi_mem[i].coupe = -1;
                        }
                        
                        // Update of paragraph length with spaces' size
                        par_len += f_data.size_separator * (nbwords - 1);
                
                        // If the paragraph holds on a line, we just draw it
                        if (par_len < M) {
                                draw_wordline(outformat, nbwords, tab_words, 1);
                        } else {
                                // Else we compute the optimal jusitfication
                                int32_t parag_cost =
                                        justify_par(0, nbwords - 1, &f_data,
                                        &p_data, space_mem, 
                                        phi_mem);
                                // Uodate data about justification performance:
                                sumval_all_paragraphs += parag_cost;
                                if(parag_cost > maxval_all_paragraphs ||
                                        maxval_all_paragraphs == -1)
                                        maxval_all_paragraphs = parag_cost;
                                // We draw the pargraph justified.
                                draw_wordparagraph(outformat, &p_data,
                                        phi_mem, nbwords);
                        }
                        
                        par_len = 0;
                        nbwords = 0;
                }
                free_paragraph_data(&p_data);
        }
        free(buffer) ;
        fprintf(stderr, "\n#@alignment_cost:MAX=%ld;SUM=%ld\n",
        maxval_all_paragraphs, sumval_all_paragraphs);

        return maxval_all_paragraphs;
}

static int justify_par(int i, int n, struct file_data* f,
        struct paragraph_data* p, int* space_mem, 
        struct decoupage* phi_mem)
{
        
        int debut = i;
        int min;
        for(i = n; i >= debut; --i){
                min = INT_MAX;
                int k_max = i;
                int aux;

                // Computes space memoization of the 1st word of the paragraph :
                space_mem[i] = E(i, i, f, p);

                // We truncate words larger than M (and update meta-data).
                if (space_mem[i] < 0){
                        word_truncate_at_length(f->outformat, p->tab_words[i], f->M);
                        p->words_L[i] = f->M;
                        min = f->M;
                        phi_mem[i].cout = min;
                        continue;
                }

                // Computes space memoization for the rest of the paragraph :
                while(++k_max <= n &&
                                (space_mem[k_max] = 
                                 space_mem[k_max - 1] - 
                                 (p->words_L[k_max] + f->size_separator)) >=0 );

                // The paragraphe holds on a line : No justification to do
                if (k_max == n+1){
                        min = 0;
                        phi_mem[i].cout = min;
                        continue;
                }

                // Computes the min penalty value (between all possible cuts)
                int k;
                for(k = i; k < k_max; ++k){
                        // Computes the penality val for this cut (i to k)
                        aux = phi_mem[k+1].cout + penality(space_mem[k], f->N); 

                        // Updates the current min penality (if necessary)
                        if (aux < min) {
                                min = aux;
                                phi_mem[i].cout = min;
                                phi_mem[i].coupe = k+1;
                        }
                } 
        }
        return min;
}

static int E(int i, int k, struct file_data* f, struct paragraph_data* p)
{
        return (f->M - Delta(i, k, f, p));
}

static int Delta(int i, int k, struct file_data* f, struct paragraph_data* p)
{
        // Counts the size of the first word:
        int words_n_spaces_length = p->words_L[i];
        int j;
        // Adds the size of the other words + spaces (1 space/word)
        for (j = i + 1; j <= k; j++) {
                words_n_spaces_length += p->words_L[j] + f->size_separator;
        }
        return words_n_spaces_length;
}

static void draw_wordparagraph(struct stream* outformat, struct paragraph_data* p, 
                struct decoupage* phi_mem, int nb_words)
{
        int i = 0;
        while ((phi_mem[i].coupe) != -1)
        {
                draw_wordline(outformat, phi_mem[i].coupe - i, p->tab_words+i, 0);
                i = phi_mem[i].coupe;
        }
        draw_wordline(outformat, nb_words - i, p->tab_words+i, 1);
}


/* ************************************************************************** */
/* *********************** Utility Memory Functions ************************* */
/* ************************************************************************** */

static void allocation_error(void)
{
	errno = ENOMEM;
	perror(0);
        exit(EXIT_FAILURE);
}

static void *s_malloc(size_t size)
{
	void *mem = malloc(size);
	if (!mem)
                allocation_error();
	return mem;
}

static void *s_calloc(size_t nmemb, size_t size)
{
	void *mem = calloc(nmemb, size);
	if (!mem)
                allocation_error();
	return mem;
}

static void free_tab_words(char** tab) 
{
        int i;
        // Free the strings in the tab :
        for (i = 0; tab[i] != NULL; i++) {
                free(tab[i]);
                tab[i] = NULL;
        }
        free(tab);

}

static void free_paragraph_data(struct paragraph_data* p)
{
        free_tab_words(p->tab_words);
        free(p->words_L);
};