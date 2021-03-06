#include "wrapper.h"

int fvc_wrapper(esa_t *C){
	size_t len = C->len;

	char *FVC = C->FVC = (char*) malloc(len);
	if(!FVC){
		return 1;
	}

	const char *S = C->S;
	const int *SA = C->SA;
	const int *LCP= C->LCP;
	int *ISA = (int*) malloc(len * sizeof(int));
	if(!ISA){
		return 2;
	}

	for(size_t i=0; i< len; i++){
		ISA[SA[i]] = i;
	}

	FVC[0] = '\0';

	int l = 0;
	for(size_t i=0; i< len; i++){
		int j = ISA[i];
		if( j > 0){
			size_t k = SA[j-1];
			while( k+l < len && S[k+l] == S[i+l]){
				l++;
			}
			FVC[j] = S[i+l];
			l--;
			if( l < 0){
				l = 0;
			}
		}
	}

	free(ISA);
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
		if( C->LCP == NULL ){
			return 3;
		}
	}
	saidx_t *LCP = C->LCP;
	
	LCP[0] = -1;
	LCP[len] = -1;
	
	// Allocate temporary arrays
	saidx_t *PHI = (saidx_t *) malloc( len * sizeof(saidx_t));
	saidx_t *PLCP = (saidx_t *) malloc( len * sizeof(saidx_t));
	if( !PHI || !PLCP){
		free(PHI);
		free(PLCP);
		return 2;
	}
	
	PHI[SA[0]] = -1;
	ssize_t i, k;
	
	
	for( i=1; i< len; i++){
		PHI[SA[i]] = SA[ i-1];
	}
	
	ssize_t l = 0;
	for( i = 0; i< len ; i++){
		k = PHI[i];
		if( k != -1 ){
			while( k+l < len && S[k+l] == S[i+l]){
				l++;
			}
			PLCP[i] = l;
			l--;
			if( l < 0) l = 0;
		} else {
			PLCP[i] = -1;
		}
	}
	
	// unpermutate the LCP array
	for( i=1; i< len; i++){
		LCP[i] = PLCP[SA[i]];
	}
	
	free(PHI);
	free(PLCP);
	return 0;
}
