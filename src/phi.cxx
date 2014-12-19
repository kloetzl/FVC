#include "wrapper.h"

int fvc_wrapper(esa_t *C){

	return 0;
}

int lcp_wrapper( esa_t *C){
	const char *S = C->S;
	saidx_t *SA  = C->SA;
	saidx_t len  = C->len;
	
	// Trivial safety checks
	if( !C || S == NULL || SA == NULL || len == 0){
		return 1;
	}
	
	// Allocate new memory
	if( C->LCP == NULL){
		// The LCP array is one element longer than S.
		C->LCP = (saidx_t*) malloc((len+1)*sizeof(saidx_t));
		C->FVC = (char*) malloc(len * sizeof(char));
		if( !C->LCP || !C->FVC ){
			return 3;
		}
	}
	saidx_t *LCP = C->LCP;
	char *FVC = C->FVC;
	
	LCP[0] = -1;
	LCP[len] = -1;
	FVC[0] = '\0';
	
	// Allocate temporary arrays
	saidx_t *PHI = (saidx_t *) malloc( len * sizeof(saidx_t));
	saidx_t *PLCP = (saidx_t *) malloc( len * sizeof(saidx_t));
	char *PFVC = (char*) malloc( len* sizeof(char));
	if( !PHI || !PLCP || !PFVC){
		free(PHI);
		free(PLCP);
		return 2;
	}
	
	PHI[SA[0]] = -1;
	ssize_t i, k;
	
	
	for( i=1; i< len; i++){
		PHI[SA[i]] = SA[ i-1];
	}
	
	char fvchar;
	ssize_t l = 0;
	for( i = 0; i< len ; i++){
		k = PHI[i];
		if( k != -1 ){
			while( S[k+l] == (fvchar = S[i+l])){
				l++;
			}
			PLCP[i] = l;
			PFVC[i] = fvchar;
			l--;
			if( l < 0) l = 0;
		} else {
			PLCP[i] = -1;
		}
	}
	
	// unpermutate the LCP array
	for( i=1; i< len; i++){
		LCP[i] = PLCP[SA[i]];
		FVC[i] = PFVC[SA[i]];
	}
	
	free(PHI);
	free(PFVC);
	free(PLCP);
	return 0;
}
