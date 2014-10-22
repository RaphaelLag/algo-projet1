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
                int* phi_memoization, int* optimal_solution);
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

struct solution{
    int pos;
    struct solution* next;
};

static struct solution* solution_text_alignement(int i, int k, int* optimal_choice);


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
                        //printf("%s ", tabwords[nbwords]);
                        //fflush(stdout);
                        nbwords++;
                        // printf("%s ", buffer) ;
                        /*else { // word of length n 
                          printf("%s ", buffer) ;
                          }*/
                }
                // Stocks the choices made to obtain the optimal solution
                int optimal_choice[nbwords][nbwords];

                // memoization of penalties for the current paragraph
                int phi_memoization [nbwords][nbwords];

                int i,j;
                // Init of the memoization array with the default value -1
                for (i = 0; i < nbwords; i++) {
                        for (j = 0; j < nbwords; j++) {
                                phi_memoization[i][j] = -1;             
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
                        sumval_all_paragraphs += justify_par(0, nbwords - 1, 0, outformat, M, N, 
                                        tabwords, size_separator, (int *)phi_memoization, (int*) optimal_choice);
                        

                        printf("%d\n", optimal_choice[0][nbwords - 1]);
                        //draw_wordline(outformat, nbwords, tabwords, 1);
                        /*
                        struct solution* sol = solution_text_alignement(0, nbwords-1, (int*) optimal_choice);
                        
                        int last_pos = nbwords;
                        while(sol){
                            draw_wordline(outformat, last_pos - sol->pos, tabwords+sol->pos, 1);
                            printf("\n");
                            last_pos = sol->pos;
                            sol = sol->next;
                        }
                        //*/
                }
                //      on réinitialise les variables nécessaires pour le prochain paragraphe
                //      + on libère la mémoire
                par_len = 0;
                nbwords = 0;
                // TODO: remettre celui là :
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

        long M = 40 ; // 40
        char* input_file = "texte-test-court.txt";
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
static int justify_par(int i, int n, int previous_i, struct stream* outformat, unsigned long M,
                unsigned N, char** tabwords, long size_separator,
                 int* phi_memoization, int* optimal_choice)
{
        //TODO cas où un mot est plus long que la largeur de ligne demandée
        int min = M;
        int k = i;
        int aux, nbspaces;

        // The paragraphe (= a line in this case) is not long enough to fulfill a line
        // No opitmisation to do.
        if (E(i, n, M, tabwords, outformat, size_separator) >= 0){
                optimal_choice[(n+1)* 0 + n] = previous_i;
                return 0;
        }
        
        // Computes the minimum
        while (k <= n && 
            (nbspaces = E(i, k, M, tabwords, outformat, size_separator)) >= 0){ 
                // if the penalty value as ever been wordked we return it :
                if (phi_memoization[(n+1)*i+k] != -1)
                        aux = phi_memoization[(n+1)*i+k] + penality(nbspaces, N);
                // Else, we compute the penalty for the paragraph beginning 
                // at the word N° k+1:
                else{  
                        aux = justify_par(k+1, n, i, outformat, M, N,tabwords,
                                        size_separator, phi_memoization, optimal_choice) + penality(nbspaces, N);
                        // phi[i][k] (i: debut ligne, k: fin ligne)
                        // Cout de la ligne allant des mots i à k, + Cout optimal des lignes qui suivent (de k+1 à fin)
                        // (n+1)* i car chaque ligne du tableau est de taille nb_words (= n+1).
                        phi_memoization[(n+1)*i+k] = min; // Todo dans le else avant.
                }
                // Update of the current min penality
                if (aux < min) {
                        min = aux;
                        optimal_choice[(n+1)*i + k] = previous_i; 
                }
                k++;
        }
        

        return min;
}
/*

*** Graphical representation of the problem:

0______________I____________
|______________|____________|            } Paragraph
|________|__________________|            }
         K                   \__Nb_words 

*** Solution memoization principle: 

Sol(0,n)   = k1; 
Sol(k1,n)  = k2;
sol(k2,k1) = k3;
Sol(k3,k2) = 0;
Sol(0,k3)  = -1;

|-----|--------|-----|---|
0     k3       k2    k1  n 

*/




struct solution* solution_text_alignement(int i, int k, int* optimal_choice)
{
    struct solution* sol = NULL;
    int new_i = i;
    int new_k = k;
    // TODO: boucle infinie ici + malloc en série !! OMG !! on est trop morts !!!!!
    while(optimal_choice[(k+1)*new_i + new_k] != 0){
        // Adding of the new solution at the head of the list.
        // |- Alloc new list element
        struct solution* new_sol = (struct solution*) malloc(sizeof(struct solution));
        if(!new_sol){
            fprintf(stderr, "new_sol malloc : allocation failed");
            exit(EXIT_FAILURE);
        }
        // |- Link the new element in head of the list.
        new_sol->pos = optimal_choice[ (k+1)*new_i + new_k];
        new_sol->next = sol;
        sol = new_sol;
        
        // Update of new_i, new_k
        new_k = (new_sol->next == NULL) ? new_k : new_i; // pas pos, last interval
        new_i = new_sol->pos;
    }
    return sol;
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
        int words_n_spaces_length = wordlength(outformat, tabwords[i]);
        int j;
        // TODO : on va de j à k ou de j à k-1 ? (cf formule plus haut)
        for (j = i+1; j <= k; j++) {
                words_n_spaces_length += wordlength(outformat, tabwords[j]) +
                        size_separator;
        }
        return words_n_spaces_length;
}
